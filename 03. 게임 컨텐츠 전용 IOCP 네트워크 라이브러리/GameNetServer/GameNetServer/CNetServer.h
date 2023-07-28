#pragma once


#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <unordered_map>

#include "CPacket.h"
#include "CRingbuffer.h"
#include "CLockfreeStack.h"
#include "CLockfreeQueue.h"
#include "CMemoryPoolTLS.h"

#include "defineNetwork.h"
#include "CContents.h"
#include "CSession.h"


namespace game_netserver
{


// OnOutputDebug �Լ� ȣ�� ��ũ��
#define NET_OUTPUT_DEBUG(szOutputFormat, ...) do { \
	if (_bOutputDebug) \
		OnOutputDebug(szOutputFormat, ##__VA_ARGS__); \
} while(0) 

// OnOutputSystem �Լ� ȣ�� ��ũ��
#define NET_OUTPUT_SYSTEM(szOutputFormat, ...) do { \
	if (_bOutputSystem) \
		OnOutputSystem(szOutputFormat, ##__VA_ARGS__); \
} while(0) 



class CSession;

/* Network */
class alignas(64) CNetServer
{
	friend class CContents;

private:
	/* server */
	HANDLE _hIOCP;
	SOCKET _listenSock;
	bool _bShutdown;
	volatile bool _bTrafficCongestion;  // Ʈ���� ȥ�⿩��
	bool _bOutputDebug;    // OnOutputDebug �Լ� Ȱ��ȭ����(default:false)
	bool _bOutputSystem;   // OnOutputSystem �Լ� Ȱ��ȭ����(default:true)

	/* config */
	wchar_t _szBindIP[16];
	ULONG _bindIP;
	unsigned short _portNumber;
	int _numConcurrentThread;
	int _numWorkerThread;
	int _numMaxSession;
	bool _bUseNagle;
	bool _bUsePacketEncryption;  // ��Ŷ ��ȣȭ ��� ����
	bool _bUseTrafficCongestionControl;  // Ʈ���� ȥ������ ��뿩��

	BYTE _packetCode;
	BYTE _packetKey;
	int  _maxPacketSize;

	/* thread */
	HANDLE _hThreadAccept;                           // accept ������
	unsigned int _idThreadAccept;
	std::vector<HANDLE> _vecHThreadWorker;           // worker ������
	std::vector<unsigned int> _vecIdThreadWorker;
	HANDLE _hThreadTrafficCongestionControl;         // Ʈ���� ȥ������ ������
	unsigned int _idThreadTrafficCongestionControl;
	HANDLE _hThreadDeferredCloseSocket;              // ������ ���ϴݱ� ������
	unsigned int _idThreadDeferredCloseSocket;

	/* deferred close socket thread */
	CLockfreeQueue<SOCKET> _msgQDeferredCloseSocket;  // ������ ���ϴݱ� ������ �޽���ť
	HANDLE _hEventDeferredCloseSocket;
	bool _bEventSetDeferredCloseSocket;

	/* session */
	CSession* _arrSession;
	CLockfreeStack<unsigned short> _stackSessionIdx;

	/* ������ */
	CContents* _initialContents;                         // ���� ���� ���ӽ� �Ҵ�� ������
	std::list<CContents*> _listContents;                 // ������ ��ü ������ ����Ʈ
	SRWLOCK _srwlListContents;                           // list lock
	CMemoryPoolTLS<MsgContents> _poolMsgContents;        // ������ �������� �޽���ť���� ����� �޽����� pool

	/* ����͸� */
	LARGE_INTEGER _liPerformanceFrequency;
	// worker ������ ���� TLS ����͸�(worker ������ �� ������ �迭. worker ������ ������ _stlsTlsIndex ������ �迭 index�� �����)
	struct _StWorkerTlsMonitor
	{
		volatile __int64 GQCSWaitTime = 0;           // GQCS�� ���ϵǱ⸦ ��ٸ� �ð�(performance frequency ����)
		volatile __int64 sendCompletionCount = 0;    // send �Ϸ����� ó��Ƚ��
		volatile __int64 recvCompletionCount = 0;    // recv �Ϸ����� ó��Ƚ��
		volatile __int64 recvCount = 0;              // recv �Ϸ��������� ó���� ��Ŷ ��
		volatile __int64 disconnByKnownRecvIoError = 0;    // recv �Ϸ��������� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
		volatile __int64 disconnByUnknownRecvIoError = 0;  // recv �Ϸ��������� ���� �߻�(���κҸ�)
		volatile __int64 disconnByKnownSendIoError = 0;    // send �Ϸ��������� ���� �߻�
		volatile __int64 disconnByUnknownSendIoError = 0;  // send �Ϸ��������� ���� �߻�(���κҸ�)
		volatile __int64 disconnBy121RecvIoError = 0;      // recv �Ϸ��������� ���� �߻�(ERROR_SEM_TIMEOUT 121 ����)
		volatile __int64 disconnByNormalProcess = 0;        // Ŭ���̾�Ʈ�� ���������� ������ ����
		volatile __int64 disconnByPacketCode = 0;    // ��Ŷ�ڵ尡 �߸���
		volatile __int64 disconnByPacketLength = 0;  // ��Ŷ���̰� �߸���
		volatile __int64 disconnByPacketDecode = 0;  // ��Ŷ ��ȣȭ�� ������
	};
	_StWorkerTlsMonitor* _arrWorkerTlsMonitor;

	// accept ������ ����͸�
	volatile __int64 _acceptCount = 0;            // accept Ƚ��
	volatile __int64 _connectCount = 0;           // connect Ƚ�� (accept �� connect ���ε� Ƚ��)
	volatile __int64 _disconnBySessionLimit = 0;  // ���Ǽ� �������� ���� ����
	volatile __int64 _disconnByIOCPAssociation = 0;  // IOCP�� ���� ���迡 �����Ͽ� ����
	volatile __int64 _disconnByOnConnReq = 0;     // OnConnectionRequest �Լ��� ���� ����

	// ���̺귯�� �Լ� ���� ����͸�
	alignas(64) volatile __int64 _sendCount = 0;        // SendPost �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
	alignas(64) volatile __int64 _sendAsyncCount = 0;   // SendPostAsync �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
	volatile __int64 _disconnectCount = 0;        // disconnect Ƚ�� (���� release Ƚ��)
	volatile __int64 _deferredDisconnectCount = 0;  // ������ disconnect Ƚ��
	volatile __int64 _WSARecvKnownError = 0;      // WSARecv �Լ����� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
	volatile __int64 _WSARecvUnknownError = 0;    // WSARecv �Լ����� ���� �߻�(���κҸ�)
	volatile __int64 _WSASendKnownError = 0;      // WSASend �Լ����� ���� �߻�
	volatile __int64 _WSASendUnknownError = 0;    // WSASend �Լ����� ���� �߻�(���κҸ�)
	volatile __int64 _otherErrors = 0;            // ��Ÿ ����

	// Ʈ���� ȥ������ ������ ����͸�
	__int64 _trafficCongestionControlCount = 0;


public:
	CNetServer();
	virtual ~CNetServer();

public:
	/* Set */
	void SetOutputDebug(bool enable) { _bOutputDebug = enable; }    // OnOutputDebug �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:��Ȱ��)
	void SetOutputSystem(bool enable) { _bOutputSystem = enable; }  // OnOutputSystem �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:Ȱ��)

	/* Get ���� ���� */
	int GetNumWorkerThread() { return _numWorkerThread; }
	int GetNumSession() { return _numMaxSession - _stackSessionIdx.Size(); }
	int GetPacketPoolSize() { return CPacket::GetPoolSize(); };
	int GetPacketAllocCount() { return CPacket::GetAllocCount(); }
	int GetPacketActualAllocCount() { return CPacket::GetActualAllocCount(); }
	int GetPacketFreeCount() { return CPacket::GetFreeCount(); }
	int GetMsgPoolSize() { return _poolMsgContents.GetPoolSize(); }
	int GetMsgAllocCount() { return _poolMsgContents.GetAllocCount(); }
	int GetMsgActualAllocCount() { return _poolMsgContents.GetActualAllocCount(); }
	int GetMsgFreeCount() { return _poolMsgContents.GetFreeCount(); }
	int GetSizeMsgQDeferredCloseSocket() { return _msgQDeferredCloseSocket.Size(); }

	/* Get ����͸� */
	__int64 GetAcceptCount() { return _acceptCount; }     // accept Ƚ��
	__int64 GetConnectCount() { return _connectCount; }   // connect Ƚ�� (accept �� connect ���ε� Ƚ��)
	__int64 GetDisconnBySessionLimit() { return _disconnBySessionLimit; }     // ���Ǽ� �������� ���� ����
	__int64 GetDisconnByIOCPAssociation() { return _disconnByIOCPAssociation; }  // IOCP�� ���� ���迡 �����Ͽ� ����
	__int64 GetDisconnByOnConnectionRequest() { return _disconnByOnConnReq; } // OnConnectionRequest �Լ��� ���� ����

	void GetArrGQCSWaitTime(int* arrTime);     // worker �����庰 GQCS ��ٸ��ð�
	__int64 GetSendCompletionCount();          // send �Ϸ����� ó��Ƚ��
	__int64 GetRecvCompletionCount();          // recv �Ϸ����� ó��Ƚ��
	__int64 GetRecvCount();                    // recv �Ϸ��������� ó���� ��Ŷ ��
	__int64 GetDisconnByKnownIoError();        // WSARecv, WSASend, recv �Ϸ�����, send �Ϸ��������� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
	__int64 GetDisconnByUnknownIoError();      // WSARecv, WSASend, recv �Ϸ�����, send �Ϸ��������� ���� �߻�(���κҸ�)
	__int64 GetDisconnBy121RecvIoError();      // recv �Ϸ��������� 121(ERROR_SEM_TIMEOUT) ���� �߻�
	__int64 GetDisconnByNormalProcess();       // Ŭ���̾�Ʈ�� ���������� ������ ����
	__int64 GetDisconnByPacketCode();          // ��Ŷ�ڵ尡 �߸���
	__int64 GetDisconnByPacketLength();        // ��Ŷ���̰� �߸���
	__int64 GetDisconnByPacketDecode();        // ��Ŷ ��ȣȭ�� ������

	__int64 GetSendCount() { return _sendCount + _sendAsyncCount; } // WSASend �Լ��� ������ ��Ŷ�� ��
	__int64 GetDisconnectCount() { return _disconnectCount; }       // disconnect Ƚ�� (���� release Ƚ��)
	__int64 GetDeferredDisconnectCount() { return _deferredDisconnectCount; }   // ������ disconnect Ƚ��
	__int64 GetOtherErrorCount() { return _otherErrors; }                     // ��Ÿ ���� Ƚ��
	__int64 GetTrafficCongestionControlCount() { return _trafficCongestionControlCount; }  // Ʈ���� ȥ������ Ƚ��

	__int64 GetContentsInternalMsgCount();     // ������ ���� �޽��� ó��Ƚ��
	__int64 GetContentsSessionMsgCount();      // ������ ���� �޽��� ó��Ƚ��
	__int64 GetContentsDisconnByHeartBeat();   // ������ ���ο��� ��Ʈ��Ʈ Ÿ�Ӿƿ����� ���� ���Ǽ�


	/* server */
	bool StartUp(const wchar_t* bindIP, unsigned short port, int numConcurrentThread, int numWorkerThread, bool bUseNagle, int numMaxSession, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption, bool bUseTrafficCongestionControl);
	void StopAccept();  // accept ����
	void Shutdown();    // ���� ����(accept ���� ����)


	/* packet */
	CPacket* AllocPacket();
	bool SendPacket(__int64 sessionId, CPacket* pPacket);  // send�� ����� ����� �� ���� ���� ����� ����ǰ�, �׷��� ���� ��� �񵿱�� ����ȴ�.
	bool SendPacket(__int64 sessionId, CPacket** arrPtrPacket, size_t numPacket);
	bool SendPacketAsync(__int64 sessionId, CPacket* pPacket);  // send�� �ݵ�� �񵿱�� ����ȴ�.
	bool SendPacketAsync(__int64 sessionId, CPacket** arrPtrPacket, size_t numPacket);
	bool SendPacketAndDisconnect(__int64 sessionId, CPacket* pPacket);       // send�� �Ϸ�� �� ������ ���´�.
	bool SendPacketAndDisconnect(__int64 sessionId, CPacket** arrPtrPacket, size_t numPacket);
	bool SendPacketAsyncAndDisconnect(__int64 sessionId, CPacket* pPacket);  // �񵿱� send�� ��û�ϰ�, send�� �Ϸ�Ǹ� ������ ���´�.
	bool SendPacketAsyncAndDisconnect(__int64 sessionId, CPacket** arrPtrPacket, size_t numPacket);
	bool PostInnerRequest(CPacket* pPacket);   // IOCP �Ϸ����� ť�� �۾��� �����Ѵ�. �۾��� ȹ��Ǹ� OnInnerRequest �Լ��� ȣ��ȴ�.

	/* session */
	bool Disconnect(__int64 sessionId);
	
	/* ������ */
	void SetInitialContents(CContents* pContents);   // ������ ���� ���ӽ� �Ҵ�� �������� �����Ѵ�.
	bool AttachContents(CContents* pContents);       // �������� ���̺귯���� �߰��ϰ� �����带 �����Ѵ�.
	void DetachContents(CContents* pContents);       // �������� ���̺귯������ �����ϰ� �����带 �����Ѵ�.
	MsgContents* AllocMsg() { return _poolMsgContents.Alloc(); }

	/* dynamic alloc */
	// 64byte aligned ��ü ������ ���� new, delete overriding
	void* operator new(size_t size);
	void operator delete(void* p);

private:
	/* Send, Recv */
	void RecvPost(CSession* pSession);   // WSARecv�� �Ѵ�.	
	void SendPost(CSession* pSession);   // WSASend�� �õ��Ѵ�. 
	void SendPostAsync(CSession* pSession);  // SendPacketAsync �Լ��� ���� �񵿱� send ��û�� �޾��� �� worker ������ ������ ȣ��ȴ�. WSASend�� ȣ���Ѵ�.


	/* ���� */
	CSession* FindSession(__int64 sessionId);  // ����: FindSession �Լ��� ������ ����� ��� ���� ��� �Ŀ� �ݵ�� DecreaseIoCount �Լ��� ȣ���ؾ���
	void IncreaseIoCount(CSession* pSession);
	void DecreaseIoCount(CSession* pSession);
	CSession* AllocSession(SOCKET clntSock, SOCKADDR_IN& clntAddr);
	void CloseSocket(CSession* pSession);  // ������ �ݴ´�(������ free������ ����).
	void ReleaseSession(CSession* pSession);
	bool Disconnect(CSession* pSession);

	/* ��Ŷ ��ȣȭ */
	void EncryptPacket(CPacket* pPacket);
	bool DecipherPacket(CPacket* pPacket);

	/* thread */
	static unsigned WINAPI ThreadAccept(PVOID pParam);
	static unsigned WINAPI ThreadWorker(PVOID pParam);
	static unsigned WINAPI ThreadTrafficCongestionControl(PVOID pParam);
	static unsigned WINAPI ThreadDeferredCloseSocket(PVOID pParam);
	void UpdateAcceptThread();
	void UpdateWorkerThread();
	void UpdateTrafficCongestionControlThread();
	void UpdateThreadDeferredCloseSocketThread();

	/* Crash */
	void Crash();

private:
	/* ���̺귯�� callback �Լ� */
	virtual bool OnInnerRequest(CPacket& packet) { return false; }
	virtual bool OnConnectionRequest(unsigned long IP, unsigned short port) = 0;
	virtual void OnError(const wchar_t* szError, ...) = 0;            // ���� �߻�
	virtual void OnOutputDebug(const wchar_t* szError, ...) = 0;      // debug ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_DEBUG ��ũ�� ����� ������.
	virtual void OnOutputSystem(const wchar_t* szError, ...) = 0;     // system ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_SYSTEM ��ũ�� ����� ������.


};



}