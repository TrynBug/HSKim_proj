#pragma once

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <unordered_map>

#include "CPacket.h"
#include "../utils/CRingbuffer.h"
#include "../utils/CLockfreeStack.h"
#include "../utils/CLockfreeQueue.h"

#include "defineNetwork.h"


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

namespace netlib
{
	class CSession;

	/* IOCP Network Server */
	class alignas(64) CNetServer
	{
	public:
		class Monitor;

	public:
		CNetServer();
		virtual ~CNetServer();
		
		CNetServer(const CNetServer&) = delete;
		CNetServer(CNetServer&&) = delete;

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
		bool SendPacketAsync(__int64 sessionId, CPacket& packet);  // send�� �ݵ�� �񵿱�� ����ȴ�.
		bool SendPacketAndDisconnect(__int64 sessionId, CPacket& packet);       // send�� �Ϸ�� �� ������ ���´�.
		bool SendPacketAsyncAndDisconnect(__int64 sessionId, CPacket& packet);  // �񵿱� send�� ��û�ϰ�, send�� �Ϸ�Ǹ� ������ ���´�.
		bool PostInnerRequest(CPacket& packet);   // IOCP �Ϸ����� ť�� �۾��� �����Ѵ�. �۾��� ȹ��Ǹ� OnInnerRequest �Լ��� ȣ��ȴ�.

		/* session */
		bool Disconnect(__int64 sessionId);


	private:
		/* ���̺귯�� callback �Լ� */
		virtual bool OnRecv(__int64 sessionId, CPacket& packet) = 0;
		virtual bool OnInnerRequest(CPacket& packet) { return false; }
		virtual bool OnConnectionRequest(unsigned long IP, unsigned short port) = 0;
		virtual bool OnClientJoin(__int64 sessionId) = 0;
		virtual bool OnClientLeave(__int64 sessionId) = 0;
		virtual void OnError(const wchar_t* szError, ...) = 0;            // ���� �߻�
		virtual void OnOutputDebug(const wchar_t* szError, ...) = 0;      // debug ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_DEBUG ��ũ�� ����� ������.
		virtual void OnOutputSystem(const wchar_t* szError, ...) = 0;     // system ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_SYSTEM ��ũ�� ����� ������.


	public:
		/* Set */
		void SetOutputDebug(bool enable) { _bOutputDebug = enable; }    // OnOutputDebug �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:��Ȱ��)
		void SetOutputSystem(bool enable) { _bOutputSystem = enable; }  // OnOutputSystem �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:Ȱ��)

	public:
		/* Get ���� ���� */
		int GetNumWorkerThread() const { return _config.numWorkerThread; }
		int GetNumSession() const { return _config.numMaxSession - _stackSessionIdx.Size(); }
		int GetPacketPoolSize() const { return CPacket::GetPoolSize(); };
		int GetPacketAllocCount() const { return CPacket::GetAllocCount(); }
		int GetPacketActualAllocCount() const { return CPacket::GetActualAllocCount(); }
		int GetPacketFreeCount() const { return CPacket::GetFreeCount(); }
		int GetSizeMsgQDeferredCloseSocket() const { return _thDeferredCloseSocket.msgQ.Size(); }
		
		/* Get ����͸� count */
		const Monitor& GetMonitor() { return _monitor; }

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

		/* ��Ŷ ��ȣȭ */
		void EncryptPacket(CPacket& packet);
		bool DecipherPacket(CPacket& packet);

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
		/* server */
		HANDLE _hIOCP;
		SOCKET _listenSock;
		bool _bShutdown;
		volatile bool _bTrafficCongestion;  // Ʈ���� ȥ�⿩��
		bool _bOutputDebug;    // OnOutputDebug �Լ� Ȱ��ȭ����(default:false)
		bool _bOutputSystem;   // OnOutputSystem �Լ� Ȱ��ȭ����(default:true)

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


	public:
		/* ����͸� counter */
		class alignas(64) Monitor
		{
			friend class CNetServer;
		private:
			// ���̺귯�� �Լ� ���� ����͸�
			std::atomic<__int64> sendCount = 0;        // SendPost �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
			alignas(64) std::atomic<__int64> sendAsyncCount = 0;   // SendPostAsync �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
			alignas(64) std::atomic<__int64> disconnectCount = 0;        // disconnect Ƚ�� (���� release Ƚ��)
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

}