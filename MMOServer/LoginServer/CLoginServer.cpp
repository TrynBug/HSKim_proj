#include "CLoginServer.h"

#include <strsafe.h>

#include "../utils/cpp_redis/cpp_redis.h"
#pragma comment (lib, "NetLib.lib")
#pragma comment (lib, "LANClient.lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")

#include "../common/CommonProtocol.h"

#include "../utils/profiler.h"
#include "../utils/logger.h"
#include "../utils/CJsonParser.h"
#include "../utils/CCpuUsage.h"
#include "../utils/CPDH.h"


using namespace loginserver;


CLoginServer::CLoginServer()
	: _bShutdown(false)
	, _bTerminated(false)
	, _pRedisClient(nullptr)
	, _poolClient(0, false, 100)
	, _pLANClientMonitoring(nullptr)
	, _pCPUUsage(nullptr)
	, _pPDH(nullptr)
	, _tlsDBConnector(nullptr)
{
	_serverNo = 3;       // 서버번호(로그인서버:3)

	QueryPerformanceFrequency(&_performanceFrequency);

	// timeout
	_timeoutLogin = 15000;
	_timeoutLoginCheckPeriod = 3000;
}


CLoginServer::~CLoginServer()
{
	Shutdown();
}


CLoginServer::Config::Config()
	: szBindIP{ 0, }
	, portNumber(0)
	, szGameServerIP{ 0, }
	, gameServerPort(0)
	, szChatServerIP{ 0, }
	, chatServerPort(0)
	, szGameServerDummy1IP{ 0, }
	, szGameServerDummy2IP{ 0, }
	, szChatServerDummy1IP{ 0, }
	, szChatServerDummy2IP{ 0, }
	, numConcurrentThread(0)
	, numWorkerThread(0)
	, numMaxSession(0)
	, numMaxClient(0)
	, bUseNagle(true)
	, packetCode(0)
	, packetKey(0)
	, maxPacketSize(0)
	, monitoringServerPortNumber(0)
	, monitoringServerPacketCode(0)
	, monitoringServerPacketKey(0)
{
}

void CLoginServer::Config::LoadConfigFile(std::wstring& sFilePath)
{
	// config parse
	CJsonParser jsonParser;
	jsonParser.ParseJsonFile(sFilePath.c_str());

	const wchar_t* serverBindIP = jsonParser[L"ServerBindIP"].Str().c_str();
	wcscpy_s(szBindIP, wcslen(serverBindIP) + 1, serverBindIP);
	portNumber = jsonParser[L"LoginServerPort"].Int();
	const wchar_t* gameServerIP = jsonParser[L"GameServerIP"].Str().c_str();
	wcscpy_s(szGameServerIP, wcslen(gameServerIP) + 1, gameServerIP);
	gameServerPort = jsonParser[L"GameServerPort"].Int();
	const wchar_t* chatServerIP = jsonParser[L"ChatServerIP"].Str().c_str();
	wcscpy_s(szChatServerIP, wcslen(chatServerIP) + 1, chatServerIP);
	chatServerPort = jsonParser[L"ChatServerPort"].Int();

	const wchar_t* gameServerDummy1IP = jsonParser[L"GameServerDummy1IP"].Str().c_str();
	wcscpy_s(szGameServerDummy1IP, wcslen(gameServerDummy1IP) + 1, gameServerDummy1IP);
	const wchar_t* gameServerDummy2IP = jsonParser[L"GameServerDummy2IP"].Str().c_str();
	wcscpy_s(szGameServerDummy2IP, wcslen(gameServerDummy2IP) + 1, gameServerDummy2IP);
	const wchar_t* chatServerDummy1IP = jsonParser[L"ChatServerDummy1IP"].Str().c_str();
	wcscpy_s(szChatServerDummy1IP, wcslen(chatServerDummy1IP) + 1, chatServerDummy1IP);
	const wchar_t* chatServerDummy2IP = jsonParser[L"ChatServerDummy2IP"].Str().c_str();
	wcscpy_s(szChatServerDummy2IP, wcslen(chatServerDummy2IP) + 1, chatServerDummy2IP);

	numConcurrentThread = jsonParser[L"NetRunningWorkerThread"].Int();
	numWorkerThread = jsonParser[L"NetWorkerThread"].Int();

	bUseNagle = (bool)jsonParser[L"EnableNagle"].Int();
	numMaxSession = jsonParser[L"SessionLimit"].Int();
	numMaxClient = jsonParser[L"ClientLimit"].Int();
	packetCode = jsonParser[L"PacketHeaderCode"].Int();
	packetKey = jsonParser[L"PacketEncodeKey"].Int();
	maxPacketSize = jsonParser[L"MaximumPacketSize"].Int();

	monitoringServerPortNumber = jsonParser[L"MonitoringServerPort"].Int();
	monitoringServerPacketCode = jsonParser[L"MonitoringServerPacketHeaderCode"].Int();
	monitoringServerPacketKey = jsonParser[L"MonitoringServerPacketEncodeKey"].Int();
}


bool CLoginServer::StartUp()
{
	timeBeginPeriod(1);

	// 현재 경로와 config 파일 경로를 얻음
	WCHAR szPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szPath);
	_sCurrentPath = szPath;
	_sConfigFilePath = _sCurrentPath + L"\\login_server_config.json";

	// config 파일 로드
	_config.LoadConfigFile(_sConfigFilePath);


	// logger 초기화
	logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_INFO);
	//logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_DEBUG);

	logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_INFO);
	//logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_DEBUG);
	//logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_ERROR);


	LOGGING(LOGGING_LEVEL_INFO, L"\n********** StartUp Login Server ************\n"
		L"Config File: %s\n"
		L"Server Bind IP: %s\n"
		L"Login Server Port: %d\n"
		L"Number of Network Worker Thread: %d\n"
		L"Number of Network Running Worker Thread: %d\n"
		L"Number of Maximum Session: %d\n"
		L"Number of Maximum Client: %d\n"
		L"Enable Nagle: %s\n"
		L"Packet Header Code: 0x%x\n"
		L"Packet Encode Key: 0x%x\n"
		L"Maximum Packet Size: %d\n"
		L"*******************************************************************\n\n"
		, _sConfigFilePath.c_str()
		, _config.szBindIP
		, _config.portNumber
		, _config.numConcurrentThread
		, _config.numWorkerThread
		, _config.numMaxSession
		, _config.numMaxClient
		, _config.bUseNagle ? L"Yes" : L"No"
		, _config.packetCode
		, _config.packetKey
		, _config.maxPacketSize);


	// TLS DB Connector 생성
	_tlsDBConnector = std::make_unique<CDBConnectorTLSManager>("127.0.0.1", "root", "vmfhzkepal!", "accountdb", 3306);

	// 주기적 job 처리 스레드 start
	_thPeriodicWorker.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadPeriodicWorker, (PVOID)this, 0, &_thPeriodicWorker.id);
	if (_thPeriodicWorker.handle == 0 || _thPeriodicWorker.handle == INVALID_HANDLE_VALUE)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[Login Server] periodic worker thread failed to start. error:%u\n", GetLastError());
		Crash();
		return false;
	}


	// 모니터링 서버와 연결하는 클라이언트 start
	_pLANClientMonitoring = std::make_unique<CLANClientMonitoring>(_serverNo, L"127.0.0.1", _config.monitoringServerPortNumber, true, _config.monitoringServerPacketCode, _config.monitoringServerPacketKey, 10000, true);
	if (_pLANClientMonitoring->StartUp() == false)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[Login Server] LAN client start failed. error:%u\n", GetLastError());
		//Crash();
		//return false;
	}
	_pLANClientMonitoring->ConnectToServer();


	// 모니터링 데이터 수집 스레드 start
	_pCPUUsage = std::make_unique<CCpuUsage>();
	_pPDH = std::make_unique<CPDH>();
	_pPDH->Init();
	_thMonitoringCollector.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadMonitoringCollector, (PVOID)this, 0, &_thMonitoringCollector.id);
	if (_thMonitoringCollector.handle == 0)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[Login Server] failed to start monitoring collector thread. error:%u\n", GetLastError());
		Crash();
		return false;
	}


	// redis start
	_pRedisClient = std::make_unique<cpp_redis::client>();
	try
	{
		_pRedisClient->connect();
	}
	catch (...)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[Login Server] Redis connect failed\n", GetLastError());
		Crash();
		return false;
	}


	// 네트워크 start
	bool retNetStart = CNetServer::StartUp(_config.szBindIP, _config.portNumber, _config.numConcurrentThread, _config.numWorkerThread, _config.bUseNagle, _config.numMaxSession, _config.packetCode, _config.packetKey, _config.maxPacketSize, true, true);
	if (retNetStart == false)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[Login Server] Network Library Start failed\n", GetLastError());
		Crash();
		return false;
	}



	return true;
}


/* Shutdown */
bool CLoginServer::Shutdown()
{
	std::lock_guard<std::mutex> lock_guard(_mtxShutdown);
	if (_bShutdown == true)
		return true;

	_bShutdown = true;

	// accept 중지
	CNetServer::StopAccept();

	// 네트워크 shutdown
	CNetServer::Shutdown();

	// 로그인서버 스레드들이 종료되기를 10초간 기다림
	ULONGLONG timeout = 10000;
	ULONGLONG tick = GetTickCount64();
	DWORD retWait;
	retWait = WaitForSingleObject(_thPeriodicWorker.handle, (DWORD)timeout);
	if (retWait != WAIT_OBJECT_0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[login server] terminate periodic worker thread due to timeout. error:%u\n", GetLastError());
		TerminateThread(_thPeriodicWorker.handle, 0);
	}
	timeout = timeout - GetTickCount64() - tick;
	timeout = timeout < 1 ? 1 : timeout;
	retWait = WaitForSingleObject(_thMonitoringCollector.handle, (DWORD)timeout);
	if (retWait != WAIT_OBJECT_0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[login server] terminate monitoring collector thread due to timeout. error:%u\n", GetLastError());
		TerminateThread(_thMonitoringCollector.handle, 0);
	}

	// DB 종료
	_tlsDBConnector->DisconnectAll();

	_bTerminated = true;
	return true;
}







/* Get */
// 세션ID로 클라이언트 객체를 얻는다. 실패할경우 null을 반환한다.
CClient_t CLoginServer::GetClientBySessionId(__int64 sessionId)
{
	CClient_t pClient;
	_smtxMapClient.lock_shared();
	const auto& iter = _mapClient.find(sessionId);
	if (iter == _mapClient.end())
		pClient = nullptr;
	else
		pClient = iter->second;
	_smtxMapClient.unlock_shared();
	
	return std::move(pClient);
}


/* Client */
// 세션ID-클라이언트 map에 클라이언트를 등록한다. 
void CLoginServer::InsertClientToMap(__int64 sessionId, CClient_t pClient)
{
	_smtxMapClient.lock();
	_mapClient.insert(std::make_pair(sessionId, pClient));
	_smtxMapClient.unlock();
}

// 세션ID-클라이언트 map에서 클라이언트를 삭제한다.
void CLoginServer::DeleteClientFromMap(__int64 sessionId)
{
	_smtxMapClient.lock();
	const auto& iter = _mapClient.find(sessionId);
	if (iter != _mapClient.end())
		_mapClient.erase(iter);
	_smtxMapClient.unlock();
}

// 클라이언트의 연결을 끊는다.
void CLoginServer::DisconnectSession(__int64 sessionId)
{
	PROFILE_BEGIN("CLoginServer::DisconnectSession");

	bool bDisconnect = false;
	CClient_t pClient = GetClientBySessionId(sessionId);
	if (pClient == nullptr)
	{
		bDisconnect = true;
	}
	else
	{
		// Periodic Worker 스레드가 로그인 타임아웃 대상 세션에 DisconnectSession을 호출하고 Client 객체를 얻은 다음,
		// 해당 세션이 다른 스레드에서 연결이 끊기고 Client 객체가 재할당될 수 있다. 
		// 그래서 Client 객체의 세션ID를 비교하여 일치하는 경우에만 실제로 연결을 끊는다.
		pClient->LockExclusive();
		if (pClient->_sessionId == sessionId && pClient->_bDisconnect == false)
		{
			pClient->_bDisconnect = true;
			bDisconnect = true;
		}
		pClient->UnlockExclusive();
	}

	if (bDisconnect)
		CNetServer::Disconnect(sessionId);
}

// 클라이언트를 map에서 삭제한다.
void CLoginServer::DeleteClient(CClient_t& pClient)
{
	DeleteClientFromMap(pClient->_sessionId);
}


CClient_t CLoginServer::AllocClient(__int64 sessionId)
{
	auto Deleter = [this](CClient* pClient) {
		this->_poolClient.Free(pClient);
	};
	std::shared_ptr<CClient> pClient(_poolClient.Alloc(), Deleter);
	pClient->Init(sessionId);
	return pClient;
}




/* account */
// account번호를 account번호-세션ID map에 등록한다. 만약 accountNo에 해당하는 데이터가 이미 존재할 경우 실패하며 false를 반환한다.
bool CLoginServer::InsertAccountToMap(__int64 accountNo, __int64 sessionId)
{
	bool bInserted = false;
	_smtxMapAccountToSession.lock();
	auto iter = _mapAccountToSession.find(accountNo);
	if (iter == _mapAccountToSession.end())
	{
		_mapAccountToSession.insert(std::make_pair(accountNo, sessionId));
		bInserted = true;
	}
	_smtxMapAccountToSession.unlock();
	return bInserted;
}

// account번호를 account번호-세션ID map에서 제거한다.
void CLoginServer::DeleteAccountFromMap(__int64 accountNo)
{
	_smtxMapAccountToSession.lock();
	auto iter = _mapAccountToSession.find(accountNo);
	if (iter != _mapAccountToSession.end())
	{
		_mapAccountToSession.erase(accountNo);
	}
	_smtxMapAccountToSession.unlock();
}




/* 네트워크 전송 */
int CLoginServer::SendUnicast(__int64 sessionId, CPacket& packet)
{
	return SendPacket(sessionId, packet);
	//return SendPacketAsync(sessionId, packet);
}


int CLoginServer::SendUnicastAndDisconnect(__int64 sessionId, CPacket& packet)
{
	return SendPacketAndDisconnect(sessionId, packet);
	//return SendPacketAsyncAndDisconnect(sessionId, packet);
}



/* crash */
void CLoginServer::Crash()
{
	int* p = 0;
	*p = 0;
}




// 네트워크 라이브러리 callback 함수 구현
bool CLoginServer::OnConnectionRequest(unsigned long IP, unsigned short port)
{
	if (GetNumClient() >= _config.numMaxClient)
	{
		_monitor.disconnByClientLimit++;
		return false;
	}

	return true;
}


bool CLoginServer::OnClientJoin(__int64 sessionId)
{
	// OnClientJoin 함수가 완료되기 전 까지는 현재 세션에 recv가 걸리지 않는다.
	// 그래서 현재 세션에 대해 OnRecv나 OnClientLeave 함수가 호출되지 않는다.

	PROFILE_BEGIN("CLoginServer::OnClientJoin");

	// 클라이언트 생성 및 등록
	CClient_t pClient = AllocClient(sessionId);
	InsertClientToMap(sessionId, pClient);
	LOGGING(LOGGING_LEVEL_DEBUG, L"OnClientJoin. sessionId:%lld\n", sessionId);

	return true;
}

bool CLoginServer::OnRecv(__int64 sessionId, CPacket& packet)
{
	// OnRecv 함수가 수행되고 있는 중에는 세션이 절대 반환되지 않는다.
	// 그래서 현재 세션에 대해 OnClientLeave 함수가 호출될 일이 없고, 따라서 클라이언트 객체가 반환될 일도 없다.
	// 그러니까 같은 세션, 클라이언트에 대해 OnRecv 함수와 OnClientLeave 함수가 동시에 호출될 일은 없다는 것이다.
	// 그래서 map에서 클라이언트 객체를 얻은 다음 클라이언트 객체를 lock 하여도 늦지 않는다.
	// OnRecv 함수 자체는 같은 세션, 클라이언트에 대해 동시에 호출될 수 있다.

	WORD packetType;

	// 패킷 타입에 따른 메시지 처리
	packet >> packetType;
	switch (packetType)
	{
	// 로그인서버 로그인 요청
	case en_PACKET_CS_LOGIN_REQ_LOGIN:
	{
		PacketProc_LoginRequest(sessionId, packet);
		break;
	}

	default:
	{
		PROFILE_BEGIN("CLoginServer::OnRecv::DEFAULT");

		DisconnectSession(sessionId);
		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv received invalid packet type. session:%lld, packet type:%d\n", sessionId, packetType);

		_monitor.disconnByInvalidMessageType++;
		break;
	}
	}

	return true;
}




bool CLoginServer::OnClientLeave(__int64 sessionId)
{
	// OnClientLeave 함수는 세션에 걸린 IO가 하나도 없어서 세션이 반환된 뒤에 호출된다.
	// OnRecv 함수가 수행되고 있는 중에는 세션이 절대 반환되지 않기 때문에 같은 세션에 대해 OnRecv 함수와 OnClientLeave 함수가 동시에 수행될 수는 없다.

	PROFILE_BEGIN("CLoginServer::OnClientLeave");
	LOGGING(LOGGING_LEVEL_DEBUG, L"OnClientLeave. sessionId:%lld\n", sessionId);

	CClient_t pClient = GetClientBySessionId(sessionId);
	if (pClient == nullptr)
	{
		Crash();
		return false;
	}


	LOGGING(LOGGING_LEVEL_DEBUG, L"OnClientLeave leave client. sessionId:%lld, accountNo:%lld\n", sessionId, pClient->_bLogin ? pClient->_accountNo : -1);
	DeleteClient(pClient);

	return true;
}


void CLoginServer::OnError(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_ERROR, szError, vaList);
	va_end(vaList);
}


void CLoginServer::OnOutputDebug(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_DEBUG, szError, vaList);
	va_end(vaList);
}

void CLoginServer::OnOutputSystem(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_INFO, szError, vaList);
	va_end(vaList);
}










void CLoginServer::PacketProc_LoginRequest(__int64 sessionId, CPacket& packet)
{
	PROFILE_BEGIN("CLoginServer::OnRecv::en_PACKET_CS_LOGIN_REQ_LOGIN");

	INT64 accountNo;
	char  sessionKey[65];
	packet >> accountNo;
	packet.TakeData(sessionKey, sizeof(sessionKey) - 1);
	sessionKey[64] = '\0';
	LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv login request. session:%lld, accountNo:%lld\n", sessionId, accountNo);


	// 클라이언트객체 존재 체크
	CClient_t pClient = GetClientBySessionId(sessionId);
	if (pClient == nullptr)
	{
		// 로그인실패 응답 발송
		CPacket& sendPacket = AllocPacket();
		Msg_LoginResponseFail(sendPacket, accountNo);
		SendUnicast(sessionId, sendPacket);
		sendPacket.SubUseCount();

		// 세션ID-플레이어 map에 플레이어객체가 없으므로 세션만 끊는다.
		CNetServer::Disconnect(sessionId);
		_monitor.disconnByNoClient++;

		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv send login failed. session:%lld, accountNo:%lld\n", sessionId, accountNo);
		return;
	}
	pClient->SetLogin();

	// 동일 account의 계정이 로그인을 시도하고있는지 체크
	/* # 서로다른 클라가 동일 account로 로그인요청을 동시에 보내는 경우
		status를 조회해보니 0이라서 정상적인 로그인 프로세스가 2개 스레드에서 동시에 진행됨
		1번 스레드에서 redis에 1번 클라의 세션key를 등록하고, 클라가 채팅서버에 로그인요청을 하고, 채팅서버가 세션key를 보고 로그인을 통과시킴
		그런다음 2번 스레드에서 redis에 2번 클라의 세션key를 등록하고, 클라가 채팅서버에 로그인요청을 하고, 채팅서버가 key를 보고 로그인을 통과시킴
		이렇게되면 중복 로그인이 발생하므로 현재 로그인처리를 진행하고 있는 account번호를 set에 등록해놓고, 만약 set에 account번호가 이미 존재한다면 로그인을 실패함.
		로그인처리를 완료했다면 account번호를 set에서 제거함 */
	bool bInserted = InsertAccountToMap(accountNo, sessionId);
	if (bInserted == false)
	{
		// 동일 account번호의 계정이 현재 다른 스레드에서 로그인을 진행하고 있다면 로그인 실패패킷을 보냄
		CPacket& sendPacket = AllocPacket();
		Msg_LoginResponseFail(sendPacket, accountNo);
		SendUnicast(sessionId, sendPacket);
		sendPacket.SubUseCount();

		DisconnectSession(sessionId);
		_monitor.disconnByDupAccount++;

		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv another session is handling same account number. session:%lld, accountNo:%lld\n", sessionId, accountNo);
		return;
	}

	// DB에서 계정 정보, status 조회
	CDBConnector* pDBConn = _tlsDBConnector->GetTlsDBConnector();
	MYSQL_RES* myRes = pDBConn->ExecuteQueryAndGetResult(
		L" SELECT a.`userid`, a.`userpass`, a.`usernick`, b.`status`"
		L" FROM `accountdb`.`account` a"
		L" INNER JOIN `accountdb`.`status` b"
		L" ON a.`accountno` = b.`accountno`"
		L" WHERE a.`accountno` = %lld"
		L" AND b.`accountno` = %lld"
		, accountNo, accountNo);

	int numRows = (int)pDBConn->GetNumQueryRows(myRes);
	if (numRows == 0)
	{
		// account 번호에 대한 데이터가 검색되지 않을 경우 로그인 실패
		LOGGING(LOGGING_LEVEL_DEBUG, L"Can't find account data from account table. accountNo:%lld\n", accountNo);
		CPacket& sendPacket = AllocPacket();
		Msg_LoginResponseAccountMiss(sendPacket, accountNo);
		SendUnicast(sessionId, sendPacket);
		sendPacket.SubUseCount();

		DisconnectSession(sessionId);
		DeleteAccountFromMap(accountNo);
		pDBConn->FreeResult(myRes);
		_monitor.disconnByNoAccount++;
		return;
	}

	// fetch row
	WCHAR userId[20] = { 0, };
	WCHAR userNick[20] = { 0, };
	int status;
	MYSQL_ROW myRow = pDBConn->FetchRow(myRes);
	MultiByteToWideChar(CP_UTF8, 0, myRow[0], -1, userId, sizeof(userId) / sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, myRow[2], -1, userNick, sizeof(userNick) / sizeof(WCHAR));
	status = std::stoi(myRow[3]);
	pDBConn->FreeResult(myRes);

	// 클라이언트 정보 세팅
	pClient->LockExclusive();  // lock
	pClient->SetClientInfo(accountNo, userId, userNick, sessionKey);
	pClient->SetLogin();
	pClient->UnlockExclusive();  // unlock

	// status가 0이면 1로 업데이트 한다. (현재 DB 업데이트는 하지않고있음)
	//if (status == 0)
	//{
	//	DB_UpdateClientStatus(pClient);
	//}

	// redis에 세션key를 세팅함
	std::string redisKey = std::to_string(accountNo);
	_pRedisClient->set(redisKey, sessionKey);
	_pRedisClient->sync_commit();


	// 클라이언트에게 로그인성공 응답 발송
	CPacket& sendPacket = AllocPacket();
	Msg_LoginResponseSucceed(sendPacket, pClient);
	SendUnicast(sessionId, sendPacket);
	//SendUnicastAndDisconnect(sessionId, sendPacket);  // 보내고 연결끊음
	sendPacket.SubUseCount();


	// account 번호를 map에서 제거
	DeleteAccountFromMap(accountNo);

	_monitor.loginCount++;

	LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv send login succeed. session:%lld, accountNo:%lld\n", sessionId, accountNo);
}

void CLoginServer::Msg_LoginResponseFail(CPacket& packet, __int64 accountNo)
{
	packet << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN << accountNo << (BYTE)en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_FAIL;
}

void CLoginServer::Msg_LoginResponseAccountMiss(CPacket& packet, __int64 accountNo)
{
	packet << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN << accountNo << (BYTE)en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_ACCOUNT_MISS;
}

void CLoginServer::Msg_LoginResponseSucceed(CPacket& packet, CClient_t& pClient)
{
	packet << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN << pClient->_accountNo << (BYTE)en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK;
	packet.PutData((char*)pClient->_id, sizeof(pClient->_id));
	packet.PutData((char*)pClient->_nickname, sizeof(pClient->_nickname));
	if (pClient->_accountNo < 50000)
	{
		packet.PutData((char*)_config.szGameServerDummy1IP, 32);
		packet << _config.gameServerPort;
		packet.PutData((char*)_config.szChatServerDummy1IP, 32);
		packet << _config.chatServerPort;
	}
	else if (pClient->_accountNo < 100000)
	{
		packet.PutData((char*)_config.szGameServerDummy2IP, 32);
		packet << _config.gameServerPort;
		packet.PutData((char*)_config.szChatServerDummy2IP, 32);
		packet << _config.chatServerPort;
	}
	else
	{
		packet.PutData((char*)_config.szGameServerIP, 32);
		packet << _config.gameServerPort;
		packet.PutData((char*)_config.szChatServerIP, 32);
		packet << _config.chatServerPort;
	}
}

void CLoginServer::DB_UpdateClientStatus(CClient_t& pClient)
{
	CDBConnector* pDBConn = _tlsDBConnector->GetTlsDBConnector();
	int numAffectedRow = pDBConn->ExecuteQuery(
		L" UPDATE `accountdb`.`status`"
		L" SET `status` = 1"
		L" WHERE `accountno` = %lld"
		, pClient->_accountNo);
	// numAffectedRow 는 update할 때 대상데이터는 있어도 변경되는 데이터가 없으면 0이 리턴됨
}









/* 주기적 작업 처리 스레드 */
unsigned WINAPI CLoginServer::ThreadPeriodicWorker(PVOID pParam)
{
	wprintf(L"[Login Server] begin periodic worker thread. id:%u\n", GetCurrentThreadId());
	CLoginServer& server = *(CLoginServer*)pParam;

	const int numJob = 1;       // job 타입 수
	int jogPeriod[numJob] = { server._timeoutLoginCheckPeriod };  // job 발생 주기
	EServerJobType job[numJob] = { EServerJobType::JOB_CHECK_LOGIN_TIMEOUT }; // job 타입
	ULONGLONG lastJobTime[numJob] = { 0 };  // 마지막으로 해당 타입 job을 처리한 시간

	ULONGLONG currentTime;  // 현재 시간
	int nextJobIdx;         // 다음에 처리할 job 타입
	ULONGLONG nextJobTime;  // 다음 job 처리 시간
	while (true)
	{
		currentTime = GetTickCount64();

		// 다음에 처리할 job을 선택한다.
		nextJobIdx = -1;
		nextJobTime = UINT64_MAX;
		for (int i = 0; i < numJob; i++)
		{
			// 다음 job 처리시간(마지막으로 job을 처리한 시간 + job 처리 주기)이 currentTime보다 작으면 바로 job 처리
			ULONGLONG t = lastJobTime[i] + jogPeriod[i];
			if (t <= currentTime)
			{
				nextJobIdx = i;
				nextJobTime = currentTime;
				break;
			}
			// 그렇지 않으면 기다려야하는 최소시간을 찾음
			else
			{
				if (t < nextJobTime)
				{
					nextJobIdx = i;
					nextJobTime = t;
				}
			}
		}

		if (server._bShutdown == true)
			break;
		// 다음 job 처리시간까지 기다림
		if (nextJobTime > currentTime)
		{
			Sleep((DWORD)(nextJobTime - currentTime));
		}
		if (server._bShutdown == true)
			break;

		// 마지막으로 job을 처리한 시간 업데이트
		lastJobTime[nextJobIdx] = nextJobTime;

		// 타입에 따른 job 처리
		switch (job[nextJobIdx])
		{
			// 로그인 타임아웃 처리
		case EServerJobType::JOB_CHECK_LOGIN_TIMEOUT:
		{
			PROFILE_BEGIN("CLoginServer::ThreadPeriodicJobHandler::JOB_CHECK_LOGIN_TIMEOUT");

			LOGGING(LOGGING_LEVEL_DEBUG, L"job check login timeout\n");

			// 로그인 타임아웃 대상 세션ID를 벡터에 저장한다.
			std::vector<__int64> vecTimeoutSessionId;
			vecTimeoutSessionId.reserve(100);
			CClient_t pClient;
			ULONGLONG currentTime;
			server._smtxMapClient.lock();
			for (auto iter = server._mapClient.begin(); iter != server._mapClient.end(); ++iter)
			{
				currentTime = GetTickCount64();
				pClient = iter->second;
				//if (pClient->_bLogin == false
				//	&& pClient->_bDisconnect == false
				//	&& currentTime - min(currentTime, pClient->_lastHeartBeatTime) > server._timeoutLogin)
				if (pClient->_bDisconnect == false
					&& currentTime - min(currentTime, pClient->_lastHeartBeatTime) > server._timeoutLogin)
				{
					vecTimeoutSessionId.push_back(pClient->_sessionId);
				}
			}
			server._smtxMapClient.unlock();

			server._monitor.disconnByLoginTimeout += vecTimeoutSessionId.size();
			// 로그인 타임아웃 대상 세션의 연결을 끊는다.
			for (int i = 0; i < vecTimeoutSessionId.size(); i++)
			{
				LOGGING(LOGGING_LEVEL_DEBUG, L"timeout login. sessionId:%lld\n", vecTimeoutSessionId[i]);
				server.DisconnectSession(vecTimeoutSessionId[i]);
			}

			break;
		}

		default:
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"periodic worker thread got invalid job type. index:%d, type:%d\n", nextJobIdx, job[nextJobIdx]);
			break;
		}
		}

	}

	wprintf(L"[Login Server] end periodic worker thread. id:%u\n", GetCurrentThreadId());
	return 0;
}





/* (static) 모니터링 데이터 수집 스레드 */
unsigned WINAPI CLoginServer::ThreadMonitoringCollector(PVOID pParam)
{
	wprintf(L"begin monitoring collector thread\n");
	CLoginServer& server = *(CLoginServer*)pParam;

	PDHCount pdhCount;
	LARGE_INTEGER liFrequency;
	LARGE_INTEGER liStartTime;
	LARGE_INTEGER liEndTime;
	time_t collectTime;
	__int64 spentTime;
	__int64 sleepTime;
	DWORD dwSleepTime;


	__int64 prevLoginCount = 0;
	__int64 currLoginCount = 0;

	QueryPerformanceFrequency(&liFrequency);

	// 최초 update
	server._pCPUUsage->UpdateCpuTime();
	server._pPDH->Update();

	// 최초 sleep
	Sleep(990);

	// 1초마다 모니터링 데이터를 수집하여 모니터링 서버에게 데이터를 보냄
	QueryPerformanceCounter(&liStartTime);
	while (server._bShutdown == false)
	{
		// 데이터 수집
		time(&collectTime);
		server._pCPUUsage->UpdateCpuTime();
		server._pPDH->Update();
		pdhCount = server._pPDH->GetPDHCount();
		currLoginCount = server._monitor.GetLoginCount();

		// 모니터링 서버에 send
		lanlib::CPacket& packet = server._pLANClientMonitoring->AllocPacket();
		packet << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN << (int)1 << (int)collectTime; // 로그인서버 실행여부 ON / OFF
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU << (int)server._pCPUUsage->ProcessTotal() << (int)collectTime; // 로그인서버 CPU 사용률
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM << (int)((double)pdhCount.processPrivateBytes / 1048576.0) << (int)collectTime; // 로그인서버 메모리 사용 MByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_SESSION << server.GetNumSession() << (int)collectTime; // 로그인서버 세션 수 (컨넥션 수)
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS << (int)(currLoginCount - prevLoginCount) << (int)collectTime; // 로그인서버 인증 처리 초당 횟수
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL << server.GetPacketAllocCount() << (int)collectTime; // 로그인서버 패킷풀 사용량

		prevLoginCount = currLoginCount;

		server._pLANClientMonitoring->SendPacket(packet);
		packet.SubUseCount();


		// 앞으로 sleep할 시간을 계산한다.
		QueryPerformanceCounter(&liEndTime);
		spentTime = max(0, liEndTime.QuadPart - liStartTime.QuadPart);
		sleepTime = liFrequency.QuadPart - spentTime;  // performance counter 단위의 sleep 시간

		// sleep 시간을 ms 단위로 변환하여 sleep 한다.
		dwSleepTime = 0;
		if (sleepTime > 0)
		{
			dwSleepTime = (DWORD)round((double)(sleepTime * 1000) / (double)liFrequency.QuadPart);
			Sleep(dwSleepTime);
		}

		// 다음 로직의 시작시간은 [현재 로직 종료시간 + sleep한 시간] 이다.
		liStartTime.QuadPart = liEndTime.QuadPart + (dwSleepTime * (liFrequency.QuadPart / 1000));

	}

	wprintf(L"end monitoring collector thread\n");
	return 0;
}