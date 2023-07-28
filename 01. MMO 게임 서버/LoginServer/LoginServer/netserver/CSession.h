#pragma once


#define SESSION_SIZE_ARR_PACKET 50

namespace netserver
{

/* session */
class alignas(64) CSession
{
	friend class CNetServer;

private:
	union {  // 64byte aligned
		struct {
			volatile bool releaseFlag; // _releaseFlag ��ũ�η� ���� ����
			volatile long ioCount;     // _ioCount ��ũ�η� ���� ����
		} __s;
		volatile long long _ioCountAndReleaseFlag;
	};
	#define _ioCount     __s.ioCount
	#define _releaseFlag __s.releaseFlag

	alignas(64) CLockfreeQueue<CPacket*>* _sendQ;

	unsigned __int64 _sessionId;
	unsigned short _index;
	unsigned long _IP;
	wchar_t _szIP[16];
	unsigned short _port;

	SOCKET _sock;
	CRingbuffer _recvQ;
	OVERLAPPED_EX _sendOverlapped;
	OVERLAPPED_EX _recvOverlapped;
	CPacket** _arrPtrPacket;  // WSASend �Լ��� ����� ��Ŷ �ּҸ� �����صδ� �迭. SESSION_SIZE_ARR_PACKET ũ���̴�.
	int _numSendPacket;
	SOCKET _socketToBeFree;

	volatile bool _sendIoFlag;
	volatile bool _isClosed;
	volatile bool _bError;       // ���� �߻� ����
	volatile bool _bCloseWait;   // ���� ���. ��������� �Լ� ȣ��� true�� ��. true�� ��� _sendQ ���� ��� �����͸� send�� ���� ����ȴ�.
	CPacket* _lastPacket;        // _bCloseWait == true�� ���, �� ��Ŷ�� ���������� Ȯ�εǸ� ������ �ݴ´�.

	static __int64 _sNextSessionId;  // ���� ���� ID

public:
	CSession(unsigned short index);
	~CSession();

	void Init(SOCKET clntSock, SOCKADDR_IN& clntAddr);  // ���ο� ���� ��� �� ȣ��

	/* dynamic alloc */
	void* operator new(size_t size);
	void operator delete(void* p);
	void* operator new(size_t size, void* p); // for placement new
	void operator delete(void* p, void* p2);  // for placement delete

private:
	/* (static) ���� ���� ID ��� */
	static __int64 GetNextSessionId();

};


}