#pragma once

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "CPacket.h"
#include "../utils/CRingbuffer.h"
#include "../utils/CLockfreeStack.h"
#include "../utils/CLockfreeQueue.h"

#include "defineNetwork.h"


// OnOutputDebug 함수 호출 매크로
#define NET_OUTPUT_DEBUG(szOutputFormat, ...) do { \
	if (_bOutputDebug) \
		OnOutputDebug(szOutputFormat, ##__VA_ARGS__); \
} while(0) 

// OnOutputSystem 함수 호출 매크로
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
		// 64byte aligned 객체 생성을 위한 new, delete overriding
		void* operator new(size_t size);
		void operator delete(void* p);

	public:
		/* server */
		bool StartUp(const wchar_t* bindIP, unsigned short port, int numConcurrentThread, int numWorkerThread, bool bUseNagle, int numMaxSession, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption, bool bUseTrafficCongestionControl);
		void StopAccept();  // accept 종료
		void Shutdown();    // 서버 종료(accept 종료 포함)

		/* packet */
		CPacket& AllocPacket();
		bool SendPacket(__int64 sessionId, CPacket& packet);  // send가 동기로 수행될 수 있을 때는 동기로 수행되고, 그렇지 않을 경우 비동기로 수행된다.
		bool SendPacketAsync(__int64 sessionId, CPacket& packet);  // send가 반드시 비동기로 수행된다.
		bool SendPacketAndDisconnect(__int64 sessionId, CPacket& packet);       // send가 완료된 후 연결을 끊는다.
		bool SendPacketAsyncAndDisconnect(__int64 sessionId, CPacket& packet);  // 비동기 send를 요청하고, send가 완료되면 연결을 끊는다.
		bool PostInnerRequest(CPacket& packet);   // IOCP 완료통지 큐에 작업을 삽입한다. 작업이 획득되면 OnInnerRequest 함수가 호출된다.

		/* session */
		bool Disconnect(__int64 sessionId);


	private:
		/* 라이브러리 callback 함수 */
		virtual bool OnRecv(__int64 sessionId, CPacket& packet) = 0;
		virtual bool OnInnerRequest(CPacket& packet) { return false; }
		virtual bool OnConnectionRequest(unsigned long IP, unsigned short port) = 0;
		virtual bool OnClientJoin(__int64 sessionId) = 0;
		virtual bool OnClientLeave(__int64 sessionId) = 0;
		virtual void OnError(const wchar_t* szError, ...) = 0;            // 오류 발생
		virtual void OnOutputDebug(const wchar_t* szError, ...) = 0;      // debug 문자열을 함수로 전달. 이 함수를 직접 호출하지말고 NET_OUTPUT_DEBUG 매크로 사용을 권장함.
		virtual void OnOutputSystem(const wchar_t* szError, ...) = 0;     // system 문자열을 함수로 전달. 이 함수를 직접 호출하지말고 NET_OUTPUT_SYSTEM 매크로 사용을 권장함.


	public:
		/* Set */
		void SetOutputDebug(bool enable) { _bOutputDebug = enable; }    // OnOutputDebug 함수 활성화, 비활성화 (기본값:비활성)
		void SetOutputSystem(bool enable) { _bOutputSystem = enable; }  // OnOutputSystem 함수 활성화, 비활성화 (기본값:활성)

	public:
		/* Get 서버 상태 */
		int GetNumWorkerThread() const { return _config.numWorkerThread; }
		int GetNumSession() const { return _config.numMaxSession - _stackSessionIdx.Size(); }
		int GetPacketPoolSize() const { return CPacket::GetPoolSize(); };
		int GetPacketAllocCount() const { return CPacket::GetAllocCount(); }
		int GetPacketActualAllocCount() const { return CPacket::GetActualAllocCount(); }
		int GetPacketFreeCount() const { return CPacket::GetFreeCount(); }
		int GetSizeMsgQDeferredCloseSocket() const { return _thDeferredCloseSocket.msgQ.Size(); }
		
		/* Get 모니터링 count */
		const Monitor& GetMonitor() { return _monitor; }

	private:
		/* Send, Recv */
		void RecvPost(CSession* pSession);   // WSARecv를 한다.	
		void SendPost(CSession* pSession);   // WSASend를 시도한다. 
		void SendPostAsync(CSession* pSession);  // SendPacketAsync 함수를 통해 비동기 send 요청을 받았을 때 worker 스레드 내에서 호출된다. WSASend를 호출한다.

		/* 세션 */
		CSession* FindSession(__int64 sessionId);  // 주의: FindSession 함수로 세션을 얻었을 경우 세션 사용 후에 반드시 DecreaseIoCount 함수를 호출해야함
		void IncreaseIoCount(CSession* pSession);
		void DecreaseIoCount(CSession* pSession);
		CSession* AllocSession(SOCKET clntSock, SOCKADDR_IN& clntAddr);
		void CloseSocket(CSession* pSession);  // 소켓을 닫는다(세션을 free하지는 않음).
		void ReleaseSession(CSession* pSession);

		/* 패킷 암호화 */
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
		volatile bool _bTrafficCongestion;  // 트래픽 혼잡여부
		bool _bOutputDebug;    // OnOutputDebug 함수 활성화여부(default:false)
		bool _bOutputSystem;   // OnOutputSystem 함수 활성화여부(default:true)
		bool _bShutdown;
		std::mutex _mtxShutdown;

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
			bool bUsePacketEncryption;  // 패킷 암호화 사용 여부
			bool bUseTrafficCongestionControl;  // 트래픽 혼잡제어 사용여부

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
			CLockfreeQueue<SOCKET> msgQ;  // 메시지큐
			HANDLE hEvent;
			bool bEventSet;
			ThreadInfoDeferredCloseSocket() :handle(NULL), id(0), hEvent(NULL), bEventSet(false) {}
		};
		ThreadInfoDeferredCloseSocket _thDeferredCloseSocket;

		/* session */
		CSession* _arrSession;
		CLockfreeStack<unsigned short> _stackSessionIdx;


	public:
		/* 모니터링 counter */
		class alignas(64) Monitor
		{
			friend class CNetServer;
		private:
			// 라이브러리 함수 내부 모니터링
			std::atomic<__int64> sendCount = 0;        // SendPost 함수에서 WSASend 함수로 전송한 패킷의 수
			alignas(64) std::atomic<__int64> sendAsyncCount = 0;   // SendPostAsync 함수에서 WSASend 함수로 전송한 패킷의 수
			alignas(64) std::atomic<__int64> disconnectCount = 0;        // disconnect 횟수 (세션 release 횟수)
			std::atomic<__int64> deferredDisconnectCount = 0;  // 지연된 disconnect 횟수
			std::atomic<__int64> WSARecvKnownError = 0;      // WSARecv 함수에서 오류 발생(클라이언트 또는 서버에서 강제로 끊었을때)
			std::atomic<__int64> WSARecvUnknownError = 0;    // WSARecv 함수에서 오류 발생(원인불명)
			std::atomic<__int64> WSASendKnownError = 0;      // WSASend 함수에서 오류 발생
			std::atomic<__int64> WSASendUnknownError = 0;    // WSASend 함수에서 오류 발생(원인불명)
			std::atomic<__int64> otherErrors = 0;            // 기타 오류

			// accept 스레드 모니터링
			volatile __int64 acceptCount = 0;            // accept 횟수
			volatile __int64 connectCount = 0;           // connect 횟수 (accept 후 connect 승인된 횟수)
			volatile __int64 disconnBySessionLimit = 0;  // 세션수 제한으로 인해 끊김
			volatile __int64 disconnByIOCPAssociation = 0;  // IOCP와 소켓 연계에 실패하여 끊김
			volatile __int64 disconnByOnConnReq = 0;     // OnConnectionRequest 함수로 인해 끊김

			// 트래픽 혼잡제어 스레드 모니터링
			volatile __int64 trafficCongestionControlCount = 0;

			// worker 스레드 내부 TLS 모니터링
			struct TLSMonitor
			{
				volatile __int64 GQCSWaitTime = 0;           // GQCS가 리턴되기를 기다린 시간(performance frequency 단위)
				volatile __int64 sendCompletionCount = 0;    // send 완료통지 처리횟수
				volatile __int64 recvCompletionCount = 0;    // recv 완료통지 처리횟수
				volatile __int64 recvCount = 0;              // recv 완료통지에서 처리한 패킷 수
				volatile __int64 disconnByKnownRecvIoError = 0;    // recv 완료통지에서 오류 발생(클라이언트 또는 서버에서 강제로 끊었을때)
				volatile __int64 disconnByUnknownRecvIoError = 0;  // recv 완료통지에서 오류 발생(원인불명)
				volatile __int64 disconnByKnownSendIoError = 0;    // send 완료통지에서 오류 발생
				volatile __int64 disconnByUnknownSendIoError = 0;  // send 완료통지에서 오류 발생(원인불명)
				volatile __int64 disconnBy121RecvIoError = 0;      // recv 완료통지에서 오류 발생(ERROR_SEM_TIMEOUT 121 오류)
				volatile __int64 disconnByNormalProcess = 0;        // 클라이언트가 정상적으로 연결을 끊음
				volatile __int64 disconnByPacketCode = 0;    // 패킷코드가 잘못됨
				volatile __int64 disconnByPacketLength = 0;  // 패킷길이가 잘못됨
				volatile __int64 disconnByPacketDecode = 0;  // 패킷 복호화에 실패함
			};
		private:
			std::vector<TLSMonitor> _vecTlsMonitor;
			mutable SRWLOCK _srwlVecTlsMonitor_mutable;
			LARGE_INTEGER _liPerformanceFrequency;

		private:
			Monitor();
			TLSMonitor& CreateTlsMonitor();

		public:
			/* Get 모니터링 count */
			__int64 GetAcceptCount() const { return acceptCount; }     // accept 횟수
			__int64 GetConnectCount() const { return connectCount; }   // connect 횟수 (accept 후 connect 승인된 횟수)
			__int64 GetDisconnBySessionLimit() const { return disconnBySessionLimit; }     // 세션수 제한으로 인해 끊김
			__int64 GetDisconnByIOCPAssociation() const { return disconnByIOCPAssociation; }  // IOCP와 소켓 연계에 실패하여 끊김
			__int64 GetDisconnByOnConnectionRequest() const { return disconnByOnConnReq; } // OnConnectionRequest 함수로 인해 끊김

			__int64 GetSendCompletionCount() const;          // send 완료통지 처리횟수
			__int64 GetRecvCompletionCount() const;          // recv 완료통지 처리횟수
			__int64 GetRecvCount() const;                    // recv 완료통지에서 처리한 패킷 수
			__int64 GetDisconnByKnownIoError() const;        // WSARecv, WSASend, recv 완료통지, send 완료통지에서 오류 발생(클라이언트 또는 서버에서 강제로 끊었을때)
			__int64 GetDisconnByUnknownIoError() const;      // WSARecv, WSASend, recv 완료통지, send 완료통지에서 오류 발생(원인불명)
			__int64 GetDisconnBy121RecvIoError() const;      // recv 완료통지에서 121(ERROR_SEM_TIMEOUT) 오류 발생
			__int64 GetDisconnByNormalProcess() const;       // 클라이언트가 정상적으로 연결을 끊음
			__int64 GetDisconnByPacketCode() const;          // 패킷코드가 잘못됨
			__int64 GetDisconnByPacketLength() const;        // 패킷길이가 잘못됨
			__int64 GetDisconnByPacketDecode() const;        // 패킷 복호화에 실패함

			__int64 GetSendCount() const { return sendCount + sendAsyncCount; } // WSASend 함수로 전송한 패킷의 수
			__int64 GetDisconnectCount() const { return disconnectCount; }       // disconnect 횟수 (세션 release 횟수)
			__int64 GetDeferredDisconnectCount() const { return deferredDisconnectCount; }   // 지연된 disconnect 횟수
			__int64 GetOtherErrorCount() const { return otherErrors; }                     // 기타 오류 횟수
			__int64 GetTrafficCongestionControlCount() const { return trafficCongestionControlCount; }  // 트래픽 혼잡제어 횟수
		};
		Monitor _monitor;
	};

}