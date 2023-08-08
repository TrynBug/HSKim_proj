#pragma once

#include "../Netlib/CNetServer.h"
#include "CLANClientMonitoring.h"

#include "defineLoginServer.h"
#include "../utils/CMemoryPoolTLS.h"
#include "../utils/CDBConnectorTLSManager.h"

class CClient;
class CDBConnectorTLSManager;
class CCpuUsage;
class CPDH;

namespace cpp_redis
{
	class client;
}

class CLoginServer : public netlib::CNetServer
{
	using CPacket = netlib::CPacket;
public:
	class Monitor;

public:
	CLoginServer();
	virtual ~CLoginServer();

	CLoginServer(const CLoginServer&) = delete;
	CLoginServer(CLoginServer&&) = delete;
	
public:
	/* StartUp */
	bool StartUp();

	/* Shutdown */
	bool Shutdown();

private:
	/* Get */
	CClient* GetClientBySessionId(__int64 sessionId);   // 세션ID로 클라이언트 객체를 얻는다. 실패할경우 null을 반환한다.
	
	/* Client */
	void InsertClientToMap(__int64 sessionId, CClient* pClient);   // 세션ID-클라이언트 map에 클라이언트를 등록한다. 
	void DeleteClientFromMap(__int64 sessionId);                   // 세션ID-클라이언트 map에서 클라이언트를 삭제한다.
	void DisconnectSession(__int64 sessionId);                     // 클라이언트의 연결을 끊는다.
	void DeleteClient(CClient* pClient);                           // 클라이언트를 map에서 삭제하고 free 한다.

	CClient* AllocClient(__int64 sessionId);
	void FreeClient(CClient* pClient);

	/* account */
	bool InsertAccountToMap(__int64 accountNo, __int64 sessionId);     // account번호를 account번호-세션ID map에 등록한다. 만약 accountNo에 해당하는 데이터가 이미 존재할 경우 실패하며 false를 반환한다.
	void DeleteAccountFromMap(__int64 accountNo);   // account번호를 account번호-세션ID map에서 제거한다.

	/* 네트워크 전송 */
	int SendUnicast(__int64 sessionId, CPacket& packet);
	int SendUnicastAndDisconnect(__int64 sessionId, CPacket& packet);

	/* 스레드 */
	static unsigned WINAPI ThreadPeriodicWorker(PVOID pParam);       // 주기적 작업 처리 스레드
	static unsigned WINAPI ThreadMonitoringCollector(PVOID pParam);

	/* crash */
	void Crash();


private:
	// 네트워크 라이브러리 callback 함수 구현
	virtual bool OnRecv(__int64 sessionId, CPacket& packet);
	virtual bool OnConnectionRequest(unsigned long IP, unsigned short port);
	virtual bool OnClientJoin(__int64 sessionId);
	virtual bool OnClientLeave(__int64 sessionId);
	virtual void OnError(const wchar_t* szError, ...);
	virtual void OnOutputDebug(const wchar_t* szError, ...);
	virtual void OnOutputSystem(const wchar_t* szError, ...);


public:
	/* 서버 상태 */
	int GetNumClient() { return (int)_mapClient.size(); }
	int GetSizeAccountMap() { return (int)_mapAccountToSession.size(); }
	int GetClientPoolSize() { return _poolClient.GetPoolSize(); };
	int GetClientAllocCount() { return _poolClient.GetAllocCount(); }
	int GetClientActualAllocCount() { return _poolClient.GetActualAllocCount(); }
	int GetClientFreeCount() { return _poolClient.GetFreeCount(); }
	int GetNumDBConnection() { return _tlsDBConnector->GetNumConnection(); }
	__int64 GetQueryRunCount() { return _tlsDBConnector->GetQueryRunCount(); }
	float GetMaxQueryRunTime() { return _tlsDBConnector->GetMaxQueryRunTime(); }
	float GetMinQueryRunTime() { return _tlsDBConnector->GetMinQueryRunTime(); }
	float GetAvgQueryRunTime() { return _tlsDBConnector->GetAvgQueryRunTime(); }
	bool IsTerminated() { return _bTerminated; }


private:
	/* 경로 */
	std::wstring _sCurrentPath;     // 현재 디렉토리 경로
	std::wstring _sConfigFilePath;  // config 파일 경로

	/* config */
	struct Config
	{
		wchar_t szBindIP[20];
		unsigned short portNumber;
		wchar_t szGameServerIP[20];
		wchar_t szGameServerDummy1IP[20];
		wchar_t szGameServerDummy2IP[20];
		unsigned short gameServerPort;
		wchar_t szChatServerIP[20];
		wchar_t szChatServerDummy1IP[20];
		wchar_t szChatServerDummy2IP[20];
		unsigned short chatServerPort;
		int numConcurrentThread;
		int numWorkerThread;
		int numMaxSession;
		int numMaxClient;
		bool bUseNagle;

		BYTE packetCode;
		BYTE packetKey;
		int maxPacketSize;

		unsigned short monitoringServerPortNumber;
		BYTE monitoringServerPacketCode;
		BYTE monitoringServerPacketKey;

		Config();
	};
	Config _config;



	/* server */
	int _serverNo;                         // 서버번호(로그인서버:3)
	struct ThreadInfo
	{
		HANDLE handle;
		unsigned int id;
		ThreadInfo() :handle(NULL), id(0) {}
	};
	ThreadInfo _thPeriodicWorker;          // 주기적 작업 스레드
	ThreadInfo _thMonitoringCollector;     // 모니터링 데이터 수집 스레드

	volatile bool _bShutdown;               // shutdown 여부
	volatile bool _bTerminated;             // 종료됨 여부
	LARGE_INTEGER _performanceFrequency;


	/* timeout */
	int _timeoutLogin;
	int _timeoutLoginCheckPeriod;

	/* TLS DBConnector */
	std::unique_ptr<CDBConnectorTLSManager> _tlsDBConnector;

	/* Client */
	std::unordered_map<__int64, CClient*> _mapClient;
	SRWLOCK _srwlMapClient;

	/* account */
	std::unordered_map<__int64, __int64> _mapAccountToSession;
	SRWLOCK _srwlMapAccountToSession;

	/* pool */
	CMemoryPoolTLS<CClient> _poolClient;      // 클라이언트 풀

	/* redis */
	std::unique_ptr<cpp_redis::client> _pRedisClient;

	/* 모니터링 클라이언트 */
	std::unique_ptr<CLANClientMonitoring> _pLANClientMonitoring;       // 모니터링 서버와 연결할 LAN 클라이언트
	std::unique_ptr<CCpuUsage> _pCPUUsage;
	std::unique_ptr<CPDH> _pPDH;

	/* 모니터링 counter */
public:
	class Monitor
	{
		friend class CLoginServer;

		std::atomic<__int64> loginCount = 0;                  // 로그인 성공 횟수
		std::atomic<__int64> disconnByClientLimit = 0;        // 클라이언트수 제한으로 끊김
		std::atomic<__int64> disconnByNoClient = 0;           // 세션번호에 대한 클라이언트 객체를 찾지못함
		std::atomic<__int64> disconnByDBDataError = 0;        // DB 데이터 오류
		std::atomic<__int64> disconnByNoAccount = 0;          // account 테이블에서 account 번호를 찾지 못함
		std::atomic<__int64> disconnByDupAccount = 0;         // 동일 account가 다른스레드에서 로그인을 진행중임
		std::atomic<__int64> disconnByInvalidMessageType = 0; // 메시지 타입이 잘못됨
		std::atomic<__int64> disconnByLoginTimeout = 0;       // 로그인 타임아웃으로 끊김
		std::atomic<__int64> disconnByHeartBeatTimeout = 0;   // 하트비트 타임아웃으로 끊김

	public:
		__int64 GetLoginCount() { return loginCount; }
		__int64 GetDisconnByClientLimit() { return disconnByClientLimit; }
		__int64 GetDisconnByNoClient() { return disconnByNoClient; }
		__int64 GetDisconnByDBDataError() { return disconnByDBDataError; }
		__int64 GetDisconnByNoAccount() { return disconnByNoAccount; }
		__int64 GetDisconnByDupAccount() { return disconnByDupAccount; }
		__int64 GetDisconnByInvalidMessageType() { return disconnByInvalidMessageType; }
		__int64 GetDisconnByLoginTimeout() { return disconnByLoginTimeout; }
		__int64 GetDisconnByHeartBeatTimeout() { return disconnByHeartBeatTimeout; }
	};
private:
	Monitor _monitor;

};

