#include "CNetServer.h"

using namespace game_netserver;

// static ��� �ʱ�ȭ
__int64 CSession::_sNextSessionId = 1;

CSession::CSession(unsigned short index)
	: _sessionId(-1), _index(index), _IP(0), _szIP{ 0, }, _port(0)
	, _sock(INVALID_SOCKET)
	, _recvQ(SIZE_RECV_BUFFER)
	, _recvOverlapped{ 0, }, _sendOverlapped{ 0, }, _arrPtrPacket(nullptr)
	, _numSendPacket(0), _socketToBeFree(INVALID_SOCKET), _ioCountAndReleaseFlag(0)
	, _sendIoFlag(false), _isClosed(true), _bError(false), _bCloseWait(false), _lastPacket(nullptr)
	, _bOnTransfer(false), _pDestinationContents(nullptr), _pTransferData(nullptr)
	, _lastHeartBeatTime(0)
{
	_arrPtrPacket = new CPacket * [SESSION_SIZE_ARR_PACKET];
	ZeroMemory(_arrPtrPacket, sizeof(CPacket*)* SESSION_SIZE_ARR_PACKET);

	//_sendQ = new CLockfreeQueue<CPacket*>;
	_sendQ = new CPacketQueue;
	_msgQ = new CLockfreeQueue<CPacket*>;

}

CSession::~CSession()
{
	delete[] _arrPtrPacket;
	delete _sendQ;
}

void CSession::Init(SOCKET clntSock, SOCKADDR_IN& clntAddr)
{
	_sessionId = GetNextSessionId() << 16 | (__int64)_index;

	_sock = clntSock;
	_IP = clntAddr.sin_addr.S_un.S_addr;
	InetNtop(clntAddr.sin_family, &_IP, _szIP, sizeof(_szIP) / sizeof(wchar_t));
	_port = clntAddr.sin_port;
	_socketToBeFree = INVALID_SOCKET;

	// sendQ�� ���� sendQ ���� ��� ����ȭ������ ���ī��Ʈ�� ���ҽ�Ų��.
	CPacket* pPacket;
	while (_sendQ->Dequeue(pPacket) == true)
	{
		pPacket->SubUseCount();
	}

	// ���� �޽���ť�� ���� ����ȭ������ ���ī��Ʈ�� ���ҽ�Ų��.
	while (_msgQ->Dequeue(pPacket) == true)
	{
		pPacket->SubUseCount();
	}

	// �ʱ�ȭ
	_recvQ.Clear();
	ZeroMemory(&_recvOverlapped, sizeof(_recvOverlapped));
	ZeroMemory(&_sendOverlapped, sizeof(_sendOverlapped));
	_sendIoFlag = false;
	_isClosed = false;
	_releaseFlag = false;
	_bError = false;
	_bCloseWait = false;
	_lastPacket = nullptr;
	// _ioCount�� �ʱ�ȭ�� �ϸ� �ȵǴµ�, FindSession �� �� ������ ����ִ��� �ƴ��� Ȯ���ϱ� �� ioCount�� ���� �ø��� Ȯ���ϱ� �����̴�. ������ ������� �ʴٸ� ioCount�� �ٽ� ������.

	_bOnTransfer = false;
	_bContentsWaitToDisconnect = false;
	_pDestinationContents = nullptr;
	_pTransferData = nullptr;
	_lastHeartBeatTime = GetTickCount64();
	_vecContentsPacket.clear();

}



/* dynamic alloc */
void* CSession::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}

void CSession::operator delete(void* p)
{
	_aligned_free(p);
}

void* CSession::operator new(size_t size, void* p) // for placement new
{
	return p;
}

void CSession::operator delete(void* p, void* p2)  // for placement delete
{
	_aligned_free(p);
}


/* (static) ���� ���� ID ��� */
__int64 CSession::GetNextSessionId()
{
	return InterlockedIncrement64(&_sNextSessionId);
}
