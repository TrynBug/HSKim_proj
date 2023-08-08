#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "CLANClient.h"


CLANClient::CLANClient()
	: _hIOCP(NULL), _socket(NULL)
	, _szServerIP{}, _serverPort(0), _bUseNagle(false), _bUsePacketEncryption(false)
	, _packetCode(0), _packetKey(0), _maxPacketSize(0)
	, _hThreadWorker(NULL), _idThreadWorker(0)
	, _sendOverlapped{ 0, }, _recvOverlapped{ 0, }, _arrPtrPacket{ 0, }, _numSendPacket(0)
	, _sendIoFlag(false), _isClosed(false)
	, _bOutputDebug(false), _bOutputSystem(true)
{
}

CLANClient::~CLANClient()
{

}

/* server */
bool CLANClient::StartUp(const wchar_t* serverIP, unsigned short serverPort, bool bUseNagle, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption)
{
	timeBeginPeriod(1);

	wcscpy_s(_szServerIP, wcslen(serverIP) + 1, serverIP);
	_serverPort = serverPort;
	_bUseNagle = bUseNagle;
	_packetCode = packetCode;
	_packetKey = packetKey;
	_maxPacketSize = maxPacketSize;
	_bUsePacketEncryption = bUsePacketEncryption;

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
	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// SO_LINGER �ɼ� ����
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setsockopt(_socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	// nagle �ɼ� ���� ����
	if (_bUseNagle == false)
	{
		DWORD optval = TRUE;
		int retSockopt = setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));
		if (retSockopt == SOCKET_ERROR)
		{
			OnError(L"[LAN Client] Failed to set TCP_NODELAY option on listen socket. error:%u\n", WSAGetLastError());
		}
	}

	// ������ ����
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, _szServerIP, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(_serverPort);
	if (connect(_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		OnError(L"[LAN Client] Failed to connect server. error:%u\n", WSAGetLastError());
		return false;
	}

	// ���ϰ� IOCP ����
	if (CreateIoCompletionPort((HANDLE)_socket, _hIOCP, 0, 0) == NULL)
	{
		OnError(L"[LAN Client] failed to associate socket with IOCP\n");
		closesocket(_socket);
		return false;
	}


	// worker ������ ����
	_hThreadWorker = (HANDLE)_beginthreadex(NULL, 0, ThreadWorker, (PVOID)this, 0, &_idThreadWorker);
	if (_hThreadWorker == NULL)
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
		, _szServerIP
		, _serverPort
		, _bUseNagle
		, _bUsePacketEncryption
		, _packetCode
		, _packetKey
		, _maxPacketSize);


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
	BOOL retWait = WaitForSingleObject(_hThreadWorker, (DWORD)timeout);
	if (retWait != WAIT_OBJECT_0)
	{
		LAN_OUTPUT_SYSTEM(L"[LAN Client] Timeout occurred while waiting for the thread to be terminated. force terminate it. error:%u\n", GetLastError());
		TerminateThread(_hThreadWorker, 0);
	}

	// ��ü ����
	CPacket* pPacket;
	while (_sendQ.Dequeue(pPacket) == false);
	CloseHandle(_hThreadWorker);
	CloseHandle(_hIOCP);
	// WSACleanup �� �ٸ� ��Ʈ��ũ�� ������ ��ĥ�� �ֱ� ������ ȣ������ ����.

}


/* packet */
CPacket* CLANClient::AllocPacket()
{
	CPacket* pPacket = CPacket::AllocPacket();
	pPacket->Init(sizeof(LANPacketHeader));
	return pPacket;
}


bool CLANClient::SendPacket(CPacket* pPacket)
{
	LANPacketHeader header;
	if (pPacket->IsHeaderSet() == false)
	{
		// ��� ����
		header.code = _packetCode;
		header.len = pPacket->GetDataSize();
		header.randKey = (BYTE)rand();

		char* packetDataPtr;
		if (_bUsePacketEncryption == true)
		{
			packetDataPtr = pPacket->GetDataPtr();  // CheckSum ���
			header.checkSum = 0;
			for (int i = 0; i < header.len; i++)
				header.checkSum += packetDataPtr[i];
		}

		// ����ȭ���ۿ� ��� �Է�
		pPacket->PutHeader((char*)&header);
	}

	// ��Ŷ ��ȣȭ
	if (_bUsePacketEncryption == true)
		EncryptPacket(pPacket);

	// send lcokfree queue�� ����ȭ���� �Է�
	pPacket->AddUseCount();
	_sendQ.Enqueue(pPacket);

	// Send�� �õ���
	SendPost();

	return true;
}




bool CLANClient::SendPacketAsync(CPacket* pPacket)
{
	LANPacketHeader header;
	if (pPacket->IsHeaderSet() == false)
	{
		// ��� ����
		header.code = _packetCode;
		header.len = pPacket->GetDataSize();
		header.randKey = (BYTE)rand();

		char* packetDataPtr;
		if (_bUsePacketEncryption == true)
		{
			packetDataPtr = pPacket->GetDataPtr();  // CheckSum ���
			header.checkSum = 0;
			for (int i = 0; i < header.len; i++)
				header.checkSum += packetDataPtr[i];
		}

		// ����ȭ���ۿ� ��� �Է�
		pPacket->PutHeader((char*)&header);
	}

	// ��Ŷ ��ȣȭ
	if (_bUsePacketEncryption == true)
		EncryptPacket(pPacket);

	// send lcokfree queue�� ����ȭ���� �Է�
	pPacket->AddUseCount();
	_sendQ.Enqueue(pPacket);

	// ���� send�� ���������� Ȯ��
	if (_sendQ.Size() == 0) // ���� ��Ŷ�� ������ ����
	{
		return true;
	}
	else if ((bool)InterlockedExchange8((char*)&_sendIoFlag, true) == true) // ���� sendIoFlag�� false���� true�� �ٲپ��� ���� send��
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
bool CLANClient::PostInnerRequest(CPacket* pPacket)
{
	BOOL ret = PostQueuedCompletionStatus(_hIOCP, 0, (ULONG_PTR)pPacket, (LPOVERLAPPED)1);
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
	if (InterlockedExchange8((char*)&_isClosed, true) == false)
	{
		// ������ �ݴ´�.
		closesocket(_socket);
		_socket = INVALID_SOCKET;
		InterlockedIncrement64(&_disconnectCount);  // ����͸�
	}

	return true;
}



/* Send, Recv */
void CLANClient::RecvPost()
{
	DWORD flag = 0;
	ZeroMemory(&_recvOverlapped, sizeof(LAN_OVERLAPPED_EX));
	_recvOverlapped.ioType = LAN_IO_RECV;

	WSABUF WSABuf[2];
	int directFreeSize = _recvQ.GetDirectFreeSize();
	WSABuf[0].buf = _recvQ.GetRear();
	WSABuf[0].len = directFreeSize;
	WSABuf[1].buf = _recvQ.GetBufPtr();
	WSABuf[1].len = _recvQ.GetFreeSize() - directFreeSize;

	int retRecv = WSARecv(_socket, WSABuf, 2, NULL, &flag, (OVERLAPPED*)&_recvOverlapped, NULL);
	InterlockedIncrement64(&_recvCount); // ����͸�

	if (retRecv == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			if (error == WSAECONNRESET                                     // �������� ������ ����
				|| error == WSAECONNABORTED                                // Ŭ���̾�Ʈ���� ������ ����?
				|| (error == WSAENOTSOCK && _socket == INVALID_SOCKET))    // Ŭ���̾�Ʈ���� ������ �ݰ� ���ϰ��� INVALID_SOCKET���� ������
			{
				LAN_OUTPUT_DEBUG(L"[LAN Client] WSARecv failed by known error. error:%d\n", error);
				InterlockedIncrement64(&_WSARecvKnownError);  // ����͸�
			}
			else
			{
				LAN_OUTPUT_SYSTEM(L"[LAN Client] WSARecv failed by known error. error:%d\n", error);
				InterlockedIncrement64(&_WSARecvUnknownError);  // ����͸�
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
	if (_sendQ.Size() == 0) // ���� ��Ŷ�� ������ ����
	{
		return;
	}
	else if ((bool)InterlockedExchange8((char*)&_sendIoFlag, true) == true) // ���� sendIoFlag�� false���� true�� �ٲپ��� ���� send��
	{
		return;
	}

	// sendQ���� ����ȭ���� �ּҸ� ������ WSABUF ����ü�� �ִ´�.
	const int numMaxPacket = LAN_CLIENT_SIZE_ARR_PACKTE;
	WSABUF arrWSABuf[numMaxPacket];
	int numPacket = 0;
	for (int i = 0; i < numMaxPacket; i++)
	{
		if (_sendQ.Dequeue(_arrPtrPacket[i]) == false)
			break;

		arrWSABuf[i].buf = (CHAR*)_arrPtrPacket[i]->GetHeaderPtr();
		arrWSABuf[i].len = _arrPtrPacket[i]->GetUseSize();
		numPacket++;
	}
	// ������ numSendPacket ����
	_numSendPacket = numPacket;
	// ���� �����Ͱ� ������ ����
	if (numPacket == 0)
	{
		_sendIoFlag = false;
		return;
	}

	// overlapped ����ü ����
	ZeroMemory(&_sendOverlapped, sizeof(LAN_OVERLAPPED_EX));
	_sendOverlapped.ioType = LAN_IO_SEND;

	// send
	int retSend = WSASend(_socket, arrWSABuf, numPacket, NULL, 0, (OVERLAPPED*)&_sendOverlapped, NULL);
	InterlockedAdd64(&_sendCount, numPacket);  // ����͸�

	if (retSend == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			// Send �Ϸ������� �������� ���̹Ƿ� ���⼭ ����ȭ������ count�� ������.
			for (int i = 0; i < _numSendPacket; i++)
			{
				_arrPtrPacket[i]->SubUseCount();
			}

			_sendIoFlag = false;
			if (error == WSAECONNRESET                                              // �������� ������ ����
				|| error == WSAECONNABORTED                                         // Ŭ���̾�Ʈ���� ������ ����?
				|| (error == WSAENOTSOCK && _socket == INVALID_SOCKET))     // Ŭ���̾�Ʈ���� ������ �ݰ� ���ϰ��� INVALID_SOCKET���� ������
			{
				InterlockedIncrement64(&_WSASendKnownError);  // ����͸�
				LAN_OUTPUT_DEBUG(L"[LAN Client] WSASend failed with known error. error:%d\n", error);
			}
			else
			{
				InterlockedIncrement64(&_WSASendUnknownError); // ����͸�
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
	const int numMaxPacket = LAN_CLIENT_SIZE_ARR_PACKTE;
	WSABUF arrWSABuf[numMaxPacket];
	int numPacket = 0;
	for (int i = 0; i < numMaxPacket; i++)
	{
		if (_sendQ.Dequeue(_arrPtrPacket[i]) == false)
			break;

		arrWSABuf[i].buf = (CHAR*)_arrPtrPacket[i]->GetHeaderPtr();
		arrWSABuf[i].len = _arrPtrPacket[i]->GetUseSize();
		numPacket++;
	}
	// ������ numSendPacket ����
	_numSendPacket = numPacket;
	// ���� �����Ͱ� ������ ����
	if (numPacket == 0)
	{
		_sendIoFlag = false;
		return;
	}

	// overlapped ����ü ����
	ZeroMemory(&_sendOverlapped, sizeof(LAN_OVERLAPPED_EX));
	_sendOverlapped.ioType = LAN_IO_SEND;

	// send
	int retSend = WSASend(_socket, arrWSABuf, numPacket, NULL, 0, (OVERLAPPED*)&_sendOverlapped, NULL);
	InterlockedAdd64(&_sendAsyncCount, numPacket);  // ����͸�

	if (retSend == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			// Send �Ϸ������� �������� ���̹Ƿ� ���⼭ ����ȭ������ count�� ������.
			for (int i = 0; i < _numSendPacket; i++)
			{
				_arrPtrPacket[i]->SubUseCount();
			}

			_sendIoFlag = false;
			if (error == WSAECONNRESET                                              // �������� ������ ����
				|| error == WSAECONNABORTED                                         // Ŭ���̾�Ʈ���� ������ ����?
				|| (error == WSAENOTSOCK && _socket == INVALID_SOCKET))     // Ŭ���̾�Ʈ���� ������ �ݰ� ���ϰ��� INVALID_SOCKET���� ������
			{
				InterlockedIncrement64(&_WSASendKnownError);  // ����͸�
				LAN_OUTPUT_DEBUG(L"[LAN Client] WSASend failed with known error. error:%d\n", error);
			}
			else
			{
				InterlockedIncrement64(&_WSASendUnknownError); // ����͸�
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
void CLANClient::EncryptPacket(CPacket* pPacket)
{
	if (pPacket->IsEncoded() == true)
		return;

	LANPacketHeader* pHeader = (LANPacketHeader*)pPacket->GetHeaderPtr();
	char* pTargetData = pPacket->GetDataPtr() - 1;  // checksum�� �����Ͽ� ��ȣȭ

	BYTE valP = 0;
	BYTE prevData = 0;
	for (int i = 0; i < pHeader->len + 1; i++)
	{
		valP = pTargetData[i] ^ (valP + pHeader->randKey + i + 1);
		pTargetData[i] = valP ^ (prevData + _packetKey + i + 1);
		prevData = pTargetData[i];
	}

	pPacket->SetEncoded();
}

bool CLANClient::DecipherPacket(CPacket* pPacket)
{
	LANPacketHeader* pHeader = (LANPacketHeader*)pPacket->GetHeaderPtr();
	char* pTargetData = pPacket->GetDataPtr() - 1;  // checksum�� �����Ͽ� ��ȣȭ

	BYTE valP = 0;
	BYTE prevData = 0;
	BYTE prevValP = 0;
	for (int i = 0; i < pHeader->len + 1; i++)
	{
		valP = pTargetData[i] ^ (prevData + _packetKey + i + 1);
		prevData = pTargetData[i];
		pTargetData[i] = valP ^ (prevValP + pHeader->randKey + i + 1);
		prevValP = valP;
	}

	pPacket->SetDecoded();

	// CheckSum ���
	char* payloadPtr = pPacket->GetDataPtr();
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
	LAN_OVERLAPPED_EX* pOverlapped;
	CPacket* pRecvPacket;
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
		else if (pOverlapped == (LAN_OVERLAPPED_EX*)1)
		{
			CPacket* pInnerPacket = (CPacket*)compKey;
			OnInnerRequest(*pInnerPacket);
		}

		// SendPacketAsync �Լ��� ���ؼ� SnedPost ��û�� ����
		else if (pOverlapped == (LAN_OVERLAPPED_EX*)2)
		{
			SendPostAsync();
		}

		// recv �Ϸ����� ó��
		else if (pOverlapped->ioType == LAN_IO_RECV)
		{
			_recvCompletionCount++;

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
					_disconnByKnownRecvIoError++;
				}
				else
				{
					LAN_OUTPUT_SYSTEM(L"[LAN Client] recv socket unknown error. error:%u, WSAError:%u, numByteTrans:%d\n", error, WSAError, numByteTrans);
					_disconnByUnknownRecvIoError++;
				}
			}

			// �����κ��� ������� �޽����� ���� ���
			else if (numByteTrans == 0)
			{
				bRecvSucceed = false;
				LAN_OUTPUT_DEBUG(L"[LAN Client] recv closed by client's close request.\n");
				_disconnByNormalProcess++;
			}

			// �������� �޽��� ó��
			else if (numByteTrans > 0)
			{
				// recvQ ���� ��� �޽����� ó���Ѵ�.
				char bufferHeader[sizeof(LANPacketHeader)];  // ����� ���� ����
				_recvQ.MoveRear(numByteTrans);
				while (true)
				{
					if (_recvQ.GetUseSize() < sizeof(LANPacketHeader)) // �����Ͱ� ������̺��� ����
						break;

					// ����� ����
					_recvQ.Peek(bufferHeader, sizeof(LANPacketHeader));
					LANPacketHeader header = *(LANPacketHeader*)bufferHeader;          // ? ���⼭ header ������ ���۳����� �ٽ�����ִ� ������??
					if (header.code != _packetCode) // ��Ŷ �ڵ尡 �߸����� ��� error
					{
						LAN_OUTPUT_DEBUG(L"[LAN Client] header.code is not valid. code:%d\n", header.code);
						bRecvSucceed = false;
						_disconnByPacketCode++;
						break;
					}

					if (header.len > _maxPacketSize) // ������ ũ�Ⱑ �ִ� ��Ŷũ�⺸�� ū ��� error
					{
						LAN_OUTPUT_DEBUG(L"[LAN Client] header.len is larger than max packet size. len:%d, max packet size:%d\n", header.len, _maxPacketSize);
						bRecvSucceed = false;
						_disconnByPacketLength++;
						break;
					}

					if (sizeof(LANPacketHeader) + header.len > LAN_SIZE_RECV_BUFFER) // �������� ũ�Ⱑ ���� ũ�⺸�� Ŭ ��� error
					{
						LAN_OUTPUT_DEBUG(L"[LAN Client] packet data length is longer than recv buffer. len:%d, size of ringbuffer:%d\n"
							, header.len, _recvQ.GetSize());
						bRecvSucceed = false;
						_disconnByPacketLength++;
						break;
					}

					// ���۳��� ������ ũ�� Ȯ��
					if (_recvQ.GetUseSize() < sizeof(LANPacketHeader) + header.len) // �����Ͱ� ��� �������� �ʾ���
					{
						break;
					}

					// ����ȭ���� �غ�
					pRecvPacket = CPacket::AllocPacket();
					pRecvPacket->Init(sizeof(LANPacketHeader));

					// ����ȭ ���۷� �����͸� ����
					_recvQ.MoveFront(sizeof(LANPacketHeader));
					_recvQ.Dequeue(pRecvPacket->GetDataPtr(), header.len);
					pRecvPacket->MoveWritePos(header.len);

					// ��ȣȭ
					if (_bUsePacketEncryption == true)
					{
						pRecvPacket->PutHeader((const char*)&header); // ��ȣȭ�� ���� ����� �־���
						bool retDecipher = DecipherPacket(pRecvPacket);
						if (retDecipher == false) // ��ȣȭ�� ������(CheckSum�� �߸���)
						{
							LAN_OUTPUT_DEBUG(L"[LAN Client] packet decoding failed.\n");
							pRecvPacket->SubUseCount();
							bRecvSucceed = false;
							_disconnByPacketDecode++;
							break;
						}
					}

					LAN_OUTPUT_DEBUG(L"[LAN Client] recved. data len:%d, packet type:%d\n", header.len, *(WORD*)pRecvPacket->GetDataPtr());

					// ����� ��Ŷó�� �Լ� ȣ��
					OnRecv(*pRecvPacket);

					// ���ī��Ʈ ����
					pRecvPacket->SubUseCount();

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
		else if (pOverlapped->ioType == LAN_IO_SEND)
		{
			_sendCompletionCount++;

			// send�� ����� ����ȭ������ ���ī��Ʈ�� ���ҽ�Ų��.
			for (int i = 0; i < _numSendPacket; i++)
			{
				long useCount = _arrPtrPacket[i]->SubUseCount();
			}

			// sendIoFlag �� �����Ѵ�.
			// ����ȭ������ ���ī��Ʈ�� ���ҽ�Ų���� sendIoFlag�� �����ؾ� _arrPtrPacket �� �����Ͱ� ������������� ����.
			_sendIoFlag = false;

			// send IO�� ������ ���
			if (retGQCS == 0)
			{
				if (error == ERROR_NETNAME_DELETED        // ERROR_NETNAME_DELETED 64 �� WSAECONNRESET �� �����ϴ�?
					|| error == ERROR_CONNECTION_ABORTED  // ERROR_CONNECTION_ABORTED 1236 �� Ŭ���̾�Ʈ���� ������ ������ ��� �߻���. WSAECONNABORTED 10053 �� ����?
					|| error == ERROR_OPERATION_ABORTED)  // ERROR_OPERATION_ABORTED 995 �� �񵿱� IO�� �����߿� Ŭ���̾�Ʈ���� ������ ������ ��� �߻���.
				{
					LAN_OUTPUT_DEBUG(L"[LAN Client] send socket known error. error:%u, WSAError:%u, numByteTrans:%d\n", error, WSAError, numByteTrans);
					_disconnByKnownSendIoError++;
				}
				else
				{
					LAN_OUTPUT_SYSTEM(L"[LAN Client] send socket unknown error. NumberOfBytesTransferred:%d, error:%u, WSAError:%u\n", numByteTrans, error, WSAError);
					_disconnByUnknownSendIoError++;
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











