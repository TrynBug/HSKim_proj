#pragma once


#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <unordered_map>
#include <shared_mutex>

#include "CPacket.h"
#include "../utils/CRingbuffer.h"
#include "../utils/CLockfreeStack.h"
#include "../utils/CLockfreeQueue.h"
#include "../utils/CMemoryPoolTLS.h"

#include "defineNetwork.h"
#include "CTimeMgr.h"
#include "CContents.h"
#include "CSession.h"

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


namespace netlib_game
{
	class CNetServerAccessor;
	class CSession;

	/* Network */
	class alignas(64) CNetServer
	{
	public:
		friend class CNetServerAccessor;
		class Monitor;

	public:
		CNetServer();
		virtual ~CNetServer();

		/* dynamic alloc */
		// 64byte aligned ��ü ������ ���� new, delete overriding
		void* operator new(size_t size);
		void operator delete(void* p);

	public:
		/* server */
		bool StartUp(const wchar_t* bindIP, unsigned short port, int numConcurrentThread, int numWorkerThread, bool bUseNagle, int numMaxSession, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption, bool bUseTrafficCongestionControl);
		void StopAccept();  // accept ����
		void Shutdown();    // ���� ����(accept ���� ����)


		/* packet */
		CPacket& AllocPacket();
		bool SendPacket(__int64 sessionId, CPacket& packet);  // send�� ����� ����� �� ���� ���� ����� ����ǰ�, �׷��� ���� ��� �񵿱�� ����ȴ�.
		bool SendPacket(__int64 sessionId, const std::vector<CPacket*>& vecPacket);
		bool SendPacketAsync(__int64 sessionId, CPacket& packet);  // send�� �ݵ�� �񵿱�� ����ȴ�.
		bool SendPacketAsync(__int64 sessionId, const std::vector<CPacket*>& vecPacket);
		bool SendPacketAndDisconnect(__int64 sessionId, CPacket& packet);       // send�� �Ϸ�� �� ������ ���´�.
		bool SendPacketAndDisconnect(__int64 sessionId, const std::vector<CPacket*>& vecPacket);
		bool SendPacketAsyncAndDisconnect(__int64 sessionId, CPacket& packet);  // �񵿱� send�� ��û�ϰ�, send�� �Ϸ�Ǹ� ������ ���´�.
		bool SendPacketAsyncAndDisconnect(__int64 sessionId, const std::vector<CPacket*>& vecPacket);
		bool PostInnerRequest(CPacket& packet);   // IOCP �Ϸ����� ť�� �۾��� �����Ѵ�. �۾��� ȹ��Ǹ� OnInnerRequest �Լ��� ȣ��ȴ�.

		/* session */
		bool Disconnect(__int64 sessionId);
	
		/* ������ */
		void SetInitialContents(std::shared_ptr<CContents> spContents);   // ������ ���� ���ӽ� �Ҵ�� �������� �����Ѵ�.
		bool AttachContents(std::shared_ptr<CContents> spContents);       // �������� ���̺귯���� �߰��ϰ� �����带 �����Ѵ�.
		void DetachContents(std::shared_ptr<CContents> spContents);       // �������� ���̺귯������ �����ϰ� �����带 �����Ѵ�.
		MsgContents& AllocMsg() { return *_poolMsgContents.Alloc(); }

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
		void EncryptPacket(CPacket& packet);
		bool DecipherPacket(CPacket& packet);

		/* thread */
		static unsigned WINAPI ThreadAccept(PVOID pParam);
		static unsigned WINAPI ThreadWorker(PVOID pParam);
		static unsigned WINAPI ThreadTrafficCongestionControl(PVOID pParam);
		static unsigned WINAPI ThreadDeferredCloseSocket(PVOID pParam);
		void RunAcceptThread();
		void RunWorkerThread();
		void RunTrafficCongestionControlThread();
		void RunThreadDeferredCloseSocketThread();

		/* Crash */
		void Crash();

	private:
		/* ���̺귯�� callback �Լ� */
		virtual bool OnInnerRequest(CPacket& packet) { return false; }
		virtual bool OnConnectionRequest(unsigned long IP, unsigned short port) = 0;
		virtual void OnError(const wchar_t* szError, ...) = 0;            // ���� �߻�
		virtual void OnOutputDebug(const wchar_t* szError, ...) = 0;      // debug ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_DEBUG ��ũ�� ����� ������.
		virtual void OnOutputSystem(const wchar_t* szError, ...) = 0;     // system ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_SYSTEM ��ũ�� ����� ������.


	public:
		/* Set */
		void SetOutputDebug(bool enable) { _bOutputDebug = enable; }    // OnOutputDebug �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:��Ȱ��)
		void SetOutputSystem(bool enable) { _bOutputSystem = enable; }  // OnOutputSystem �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:Ȱ��)

		/* Get ���� ���� */
		int GetNumWorkerThread() const { return _config.numWorkerThread; }
		int GetNumSession() const { return _config.numMaxSession - _stackSessionIdx.Size(); }
		int GetPacketPoolSize() const { return CPacket::GetPoolSize(); };
		int GetPacketAllocCount() const { return CPacket::GetAllocCount(); }
		int GetPacketActualAllocCount() const { return CPacket::GetActualAllocCount(); }
		int GetPacketFreeCount() const { return CPacket::GetFreeCount(); }
		int GetMsgPoolSize() const { return _poolMsgContents.GetPoolSize(); }
		int GetMsgAllocCount() const { return _poolMsgContents.GetAllocCount(); }
		int GetMsgActualAllocCount() const { return _poolMsgContents.GetActualAllocCount(); }
		int GetMsgFreeCount() const { return _poolMsgContents.GetFreeCount(); }
		int GetSizeMsgQDeferredCloseSocket() const { return _thDeferredCloseSocket.msgQ.Size(); }

		__int64 GetContentsInternalMsgCount() const;     // ������ ���� �޽��� ó��Ƚ��
		__int64 GetContentsSessionMsgCount() const;      // ������ ���� �޽��� ó��Ƚ��
		__int64 GetContentsDisconnByHeartBeat() const;   // ������ ���ο��� ��Ʈ��Ʈ Ÿ�Ӿƿ����� ���� ���Ǽ�

		/* Get ����͸� count */
		const Monitor& GetMonitor() { return _monitor; }
		const CNetServer::Monitor& GetNetworkMonitor() { return CNetServer::_monitor; }

	private:
		/* server */
		HANDLE _hIOCP;
		SOCKET _listenSock;
		bool _bShutdown;
		volatile bool _bTrafficCongestion;  // Ʈ���� ȥ�⿩��
		bool _bOutputDebug;    // OnOutputDebug �Լ� Ȱ��ȭ����(default:false)
		bool _bOutputSystem;   // OnOutputSystem �Լ� Ȱ��ȭ����(default:true)
		LARGE_INTEGER _liPerformanceFrequency;

		/* network config */
		struct Config
		{
			wchar_t szBindIP[16];
			ULONG bindIP;
			unsigned short portNumber;
			int numConcurrentThread;
			int numWorkerThread;
			int numMaxSession;
			bool bUseNagle;
			bool bUsePacketEncryption;  // ��Ŷ ��ȣȭ ��� ����
			bool bUseTrafficCongestionControl;  // Ʈ���� ȥ������ ��뿩��

			BYTE packetCode;
			BYTE packetKey;
			int  maxPacketSize;

			Config();
		};
		Config _config;

		/* thread info */
		struct ThreadInfo
		{
			HANDLE handle;
			unsigned int id;
			ThreadInfo() :handle(NULL), id(0) {}
		};
		ThreadInfo _thAccept;
		ThreadInfo _thTrafficCongestionControl;
		std::vector<ThreadInfo> _vecThWorker;

		/* deferred close socket thread */
		struct ThreadInfoDeferredCloseSocket
		{
			HANDLE handle;
			unsigned int id;
			CLockfreeQueue<SOCKET> msgQ;  // �޽���ť
			HANDLE hEvent;
			bool bEventSet;
			ThreadInfoDeferredCloseSocket() :handle(NULL), id(0), hEvent(NULL), bEventSet(false) {}
		};
		ThreadInfoDeferredCloseSocket _thDeferredCloseSocket;

		/* session */
		CSession* _arrSession;
		CLockfreeStack<unsigned short> _stackSessionIdx;

		/* ������ */
		std::shared_ptr<CContents> _spInitialContents;       // ���� ���� ���ӽ� �Ҵ�� ������
		std::list<std::shared_ptr<CContents>> _listContents; // ������ ��ü ������ ����Ʈ
		mutable SRWLOCK _srwlListContents;                   // list lock
		CMemoryPoolTLS<MsgContents> _poolMsgContents;        // ������ �������� �޽���ť���� ����� �޽����� pool

	public:
		/* ����͸� counter */
		class alignas(64) Monitor
		{
			friend class CNetServer;
		private:
			// ���̺귯�� �Լ� ���� ����͸�
			alignas(64) std::atomic<__int64> sendCount = 0;        // SendPost �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
			alignas(64) std::atomic<__int64> sendAsyncCount = 0;   // SendPostAsync �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
			std::atomic<__int64> disconnectCount = 0;        // disconnect Ƚ�� (���� release Ƚ��)
			std::atomic<__int64> deferredDisconnectCount = 0;  // ������ disconnect Ƚ��
			std::atomic<__int64> WSARecvKnownError = 0;      // WSARecv �Լ����� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
			std::atomic<__int64> WSARecvUnknownError = 0;    // WSARecv �Լ����� ���� �߻�(���κҸ�)
			std::atomic<__int64> WSASendKnownError = 0;      // WSASend �Լ����� ���� �߻�
			std::atomic<__int64> WSASendUnknownError = 0;    // WSASend �Լ����� ���� �߻�(���κҸ�)
			std::atomic<__int64> otherErrors = 0;            // ��Ÿ ����

			// accept ������ ����͸�
			volatile __int64 acceptCount = 0;            // accept Ƚ��
			volatile __int64 connectCount = 0;           // connect Ƚ�� (accept �� connect ���ε� Ƚ��)
			volatile __int64 disconnBySessionLimit = 0;  // ���Ǽ� �������� ���� ����
			volatile __int64 disconnByIOCPAssociation = 0;  // IOCP�� ���� ���迡 �����Ͽ� ����
			volatile __int64 disconnByOnConnReq = 0;     // OnConnectionRequest �Լ��� ���� ����

			// Ʈ���� ȥ������ ������ ����͸�
			volatile __int64 trafficCongestionControlCount = 0;

			// worker ������ ���� TLS ����͸�
			struct TLSMonitor
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
		private:
			std::vector<TLSMonitor> _vecTlsMonitor;
			mutable SRWLOCK _srwlVecTlsMonitor_mutable;
			LARGE_INTEGER _liPerformanceFrequency;

		private:
			Monitor();
			TLSMonitor& CreateTlsMonitor();

		public:
			/* Get ����͸� count */
			__int64 GetAcceptCount() const { return acceptCount; }     // accept Ƚ��
			__int64 GetConnectCount() const { return connectCount; }   // connect Ƚ�� (accept �� connect ���ε� Ƚ��)
			__int64 GetDisconnBySessionLimit() const { return disconnBySessionLimit; }     // ���Ǽ� �������� ���� ����
			__int64 GetDisconnByIOCPAssociation() const { return disconnByIOCPAssociation; }  // IOCP�� ���� ���迡 �����Ͽ� ����
			__int64 GetDisconnByOnConnectionRequest() const { return disconnByOnConnReq; } // OnConnectionRequest �Լ��� ���� ����

			__int64 GetSendCompletionCount() const;          // send �Ϸ����� ó��Ƚ��
			__int64 GetRecvCompletionCount() const;          // recv �Ϸ����� ó��Ƚ��
			__int64 GetRecvCount() const;                    // recv �Ϸ��������� ó���� ��Ŷ ��
			__int64 GetDisconnByKnownIoError() const;        // WSARecv, WSASend, recv �Ϸ�����, send �Ϸ��������� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
			__int64 GetDisconnByUnknownIoError() const;      // WSARecv, WSASend, recv �Ϸ�����, send �Ϸ��������� ���� �߻�(���κҸ�)
			__int64 GetDisconnBy121RecvIoError() const;      // recv �Ϸ��������� 121(ERROR_SEM_TIMEOUT) ���� �߻�
			__int64 GetDisconnByNormalProcess() const;       // Ŭ���̾�Ʈ�� ���������� ������ ����
			__int64 GetDisconnByPacketCode() const;          // ��Ŷ�ڵ尡 �߸���
			__int64 GetDisconnByPacketLength() const;        // ��Ŷ���̰� �߸���
			__int64 GetDisconnByPacketDecode() const;        // ��Ŷ ��ȣȭ�� ������

			__int64 GetSendCount() const { return sendCount + sendAsyncCount; } // WSASend �Լ��� ������ ��Ŷ�� ��
			__int64 GetDisconnectCount() const { return disconnectCount; }       // disconnect Ƚ�� (���� release Ƚ��)
			__int64 GetDeferredDisconnectCount() const { return deferredDisconnectCount; }   // ������ disconnect Ƚ��
			__int64 GetOtherErrorCount() const { return otherErrors; }                     // ��Ÿ ���� Ƚ��
			__int64 GetTrafficCongestionControlCount() const { return trafficCongestionControlCount; }  // Ʈ���� ȥ������ Ƚ��
		};
		Monitor _monitor;



	};



	// CNetServer�� �Ϻ� ��ɿ��� ���ٰ����ϵ��� ���� Ŭ����. CContents Ŭ�������� ����Ѵ�.
	class CNetServerAccessor
	{
		friend class CNetServer;

	private:
		CNetServerAccessor(CNetServer* pNetServer);   // �� Ŭ������ �ν��Ͻ��� CNetServer �κ��͸� ������ �� �ִ�.
	public:
		~CNetServerAccessor();

	public:
		/* packet */
		CPacket& AllocPacket() { return _pNetServer->AllocPacket(); }
		bool SendPacket(__int64 sessionId, CPacket& packet) { return _pNetServer->SendPacket(sessionId, packet); }
		bool SendPacket(__int64 sessionId, const std::vector<CPacket*>& vecPacket) { return _pNetServer->SendPacket(sessionId, vecPacket); }
		bool SendPacketAsync(__int64 sessionId, CPacket& packet) { return _pNetServer->SendPacketAsync(sessionId, packet); }
		bool SendPacketAsync(__int64 sessionId, const std::vector<CPacket*>& vecPacket) { return _pNetServer->SendPacketAsync(sessionId, vecPacket); }
		bool SendPacketAndDisconnect(__int64 sessionId, CPacket& packet) { return _pNetServer->SendPacketAndDisconnect(sessionId, packet); }
		bool SendPacketAndDisconnect(__int64 sessionId, const std::vector<CPacket*>& vecPacket) { return SendPacketAndDisconnect(sessionId, vecPacket); }
		bool SendPacketAsyncAndDisconnect(__int64 sessionId, CPacket& packet) { return _pNetServer->SendPacketAsyncAndDisconnect(sessionId, packet); }
		bool SendPacketAsyncAndDisconnect(__int64 sessionId, const std::vector<CPacket*>& vecPacket) { return _pNetServer->SendPacketAsyncAndDisconnect(sessionId, vecPacket); }
		bool PostInnerRequest(CPacket& packet) { return _pNetServer->PostInnerRequest(packet); }

		/* msg*/
		MsgContents& AllocMsg() { return _pNetServer->AllocMsg(); }
		void FreeMsg(MsgContents& msg) { return _pNetServer->_poolMsgContents.Free(&msg); }
		bool Disconnect(CSession* pSession) { return _pNetServer->Disconnect(pSession); }

		/* session */
		void IncreaseIoCount(CSession* pSession) { return _pNetServer->IncreaseIoCount(pSession); }
		void DecreaseIoCount(CSession* pSession) { return _pNetServer->DecreaseIoCount(pSession); }


	private:
		CNetServer* _pNetServer;
	};




}