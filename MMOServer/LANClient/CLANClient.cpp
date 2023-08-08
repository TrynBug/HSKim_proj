#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "CLANClient.h"

using namespace lanlib;

CLANClient::CLANClient()
	: _hIOCP(NULL)
	, _bOutputDebug(false)
	, _bOutputSystem(true)
{
}

CLANClient::~CLANClient()
{
}

CLANClient::Config::Config()
	: szServerIP{}
	, serverPort(0)
	, bUseNagle(false)
	, bUsePacketEncryption(false)
	, packetCode(0)
	, packetKey(0)
	, maxPacketSize(0)
{
}

CLANClient::Session::Session()
	: socket(NULL)
	, sendOverlapped{ 0, }
	, recvOverlapped{ 0, }
	, arrPtrPacket{ 0, }
	, numSendPacket(0)
	, sendIoFlag(false)
	, isClosed(false)
{
}

/* server */
bool CLANClient::StartUp(const wchar_t* serverIP, unsigned short serverPort, bool bUseNagle, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption)
{
	timeBeginPeriod(1);

	wcscpy_s(_config.szServerIP, wcslen(serverIP) + 1, serverIP);
	_config.serverPort = serverPort;
	_config.bUseNagle = bUseNagle;
	_config.packetCode = packetCode;
	_config.packetKey = packetKey;
	_config.maxPacketSize = maxPacketSize;
	_config.bUsePacketEncryption = bUsePacketEncryption;

	// WSAStartup
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		OnError(L"[LAN Client] Failed to initiate Winsock DLL. error:%u\n", WSAGetLastError());
		return false;
	}

	// IOCP ����
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	if (_hIOCP == NULL)
	{
		OnError(L"[LAN Client] Failed to create IOCP. error:%u\n", GetLastError());
		return false;
	}

	// ���� ����
	_session.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// SO_LINGER �ɼ� ����
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(_session.socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	// nagle �ɼ� ���� ����
	if (_config.bUseNagle == false)
	{
		DWORD optval = TRUE;
		int retSockopt = setsockopt(_session.socket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
		if (retSockopt == SOCKET_ERROR)
		{
			OnError(L"[LAN Client] Failed to set TCP_NODELAY option on listen socket. error:%u\n", WSAGetLastError());
		}
	}

	// ������ ����
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, _config.szServerIP, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(_config.serverPort);
	if (connect(_session.socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		OnError(L"[LAN Client] Failed to connect server. error:%u\n", WSAGetLastError());
		return false;
	}

	// ���ϰ� IOCP ����
	if (CreateIoCompletionPort((HANDLE)_session.socket, _hIOCP, 0, 0) == NULL)
	{
		OnError(L"[LAN Client] failed to associate socket with IOCP\n");
		closesocket(_session.socket);
		return false;
	}


	// worker ������ ����
	_thWorker.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadWorker, (PVOID)this, 0, &_thWorker.id);
	if (_thWorker.handle == NULL)
	{
		OnError(L"[LAN Client] An error occurred when starting the worker thread. error:%u\n", GetLastError());
		return false;
	}


	OnOutputSystem(L"[LAN Client] Start Up LAN Client\n"
		L"\tLAN Server IP : % s\n"
		L"\tLAN Server Port: %d\n"
		L"\tEnable Nagle: %d\n"
		L"\tUse Packet Encryption: %d\n"
		L"\tPacket Header Code: 0x%x\n"
		L"\tPacket Encode Key: 0x%x\n"
		L"\tMaximum Packet Size: %d\n"
		, _config.szServerIP
		, _config.serverPort
		, _config.bUseNagle
		, _config.bUsePacketEncryption
		, _config.packetCode
		, _config.packetKey
		, _config.maxPacketSize);


	// recv ����
	RecvPost();

	return true;
}





// ��Ʈ��ũ ����
void CLANClient::Shutdown()
{
	// �������� ������ ����
	Disconnect();

	// worker ������ ���� �޽����� ������.
	PostQueuedCompletionStatus(_hIOCP, 0, NULL, NULL);

	// worker �����尡 ����Ǳ⸦ 1�ʰ� ��ٸ���.
	ULONGLONG timeout = 1000;
	BOOL retWait = WaitForSingleObject(_thWorker.handle, (DWORD)timeout);
	if (retWait != WAIT_OBJECT_0)
	{
		LAN_OUTPUT_SYSTEM(L"[LAN Client] Timeout occurred while waiting for the thread to be terminated. force terminate it. error:%u\n", GetLastError());
		TerminateThread(_thWorker.handle, 0);
	}

	// ��ü ����
	CPacket* pPacket;
	while (_session.sendQ.Dequeue(pPacket) == false);
	CloseHandle(_thWorker.handle);
	CloseHandle(_hIOCP);

	// WSACleanup �� �ٸ� ��Ʈ��ũ�� ������ ��ĥ�� �ֱ� ������ ȣ������ ����.

}


/* packet */
CPacket& CLANClient::AllocPacket()
{
	CPacket* pPacket = CPacket::AllocPacket();
	pPacket->Init(sizeof(PacketHeader));
	return *pPacket;
}


bool CLANClient::SendPacket(CPacket& packet)
{
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
	_session.sendQ.Enqueue(&packet);

	// Send�� �õ���
	SendPost();

	return true;
}




bool CLANClient::SendPacketAsync(CPacket& packet)
{
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
	_session.sendQ.Enqueue(&packet);

	// ���� send�� ���������� Ȯ��
	if (_session.sendQ.Size() == 0) // ���� ��Ŷ�� ������ ����
	{
		return true;
	}
	else if ((bool)InterlockedExchange8((char*)&_session.sendIoFlag, true) == true) // ���� sendIoFlag�� false���� true�� �ٲپ��� ���� send��
	{
		return true;
	}

	// ���� send�� �������� �ƴ϶�� SendPost ��û�� IOCP ť�� ������
	BOOL retPost = PostQueuedCompletionStatus(_hIOCP, 0, 0, (LPOVERLAPPED)2);
	if (retPost == 0)
	{
		OnError(L"[LAN Client] Failed to post send request to IOCP. error:%u\n", GetLastError());
		return false;
	}

	return true;
}



// IOCP �Ϸ����� ť�� �۾��� �����Ѵ�. �۾��� ȹ��Ǹ� OnInnerRequest �Լ��� ȣ��ȴ�.
bool CLANClient::PostInnerRequest(CPacket& packet)
{
	BOOL ret = PostQueuedCompletionStatus(_hIOCP, 0, (ULONG_PTR)&packet, (LPOVERLAPPED)1);
	if (ret == 0)
	{
		OnError(L"[LAN Client] failed to post completion status to IOCP. error:%u\n", GetLastError());
		return false;
	}
	return true;
}



/* dynamic alloc */
// 64byte aligned ��ü ������ ���� new, delete overriding
void* CLANClient::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}

void CLANClient::operator delete(void* p)
{
	_aligned_free(p);
}




/* client */
bool CLANClient::Disconnect()
{
	// isClosed�� true�� ���ʷ� �ٲپ��� ���� ������
	if (InterlockedExchange8((char*)&_session.isClosed, true) == false)
	{
		// ������ �ݴ´�.
		closesocket(_session.socket);
		_session.socket = INVALID_SOCKET;
		_monitor.disconnectCount++;
	}

	return true;
}



/* Send, Recv */
void CLANClient::RecvPost()
{
	DWORD flag = 0;
	ZeroMemory(&_session.recvOverlapped, sizeof(OVERLAPPED_EX));
	_session.recvOverlapped.ioType = IO_RECV;

	WSABUF WSABuf[2];
	int directFreeSize = _session.recvQ.GetDirectFreeSize();
	WSABuf[0].buf = _session.recvQ.GetRear();
	WSABuf[0].len = directFreeSize;
	WSABuf[1].buf = _session.recvQ.GetBufPtr();
	WSABuf[1].len = _session.recvQ.GetFreeSize() - directFreeSize;

	int retRecv = WSARecv(_session.socket, WSABuf, 2, NULL, &flag, (OVERLAPPED*)&_session.recvOverlapped, NULL);
	_monitor.recvCount++;

	if (retRecv == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			if (error == WSAECONNRESET                                     // �������� ������ ����
				|| error == WSAECONNABORTED                                // Ŭ���̾�Ʈ���� ������ ����?
				|| (error == WSAENOTSOCK && _session.socket == INVALID_SOCKET))    // Ŭ���̾�Ʈ���� ������ �ݰ� ���ϰ��� INVALID_SOCKET���� ������
			{
				LAN_OUTPUT_DEBUG(L"[LAN Client] WSARecv failed by known error. error:%d\n", error);
				_monitor.WSARecvKnownError++;
			}
			else
			{
				LAN_OUTPUT_SYSTEM(L"[LAN Client] WSARecv failed by known error. error:%d\n", error);
				_monitor.WSARecvUnknownError++;
			}

			// ���� �߻��� ������ ������ ���´�.
			Disconnect();
			return;
		}
	}

	return;
}



// WSASend�� �õ��Ѵ�. 
void CLANClient::SendPost()
{
	if (_session.sendQ.Size() == 0) // ���� ��Ŷ�� ������ ����
	{
		return;
	}
	else if ((bool)InterlockedExchange8((char*)&_session.sendIoFlag, true) == true) // ���� sendIoFlag�� false���� true�� �ٲپ��� ���� send��
	{
		return;
	}

	// sendQ���� ����ȭ���� �ּҸ� ������ WSABUF ����ü�� �ִ´�.
	const int numMaxPacket = SIZE_ARR_PACKTE;
	WSABUF arrWSABuf[numMaxPacket];
	int numPacket = 0;
	for (int i = 0; i < numMaxPacket; i++)
	{
		if (_session.sendQ.Dequeue(_session.arrPtrPacket[i]) == false)
			break;

		arrWSABuf[i].buf = reinterpret_cast<CHAR*>(_session.arrPtrPacket[i]->GetHeaderPtr());
		arrWSABuf[i].len = _session.arrPtrPacket[i]->GetUseSize();
		numPacket++;
	}
	// ������ numSendPacket ����
	_session.numSendPacket = numPacket;
	// ���� �����Ͱ� ������ ����
	if (numPacket == 0)
	{
		_session.sendIoFlag = false;
		return;
	}

	// overlapped ����ü ����
	ZeroMemory(&_session.sendOverlapped, sizeof(OVERLAPPED_EX));
	_session.sendOverlapped.ioType = IO_SEND;

	// send
	int retSend = WSASend(_session.socket, arrWSABuf, numPacket, NULL, 0, (OVERLAPPED*)&_session.sendOverlapped, NULL);
	_monitor.sendCount;

	if (retSend == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			// Send �Ϸ������� �������� ���̹Ƿ� ���⼭ ����ȭ������ count�� ������.
			for (int i = 0; i < _session.numSendPacket; i++)
			{
				_session.arrPtrPacket[i]->SubUseCount();
			}

			_session.sendIoFlag = false;
			if (error == WSAECONNRESET                                              // �������� ������ ����
				|| error == WSAECONNABORTED                                         // Ŭ���̾�Ʈ���� ������ ����?
				|| (error == WSAENOTSOCK && _session.socket == INVALID_SOCKET))     // Ŭ���̾�Ʈ���� ������ �ݰ� ���ϰ��� INVALID_SOCKET���� ������
			{
				_monitor.WSASendKnownError;
				LAN_OUTPUT_DEBUG(L"[LAN Client] WSASend failed with known error. error:%d\n", error);
			}
			else
			{
				_monitor.WSASendUnknownError;
				LAN_OUTPUT_SYSTEM(L"[LAN Client] WSASend failed with unknown error. error:%d\n", error);
			}

			// ���� �߻��� Ŭ���̾�Ʈ�� ������ ���´�.
			Disconnect();
			return;
		}
	}

	return;
}


// SendPacketAsync �Լ��� ���� �񵿱� send ��û�� �޾��� �� worker ������ ������ ȣ��ȴ�. WSASend�� ȣ���Ѵ�.
void CLANClient::SendPostAsync()
{
	// �� �Լ��� ȣ��Ǵ� ������ SendPacketAsync �Լ����� sendIoFlag�� true�� ������ �����̴�. �׷��� sendIoFlag�� �˻����� �ʴ´�.

	// sendQ���� ����ȭ���� �ּҸ� ������ WSABUF ����ü�� �ִ´�.
	const int numMaxPacket = SIZE_ARR_PACKTE;
	WSABUF arrWSABuf[numMaxPacket];
	int numPacket = 0;
	for (int i = 0; i < numMaxPacket; i++)
	{
		if (_session.sendQ.Dequeue(_session.arrPtrPacket[i]) == false)
			break;

		arrWSABuf[i].buf = (CHAR*)_session.arrPtrPacket[i]->GetHeaderPtr();
		arrWSABuf[i].len = _session.arrPtrPacket[i]->GetUseSize();
		numPacket++;
	}
	// ������ numSendPacket ����
	_session.numSendPacket = numPacket;
	// ���� �����Ͱ� ������ ����
	if (numPacket == 0)
	{
		_session.sendIoFlag = false;
		return;
	}

	// overlapped ����ü ����
	ZeroMemory(&_session.sendOverlapped, sizeof(OVERLAPPED_EX));
	_session.sendOverlapped.ioType = IO_SEND;

	// send
	int retSend = WSASend(_session.socket, arrWSABuf, numPacket, NULL, 0, (OVERLAPPED*)&_session.sendOverlapped, NULL);
	_monitor.sendAsyncCount += numPacket;

	if (retSend == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			// Send �Ϸ������� �������� ���̹Ƿ� ���⼭ ����ȭ������ count�� ������.
			for (int i = 0; i < _session.numSendPacket; i++)
			{
				_session.arrPtrPacket[i]->SubUseCount();
			}

			_session.sendIoFlag = false;
			if (error == WSAECONNRESET                                              // �������� ������ ����
				|| error == WSAECONNABORTED                                         // Ŭ���̾�Ʈ���� ������ ����?
				|| (error == WSAENOTSOCK && _session.socket == INVALID_SOCKET))     // Ŭ���̾�Ʈ���� ������ �ݰ� ���ϰ��� INVALID_SOCKET���� ������
			{
				_monitor.WSASendKnownError++;
				LAN_OUTPUT_DEBUG(L"[LAN Client] WSASend failed with known error. error:%d\n", error);
			}
			else
			{
				_monitor.WSASendUnknownError++;
				LAN_OUTPUT_SYSTEM(L"[LAN Client] WSASend failed with unknown error. error:%d\n", error);
			}

			// ���� �߻��� Ŭ���̾�Ʈ�� ������ ���´�.
			Disconnect();
			return;
		}
	}

	return;
}




/* ��Ŷ ��ȣȭ */
void CLANClient::EncryptPacket(CPacket& packet)
{
	if (packet.IsEncoded() == true)
		return;

	PacketHeader* pHeader = reinterpret_cast<PacketHeader*>(packet.GetHeaderPtr());
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

bool CLANClient::DecipherPacket(CPacket& packet)
{
	PacketHeader* pHeader = reinterpret_cast<PacketHeader*>(packet.GetHeaderPtr());
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










// worker ������
unsigned WINAPI CLANClient::ThreadWorker(PVOID pParam)
{
	CLANClient& client = *(CLANClient*)pParam;
	if (client._bOutputSystem)
		client.OnOutputSystem(L"[LAN Client] worker thread begin. id:%u\n", GetCurrentThreadId());
	client.UpdateWorkerThread();

	if (client._bOutputSystem)
		client.OnOutputSystem(L"[LAN Client] worker thread end. id:%u\n", GetCurrentThreadId());

	return 0;
}

// worker ������ ������Ʈ
void CLANClient::UpdateWorkerThread()
{
	DWORD numByteTrans;
	ULONG_PTR compKey;
	OVERLAPPED_EX* pOverlapped;
	// IOCP �Ϸ����� ó��
	while (true)
	{
		numByteTrans = 0;
		compKey = NULL;
		pOverlapped = nullptr;
		BOOL retGQCS = GetQueuedCompletionStatus(_hIOCP, &numByteTrans, &compKey, (OVERLAPPED**)&pOverlapped, INFINITE);

		DWORD error = GetLastError();
		DWORD WSAError = WSAGetLastError();

		// IOCP dequeue ����, �Ǵ� timeout. �� ��� numByteTrans, compKey �������� ���� ������ �ʱ� ������ ����üũ�� ����� �� ����.
		if (retGQCS == 0 && pOverlapped == nullptr)
		{
			// timeout
			if (error == WAIT_TIMEOUT)
			{
				OnError(L"[LAN Client] GetQueuedCompletionStatus timeout. error:%u\n", error);
			}
			// error
			else
			{
				OnError(L"[LAN Client] GetQueuedCompletionStatus failed. error:%u\n", error);
			}
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
			SendPostAsync();
		}

		// recv �Ϸ����� ó��
		else if (pOverlapped->ioType == IO_RECV)
		{
			_monitor.recvCompletionCount++;

			bool bRecvSucceed = true;  // recv ���� ����
			// recv IO�� ������ ���
			if (retGQCS == 0)
			{
				bRecvSucceed = false;  // �߰�
				if (error == ERROR_NETNAME_DELETED     // ERROR_NETNAME_DELETED 64 �� WSAECONNRESET �� �����ϴ�?
					|| error == ERROR_CONNECTION_ABORTED  // ERROR_CONNECTION_ABORTED 1236 �� Ŭ���̾�Ʈ���� ������ ������ ��� �߻���. WSAECONNABORTED 10053 �� ����?
					|| error == ERROR_OPERATION_ABORTED)  // ERROR_OPERATION_ABORTED 995 �� �񵿱� IO�� �����߿� Ŭ���̾�Ʈ���� ������ ������ ��� �߻���.
				{
					LAN_OUTPUT_DEBUG(L"[LAN Client] recv socket error. error:%u, WSAError:%u, numByteTrans:%d\n", error, WSAError, numByteTrans);
					_monitor.disconnByKnownRecvIoError++;
				}
				else
				{
					LAN_OUTPUT_SYSTEM(L"[LAN Client] recv socket unknown error. error:%u, WSAError:%u, numByteTrans:%d\n", error, WSAError, numByteTrans);
					_monitor.disconnByUnknownRecvIoError++;
				}
			}

			// �����κ��� ������� �޽����� ���� ���
			else if (numByteTrans == 0)
			{
				bRecvSucceed = false;
				LAN_OUTPUT_DEBUG(L"[LAN Client] recv closed by client's close request.\n");
				_monitor.disconnByNormalProcess++;
			}

			// �������� �޽��� ó��
			else if (numByteTrans > 0)
			{
				// recvQ ���� ��� �޽����� ó���Ѵ�.
				char bufferHeader[sizeof(PacketHeader)];  // ����� ���� ����
				_session.recvQ.MoveRear(numByteTrans);
				while (true)
				{
					if (_session.recvQ.GetUseSize() < sizeof(PacketHeader)) // �����Ͱ� ������̺��� ����
						break;

					// ����� ����
					_session.recvQ.Peek(bufferHeader, sizeof(PacketHeader));
					PacketHeader header = *reinterpret_cast<PacketHeader*>(bufferHeader);
					if (header.code != _config.packetCode) // ��Ŷ �ڵ尡 �߸����� ��� error
					{
						LAN_OUTPUT_DEBUG(L"[LAN Client] header.code is not valid. code:%d\n", header.code);
						bRecvSucceed = false;
						_monitor.disconnByPacketCode++;
						break;
					}

					if (header.len > _config.maxPacketSize) // ������ ũ�Ⱑ �ִ� ��Ŷũ�⺸�� ū ��� error
					{
						LAN_OUTPUT_DEBUG(L"[LAN Client] header.len is larger than max packet size. len:%d, max packet size:%d\n", header.len, _config.maxPacketSize);
						bRecvSucceed = false;
						_monitor.disconnByPacketLength++;
						break;
					}

					if (sizeof(PacketHeader) + header.len > SIZE_RECV_BUFFER) // �������� ũ�Ⱑ ���� ũ�⺸�� Ŭ ��� error
					{
						LAN_OUTPUT_DEBUG(L"[LAN Client] packet data length is longer than recv buffer. len:%d, size of ringbuffer:%d\n"
							, header.len, _session.recvQ.GetSize());
						bRecvSucceed = false;
						_monitor.disconnByPacketLength++;
						break;
					}

					// ���۳��� ������ ũ�� Ȯ��
					if (_session.recvQ.GetUseSize() < sizeof(PacketHeader) + header.len) // �����Ͱ� ��� �������� �ʾ���
					{
						break;
					}

					// ����ȭ���� �غ�
					CPacket& recvPacket = AllocPacket();
					recvPacket.Init(sizeof(PacketHeader));

					// ����ȭ ���۷� �����͸� ����
					_session.recvQ.MoveFront(sizeof(PacketHeader));
					_session.recvQ.Dequeue(recvPacket.GetDataPtr(), header.len);
					recvPacket.MoveWritePos(header.len);

					// ��ȣȭ
					if (_config.bUsePacketEncryption == true)
					{
						recvPacket.PutHeader((const char*)&header); // ��ȣȭ�� ���� ����� �־���
						bool retDecipher = DecipherPacket(recvPacket);
						if (retDecipher == false) // ��ȣȭ�� ������(CheckSum�� �߸���)
						{
							LAN_OUTPUT_DEBUG(L"[LAN Client] packet decoding failed.\n");
							recvPacket.SubUseCount();
							bRecvSucceed = false;
							_monitor.disconnByPacketDecode++;
							break;
						}
					}

					LAN_OUTPUT_DEBUG(L"[LAN Client] recved. data len:%d, packet type:%d\n", header.len, *reinterpret_cast<WORD*>(recvPacket.GetDataPtr()));

					// ����� ��Ŷó�� �Լ� ȣ��
					OnRecv(recvPacket);

					// ���ī��Ʈ ����
					recvPacket.SubUseCount();

				}
			}

			// numByteTrans�� 0���� ����. error?
			else
			{
				bRecvSucceed = false;  // �߰�
				OnError(L"[LAN Client] recv error. NumberOfBytesTransferred is %d, error:%u, WSAError:%u\n", numByteTrans, error, WSAError);
				Crash();
			}

			// recv�� ���������� �������� �ٽ� recv �Ѵ�. ���������� closesocket
			if (bRecvSucceed)
				RecvPost();
			else
				Disconnect();
		}

		// send IO �Ϸ����� ó��
		else if (pOverlapped->ioType == IO_SEND)
		{
			_monitor.sendCompletionCount++;

			// send�� ����� ����ȭ������ ���ī��Ʈ�� ���ҽ�Ų��.
			for (int i = 0; i < _session.numSendPacket; i++)
			{
				long useCount = _session.arrPtrPacket[i]->SubUseCount();
			}

			// sendIoFlag �� �����Ѵ�.
			// ����ȭ������ ���ī��Ʈ�� ���ҽ�Ų���� sendIoFlag�� �����ؾ� _arrPtrPacket �� �����Ͱ� ������������� ����.
			_session.sendIoFlag = false;

			// send IO�� ������ ���
			if (retGQCS == 0)
			{
				if (error == ERROR_NETNAME_DELETED        // ERROR_NETNAME_DELETED 64 �� WSAECONNRESET �� �����ϴ�?
					|| error == ERROR_CONNECTION_ABORTED  // ERROR_CONNECTION_ABORTED 1236 �� Ŭ���̾�Ʈ���� ������ ������ ��� �߻���. WSAECONNABORTED 10053 �� ����?
					|| error == ERROR_OPERATION_ABORTED)  // ERROR_OPERATION_ABORTED 995 �� �񵿱� IO�� �����߿� Ŭ���̾�Ʈ���� ������ ������ ��� �߻���.
				{
					LAN_OUTPUT_DEBUG(L"[LAN Client] send socket known error. error:%u, WSAError:%u, numByteTrans:%d\n", error, WSAError, numByteTrans);
					_monitor.disconnByKnownSendIoError++;
				}
				else
				{
					LAN_OUTPUT_SYSTEM(L"[LAN Client] send socket unknown error. NumberOfBytesTransferred:%d, error:%u, WSAError:%u\n", numByteTrans, error, WSAError);
					_monitor.disconnByUnknownSendIoError++;
				}

				Disconnect();
			}
			// �Ϸ����� ó�� 
			else if (numByteTrans > 0)
			{
				// sendQ ���� ������ ������ �õ���
				SendPost();
			}
			// numByteTrans�� 0���� ����. error?
			else
			{
				OnError(L"[LAN Client] send error. NumberOfBytesTransferred is %d, error:%u, WSAError:%u\n", numByteTrans, error, WSAError);
				Disconnect();
				Crash();
			}
		}
		else
		{
			// overlapped ����ü�� IO Type�� �ùٸ��� ����
			OnError(L"[LAN Client] IO Type error. ioType:%d\n", pOverlapped->ioType);
			Disconnect();
			// crash
			Crash();
		}
	}
}




/* Crash */
void CLANClient::Crash()
{
	int* p = 0;
	*p = 0;
}











