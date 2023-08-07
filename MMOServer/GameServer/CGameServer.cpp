#pragma comment (lib, "GameNetLib.lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")

#include "CGameServer.h"
#include "CGameContents.h"

#include "CGameContentsAuth.h"
#include "CGameContentsField.h"

#include "CommonProtocol.h"
#include "CLANClientMonitoring.h"
#include "../utils/logger.h"
#include "../utils/CJsonParser.h"
#include "../utils/CCpuUsage.h"
#include "../utils/CPDH.h"

CGameServer::CGameServer()
	: _bShutdown(false)
	, _bTerminated(false)
	, _pRedisClient(nullptr)
	//, _poolPlayer(0, false, 100)
	, _poolPlayer(10000, true, false)
	, _pLANClientMonitoring(nullptr)
	, _pCPUUsage(nullptr)
	, _pPDH(nullptr)
{
	_serverNo = 2;   // 서버번호(게임서버:2)
}

CGameServer::~CGameServer()
{
}


CGameServer::Config::Config()
	: jsonParser(nullptr)
	, szBindIP{ 0, }
	, portNumber(0)
	, numConcurrentThread(0)
	, numWorkerThread(0)
	, numMaxSession(0)
	, numMaxPlayer(0)
	, bUseNagle(true)
	, packetCode(0)
	, packetKey(0)
	, maxPacketSize(0)
	, monitoringServerPortNumber(0)
	, monitoringServerPacketCode(0)
	, monitoringServerPacketKey(0)
{
}

const CJsonParser& CGameServer::Config::GetJsonParser() const
{
	return *jsonParser;
}


bool CGameServer::StartUp()
{
	// 패킷풀 설정
	netlib_game::CPacket::RegeneratePacketPool(10000, 0, 1000);

	// config 파일 읽기
	// 현재 경로와 config 파일 경로를 얻음
	WCHAR szPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szPath);
	_sCurrentPath = szPath;
	_sConfigFilePath = _sCurrentPath + L"\\config_game_server.json";

	// config parse
	_config.jsonParser = std::make_unique<CJsonParser>();
	_config.jsonParser->ParseJsonFile(_sConfigFilePath.c_str());
	CJsonParser& jsonParser = *_config.jsonParser;

	// 네트워크 config
	const wchar_t* serverBindIP = jsonParser[L"ServerBindIP"].Str().c_str();
	wcscpy_s(_config.szBindIP, wcslen(serverBindIP) + 1, serverBindIP);
	_config.portNumber = jsonParser[L"GameServerPort"].Int();
	_config.numConcurrentThread = jsonParser[L"NetRunningWorkerThread"].Int();
	_config.numWorkerThread = jsonParser[L"NetWorkerThread"].Int();
	_config.bUseNagle = (bool)jsonParser[L"EnableNagle"].Int();
	_config.numMaxSession = jsonParser[L"SessionLimit"].Int();
	_config.numMaxPlayer = jsonParser[L"PlayerLimit"].Int();
	_config.packetCode = jsonParser[L"PacketHeaderCode"].Int();
	_config.packetKey = jsonParser[L"PacketEncodeKey"].Int();
	_config.maxPacketSize = jsonParser[L"MaximumPacketSize"].Int();

	// 모니터링 서버 config
	_config.monitoringServerPortNumber = jsonParser[L"MonitoringServerPort"].Int();
	_config.monitoringServerPacketCode = jsonParser[L"MonitoringServerPacketHeaderCode"].Int();
	_config.monitoringServerPacketKey = jsonParser[L"MonitoringServerPacketEncodeKey"].Int();

	// Authentication 컨텐츠 config
	int AuthFPS = jsonParser[L"ContentsAuthentication"][L"FPS"].Int();
	int AuthSessionPacketProcessLimit = jsonParser[L"ContentsAuthentication"][L"SessionPacketProcessLimit"].Int();
	int AuthSessionTransferLimit = jsonParser[L"ContentsAuthentication"][L"SessionTransferLimit"].Int();
	int AuthHeartBeatTimeout = jsonParser[L"ContentsAuthentication"][L"HeartBeatTimeout"].Int();

	// Field 컨텐츠 config
	int FieldFPS = jsonParser[L"ContentsField"][L"FPS"].Int();
	int FieldSessionPacketProcessLimit = jsonParser[L"ContentsField"][L"SessionPacketProcessLimit"].Int();
	int FieldHeartBeatTimeout = jsonParser[L"ContentsField"][L"HeartBeatTimeout"].Int();

	// logger 초기화
	//logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_NONE);
	logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_INFO);
	//logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_DEBUG);

	logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_INFO);
	//logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_DEBUG);
	//logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_NONE);

	CNetServer::SetOutputDebug(true);


	LOGGING(LOGGING_LEVEL_INFO, L"\n********** StartUp Game Server (goddambug online) ************\n"
		L"Config File: %s\n"
		L"Server Bind IP: %s\n"
		L"Game Server Port: %d\n"
		L"Number of Network Worker Thread: %d\n"
		L"Number of Network Running Worker Thread: %d\n"
		L"Number of Maximum Session: %d\n"
		L"Number of Maximum Player: %d\n"
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
		, _config.numMaxPlayer
		, _config.bUseNagle ? L"Yes" : L"No"
		, _config.packetCode
		, _config.packetKey
		, _config.maxPacketSize);



	// 컨텐츠 클래스 생성 및 설정
	std::shared_ptr<CGameContentsAuth> pAuth(new CGameContentsAuth(EContentsThread::AUTH, CreateAccessor(), AuthFPS));
	_mapContents.insert(std::make_pair(EContentsThread::AUTH, pAuth));
	pAuth->SetSessionPacketProcessLimit(AuthSessionPacketProcessLimit);
	pAuth->EnableHeartBeatTimeout(AuthHeartBeatTimeout);
	pAuth->SetSessionTransferLimit(AuthSessionTransferLimit);

	std::shared_ptr<CGameContentsField> pField(new CGameContentsField(EContentsThread::FIELD, CreateAccessor(), FieldFPS));
	_mapContents.insert(std::make_pair(EContentsThread::FIELD, pField));
	pField->SetSessionPacketProcessLimit(FieldSessionPacketProcessLimit);
	pField->EnableHeartBeatTimeout(FieldHeartBeatTimeout);
	pField->Init();


	// 최초 접속 컨텐츠 지정
	CNetServer::SetInitialContents(pAuth);


	// 모니터링 서버와 연결하는 클라이언트 start
	_pLANClientMonitoring = std::make_unique<CLANClientMonitoring>();
	if (_pLANClientMonitoring->StartUp(L"127.0.0.1", _config.monitoringServerPortNumber, true, _config.monitoringServerPacketCode, _config.monitoringServerPacketKey, 10000, true) == false)
	{
		wprintf(L"LAN client start failed\n");
		//return false;
	}

	// 모니터링 서버에 접속
	CPacket* pPacket = _pLANClientMonitoring->AllocPacket();
	*pPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN << _serverNo;
	_pLANClientMonitoring->SendPacket(pPacket);
	pPacket->SubUseCount();

	// 모니터링 데이터 수집 스레드 start
	_pCPUUsage = std::make_unique<CCpuUsage>();
	_pPDH = std::make_unique<CPDH>();
	_pPDH->Init();
	_thMonitoring.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadMonitoringCollector, (PVOID)this, 0, &_thMonitoring.id);
	if (_thMonitoring.handle == 0)
	{
		wprintf(L"failed to start monitoring collector thread. error:%u\n", GetLastError());
		return false;
	}



	// 네트워크 start
	bool retNetStart = CNetServer::StartUp(_config.szBindIP, _config.portNumber, _config.numConcurrentThread, _config.numWorkerThread, _config.bUseNagle, _config.numMaxSession, _config.packetCode, _config.packetKey, _config.maxPacketSize, true, true);
	if (retNetStart == false)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[GameServer] Network Library Start failed\n");
		return false;
	}


	// redis start
	_pRedisClient = std::make_unique<cpp_redis::client>();
	_pRedisClient->connect();


	// 컨텐츠 스레드 attach 및 start
	if (CNetServer::AttachContents(pAuth) == false)
		LOGGING(LOGGING_LEVEL_ERROR, L"[GameServer] Field thread start failed\n");

	if(CNetServer::AttachContents(pField) == false)
		LOGGING(LOGGING_LEVEL_ERROR, L"[GameServer] Authentication thread start failed\n");


	return true;
}


void CGameServer::Shutdown()
{
	_bShutdown = true;

	// 네트워크 shutdown
	CNetServer::Shutdown();

	// DB 연결 종료
	//_pDBConn->Close();

	// 컨텐츠 객체 삭제
	Sleep(1000);
	_mapContents.clear();

	_bTerminated = true;
}


/* 컨텐츠 */
const std::shared_ptr<CGameContents> CGameServer::GetGameContents(EContentsThread contents)
{
	auto iter = _mapContents.find(contents);
	if (iter == _mapContents.end())
		return nullptr;

	return std::dynamic_pointer_cast<CGameContents>(iter->second);
}



/* 컨텐츠 */
std::shared_ptr<netlib_game::CContents> CGameServer::GetContents(EContentsThread contents)
{
	auto iter = _mapContents.find(contents);
	if (iter == _mapContents.end())
		return nullptr;
	
	return iter->second;
}


/* accessor */
std::unique_ptr<CGameServerAccessor> CGameServer::CreateAccessor()
{
	return std::unique_ptr<CGameServerAccessor>(new CGameServerAccessor(*this));
}


/* object pool */
void CGameServer::FreePlayer(CPlayer& player) {
	for (int i = 0; i < player._vecPacketBuffer.size(); i++)
	{
		player._vecPacketBuffer[i]->SubUseCount();
	}
	player._vecPacketBuffer.clear();
	_poolPlayer.Free(&player);
}

/* (static) 모니터링 데이터 수집 스레드 */
unsigned WINAPI CGameServer::ThreadMonitoringCollector(PVOID pParam)
{
	wprintf(L"begin monitoring collector thread\n");
	CGameServer& server = *(CGameServer*)pParam;

	CPacket* pPacket;
	PDHCount pdhCount;
	LARGE_INTEGER liFrequency;
	LARGE_INTEGER liStartTime;
	LARGE_INTEGER liEndTime;
	time_t collectTime;
	__int64 spentTime;
	__int64 sleepTime;
	DWORD dwSleepTime;

	__int64 prevAcceptCount = 0;
	__int64 currAcceptCount = 0;
	__int64 prevRecvCount = 0;
	__int64 currRecvCount = 0;
	__int64 prevSendCount = 0;
	__int64 currSendCount = 0;
	__int64 currDBQueryCount = 0;
	__int64 prevDBQueryCount = 0;


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
		std::shared_ptr<CGameContents> pContentsAuth = server.GetGameContents(EContentsThread::AUTH);
		std::shared_ptr<CGameContents> pContentsField = server.GetGameContents(EContentsThread::FIELD);

		// 데이터 수집
		time(&collectTime);
		server._pCPUUsage->UpdateCpuTime();
		server._pPDH->Update();
		pdhCount = server._pPDH->GetPDHCount();
		currAcceptCount = server._monitor.GetAcceptCount();
		currRecvCount = server._monitor.GetRecvCount();
		currSendCount = server._monitor.GetSendCount();
		currDBQueryCount = pContentsField->GetQueryRunCount();

		// 모니터링 서버에 send
		pPacket = server._pLANClientMonitoring->AllocPacket();
		*pPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_SERVER_RUN << (int)1 << (int)collectTime; // GameServer 실행 여부 ON / OFF
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_SERVER_CPU << (int)server._pCPUUsage->ProcessTotal() << (int)collectTime; // GameServer CPU 사용률
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_SERVER_MEM << (int)((double)pdhCount.processPrivateBytes / 1048576.0) << (int)collectTime; // GameServer 메모리 사용 MByte
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_SESSION << (int)server.GetNumSession() << (int)collectTime; // 게임서버 세션 수 (컨넥션 수)
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER << (int)pContentsAuth->GetNumPlayer() << (int)collectTime; // 게임서버 AUTH MODE 플레이어 수
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER << (int)pContentsField->GetNumPlayer() << (int)collectTime; // 게임서버 GAME MODE 플레이어 수
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS << (int)(currAcceptCount - prevAcceptCount) << (int)collectTime; // 게임서버 Accept 처리 초당 횟수
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS << (int)(currRecvCount - prevRecvCount) << (int)collectTime; // 게임서버 패킷처리 초당 횟수
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS << (int)(currSendCount - prevSendCount) << (int)collectTime; // 게임서버 패킷 보내기 초당 완료 횟수
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS << (int)(currDBQueryCount - prevDBQueryCount) << (int)collectTime; // 게임서버 DB 저장 메시지 초당 처리 횟수
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG << (int)pContentsField->GetRemainQueryCount() << (int)collectTime; // 게임서버 DB 저장 메시지 큐 개수 (남은 수)
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS << (int)pContentsAuth->GetFPS() << (int)collectTime; // 게임서버 AUTH 스레드 초당 프레임 수 (루프 수)
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS << (int)pContentsField->GetFPS() << (int)collectTime; // 게임서버 GAME 스레드 초당 프레임 수 (루프 수)
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_GAME_PACKET_POOL << (int)server.GetPacketActualAllocCount() << (int)collectTime; // 게임서버 패킷풀 사용량

		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL << (int)server._pCPUUsage->ProcessorTotal() << (int)collectTime; // 서버컴퓨터 CPU 전체 사용률
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY << (int)((double)pdhCount.systemNonpagedPoolBytes / 1048576.0) << (int)collectTime; // 서버컴퓨터 논페이지 메모리 MByte
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV << (int)(pdhCount.networkRecvBytes / 1024.0) << (int)collectTime; // 서버컴퓨터 네트워크 수신량 KByte
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND << (int)(pdhCount.networkSendBytes / 1024.0) << (int)collectTime; // 서버컴퓨터 네트워크 송신량 KByte
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY << (int)((double)pdhCount.systemAvailableBytes / 1048576.0) << (int)collectTime; // 서버컴퓨터 사용가능 메모리 MByte

		prevAcceptCount = currAcceptCount;
		prevRecvCount = currRecvCount;
		prevSendCount = currSendCount;
		prevDBQueryCount = currDBQueryCount;

		server._pLANClientMonitoring->SendPacket(pPacket);
		pPacket->SubUseCount();


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









/* 네트워크 callback 함수 구현 */
bool CGameServer::OnConnectionRequest(unsigned long IP, unsigned short port)
{
	return true;
}


void CGameServer::OnError(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_ERROR, szError, vaList);
	va_end(vaList);
}


void CGameServer::OnOutputDebug(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_DEBUG, szError, vaList);
	va_end(vaList);
}

void CGameServer::OnOutputSystem(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_INFO, szError, vaList);
	va_end(vaList);
}








CGameServerAccessor::CGameServerAccessor(CGameServer& gameServer)
	: _gameServer(gameServer)
{
}

CGameServerAccessor::~CGameServerAccessor()
{
}