#pragma once
#include <queue>

constexpr int SESSION_SIZE_ARR_PACKET = 50;

namespace netlib_game
{

	class CPacket;
	class CContents;

	// ������ť ��� ��Ŷť
	class CPacketQueue
	{
	public:
		CPacketQueue()
		{
			InitializeSRWLock(&_srwl);
		}

		int Size() const { return (int)_queue.size(); }
		int GetPoolSize() const { return 0; }
		void Enqueue(CPacket* packet)
		{
			AcquireSRWLockExclusive(&_srwl);
			_queue.push(packet);
			ReleaseSRWLockExclusive(&_srwl);
		}
		bool Dequeue(CPacket*& packet)
		{
			AcquireSRWLockExclusive(&_srwl);
			if (_queue.size() == 0)
			{
				ReleaseSRWLockExclusive(&_srwl);
				return false;
			}
			else
			{
				packet = _queue.front();
				_queue.pop();
				ReleaseSRWLockExclusive(&_srwl);
				return true;
			}
		}

	private:
		std::queue<CPacket*> _queue;
		SRWLOCK _srwl;
	};


	/* session */
	class alignas(64) CSession
	{
		friend class CNetServer;
		friend class CContents;

	public:
		CSession(unsigned short index);
		~CSession();

		const wchar_t* GetIP() const { return _szIP; }
		unsigned short GetPort() const { return _port; }

		void Init(SOCKET clntSock, SOCKADDR_IN& clntAddr);  // ���ο� ���� ��� �� ȣ��
		void SetHeartBeatTime() { _lastHeartBeatTime = GetTickCount64(); }

		/* dynamic alloc */
		void* operator new(size_t size);
		void operator delete(void* p);
		void* operator new(size_t size, void* p); // for placement new
		void operator delete(void* p, void* p2);  // for placement delete

	private:
		/* (static) ���� ���� ID ��� */
		static __int64 GetNextSessionId();

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

		//alignas(64) CLockfreeQueue<CPacket*>* _sendQ;  // sendQ
		alignas(64) CPacketQueue* _sendQ;  // sendQ
		CLockfreeQueue<CPacket*>* _msgQ;   // �޽���ť

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
		volatile bool _bCloseWait;   // ���� ���. ���������� �Լ� ȣ��� true�� ��. true�� ��� _sendQ ���� ��� �����͸� send�� ���� ����ȴ�.
		const CPacket* _lastPacket;  // _bCloseWait == true�� ���, �� ��Ŷ�� ���������� Ȯ�εǸ� ������ �ݴ´�.

		static __int64 _sNextSessionId;  // ���� ���� ID

		/* ������ */
		bool _bContentsWaitToDisconnect;   // ������ �ڵ忡�� ���ǿ� ���� ���������� �Լ��� ȣ���
		ULONGLONG _lastHeartBeatTime;  // ���������� �޼����� ó���� �ð�
		std::vector<CPacket*> _vecContentsPacket;   // ���������� �����⸦ ��û�� ��Ŷ�� ��Ƶδ� ����

		/* ������ �̵� */
		struct Transfer
		{
			bool bOnTransfer;    // �ٸ� �������� �̵����� ��� true�� �ȴ�.
			std::shared_ptr<CContents> spDestination;  // �̵��� ������ �ּ�
			PVOID pData;              // �̵��� �� ������ ������
			Transfer() : bOnTransfer(false), spDestination(nullptr), pData(nullptr) {}
		};
		Transfer _transfer;
	};


}