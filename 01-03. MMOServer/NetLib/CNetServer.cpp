#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include "CNetServer.h"
#include "CSession.h"

#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

using namespace netlib;

CNetServer::Config::Config()
	: szBindIP{}
	, bindIP(0)
	, portNumber(0)
	, numConcurrentThread(0)
	, numWorkerThread(0)
	, numMaxSession(0)
	, bUseNagle(false)
	, packetCode(0)
	, packetKey(0)
	, maxPacketSize(0)
	, bUsePacketEncryption(true)
	, bUseTrafficCongestionControl(false)
{
}

CNetServer::CNetServer()
	: _hIOCP(NULL)
	, _listenSock(NULL)
	, _arrSession(nullptr)
	, _bTrafficCongestion(false)
	, _bShutdown(false)
	, _bOutputDebug(false)
	, _bOutputSystem(true)
{

#ifdef NET_ENABLE_MEMORY_LOGGING
	_sizeLogBuffer = 0xfffff;  // �α� ���� ũ��. 2���� ���� ��� 1�� �̷������ ��
	_logBuffer = (MemLog*)malloc(sizeof(MemLog) * (_sizeLogBuffer + 1));
	memset(_logBuffer, 0, sizeof(MemLog)* (_sizeLogBuffer + 1));
	_logBufferIndex = -1;
#endif
}

CNetServer::~CNetServer()
{

}

/* server */
bool CNetServer::StartUp(const wchar_t* bindIP, unsigned short port, int numConcurrentThread, int numWorkerThread, bool bUseNagle, int numMaxSession, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption, bool bUseTrafficCongestionControl)
{
	timeBeginPeriod(1);

	INT retPton = InetPton(AF_INET, bindIP, &_config.bindIP);
	if (retPton != 1)
	{
		OnError(L"[Network] Server IP is not valid. error:%u\n", WSAGetLastError());
		return false;
	}
	wcscpy_s(_config.szBindIP, wcslen(bindIP) + 1, bindIP);
	
	_config.portNumber = port;
	_config.numConcurrentThread = numConcurrentThread;
	_config.numWorkerThread = numWorkerThread;
	_config.bUseNagle = bUseNagle;
	_config.numMaxSession = numMaxSession;
	_config.packetCode = packetCode;
	_config.packetKey = packetKey;
	_config.maxPacketSize = maxPacketSize;
	_config.bUsePacketEncryption = bUsePacketEncryption;
	_config.bUseTrafficCongestionControl = bUseTrafficCongestionControl;


	// ���� ����
	_arrSession = (CSession*)_aligned_malloc(sizeof(CSession) * _config.numMaxSession, 64);
	for (int i = 0; i < _config.numMaxSession; i++)
	{
		new (_arrSession + i) CSession(i);
	}

	for (int i = _config.numMaxSession - 1; i >= 0; i--)
	{
		_stackSessionIdx.Push(i);
	}


	// �޸� ���� üũ
	if ((unsigned long long)this % 64 != 0)
		OnError(L"[Network] network object is not aligned as 64\n");
	for (int i = 0; i < _config.numMaxSession; i++)
	{
		if ((unsigned long long)&_arrSession[i] % 64 != 0)
			OnError(L"[Network] %d'th session is not aligned as 64\n", i);
	}
	if ((unsigned long long) & _stackSessionIdx % 64 != 0)
		OnError(L"[Network] session index stack is not aligned as 64\n");
	if ((unsigned long long)&_monitor.sendCount % 64 != 0)
		OnError(L"[Network] sendCount is not aligned as 64\n");
	if ((unsigned long long)&_monitor.sendAsyncCount % 64 != 0)
		OnError(L"[Network] sendAsyncCount is not aligned as 64\n");



	// WSAStartup
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		OnError(L"[Network] Failed to initiate Winsock DLL. error:%u\n", WSAGetLastError());
		return false;
	}

	// IOCP ����
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, _config.numConcurrentThread);
	if (_hIOCP == NULL)
	{
		OnError(L"[Network] Failed to create IOCP. error:%u\n", GetLastError());
		return false;
	}

	// �������� ����
	_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// SO_LINGER �ɼ� ����
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(_listenSock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	// nagle �ɼ� ���� ����
	if (_config.bUseNagle == false)
	{
		DWORD optval = TRUE;
		int retSockopt = setsockopt(_listenSock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
		if (retSockopt == SOCKET_ERROR)
		{
			OnError(L"[Network] Failed to set TCP_NODELAY option on listen socket. error:%u\n", WSAGetLastError());
		}
	}

	// bind
	SOCKADDR_IN addrServ;
	ZeroMemory(&addrServ, sizeof(SOCKADDR_IN));
	addrServ.sin_family = AF_INET;
	addrServ.sin_addr.s_addr = _config.bindIP; //htonl(_bindIP);  InetPton �Լ����� �̸� network order�� ��ȯ�Ͽ� ��ȯ���ֱ� ������ htonl �Լ��� ����� �ʿ䰡 ����
	addrServ.sin_port = htons(_config.portNumber);
	if (bind(_listenSock, (SOCKADDR*)&addrServ, sizeof(addrServ)) == SOCKET_ERROR)
	{
		OnError(L"[Network] Listen socket binding error. error:%u\n", WSAGetLastError());
		return false;
	}

	if (listen(_listenSock, SOMAXCONN_HINT(_config.numMaxSession)) == SOCKET_ERROR)
	{
		OnError(L"[Network] Failed to start listening. error:%u\n", WSAGetLastError());
		return false;
	}


	// worker ������ ����
	ThreadInfo th;
	for (int i = 0; i < _config.numWorkerThread; i++)
	{
		th.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadWorker, (PVOID)this, 0, &th.id);
		if (th.handle == NULL)
		{
			OnError(L"[Network] An error occurred when starting the worker thread. error:%u\n", GetLastError());
			return false;
		}
		_vecThWorker.push_back(th);
	}

	// Ʈ���� ȥ������ ������ ����
	if (_config.bUseTrafficCongestionControl == true)
	{
		_thTrafficCongestionControl.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadTrafficCongestionControl, (PVOID)this, 0, &_thTrafficCongestionControl.id);
		if (_thTrafficCongestionControl.handle == NULL)
		{
			OnError(L"[Network] An error occurred when starting the traffic congestion control thread. error:%u\n", GetLastError());
			return false;
		}
	}

	// ������ ���ϴݱ� �̺�Ʈ ����
	_thDeferredCloseSocket.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (_thDeferredCloseSocket.hEvent == NULL)
	{
		OnError(L"[Network] failed to create event for deferred close socket thread, error:%d\n", GetLastError());
		return false;
	}
	// ������ ���ϴݱ� ������ ����
	_thDeferredCloseSocket.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadDeferredCloseSocket, (PVOID)this, 0, &_thDeferredCloseSocket.id);
	if (_thDeferredCloseSocket.handle == NULL)
	{
		OnError(L"[Network] An error occurred when starting the deferred close socket thread. error:%u\n", GetLastError());
		return false;
	}


	// accept ������ ����
	_thAccept.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadAccept, (PVOID)this, 0, &_thAccept.id);
	if (_thAccept.handle == NULL)
	{
		OnError(L"[Network] An error occurred when starting the accept thread. error:%u\n", GetLastError());
		return false;
	}


	return true;
}




// accept ����
void CNetServer::StopAccept()
{
	if (_listenSock != INVALID_SOCKET)
		closesocket(_listenSock);
	_listenSock = INVALID_SOCKET;
}


// ���� ����(accept ���� ����)
void CNetServer::Shutdown()
{
	std::lock_guard<std::mutex> lock_guard(_mtxShutdown);
	if (_bShutdown == true)
		return;
	_bShutdown = true;

	// accept �����带 �����Ų��.
	StopAccept();


	// ��� ������ �ݴ´�.
	// ������ ��ȯ�� worker �����忡�� ����� ����.
	for (int i=0; i< _config.numMaxSession; i++)
	{
		CSession* pSession = &_arrSession[i];
		if(pSession->_sock != INVALID_SOCKET)
			CancelIoEx((HANDLE)pSession->_sock, NULL);
	}
	Sleep(1000);  // IO ���� �Ϸ������� ��� ó���Ǿ� ��� ������ ������ ���浿�� ��� ��ٸ�

	// worker ������ ���� �޽����� ������.
	for (int i = 0; i < _config.numWorkerThread; i++)
	{
		PostQueuedCompletionStatus(_hIOCP, 0, NULL, NULL);
	}

	// ��� worker �����尡 ����Ǳ⸦ 10�ʰ� ��ٸ���.
	ULONGLONG timeout = 10000;
	ULONGLONG tick;
	DWORD retWait;
	for (int i = 0; i < _vecThWorker.size(); i++)
	{
		tick = GetTickCount64();
		retWait = WaitForSingleObject(_vecThWorker[i].handle, (DWORD)timeout);
		if (retWait != WAIT_OBJECT_0)
		{
			NET_OUTPUT_SYSTEM(L"[Network] Timeout occurred while waiting for the thread to be terminated. force terminate it. error:%u\n", GetLastError());
			TerminateThread(_vecThWorker[i].handle, 0);
		}
		
		timeout -= GetTickCount64() - tick;
	}

	// ��ü ����
	for (int i = 0; i < _config.numMaxSession; i++)
		(_arrSession[i]).~CSession();
	unsigned short pop;
	while (_stackSessionIdx.Pop(pop) == false);
	_aligned_free(_arrSession);
	CloseHandle(_thAccept.handle);
	if(_config.bUseTrafficCongestionControl == true)
		CloseHandle(_thTrafficCongestionControl.handle);
	CloseHandle(_thDeferredCloseSocket.handle);
	for (int i = 0; i < _vecThWorker.size(); i++)
		CloseHandle(_vecThWorker[i].handle);
	CloseHandle(_hIOCP);
	// WSACleanup �� �ٸ� ��Ʈ��ũ�� ������ ��ĥ�� �ֱ� ������ ȣ������ ����.
}


/* packet */
CPacket& CNetServer::AllocPacket()
{
	CPacket& packet = CPacket::AllocPacket();
	packet.Init(sizeof(PacketHeader));
	return packet;
}


bool CNetServer::SendPacket(__int64 sessionId, CPacket& packet)
{
	CSession* pSession = FindSession(sessionId);  // increase IO count
	if (pSession == nullptr)
		return false;

	PacketHeader header;
	if (packet.IsHeaderSet() == false)
	{
		// ��� ����
		header.code = _config.packetCode;
		header.len = packet.GetDataSize();
		header.randKey = (BYTE)rand();

		char* packetDataPtr;
		if (_config.bUsePacketEncryption == true)
		{
			packetDataPtr = packet.GetDataPtr();  // CheckSum ���
			header.checkSum = 0;
			for (int i = 0; i < header.len; i++)
				header.checkSum += packetDataPtr[i];
		}
		// ����ȭ���ۿ� ��� �Է�
		packet.PutHeader((char*)&header);
	}

	// ��Ŷ ��ȣȭ
	if (_config.bUsePacketEncryption == true)
		EncryptPacket(packet);

	// send lcokfree queue�� ����ȭ���� �Է�
	packet.AddUseCount();
	pSession->_sendQ->Enqueue(&packet);

	// Send�� �õ���
	SendPost(pSession);


	DecreaseIoCount(pSession);
	return true;
}




bool CNetServer::SendPacketAsync(__int64 sessionId, CPacket& packet)
{
	CSession* pSession = FindSession(sessionId);  // increase IO count
	if (pSession == nullptr)
		return false;

	PacketHeader header;
	if (packet.IsHeaderSet() == false)
	{
		// ��� ����
		header.code = _config.packetCode;
		header.len = packet.GetDataSize();
		header.randKey = (BYTE)rand();

		char* packetDataPtr;
		if (_config.bUsePacketEncryption == true)
		{
			packetDataPtr = packet.GetDataPtr();  // CheckSum ���
			header.checkSum = 0;
			for (int i = 0; i < header.len; i++)
				header.checkSum += packetDataPtr[i];
		}
		// ����ȭ���ۿ� ��� �Է�
		packet.PutHeader((char*)&header);
	}

	// ��Ŷ ��ȣȭ
	if (_config.bUsePacketEncryption == true)
		EncryptPacket(packet);

	// send lcokfree queue�� ����ȭ���� �Է�
	packet.AddUseCount();
	pSession->_sendQ->Enqueue(&packet);


	// ���� send�� ���������� Ȯ��
	if (pSession->_sendQ->Size() == 0) // ���� ��Ŷ�� ������ ����
	{
		DecreaseIoCount(pSession);
		return true;
	}
	else if ((bool)InterlockedExchange8((char*)&pSession->_sendIoFlag, true) == true) // ���� sendIoFlag�� false���� true�� �ٲپ��� ���� send��
	{
		DecreaseIoCount(pSession);
		return true;
	}

	// ���� send�� �������� �ƴ϶�� SendPost ��û�� IOCP ť�� ������
	BOOL retPost = PostQueuedCompletionStatus(_hIOCP, 0, (ULONG_PTR)pSession, (LPOVERLAPPED)2);
	if (retPost == 0)
	{
		OnError(L"[Network] Failed to post send request to IOCP. error:%u\n", GetLastError());
		pSession->_bError = true;
		CloseSocket(pSession);
		pSession->_sendIoFlag = false;
		DecreaseIoCount(pSession);
		_monitor.otherErrors++;
		return false;
	}
	
	return true;
}


// send�� �Ϸ�� �� ������ ���´�.
bool CNetServer::SendPacketAndDisconnect(__int64 sessionId, CPacket& packet)
{
	CSession* pSession = FindSession(sessionId);
	if (pSession == nullptr)
		return false;
	
	// ������ SO_LINGER �ɼǿ� 1ms timeout�� �����Ѵ��� send�Ѵ�.
	// SO_LINGER �ɼ��� �� ��ġ���� �����ϴ� ������ SO_LINGER �ɼ��� ������ ���� �ּ� 1���� send�� �̷������ closesocket �� �� timeout�� ����Ǳ� �����̴�.
	// SO_LINGER �ɼǿ� 1ms timeout�� �����Ѵ��� send�� �����ʰ� closesocket �ϸ� timeout�� 0�϶��� �����ϰ� �۵��Ѵ�.
	if (InterlockedExchange8((char*)&pSession->_bCloseWait, true) == false)
	{
		pSession->_lastPacket = &packet;

		LINGER linger;
		linger.l_onoff = 1;
		linger.l_linger = 100;
		if (setsockopt(pSession->_sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger)) == SOCKET_ERROR)
		{
			OnError(L"setsockopt SO_LINGER error: %u\n", WSAGetLastError());
			_monitor.otherErrors++;
		}
	}
	bool result = SendPacket(sessionId, packet);

	DecreaseIoCount(pSession);
	return result;
}

// �񵿱� send�� ��û�ϰ�, send�� �Ϸ�Ǹ� ������ ���´�.
bool CNetServer::SendPacketAsyncAndDisconnect(__int64 sessionId, CPacket& packet)
{
	CSession* pSession = FindSession(sessionId);
	if (pSession == nullptr)
		return false;

	// ������ SO_LINGER �ɼǿ� 1ms timeout�� �����Ѵ��� send�Ѵ�.
	// SO_LINGER �ɼ��� �� ��ġ���� �����ϴ� ������ SO_LINGER �ɼ��� ������ ���� �ּ� 1���� send�� �̷������ closesocket �� �� timeout�� ����Ǳ� �����̴�.
	// SO_LINGER �ɼǿ� 1ms timeout�� �����Ѵ��� send�� �����ʰ� closesocket �ϸ� timeout�� 0�϶��� �����ϰ� �۵��Ѵ�.
	if (InterlockedExchange8((char*)&pSession->_bCloseWait, true) == false)
	{
		pSession->_lastPacket = &packet;

		LINGER linger;
		linger.l_onoff = 1;
		linger.l_linger = 100;
		if (setsockopt(pSession->_sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger)) == SOCKET_ERROR)
		{
			OnError(L"setsockopt SO_LINGER error: %u\n", WSAGetLastError());
			_monitor.otherErrors++;
		}
	}
	bool result = SendPacketAsync(sessionId, packet);

	DecreaseIoCount(pSession);
	return result;
}

// IOCP �Ϸ����� ť�� �۾��� �����Ѵ�. �۾��� ȹ��Ǹ� OnInnerRequest �Լ��� ȣ��ȴ�.
bool CNetServer::PostInnerRequest(CPacket& packet)
{
	BOOL ret = PostQueuedCompletionStatus(_hIOCP, 0, (ULONG_PTR)&packet, (LPOVERLAPPED)1);
	if (ret == 0)
	{
		OnError(L"failed to post completion status to IOCP. error:%u\n", GetLastError());
		_monitor.otherErrors++;
		return false;
	}
	return true;
}



/* session */
bool CNetServer::Disconnect(__int64 sessionId)
{
	CSession* pSession = FindSession(sessionId);
	if (pSession == nullptr)
		return false;

	// ������ �ݴ´�.
	CloseSocket(pSession);

	DecreaseIoCount(pSession);
	return true;
}


/* dynamic alloc */
// 64byte aligned ��ü ������ ���� new, delete overriding
void* CNetServer::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}

void CNetServer::operator delete(void* p)
{
	_aligned_free(p);
}




/* Send, Recv */
void CNetServer::RecvPost(CSession* pSession)
{
	IncreaseIoCount(pSession);  // ioCount++; ���� recv IO�� �ִٴ� ���� ǥ����

	DWORD flag = 0;
	ZeroMemory(&pSession->_recvOverlapped, sizeof(OVERLAPPED_EX));
	pSession->_recvOverlapped.ioType = IO_RECV;

	WSABUF WSABuf[2];
	int directFreeSize = pSession->_recvQ.GetDirectFreeSize();
	WSABuf[0].buf = pSession->_recvQ.GetRear();
	WSABuf[0].len = directFreeSize;
	WSABuf[1].buf = pSession->_recvQ.GetBufPtr();
	WSABuf[1].len = pSession->_recvQ.GetFreeSize() - directFreeSize;

	int retRecv = WSARecv(pSession->_sock, WSABuf, 2, NULL, &flag, (OVERLAPPED*)&pSession->_recvOverlapped, NULL);

	if (retRecv == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			if (error == WSAECONNRESET                                             // Ŭ���̾�Ʈ���� ������ ����
				|| error == WSAECONNABORTED                                        // �������� ������ ����?
				|| (error == WSAENOTSOCK && pSession->_sock == INVALID_SOCKET))    // �������� ������ �ݰ� ���ϰ��� INVALID_SOCKET���� ������
			{
				NET_OUTPUT_DEBUG(L"[Network] WSARecv failed by known error. error:%d, session:%lld\n", error, pSession->_sessionId);
				_monitor.WSARecvKnownError++;  // ����͸�
			}
			else
			{
				NET_OUTPUT_SYSTEM(L"[Network] WSARecv failed by known error. error:%d, session:%lld\n", error, pSession->_sessionId);
				_monitor.WSARecvUnknownError++;  // ����͸�
			}

			// ���� �߻��� ������ ������ ���´�.
			CloseSocket(pSession);
			DecreaseIoCount(pSession);
			return;
		}
		else
		{
			// error�� �߻����� �ʾҴٸ� WSARecv�� ����� ����Ǿ� WSARecv �Լ��� ����� �� I/O�� �Ϸ�ƴٴ� ���̴�.
			// error�� �߻��ߴµ� WSA_IO_PENDING �̶�� �񵿱� I/O�� ���������� ��û�Ǿ��ٴ� ���̴�.
			// �׷��� �񵿱� I/O�� ���������� ��û�ƴµ� WSARecv �Լ� ��������� pSession->_sock ���� INVALID_SOCKET ���� ����Ǿ��� �� �ִ�.
			// �̷��ԵǴ� ���� WSARecv �Լ� ���η� ���ϰ��� ���޵Ǿ��� ���� I/O�� ��û���� ���� ���¿���, 
			// �ٸ������忡�� ������ Disconnect �Ͽ� ���ϰ��� INVALID_SOCKET ���� �ٲٰ� CancelIO �Լ��� ���Ͽ� �ɸ� I/O�� ����� �ڿ�, 
			// WSARecv �Լ����ο��� �����صξ��� ���ϰ����� I/O�� ��û�� ����̴�.
			// �׷��� WSARecv �Լ����� �񵿱� I/O�� ���������� ��û�ƴµ� ���ϰ��� INVALID_SOCKET ���� �ٲ�� �־��ٸ� ���� �ɸ� I/O�� ������־�� �Ѵ�.
			if (pSession->_sock == INVALID_SOCKET)
			{
				CancelIoEx((HANDLE)pSession->_socketToBeFree, NULL);
			}
			return;
		}
	}

	return;
}



// WSASend�� �õ��Ѵ�. 
void CNetServer::SendPost(CSession* pSession)
{
	if (pSession->_sendQ->Size() == 0) // ���� ��Ŷ�� ������ ����
	{
		// SendPacket �Լ������� sendQ�� ��Ŷ�� ���� ���� SendPost�� ȣ����. SendPost�� ȣ���ߴµ� ��Ŷ�� ���ٸ� �ٸ� �����尡 �̹� WSASend �� ����.
		// Send IO �Ϸ����������� sendIoFlag�� �����Ѵ��� SendPost ��. SendPost�� ȣ���ߴµ� ��Ŷ�� ���ٴ� ����, 
		// �ٸ� � �����忡���� ��Ŷ�� ���� �ʾҰų�, sendIoFlag�� ������ ���� �ٸ� �����忡�� ��Ŷ�� �������� WSASend �� ����. 
		return;
	}
	else if ((bool)InterlockedExchange8((char*)&pSession->_sendIoFlag, true) == true) // ���� sendIoFlag�� false���� true�� �ٲپ��� ���� send��
	{
		return;
	}

	IncreaseIoCount(pSession);

	// Ʈ������ ȥ���� ��� send���� �ʰ� send ��û�� IOCP ť�� �ٽ� ����ִ´�.
	if (_bTrafficCongestion)
	{
		BOOL retPost = PostQueuedCompletionStatus(_hIOCP, 0, (ULONG_PTR)pSession, (LPOVERLAPPED)2);
		if (retPost == 0)
		{
			OnError(L"[Network] Failed to post send request to IOCP. error:%u\n", GetLastError());
			pSession->_bError = true;
			CloseSocket(pSession);
			pSession->_sendIoFlag = false;
			DecreaseIoCount(pSession);
			_monitor.otherErrors++;
		}
		return;
	}

	// sendQ���� ����ȭ���� �ּҸ� ������ WSABUF ����ü�� �ִ´�.
	constexpr int numMaxPacket = SESSION_SIZE_ARR_PACKET;
	WSABUF arrWSABuf[numMaxPacket];
	int numPacket = 0;
	for (int i = 0; i < numMaxPacket; i++)
	{
		if (pSession->_sendQ->Dequeue(pSession->_arrPtrPacket[i]) == false)
			break;

		arrWSABuf[i].buf = (CHAR*)pSession->_arrPtrPacket[i]->GetHeaderPtr();
		arrWSABuf[i].len = pSession->_arrPtrPacket[i]->GetUseSize();
		numPacket++;
	}
	// ������ numSendPacket ����
	pSession->_numSendPacket = numPacket;
	// ���� �����Ͱ� ������ ����
	if (numPacket == 0)
	{
		pSession->_sendIoFlag = false;
		DecreaseIoCount(pSession);
		return;
	}

	// overlapped ����ü ����
	ZeroMemory(&pSession->_sendOverlapped, sizeof(OVERLAPPED_EX));
	pSession->_sendOverlapped.ioType = IO_SEND;

	// send
	int retSend = WSASend(pSession->_sock, arrWSABuf, numPacket, NULL, 0, (OVERLAPPED*)&pSession->_sendOverlapped, NULL);
	_monitor.sendCount += numPacket;  // ����͸�

	if (retSend == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			// Send �Ϸ������� �������� ���̹Ƿ� ���⼭ ����ȭ������ count�� ������.
			for (int i = 0; i < pSession->_numSendPacket; i++)
			{
				pSession->_arrPtrPacket[i]->SubUseCount();
			}

			pSession->_sendIoFlag = false;
			if (error == WSAECONNRESET                                              // Ŭ���̾�Ʈ���� ������ ����
				|| error == WSAECONNABORTED                                         // �������� ������ ����?
				|| (error == WSAENOTSOCK && pSession->_sock == INVALID_SOCKET))     // �������� ������ �ݰ� ���ϰ��� INVALID_SOCKET���� ������
			{
				_monitor.WSASendKnownError++;  // ����͸�
				NET_OUTPUT_DEBUG(L"[Network] WSASend failed with known error. error:%d, session:%lld\n", error, pSession->_sessionId);
			}
			else
			{
				_monitor.WSASendUnknownError++; // ����͸�
				NET_OUTPUT_SYSTEM(L"[Network] WSASend failed with unknown error. error:%d, session:%lld\n", error, pSession->_sessionId);
			}

			// ���� �߻��� ������ ������ ���´�.
			CloseSocket(pSession);
			DecreaseIoCount(pSession);
			return;
		}
		else
		{
			if (pSession->_sock == INVALID_SOCKET)
			{
				CancelIoEx((HANDLE)pSession->_socketToBeFree, NULL);
			}
			return;
		}
	}

	return;
}


// SendPacketAsync �Լ��� ���� �񵿱� send ��û�� �޾��� �� worker ������ ������ ȣ��ȴ�. WSASend�� ȣ���Ѵ�.
void CNetServer::SendPostAsync(CSession* pSession)
{
	// �� �Լ��� ȣ��Ǵ� ������ SendPacketAsync �Լ����� sendIoFlag�� true�� �����ϰ�, IoCount�� ������Ų �����̴�. 
	// �׷��� sendIoFlag�� �˻��ϰų� IoCount�� ������Ű�� �ʴ´�.

	// Ʈ������ ȥ���� ��� send���� �ʰ� send ��û�� IOCP ť�� �ٽ� ����ִ´�.
	if (_bTrafficCongestion)
	{
		BOOL retPost = PostQueuedCompletionStatus(_hIOCP, 0, (ULONG_PTR)pSession, (LPOVERLAPPED)2);
		if (retPost == 0)
		{
			OnError(L"[Network] Failed to post send request to IOCP. error:%u\n", GetLastError());
			pSession->_bError = true;
			CloseSocket(pSession);
			pSession->_sendIoFlag = false;
			DecreaseIoCount(pSession);
			_monitor.otherErrors++;
		}
		return;
	}

	// sendQ���� ����ȭ���� �ּҸ� ������ WSABUF ����ü�� �ִ´�.
	constexpr int numMaxPacket = SESSION_SIZE_ARR_PACKET;
	WSABUF arrWSABuf[numMaxPacket];
	int numPacket = 0;
	for (int i = 0; i < numMaxPacket; i++)
	{
		if (pSession->_sendQ->Dequeue(pSession->_arrPtrPacket[i]) == false)
			break;

		arrWSABuf[i].buf = (CHAR*)pSession->_arrPtrPacket[i]->GetHeaderPtr();
		arrWSABuf[i].len = pSession->_arrPtrPacket[i]->GetUseSize();
		numPacket++;
	}
	// ������ numSendPacket ����
	pSession->_numSendPacket = numPacket;
	// ���� �����Ͱ� ������ ����
	if (numPacket == 0)
	{
		pSession->_sendIoFlag = false;
		DecreaseIoCount(pSession);
		return;
	}

	// overlapped ����ü ����
	ZeroMemory(&pSession->_sendOverlapped, sizeof(OVERLAPPED_EX));
	pSession->_sendOverlapped.ioType = IO_SEND;

	// send
	int retSend = WSASend(pSession->_sock, arrWSABuf, numPacket, NULL, 0, (OVERLAPPED*)&pSession->_sendOverlapped, NULL);
	_monitor.sendAsyncCount += numPacket;  // ����͸�

	if (retSend == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			// Send �Ϸ������� �������� ���̹Ƿ� ���⼭ ����ȭ������ count�� ������.
			for (int i = 0; i < pSession->_numSendPacket; i++)
			{
				pSession->_arrPtrPacket[i]->SubUseCount();
			}
			pSession->_sendIoFlag = false;

			if (error == WSAECONNRESET                                              // Ŭ���̾�Ʈ���� ������ ����
				|| error == WSAECONNABORTED                                         // �������� ������ ����?
				|| (error == WSAENOTSOCK && pSession->_sock == INVALID_SOCKET))     // �������� ������ �ݰ� ���ϰ��� INVALID_SOCKET���� ������
			{
				_monitor.WSASendKnownError++;
				NET_OUTPUT_DEBUG(L"[Network] WSASend failed with known error. error:%d, session:%lld\n", error, pSession->_sessionId);
			}
			else
			{
				_monitor.WSASendUnknownError++;
				NET_OUTPUT_SYSTEM(L"[Network] WSASend failed with unknown error. error:%d, session:%lld\n", error, pSession->_sessionId);
			}

			// ���� �߻��� ������ ������ ���´�.
			CloseSocket(pSession);
			DecreaseIoCount(pSession);
			return;
		}
		else
		{
			if (pSession->_sock == INVALID_SOCKET)
			{
				CancelIoEx((HANDLE)pSession->_socketToBeFree, NULL);
			}
			return;
		}
	}

	return;
}









/* ���� */
// ����: FindSession �Լ��� ������ ����� ��� ���� ��� �Ŀ� �ݵ�� DecreaseIoCount �Լ��� ȣ���ؾ���
CSession* CNetServer::FindSession(__int64 sessionId)
{
	unsigned short sessionIdx = sessionId & 0xFFFF;
	CSession* pSession = &_arrSession[sessionIdx];

	IncreaseIoCount(pSession);
	if (pSession->_releaseFlag == true)
	{
		// ioCount�� ������������ release�� ������
		DecreaseIoCount(pSession);
		return nullptr;
	}
	if (pSession->_sessionId != sessionId)
	{
		// release�� ������ �ƴ����� ���� ���ϴ� ������ �ƴ�
		DecreaseIoCount(pSession);
		return nullptr;
	}

	return pSession;
}

void CNetServer::IncreaseIoCount(CSession* pSession)
{
	InterlockedIncrement(&pSession->_ioCount);
}

// ������ ioCount�� -1 �ϰ�, ioCount�� 0�̰� releaseFlag�� false��� ������ release�Ѵ�.
void CNetServer::DecreaseIoCount(CSession* pSession)
{
	InterlockedDecrement(&pSession->_ioCount);

	// IoCount�� 0�̸� releaseFlag�� true�� �ٲ۴�.
	long long ioCountAndReleaseFlag = InterlockedCompareExchange64(&pSession->_ioCountAndReleaseFlag, 1, 0);
	// IoCount�� 0�̰� releaseFlag�� true�� �ٲ��� ��� ������ release �Ѵ�.
	if (ioCountAndReleaseFlag == 0)
	{
		ReleaseSession(pSession);
	}
}






CSession* CNetServer::AllocSession(SOCKET clntSock, SOCKADDR_IN& clntAddr)
{
	unsigned short sessionIdx;
	bool retPop = _stackSessionIdx.Pop(sessionIdx);
	if (retPop == false)
	{
		OnError(L"[Network] pop stack failed\n");
		_monitor.otherErrors++;
		Crash();
		return nullptr;
	}

	
	CSession* pSession = &_arrSession[sessionIdx];
	// ������ �ʱ�ȭ�ϱ� �� ioCount�� �������Ѿ� ���ο� ����id�� �Ҵ�� �� ��ٷ� ��������(FindSession �Լ� ��)������ release���� ���Ѵ�. 
	// ���⼭ ������ ioCount�� accept �����尡 RecvPost�� ȣ���ѵ� ���ҽ�Ų��.
	IncreaseIoCount(pSession); 
	pSession->Init(clntSock, clntAddr);
	return pSession;
}

// ���Ͽ� �������� ��� I/O�� ����ϰ�, ������ INVALID_SOCKET���� �����Ѵ�.
// ������ �ٷ� ��ȯ������ �ʴµ�, ������ ��ȯ�ع����� ���� ������ ������� ������ ���� alloc�Ǵ� ������ �Ҵ���� �� �ְ�,
// �׷��ԵǸ� ���� ������ ���� ��ȯ�Ȱ��� �ƴϱ� ������ ���� ���ǿ��� ���� ���������� WSARecv, WSASend �Լ� call ���� �ٸ� ������ ���Ͽ� ����� �� �ִ�.
// ���ǰ� ������ ��ȯ�� ReleaseSession �Լ����� �����Ѵ�.
void CNetServer::CloseSocket(CSession* pSession)
{
	NET_OUTPUT_DEBUG(L"[Network] CloseSocket. session:%lld\n", pSession->_sessionId);

	IncreaseIoCount(pSession);  // ���� IO ��Ұ� ������ �������� ������ ��ȯ���� ���ϵ��� �Ѵ�.
	// isClosed�� true�� ���ʷ� �ٲپ��� ���� ������
	if (InterlockedExchange8((char*)&pSession->_isClosed, true) == false)
	{
		pSession->_socketToBeFree = pSession->_sock;
		pSession->_sock = INVALID_SOCKET;
		BOOL retCancel = CancelIoEx((HANDLE)pSession->_socketToBeFree, NULL);  // ���ϰ� ������ ��� �񵿱� I/O�� ����Ѵ�.
		if (retCancel == 0)
		{
			DWORD error = GetLastError();
			if (error == ERROR_NOT_FOUND)  // ���ϰ� ������ I/O�� ����
				;
			else
			{
				NET_OUTPUT_SYSTEM(L"[Network] CancelIoEx failed. error:%u, session:%lld\n", error, pSession->_sessionId);
				_monitor.otherErrors++;
			}
		}
	}
	DecreaseIoCount(pSession);
}

// ���ǰ� ������ ��ȯ�Ѵ�.
void CNetServer::ReleaseSession(CSession* pSession)
{
	NET_OUTPUT_DEBUG(L"[Network] ReleaseSession. session:%lld\n", pSession->_sessionId);
	SOCKET targetSocket = pSession->_sock != INVALID_SOCKET ? pSession->_sock : pSession->_socketToBeFree;

	// ������ ������ ���ϴݱ� ����̰�, ������ �߻����� �ʾҴٸ� ������ ���� ���� �ʰ� ���� �����忡�� �����Ѵ�. ������ ��ȯ�Ѵ�.
	if (pSession->_bCloseWait == true && pSession->_bError == false)
	{
		_thDeferredCloseSocket.msgQ.Enqueue(targetSocket);
		if (InterlockedExchange8((char*)&_thDeferredCloseSocket.bEventSet, true) == false)
			SetEvent(_thDeferredCloseSocket.hEvent);
	}
	else
	{
		// ������ ������ ���ϴݱ� ����ε� ������ �߻��ߴٸ� SO_LINGER �ɼ��� timeout�� �����Ѵ�.
		if (pSession->_bCloseWait == true && pSession->_bError == true)
		{
			LINGER linger;
			linger.l_onoff = 1;
			linger.l_linger = 0;
			if (setsockopt(targetSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger)) == SOCKET_ERROR)
			{
				OnError(L"setsockopt SO_LINGER error: %u\n", WSAGetLastError());
				_monitor.otherErrors++;
			}
		}

		// ���� �ݱ�
		closesocket(targetSocket);
	}

	// ���� ��ȯ
	__int64 sessionId = pSession->_sessionId;
	_stackSessionIdx.Push(pSession->_index);
	_monitor.disconnectCount++;

	// ������ ��ȯ�ϱ� ���� OnClientLeave �Լ��� ȣ��ɰ�� ������ ���� ������ �߻��� �� �ִ�:
	// OnClientLeave �Լ������� SendPacket ������ ������ ����� -> ���� ioCount�� 0�̱� ������ IncreaseIoCount, DecreaseIoCount �ϴ� �������� ioCount�� �Ǵٽ� 0�� �Ǿ� ReleaseSession �Լ��� �ѹ��� ȣ���
	// �׷��� OnClientLeave �Լ��� ������ ��ȯ�� �ڿ� ȣ��Ǿ�� �Ѵ�.
	OnClientLeave(sessionId);
}








/* ��Ŷ ��ȣȭ */
void CNetServer::EncryptPacket(CPacket& packet)
{
	if (packet.IsEncoded() == true)
		return;

	PacketHeader* pHeader = (PacketHeader*)packet.GetHeaderPtr();
	char* pTargetData = packet.GetDataPtr() - 1;  // checksum�� �����Ͽ� ��ȣȭ

	BYTE valP = 0;
	BYTE prevData = 0;
	for (int i = 0; i < pHeader->len + 1; i++)
	{
		valP = pTargetData[i] ^ (valP + pHeader->randKey + i + 1);
		pTargetData[i] = valP ^ (prevData + _config.packetKey + i + 1);
		prevData = pTargetData[i];
	}

	packet.SetEncoded();
}

bool CNetServer::DecipherPacket(CPacket& packet)
{
	PacketHeader* pHeader = (PacketHeader*)packet.GetHeaderPtr();
	char* pTargetData = packet.GetDataPtr() - 1;  // checksum�� �����Ͽ� ��ȣȭ

	BYTE valP = 0;
	BYTE prevData = 0;
	BYTE prevValP = 0;
	for (int i = 0; i < pHeader->len + 1; i++)
	{
		valP = pTargetData[i] ^ (prevData + _config.packetKey + i + 1);
		prevData = pTargetData[i];
		pTargetData[i] = valP ^ (prevValP + pHeader->randKey + i + 1);
		prevValP = valP;
	}
	
	packet.SetDecoded();

	// CheckSum ���
	char* payloadPtr = packet.GetDataPtr(); 
	BYTE checkSum = 0;
	for (int i = 0; i < pHeader->len; i++)
		checkSum += payloadPtr[i];

	// CheckSum ����
	if (pHeader->checkSum == checkSum)
		return true;
	else
		return false;
}




CNetServer::Monitor::Monitor()
{
	InitializeSRWLock(&_srwlVecTlsMonitor_mutable);
	QueryPerformanceFrequency(&_liPerformanceFrequency);
}

CNetServer::Monitor::TLSMonitor& CNetServer::Monitor::CreateTlsMonitor()
{
	AcquireSRWLockExclusive(&_srwlVecTlsMonitor_mutable);
	_vecTlsMonitor.push_back(TLSMonitor());
	TLSMonitor& ret = _vecTlsMonitor.back();
	ReleaseSRWLockExclusive(&_srwlVecTlsMonitor_mutable);
	return ret;
}

/* Get ����͸� */
__int64 CNetServer::Monitor::GetSendCompletionCount() const
{
	__int64 sum = 0;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
		sum += _vecTlsMonitor[i].sendCompletionCount;
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}
__int64 CNetServer::Monitor::GetRecvCompletionCount() const
{
	__int64 sum = 0;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
		sum += _vecTlsMonitor[i].recvCompletionCount;
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}
__int64 CNetServer::Monitor::GetRecvCount() const
{
	__int64 sum = 0;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
		sum += _vecTlsMonitor[i].recvCount;
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}
__int64 CNetServer::Monitor::GetDisconnByKnownIoError() const
{
	__int64 sum = WSARecvKnownError + WSASendKnownError;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
	{
		sum += _vecTlsMonitor[i].disconnByKnownRecvIoError;
		sum += _vecTlsMonitor[i].disconnByKnownSendIoError;
	}
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}
__int64 CNetServer::Monitor::GetDisconnByUnknownIoError() const
{
	__int64 sum = WSARecvUnknownError + WSASendUnknownError;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
	{
		sum += _vecTlsMonitor[i].disconnByUnknownRecvIoError;
		sum += _vecTlsMonitor[i].disconnByUnknownSendIoError;
	}
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}
__int64 CNetServer::Monitor::GetDisconnBy121RecvIoError() const
{
	__int64 sum = 0;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
		sum += _vecTlsMonitor[i].disconnBy121RecvIoError;
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}
__int64 CNetServer::Monitor::GetDisconnByNormalProcess() const
{
	__int64 sum = 0;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
		sum += _vecTlsMonitor[i].disconnByNormalProcess;
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}
__int64 CNetServer::Monitor::GetDisconnByPacketCode() const
{
	__int64 sum = 0;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
		sum += _vecTlsMonitor[i].disconnByPacketCode;
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}
__int64 CNetServer::Monitor::GetDisconnByPacketLength() const
{
	__int64 sum = 0;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
		sum += _vecTlsMonitor[i].disconnByPacketLength;
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}
__int64 CNetServer::Monitor::GetDisconnByPacketDecode() const
{
	__int64 sum = 0;
	AcquireSRWLockShared(&_srwlVecTlsMonitor_mutable);
	for (int i = 0; i < _vecTlsMonitor.size(); i++)
		sum += _vecTlsMonitor[i].disconnByPacketDecode;
	ReleaseSRWLockShared(&_srwlVecTlsMonitor_mutable);
	return sum;
}







/* thread */
// accept ������
unsigned WINAPI CNetServer::ThreadAccept(PVOID pParam)
{
	printf("accept thread begin. id:%u\n", GetCurrentThreadId());
	CNetServer& server = *(CNetServer*)pParam;

	server.UpdateAcceptThread();

	printf("accept thread end. id:%u\n", GetCurrentThreadId());
	return 0;
}

// accept ������ update
void CNetServer::UpdateAcceptThread()
{
	while (true)
	{
		// accept
		SOCKADDR_IN clntAddr;
		ZeroMemory(&clntAddr, sizeof(SOCKADDR_IN));
		int lenClntAddr = sizeof(clntAddr);
		SOCKET clntSock = accept(_listenSock, (SOCKADDR*)&clntAddr, &lenClntAddr);
		if (clntSock == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			if (error == WSAEINTR || error == WSAENOTSOCK) // ���������� ����. accept ������ ����
			{
				break;
			}
			else
			{
				NET_OUTPUT_SYSTEM(L"[Network] accept failed. error:%u\n", WSAGetLastError());
				_monitor.otherErrors++;
				continue;
			}
		}
		_monitor.acceptCount++;

		// session ���� max�̸� ������ ���´�.
		if (GetNumSession() >= _config.numMaxSession)
		{
			NET_OUTPUT_DEBUG(L"[Network] there is no room for the new session!!\n");
			closesocket(clntSock);
			_monitor.disconnBySessionLimit++;
			continue;
		}

		// connect ���� Ȯ��
		bool bAccept = OnConnectionRequest(clntAddr.sin_addr.S_un.S_addr, clntAddr.sin_port);
		if (bAccept == false)
		{
			closesocket(clntSock);
			_monitor.disconnByOnConnReq++;
			continue;
		}
		_monitor.connectCount++;

		// ���� ����
		CSession* pNewSession = AllocSession(clntSock, clntAddr);

		// ���ϰ� IOCP ����
		if (CreateIoCompletionPort((HANDLE)clntSock, _hIOCP, (ULONG_PTR)pNewSession, 0) == NULL)
		{
			OnError(L"[Network] failed to associate socket with IOCP\n");
			closesocket(clntSock);
			_monitor.disconnByIOCPAssociation++;
			continue;
		}

		// client join
		OnClientJoin(pNewSession->_sessionId);

		// recv
		RecvPost(pNewSession);

		// AllocSession ���� �������״� ioCount�� �����.
		DecreaseIoCount(pNewSession);

		// Ʈ������ ȥ���ϸ� accept�� ��� �����.
		if (_bTrafficCongestion == true)
		{
			Sleep(10);
		}
	}
}



// worker ������
unsigned WINAPI CNetServer::ThreadWorker(PVOID pParam)
{
	printf("worker thread begin. id:%u\n", GetCurrentThreadId());
	CNetServer& server = *(CNetServer*)pParam;

	server.UpdateWorkerThread();

	printf("worker thread end. id:%u\n", GetCurrentThreadId());
	return 0;
}

// worker ������ ������Ʈ
void CNetServer::UpdateWorkerThread()
{
	// worker ������ ���� TLS ����͸��� ���� counter ����
	Monitor::TLSMonitor& monitorTLS = _monitor.CreateTlsMonitor();
	
	DWORD numByteTrans;
	ULONG_PTR compKey;
	OVERLAPPED_EX* pOverlapped;
	CSession* pSession;
	// IOCP �Ϸ����� ó��
	while (true)
	{
		numByteTrans = 0;
		compKey = NULL;
		pOverlapped = nullptr;
		BOOL retGQCS = GetQueuedCompletionStatus(_hIOCP, &numByteTrans, &compKey, (OVERLAPPED**)&pOverlapped, INFINITE);

		DWORD error = GetLastError();
		DWORD WSAError = WSAGetLastError();

		// ���� ���
		pSession = (CSession*)compKey;

		// IOCP dequeue ����, �Ǵ� timeout. �� ��� numByteTrans, compKey �������� ���� ������ �ʱ� ������ ����üũ�� ����� �� ����.
		if (retGQCS == 0 && pOverlapped == nullptr)
		{
			// timeout
			if (error == WAIT_TIMEOUT)
			{
				OnError(L"[Network] GetQueuedCompletionStatus timeout. error:%u\n", error);
			}
			// error
			else
			{
				OnError(L"[Network] GetQueuedCompletionStatus failed. error:%u\n", error);
			}
			_monitor.otherErrors++;
			break;
		}

		// ������ ���� �޽����� ����
		else if (retGQCS != 0 && pOverlapped == nullptr)
		{
			break;
		}

		// PostInnerRequest �Լ��� ���� ���������� ���Ե� �Ϸ������� ����
		else if (pOverlapped == (OVERLAPPED_EX*)1)
		{
			CPacket* pInnerPacket = (CPacket*)compKey;
			OnInnerRequest(*pInnerPacket);
		}

		// SendPacketAsync �Լ��� ���ؼ� SnedPost ��û�� ����
		else if (pOverlapped == (OVERLAPPED_EX*)2)
		{
			SendPostAsync(pSession);
		}

		// recv �Ϸ����� ó��
		else if (pOverlapped->ioType == IO_RECV)
		{
			monitorTLS.recvCompletionCount++;

			bool bRecvSucceed = true;  // recv ���� ����

			// recv IO�� ������ ���
			if (retGQCS == 0)
			{
				bRecvSucceed = false;
				if (error == ERROR_NETNAME_DELETED     // ERROR_NETNAME_DELETED 64 �� WSAECONNRESET �� �����ϴ�?
					|| error == ERROR_CONNECTION_ABORTED)  // ERROR_CONNECTION_ABORTED 1236 �� ������ ������ �ڵ忡�� Ŭ�� ������ �ִٰ� �ǴܵǾ� ������ ��� �߻���. WSAECONNABORTED 10053 �� ����?
				{
					pSession->_bError = true;
					NET_OUTPUT_DEBUG(L"[Network] recv socket error. error:%u, WSAError:%u, session:%lld, numByteTrans:%d\n", error, WSAError, pSession->_sessionId, numByteTrans);
					monitorTLS.disconnByKnownRecvIoError++;
				}
				else if (error == ERROR_OPERATION_ABORTED) // ERROR_OPERATION_ABORTED 995 �� �񵿱� IO�� �����߿� CancelIo �Ͽ��ų� Ŭ���̾�Ʈ���� ������ ������ ��� �߻���.
				{
					NET_OUTPUT_DEBUG(L"[Network] recv socket error. error:%u, WSAError:%u, session:%lld, numByteTrans:%d\n", error, WSAError, pSession->_sessionId, numByteTrans);
					monitorTLS.disconnByKnownRecvIoError++;
				}
				else if (error == ERROR_SEM_TIMEOUT)     // ERROR_SEM_TIMEOUT 121 �� ��Ʈ��ũ�� ȥ���Ͽ� retransmission�� ��ӵǴ� timeout �Ǿ� �߻��ϴ°�
				{
					pSession->_bError = true;
					NET_OUTPUT_DEBUG(L"[Network] recv socket 121 error. error:%u, WSAError:%u, session:%lld, numByteTrans:%d, ip:%s, port:%d\n", error, WSAError, pSession->_sessionId, numByteTrans, pSession->_szIP, pSession->_port);
					monitorTLS.disconnBy121RecvIoError++;
				}
				else
				{
					pSession->_bError = true;
					NET_OUTPUT_SYSTEM(L"[Network] recv socket unknown error. error:%u, WSAError:%u, session:%lld, numByteTrans:%d\n", error, WSAError, pSession->_sessionId, numByteTrans);
					monitorTLS.disconnByUnknownRecvIoError++;
				}
			}

			// Ŭ���̾�Ʈ�κ��� ������� �޽����� ���� ���
			else if (numByteTrans == 0)
			{
				bRecvSucceed = false;
				NET_OUTPUT_DEBUG(L"[Network] recv closed by client's close request. session:%lld\n", pSession->_sessionId);
				monitorTLS.disconnByNormalProcess++;
			}

			// �������� �޽��� ó��
			else if (numByteTrans > 0)
			{
				// recvQ ���� ��� �޽����� ó���Ѵ�.
				PacketHeader header;
				pSession->_recvQ.MoveRear(numByteTrans);
				while (true)
				{
					if (pSession->_recvQ.GetUseSize() < sizeof(PacketHeader)) // �����Ͱ� ������̺��� ����
						break;

					// ����� ����
					pSession->_recvQ.Peek((char*)&header, sizeof(PacketHeader));
					if (header.code != _config.packetCode) // ��Ŷ �ڵ尡 �߸����� ��� error
					{
						NET_OUTPUT_DEBUG(L"[Network] header.code is not valid. sessionID:%llu, code:%d\n", pSession->_sessionId, header.code);
						pSession->_bError = true;
						bRecvSucceed = false;
						monitorTLS.disconnByPacketCode++;
						break;
					}

					if (header.len > _config.maxPacketSize) // ������ ũ�Ⱑ �ִ� ��Ŷũ�⺸�� ū ��� error
					{
						NET_OUTPUT_DEBUG(L"[Network] header.len is larger than max packet size. sessionID:%llu, len:%d, max packet size:%d\n", pSession->_sessionId, header.len, _config.maxPacketSize);
						pSession->_bError = true;
						bRecvSucceed = false;
						monitorTLS.disconnByPacketLength++;
						break;
					}

					if (sizeof(PacketHeader) + header.len > SIZE_RECV_BUFFER) // �������� ũ�Ⱑ ���� ũ�⺸�� Ŭ ��� error
					{
						NET_OUTPUT_DEBUG(L"[Network] packet data length is longer than recv buffer. sessionID:%llu, len:%d, size of ringbuffer:%d\n"
							, pSession->_sessionId, header.len, pSession->_recvQ.GetSize());
						pSession->_bError = true;
						bRecvSucceed = false;
						monitorTLS.disconnByPacketLength++;
						break;
					}

					// ���۳��� ������ ũ�� Ȯ��
					if (pSession->_recvQ.GetUseSize() < sizeof(PacketHeader) + header.len) // �����Ͱ� ��� �������� �ʾ���
					{
						break;
					}

					// ����ȭ���� �غ�
					CPacket& recvPacket = CPacket::AllocPacket();
					recvPacket.Init(sizeof(PacketHeader));

					// ����ȭ ���۷� �����͸� ����
					pSession->_recvQ.MoveFront(sizeof(PacketHeader));
					pSession->_recvQ.Dequeue(recvPacket.GetDataPtr(), header.len);
					recvPacket.MoveWritePos(header.len);

					// ��ȣȭ
					if (_config.bUsePacketEncryption == true)
					{
						recvPacket.PutHeader((const char*)&header); // ��ȣȭ�� ���� ����� �־���
						bool retDecipher = DecipherPacket(recvPacket);
						if (retDecipher == false) // ��ȣȭ�� ������(CheckSum�� �߸���)
						{
							NET_OUTPUT_DEBUG(L"[Network] packet decoding failed. sessionID:%llu\n", pSession->_sessionId);
							recvPacket.SubUseCount();
							pSession->_bError = true;
							bRecvSucceed = false;
							monitorTLS.disconnByPacketDecode++;
							break;
						}
					}

					NET_OUTPUT_DEBUG(L"[Network] recved. session:%lld, data len:%d, packet type:%d\n", pSession->_sessionId, header.len, *(WORD*)recvPacket.GetDataPtr());

					// ����� ��Ŷó�� �Լ� ȣ��
					monitorTLS.recvCount++;
					OnRecv(pSession->_sessionId, recvPacket);

					// ���ī��Ʈ ����
					recvPacket.SubUseCount();

				}
			}

			// numByteTrans�� 0���� ����. error?
			else
			{
				bRecvSucceed = false;
				OnError(L"[Network] recv error. NumberOfBytesTransferred is %d, error:%u, WSAError:%u, session:%lld\n", numByteTrans, error, WSAError, pSession->_sessionId);
				_monitor.otherErrors++;
				Crash();
			}

			// recv�� ���������� �������� �ٽ� recv �Ѵ�. ���������� CloseSocket
			if (bRecvSucceed)
				RecvPost(pSession);
			else
				CloseSocket(pSession);

			// ioCount--
			DecreaseIoCount(pSession);
		}

		// send IO �Ϸ����� ó��
		// �� ���������� ioCount > 0 �̱� ������ ������ ��ȿ�ϴ�.
		else if (pOverlapped->ioType == IO_SEND)
		{
			monitorTLS.sendCompletionCount++;

			// send�� ����� ����ȭ������ ���ī��Ʈ�� ���ҽ�Ų��.
			for (int i = 0; i < pSession->_numSendPacket; i++)
			{
				long useCount = pSession->_arrPtrPacket[i]->SubUseCount();
			}

			// ���ǿ� ��������� �Լ��� ȣ��Ǿ��� ���, ������ lastPacket�� ���������� ������ ���´�.
			if (pSession->_bCloseWait == true)
			{
				for (int i = 0; i < pSession->_numSendPacket; i++)
				{
					if (pSession->_arrPtrPacket[i] == pSession->_lastPacket)
					{
						CloseSocket(pSession);
						break;
					}
				}
			}

			// sendIoFlag �� �����Ѵ�.
			// ����ȭ������ ���ī��Ʈ�� ���ҽ�Ų���� sendIoFlag�� �����ؾ� pSession->_arrPtrPacket �� �����Ͱ� ������������� ����.
			pSession->_sendIoFlag = false;

			// send IO�� ������ ���
			if (retGQCS == 0)
			{
				if (error == ERROR_NETNAME_DELETED        // ERROR_NETNAME_DELETED 64 �� WSAECONNRESET �� �����ϴ�?
					|| error == ERROR_CONNECTION_ABORTED)  // ERROR_CONNECTION_ABORTED 1236 �� ������ ������ �ڵ忡�� Ŭ�� ������ �ִٰ� �ǴܵǾ� ������ ��� �߻���. WSAECONNABORTED 10053 �� ����?
				{
					pSession->_bError = true;
					NET_OUTPUT_DEBUG(L"[Network] send socket known error. error:%u, WSAError:%u, session:%lld, numByteTrans:%d\n", error, WSAError, pSession->_sessionId, numByteTrans);
					monitorTLS.disconnByKnownSendIoError++;
				}
				else if (error == ERROR_OPERATION_ABORTED) // ERROR_OPERATION_ABORTED 995 �� �񵿱� IO�� �����߿� CancelIo �Ͽ��ų� Ŭ���̾�Ʈ���� ������ ������ ��� �߻���.
				{
					NET_OUTPUT_DEBUG(L"[Network] send socket known error. error:%u, WSAError:%u, session:%lld, numByteTrans:%d\n", error, WSAError, pSession->_sessionId, numByteTrans);
					monitorTLS.disconnByKnownSendIoError++;
				}
				else
				{
					pSession->_bError = true;
					NET_OUTPUT_SYSTEM(L"[Network] send socket unknown error. NumberOfBytesTransferred:%d, error:%u, WSAError:%u, session:%lld\n", numByteTrans, error, WSAError, pSession->_sessionId);
					monitorTLS.disconnByUnknownSendIoError++;
				}

				CloseSocket(pSession);  // recv�� �ɷ��������̱� ������ ������ ��� IO�� ����Ѵ�.
			}
			// ������ ���� ���
			else if (pSession->_isClosed)
			{
				;
			}
			// �Ϸ����� ó�� 
			else if (numByteTrans > 0)
			{
				// sendQ ���� ������ ������ �õ���
				SendPost(pSession);
			}
			// numByteTrans�� 0���� ����. error?
			else
			{
				OnError(L"[Network] send error. NumberOfBytesTransferred is %d, error:%u, WSAError:%u, session:%lld\n", numByteTrans, error, WSAError, pSession->_sessionId);
				pSession->_bError = true;
				CloseSocket(pSession);
				_monitor.otherErrors++;
				Crash();
			}

			// ioCount--
			DecreaseIoCount(pSession);
		}
		else
		{
			// overlapped ����ü�� IO Type�� �ùٸ��� ����
			OnError(L"[Network] IO Type error. ioType:%d, session:%lld\n", pOverlapped->ioType, pSession->_sessionId);
			_monitor.otherErrors++;
			// crash
			Crash();

			// ioCount--
			DecreaseIoCount(pSession);
		}
	}
}



// Ʈ���� ȥ������ ������
unsigned WINAPI CNetServer::ThreadTrafficCongestionControl(PVOID pParam)
{
	printf("traffic congestion control thread begin. id:%u\n", GetCurrentThreadId());
	CNetServer& server = *(CNetServer*)pParam;

	server.UpdateTrafficCongestionControlThread();

	printf("traffic congestion control thread end. id:%u\n", GetCurrentThreadId());
	return 0;
}

void CNetServer::UpdateTrafficCongestionControlThread()
{
	MIB_TCPSTATS prevTCPStats = { 0, };
	MIB_TCPSTATS currTCPStats = { 0, };
	__int64 diffOutSegs = 0;
	__int64 diffRetransSegs = 0;
	__int64 retransCriterionWithin100ms;

	GetTcpStatistics(&prevTCPStats);

	while (_bShutdown == false)
	{
		Sleep(500);

		// TCP ��� ���
		GetTcpStatistics(&currTCPStats);

		// ���̰� ���
		if (currTCPStats.dwOutSegs < prevTCPStats.dwOutSegs)  // �� overflow ����ó��
			diffOutSegs = UINT_MAX - prevTCPStats.dwOutSegs + currTCPStats.dwOutSegs;
		else
			diffOutSegs = currTCPStats.dwOutSegs - prevTCPStats.dwOutSegs;

		if (currTCPStats.dwRetransSegs < prevTCPStats.dwRetransSegs)  // �� overflow ����ó��
			diffRetransSegs = UINT_MAX - prevTCPStats.dwRetransSegs + currTCPStats.dwRetransSegs;
		else
			diffRetransSegs = currTCPStats.dwRetransSegs - prevTCPStats.dwRetransSegs;

		prevTCPStats = currTCPStats;

		// ���� segment ���� 10000 �����̰ų�, �����۵� segment ���� ���� segment ���� 10% ���϶�� �Ѿ
		if (diffOutSegs < 10000 || diffOutSegs > diffRetransSegs * 10)
		{
			continue;
		}
		// ���� segment ���� 10000 �̻��̰�, �����۵� segment ���� ���� segment ���� 10% ���� ũ�ٸ� accept�� send�� ��� ����
		else
		{
			retransCriterionWithin100ms = diffOutSegs / 10;  // 100ms ������ ������ ����ġ ����(ȥ�������߿��� send�� �����������̱� ������ ������ ����ġ�� ���� ����)
			_bTrafficCongestion = true;
			_monitor.trafficCongestionControlCount++;  // ����͸�

			// ���� ȥ��flag�� on �Ǿ��ٸ�, 100ms���� �߰��� Ʈ������ �����Ѵ�.
			// 10���� �������� ���˵��� Ʈ������ �����ġ�� ������ ��� �߰� ������ �����Ѵ�.
			int normalWorkingCount = 0;
			while (true)
			{
				Sleep(100);
				GetTcpStatistics(&currTCPStats);

				// ���̰� ���
				if (currTCPStats.dwOutSegs < prevTCPStats.dwOutSegs)  // �� overflow ����ó��
					diffOutSegs = UINT_MAX - prevTCPStats.dwOutSegs + currTCPStats.dwOutSegs;
				else
					diffOutSegs = currTCPStats.dwOutSegs - prevTCPStats.dwOutSegs;

				if (currTCPStats.dwRetransSegs < prevTCPStats.dwRetransSegs)  // �� overflow ����ó��
					diffRetransSegs = UINT_MAX - prevTCPStats.dwRetransSegs + currTCPStats.dwRetransSegs;
				else
					diffRetransSegs = currTCPStats.dwRetransSegs - prevTCPStats.dwRetransSegs;

				prevTCPStats = currTCPStats;
				
				// ���� ȥ������ ���̶�� �����۵� segment ���� ����ġ ���϶�� accept�� send�� ��� �����
				if (_bTrafficCongestion == true)
				{
					if (diffRetransSegs < retransCriterionWithin100ms)
					{
						normalWorkingCount++;
						_bTrafficCongestion = false;
					}
					else
					{
						normalWorkingCount = 0;
						_monitor.trafficCongestionControlCount++;  // ����͸�
					}
				}
				// ���� ȥ������ ���� �ƴ϶�� ���� segment ���� 1000 �̻��̰�, �����۵� segment ���� ���� segment ���� 10% ���� ũ�ٸ� accept�� send�� ��� ����
				else
				{
					if (diffOutSegs > 1000 && diffRetransSegs * 10 > diffOutSegs)
					{
						normalWorkingCount = 0;
						_bTrafficCongestion = true;
						_monitor.trafficCongestionControlCount++;  // ����͸�
					}
					else
					{
						normalWorkingCount++;
					}
				}

				// 10���� �߰� ���˵��� Ʈ������ �����̾��ٸ� �߰� ���� ����
				if (normalWorkingCount >= 10)
					break;
			}
		}
	}
}


// ������ ���ϴݱ� ������
unsigned WINAPI CNetServer::ThreadDeferredCloseSocket(PVOID pParam)
{
	printf("deferred close socket thread begin. id:%u\n", GetCurrentThreadId());
	CNetServer& server = *(CNetServer*)pParam;

	server.UpdateThreadDeferredCloseSocketThread();

	printf("deferred close socket thread end. id:%u\n", GetCurrentThreadId());
	return 0;
}

void CNetServer::UpdateThreadDeferredCloseSocketThread()
{
	SOCKET socket;
	while (true)
	{
		if (WaitForSingleObject(_thDeferredCloseSocket.hEvent, INFINITE) != WAIT_OBJECT_0)
		{
			InterlockedExchange8((char*)&_thDeferredCloseSocket.bEventSet, false);
			return;
		}
		InterlockedExchange8((char*)&_thDeferredCloseSocket.bEventSet, false);

		while (_thDeferredCloseSocket.msgQ.Dequeue(socket))
		{
			// ������ �۽Ź��۳��� �����Ͱ� ��� ������������ ��ٸ����� ������ �ݴ´�.
			closesocket(socket);
			_monitor.deferredDisconnectCount;  // ����͸�
		}
	}
}









/* Crash */
void CNetServer::Crash()
{
	int* p = 0;
	*p = 0;
}


