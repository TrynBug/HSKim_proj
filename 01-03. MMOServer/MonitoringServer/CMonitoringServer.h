#pragma once

#include "../NetLib/CNetServer.h"
#include "defineMonitoringServer.h"
#include "../utils/CDBConnector.h"
#include "../utils/CMemoryPool.h"

#include <sstream>

class CUser;
class CDBConnector;

namespace monitorserver
{
	class alignas(64) CMonitoringServer : public netlib::CNetServer
	{
	public:
		class Monitor;

	public:
		CMonitoringServer();
		~CMonitoringServer();

		CMonitoringServer(const CMonitoringServer&) = delete;
		CMonitoringServer(CMonitoringServer&&) = delete;

		/* dynamic alloc */
		// 64byte aligned 객체 생성을 위한 new, delete overriding
		void* operator new(size_t size);
		void operator delete(void* p);

	public:
		/* StartUp */
		bool StartUp();

		/* Shutdown */
		bool Shutdown();

		/* Get 서버상태 */
		int IsTerminated() { return _bTerminated; }
		int GetNumUser() { return (int)_mapUser.size(); }
		int GetNumWANUser() { return (int)_mapWANUser.size(); }
		int GetUserPoolSize() { return _poolUser.GetPoolSize(); };
		int GetUserAllocCount() { return _poolUser.GetUseCount(); }
		int GetUserFreeCount() { return _poolUser.GetFreeCount(); }
		__int64 GetQueryRunCount() { return _pDBConn->GetQueryRunCount(); }
		float GetMaxQueryRunTime() { return _pDBConn->GetMaxQueryRunTime(); }
		float GetMinQueryRunTime() { return _pDBConn->GetMinQueryRunTime(); }
		float GetAvgQueryRunTime() { return _pDBConn->GetAvgQueryRunTime(); }

		/* Get 모니터링 count */
		const Monitor& GetMonitor() { return _monitor; }
		const CNetServer::Monitor& GetNetworkMonitor() { return CNetServer::_monitor; }

	private:
		/* 네트워크 전송 */
		int SendUnicast(__int64 sessionId, CPacket& packet);

		/* 유저 */
		CUser_t AllocUser(__int64 sessionId);
		CUser_t GetUserBySessionId(__int64 sessionId);
		void DisconnectSession(__int64 sessionId);
		void DisconnectSession(CUser_t pUser);

		/* Thread */
		static unsigned WINAPI ThreadJobGenerator(PVOID pParam);

		/* crash */
		void Crash();

	private:
		// 네트워크 라이브러리 함수 구현
		virtual bool OnRecv(__int64 sessionId, CPacket& packet);
		virtual bool OnInnerRequest(CPacket& packet);
		virtual bool OnConnectionRequest(unsigned long IP, unsigned short port);
		virtual bool OnClientJoin(__int64 sessionId);
		virtual bool OnClientLeave(__int64 sessionId);
		virtual void OnError(const wchar_t* szError, ...);
		virtual void OnOutputDebug(const wchar_t* szError, ...);
		virtual void OnOutputSystem(const wchar_t* szError, ...);


	private:
		/* 경로 */
		std::wstring _sCurrentPath;     // 현재 디렉토리 경로
		std::wstring _sConfigFilePath;  // config 파일 경로

		/* config */
		struct Config
		{
			wchar_t szBindIP[20];
			unsigned short portNumber;
			int numMaxSession;
			int numMaxUser;
			bool bUseNagle;
			bool bUsePacketEncryption;  // 패킷 암호화 사용 여부

			BYTE packetCode;
			BYTE packetKey;
			int maxPacketSize;

			Config();
			void LoadConfigFile(std::wstring& sFilePath);
		};
		Config _config;

		/* 유저 */
		std::unordered_map<__int64, CUser_t> _mapUser;
		std::unordered_map<__int64, CUser_t> _mapWANUser;
		CMemoryPool<CUser> _poolUser;

		/* 서버 */
		char _loginKey[32];          // login key    
		volatile bool _bShutdown;    // shutdown 여부
		volatile bool _bTerminated;  // 종료됨 여부
		std::mutex _mtxShutdown;
		int _timeoutLogin;           // 로그인 타임아웃 대상시간

		/* DB */
		std::unique_ptr<CDBConnector> _pDBConn;
		std::wostringstream _ossQuery;

		/* 스레드 */
		HANDLE _hThreadJobGenerator;
		unsigned int _idThreadJobGenerator;

		/* 서버로부터 수집한 모니터링 데이터 */
		struct MonitoringDataSet
		{
			int dataCollectCount = 0;
			int arrValue5m[300] = { 0, };
		};
		std::vector<MonitoringDataSet> _vecMonitoringDataSet;


	public:
		/* 모니터링 counter */
		class Monitor
		{
			friend class CMonitoringServer;
		private:
			volatile __int64 msgHandleCount = 0;              // 메시지 처리 횟수
			volatile __int64 receivedInvalidDatatype = 0;     // 잘못된 모니터링 데이터 타입을 받음
			volatile __int64 disconnByUserLimit = 0;          // 유저수 제한으로 끊김
			volatile __int64 disconnByInvalidMessageType = 0; // 메시지 타입이 잘못됨
			volatile __int64 disconnByLoginKeyMismatch = 0;   // 로그인 키가 잘못됨
			volatile __int64 disconnByWANUserUpdateRequest = 0;  // WAN 유저가 데이터 업데이트 요청을 보냄
			volatile __int64 DBErrorCount = 0;                // DB 오류 횟수

		public:
			/* Get 모니터링 */
			__int64 GetMsgHandleCount() const { return msgHandleCount; }
			__int64 GetReceivedInvalidDatatype() const { return receivedInvalidDatatype; }
			__int64 GetDisconnByUserLimit() const { return disconnByUserLimit; }
			__int64 GetDisconnByInvalidMessageType() const { return disconnByInvalidMessageType; }
			__int64 GetDisconnByLoginKeyMismatch() const { return disconnByLoginKeyMismatch; }
			__int64 GetDisconnByWANUserUpdateRequest() const { return disconnByWANUserUpdateRequest; }
			__int64 GetDBErrorCount() const { return DBErrorCount; }
		};
	private:
		Monitor _monitor;


	};

}