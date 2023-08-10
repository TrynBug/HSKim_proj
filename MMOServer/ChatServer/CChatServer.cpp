
#include "stdafx.h"

#pragma comment (lib, "NetLib.lib")
#pragma comment (lib, "LANClient.lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")


#include "CChatServer.h"
#include "CObject.h"
#include "CPlayer.h"
#include "../common/CommonProtocol.h"


#include "../utils/CDBAsyncWriter.h"
#include "../utils/cpp_redis/cpp_redis.h"
#include "../utils/profiler.h"
#include "../utils/logger.h"
#include "../utils/CJsonParser.h"
#include "../utils/CCpuUsage.h"
#include "../utils/CPDH.h"

using namespace chatserver;

CChatServer::CChatServer()
	: _pRedisClient(nullptr)
	, _pDBConn(nullptr)
	, _sector(SECTOR_MAX_X, SECTOR_MAX_Y, 1)
	, _hEventMsg(0)
	, _bEventSetFlag(false)
	, _bShutdown(false)
	, _bTerminated(false)
	, _poolMsg(0, false, 100)      // 메모리풀 초기화
	, _poolPlayer(0, false, 100)   // 메모리풀 초기화
	, _poolAPCData(0, false, 100)  // 메모리풀 초기화
	, _pLANClientMonitoring(nullptr)
	, _pCPUUsage(nullptr)
	, _pPDH(nullptr)
{
	// DB Connector 생성
	_pDBConn = std::make_unique<CDBAsyncWriter>();

	_serverNo = 1;  // 서버번호(채팅서버:1)

	_timeoutLogin = 10000000;
	_timeoutHeartBeat = 10000000;
	_timeoutLoginCheckPeriod = 10000;
	_timeoutHeartBeatCheckPeriod = 30000;

	QueryPerformanceFrequency(&_performanceFrequency);
}

CChatServer::~CChatServer() 
{
	Shutdown();
}


CChatServer::Config::Config()
	: szBindIP{ 0, }
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

void CChatServer::Config::LoadConfigFile(std::wstring& sFilePath)
{
	// config parse
	CJsonParser jsonParser;
	jsonParser.ParseJsonFile(sFilePath.c_str());

	const wchar_t* serverBindIP = jsonParser[L"ServerBindIP"].Str().c_str();
	wcscpy_s(szBindIP, wcslen(serverBindIP) + 1, serverBindIP);
	portNumber = jsonParser[L"ChatServerPort"].Int();

	numConcurrentThread = jsonParser[L"NetRunningWorkerThread"].Int();
	numWorkerThread = jsonParser[L"NetWorkerThread"].Int();

	bUseNagle = (bool)jsonParser[L"EnableNagle"].Int();
	numMaxSession = jsonParser[L"SessionLimit"].Int();
	numMaxPlayer = jsonParser[L"PlayerLimit"].Int();
	packetCode = jsonParser[L"PacketHeaderCode"].Int();
	packetKey = jsonParser[L"PacketEncodeKey"].Int();
	maxPacketSize = jsonParser[L"MaximumPacketSize"].Int();

	monitoringServerPortNumber = jsonParser[L"MonitoringServerPort"].Int();
	monitoringServerPacketCode = jsonParser[L"MonitoringServerPacketHeaderCode"].Int();
	monitoringServerPacketKey = jsonParser[L"MonitoringServerPacketEncodeKey"].Int();
}


bool CChatServer::StartUp()
{
	// config 파일 읽기
	// 현재 경로와 config 파일 경로를 얻음
	WCHAR szPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szPath);
	_sCurrentPath = szPath;
	_sConfigFilePath = _sCurrentPath + L"\\chat_server_config.json";

	// config 파일 로드
	_config.LoadConfigFile(_sConfigFilePath);


	// map 초기크기 지정
	_mapPlayer.max_load_factor(1.0f);
	_mapPlayer.rehash(_config.numMaxPlayer * 4);
	_mapPlayerAccountNo.max_load_factor(1.0f);
	_mapPlayerAccountNo.rehash(_config.numMaxPlayer * 4);


	// logger 초기화
	//logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_WARN);
	logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_INFO);
	//logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_DEBUG);

	logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_INFO);
	//logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_DEBUG);
	//logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_ERROR);

	//CNetServer::SetOutputDebug(true);


	LOGGING(LOGGING_LEVEL_INFO, L"\n********** StartUp Chat Server (single thread version) ************\n"
		L"Config File: %s\n"
		L"Server Bind IP: %s\n"
		L"Chat Server Port: %d\n"
		L"Monitoring Server Port: %d\n"
		L"Number of Network Worker Thread: %d\n"
		L"Number of Network Running Worker Thread: %d\n"
		L"Number of Maximum Session: %d\n"
		L"Number of Maximum Player: %d\n"
		L"Enable Nagle: %s\n"
		L"Packet Header Code: 0x%x\n"
		L"Packet Encode Key: 0x%x\n"
		L"Maximum Packet Size: %d\n"
		L"Monitoring Server Packet Header Code: 0x%x\n"
		L"Monitoring Server Packet Encode Key: 0x%x\n"
		L"*******************************************************************\n\n"
		, _sConfigFilePath.c_str()
		, _config.szBindIP
		, _config.portNumber
		, _config.monitoringServerPortNumber
		, _config.numConcurrentThread
		, _config.numWorkerThread
		, _config.numMaxSession
		, _config.numMaxPlayer
		, _config.bUseNagle ? L"Yes" : L"No"
		, _config.packetCode
		, _config.packetKey
		, _config.maxPacketSize
		, _config.monitoringServerPacketCode
		, _config.monitoringServerPacketKey);
	
	// event 객체 생성
	_hEventMsg = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (_hEventMsg == NULL)
	{
		wprintf(L"CreateEvent failed!!, error:%d\n", GetLastError());
		return false;
	}

	// 메모리 정렬 체크
	if ((unsigned long long)this % 64 != 0)
		LOGGING(LOGGING_LEVEL_ERROR, L"[chat server] chat server object is not aligned as 64\n");
	if ((unsigned long long) & _msgQ % 64 != 0)
		LOGGING(LOGGING_LEVEL_ERROR, L"[chat server] message queue object is not aligned as 64\n");


	// 채팅서버 스레드 start
	_thChatServer.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadChatServer, (PVOID)this, 0, &_thChatServer.id);
	if (_thChatServer.handle == 0)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[chat server] failed to start chat thread. error:%u\n", GetLastError());
		Crash();
		return false;
	}


	// 메시지 생성 스레드 start
	_thMsgGenerator.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadMsgGenerator, (PVOID)this, 0, &_thMsgGenerator.id);
	if (_thMsgGenerator.handle == 0)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[chat server] failed to start message generator thread. error:%u\n", GetLastError());
		Crash();
		return false;
	}

	// DB Connect 및 DB Writer 스레드 start
	if (_pDBConn->ConnectAndRunThread("127.0.0.1", "root", "vmfhzkepal!", "accountdb", 3306) == false)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[chat server] DB Connector start failed!!\n");
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
		LOGGING(LOGGING_LEVEL_FATAL, L"[chat server] failed to start monitoring collector thread. error:%u\n", GetLastError());
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
		LOGGING(LOGGING_LEVEL_FATAL, L"[chat server] Redis connect failed\n", GetLastError());
		Crash();
		return false;
	}

	// 네트워크 start
	bool retNetStart = CNetServer::StartUp(_config.szBindIP, _config.portNumber, _config.numConcurrentThread, _config.numWorkerThread, _config.bUseNagle, _config.numMaxSession, _config.packetCode, _config.packetKey, _config.maxPacketSize, true, true);
	if (retNetStart == false)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[chat server] Network Library Start failed\n");
		Crash();
		return false;
	}

	return true;
}


/* Shutdown */
bool CChatServer::Shutdown()
{
	std::lock_guard<std::mutex> lock_guard(_mtxShutdown);
	if (_bShutdown == true)
		return true;

	// ThreadMsgGenerator 는 이 값이 true가 되면 종료됨
	_bShutdown = true;  

	// 채팅서버 스레드에게 shutdown 메시지 삽입
	MsgChatServer* pMsg = _poolMsg.Alloc();
	pMsg->msgFrom = MSG_FROM_SERVER;
	pMsg->sessionId = 0;
	pMsg->pPacket = nullptr;
	pMsg->eServerMsgType = EServerMsgType::MSG_SHUTDOWN;
	_msgQ.Enqueue(pMsg);
	SetEvent(_hEventMsg);

	// accept 중지
	CNetServer::StopAccept();



	// 네트워크 shutdown
	CNetServer::Shutdown();

	// 채팅서버 스레드들이 종료되기를 60초간 기다림
	ULONGLONG timeout = 60000;
	ULONGLONG tick = GetTickCount64();
	DWORD retWait;
	retWait = WaitForSingleObject(_thChatServer.handle, (DWORD)timeout);
	if (retWait != WAIT_OBJECT_0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[chat server] terminate chat thread timeout. error:%u\n", GetLastError());
		TerminateThread(_thChatServer.handle, 0);
	}
	timeout = timeout - GetTickCount64() - tick;
	timeout = timeout < 1 ? 1 : timeout;
	retWait = WaitForSingleObject(_thMsgGenerator.handle, (DWORD)timeout);
	if (retWait != WAIT_OBJECT_0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[chat server] terminate message generator thread timeout. error:%u\n", GetLastError());
		TerminateThread(_thMsgGenerator.handle, 0);
	}

	// DB 종료
	_pDBConn->Close();

	_bTerminated = true;
	return true;
}


/* Get DB 상태 */
int CChatServer::GetUnprocessedQueryCount() { return _pDBConn->GetUnprocessedQueryCount(); }
__int64 CChatServer::GetQueryRunCount() { return _pDBConn->GetQueryRunCount(); }
float CChatServer::GetMaxQueryRunTime() { return _pDBConn->GetMaxQueryRunTime(); }
float CChatServer::Get2ndMaxQueryRunTime() { return _pDBConn->Get2ndMaxQueryRunTime(); }
float CChatServer::GetMinQueryRunTime() { return _pDBConn->GetMinQueryRunTime(); }
float CChatServer::Get2ndMinQueryRunTime() { return _pDBConn->Get2ndMinQueryRunTime(); }
float CChatServer::GetAvgQueryRunTime() { return _pDBConn->GetAvgQueryRunTime(); }
int CChatServer::GetQueryRequestPoolSize() { return _pDBConn->GetQueryRequestPoolSize(); }
int CChatServer::GetQueryRequestAllocCount() { return _pDBConn->GetQueryRequestAllocCount(); }
int CChatServer::GetQueryRequestFreeCount() { return _pDBConn->GetQueryRequestFreeCount(); }


/* Get */
CPlayer_t CChatServer::GetPlayerBySessionId(__int64 sessionId)
{
	const auto& iter = _mapPlayer.find(sessionId);
	if (iter == _mapPlayer.end())
		return nullptr;
	else
		return iter->second;
}


CPlayer_t CChatServer::GetPlayerByAccountNo(__int64 accountNo)
{
	const auto& iter = _mapPlayerAccountNo.find(accountNo);
	if (iter == _mapPlayerAccountNo.end())
		return nullptr;
	else
		return iter->second;
}

void CChatServer::ReplacePlayerByAccountNo(__int64 accountNo, CPlayer_t& replacePlayer)
{
	auto iter = _mapPlayerAccountNo.find(accountNo);
	iter->second = replacePlayer;
}


/* 네트워크 전송 */
int CChatServer::SendUnicast(__int64 sessionId, CPacket& packet)
{
	//return SendPacket(sessionId, packet);
	return SendPacketAsync(sessionId, packet);
}

int CChatServer::SendUnicast(const CPlayer_t& pPlayer, CPacket& packet)
{
	//return SendPacket(pPlayer->_sessionId, packet);
	return SendPacketAsync(pPlayer->_sessionId, packet);
}

int CChatServer::SendBroadcast(CPacket& packet)
{
	int sendCount = 0;
	for (const auto& iter : _mapPlayer)
	{
		//sendCount += SendPacket(iter.second->_sessionId, packet);
		sendCount += SendPacketAsync(iter.second->_sessionId, packet);
	}

	return sendCount;
}

int CChatServer::SendOneSector(const CPlayer_t& pPlayer, CPacket& packet, const CPlayer_t& except)
{
	int sendCount = 0;
	auto& vecPlayer = _sector.GetObjectVector(pPlayer->_sectorX, pPlayer->_sectorY, ESectorObjectType::PLAYER);

	for (int i=0; i<vecPlayer.size(); i++)
	{
		const CPlayer_t elmPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
		if (elmPlayer == except)
			continue;
		//sendCount += SendPacket(elmPlayer->_sessionId, packet);
		sendCount += SendPacketAsync(elmPlayer->_sessionId, packet);
	}

	return sendCount;
}

int CChatServer::SendAroundSector(const CPlayer_t& pPlayer, CPacket& packet, const CPlayer_t& except)
{
	int sendCount = 0;
	auto& vecSector = _sector.GetAroundSector(pPlayer->_sectorX, pPlayer->_sectorY);
	for (int i = 0; i < vecSector.size(); i++)
	{
		auto& vecPlayer = vecSector[i]->GetObjectVector(ESectorObjectType::PLAYER);
		for (int i = 0; i < vecPlayer.size(); i++)
		{
			const CPlayer_t elmPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
			if (elmPlayer == except)
				continue;
			//sendCount += SendPacket(elmPlayer->_sessionId, packet);
			sendCount += SendPacketAsync(elmPlayer->_sessionId, packet);
		}
	}

	return sendCount;
}


/* player */
CPlayer_t CChatServer::AllocPlayer(__int64 sessionId)
{
	auto Deleter = [this](CPlayer* pPlayer) {
		this->_poolPlayer.Free(pPlayer);
	};
	std::shared_ptr<CPlayer> pPlayer(_poolPlayer.Alloc(), Deleter);
	pPlayer->Init(sessionId);
	return pPlayer;
}

void CChatServer::MoveSector(CPlayer_t& pPlayer, WORD x, WORD y)
{
	if (pPlayer->_sectorX == x && pPlayer->_sectorY == y)
		return;

	if (pPlayer->_sectorX >= 0 && pPlayer->_sectorX < SECTOR_MAX_X
		&& pPlayer->_sectorY >= 0 && pPlayer->_sectorY < SECTOR_MAX_Y)
	{
		CObject_t o = std::static_pointer_cast<CObject>(pPlayer);
		CObject_t o2 = pPlayer;
		_sector.RemoveObject(pPlayer->_sectorX, pPlayer->_sectorY, ESectorObjectType::PLAYER, pPlayer);
	}
	
	pPlayer->_sectorX = min(max(x, 0), SECTOR_MAX_X - 1);
	pPlayer->_sectorY = min(max(y, 0), SECTOR_MAX_Y - 1);
	_sector.AddObject(pPlayer->_sectorX, pPlayer->_sectorY, ESectorObjectType::PLAYER, pPlayer);
}

void CChatServer::DisconnectPlayer(CPlayer_t& pPlayer)
{
	if (pPlayer->_bDisconnect == false)
	{
		pPlayer->_bDisconnect = true;
		CNetServer::Disconnect(pPlayer->_sessionId);
	}
}


void CChatServer::DeletePlayer(__int64 sessionId)
{
	const auto& iter = _mapPlayer.find(sessionId);
	if (iter == _mapPlayer.end())
		return;

	CPlayer_t& pPlayer = iter->second;
	if (pPlayer->_sectorX != SECTOR_NOT_SET && pPlayer->_sectorY != SECTOR_NOT_SET)
	{
		_sector.RemoveObject(pPlayer->_sectorX, pPlayer->_sectorY, ESectorObjectType::PLAYER, pPlayer);
	}

	const auto& iterAccountNo = _mapPlayerAccountNo.find(pPlayer->_accountNo);
	if(iterAccountNo != _mapPlayerAccountNo.end() 
		&& iterAccountNo->second->_sessionId == sessionId)  // 중복로그인이 발생하면 map의 player 객체가 교체되어 세션주소가 다를 수 있음. 이 경우 삭제하면 안됨
		_mapPlayerAccountNo.erase(iterAccountNo);

	_mapPlayer.erase(iter);
}


/* (static) 로그인 나머지 처리 APC 함수 */
void CChatServer::CompleteUnfinishedLogin(ULONG_PTR pStAPCData)
{
	StAPCData& APCData = *((StAPCData*)pStAPCData);
	CChatServer& chatServer = *APCData.pChatServer;
	CPacket* pRecvPacket = APCData.pPacket;
	__int64 sessionId = APCData.sessionId;
	__int64 accountNo = APCData.accountNo;
	bool isNull = APCData.isNull;
	char* redisSessionKey = APCData.sessionKey;

	LOGGING(LOGGING_LEVEL_DEBUG, L"CompleteUnfinishedLogin start. session:%lld, accountNo:%lld\n", sessionId, accountNo);

	CPacket& sendPacket = chatServer.AllocPacket();
	do  // do..while(0)
	{
		if (isNull == true)
		{
			// redis에 세션key가 없으므로 로그인 실패
			// 로그인실패 응답 발송
			sendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)0 << accountNo;
			chatServer.SendUnicast(sessionId, sendPacket);
			chatServer._monitor.disconnByNoSessionKey++; // 모니터링
			LOGGING(LOGGING_LEVEL_DEBUG, L"login failed by no session key. session:%lld, accountNo:%lld\n", sessionId, accountNo);

			// 연결 끊기
			chatServer.Disconnect(sessionId);
			break;
		}

		WCHAR id[20];
		WCHAR nickname[20];
		char  sessionKey[64];
		pRecvPacket->TakeData((char*)id, sizeof(id));
		pRecvPacket->TakeData((char*)nickname, sizeof(nickname));
		pRecvPacket->TakeData((char*)sessionKey, sizeof(sessionKey));

		if (memcmp(redisSessionKey, sessionKey, 64) != 0)
		{
			// 세션key가 다르기 때문에 로그인 실패
			// 로그인실패 응답 발송
			sendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)0 << accountNo;
			chatServer.SendUnicast(sessionId, sendPacket);
			chatServer._monitor.disconnByInvalidSessionKey++; // 모니터링
			LOGGING(LOGGING_LEVEL_DEBUG, L"login failed by invalid session key. session:%lld, accountNo:%lld\n", sessionId, accountNo);

			// 연결끊기
			chatServer.Disconnect(sessionId);
			break;
		}

		// 플레이어 얻기
		CPlayer_t pPlayer = chatServer.GetPlayerBySessionId(sessionId);
		if (pPlayer == nullptr)   // 플레이어를 찾지못했다면 APC큐에 요청이 삽입된다음 leave된 플레이어임
			break;

		// 플레이어 중복로그인 체크
		CPlayer_t pPlayerDup = chatServer.GetPlayerByAccountNo(accountNo);
		if (pPlayerDup != nullptr)
		{
			// 중복 로그인인 경우 기존 플레이어를 끊는다.
			chatServer.DisconnectPlayer(pPlayerDup);
			chatServer._monitor.disconnByDupPlayer++; // 모니터링

			// accountNo-player map에서 플레이어 객체를 교체한다.
			chatServer.ReplacePlayerByAccountNo(accountNo, pPlayer);
		}
		else
		{
			// 중복 로그인이 아닐 경우 accountNo-player 맵에 등록
			chatServer._mapPlayerAccountNo.insert(std::make_pair(accountNo, pPlayer));
		}


		// 클라이언트의 로그인 status 업데이트 (현재 DB 업데이트는 하지않고있음)
		//bool retDB = chatServer._pDBConn->PostQueryRequest(
		//	L" UPDATE `accountdb`.`status`"
		//	L" SET `status` = 2"
		//	L" WHERE `accountno` = %lld"
		//	, accountNo);
		//if (retDB == false)
		//{
		//	LOGGING(LOGGING_LEVEL_ERROR, L"posting DB status update request failed. session:%lld, accountNo:%lld\n", sessionId, accountNo);
		//}

		// 플레이어 정보 세팅
		pPlayer->SetPlayerInfo(accountNo, id, nickname, sessionKey);
		pPlayer->SetLogin();

		// 클라이언트에게 로그인 성공 패킷 발송
		sendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)1 << accountNo;
		chatServer.SendUnicast(pPlayer, sendPacket);
		LOGGING(LOGGING_LEVEL_DEBUG, L"send login succeed. session:%lld, accountNo:%lld\n", sessionId, accountNo);

		break;
	} while (0);


	chatServer._poolAPCData.Free((StAPCData*)pStAPCData);
	pRecvPacket->SubUseCount();
	sendPacket.SubUseCount();
}



void CChatServer::PacketProc_LoginRequest(__int64 sessionId, CPacket& packet)
{
	PROFILE_BEGIN("CChatServer::ThreadChatServer::en_PACKET_CS_CHAT_REQ_LOGIN");

	INT64 accountNo;
	packet >> accountNo;
	LOGGING(LOGGING_LEVEL_DEBUG, L"receive login. session:%lld, accountNo:%lld\n", sessionId, accountNo);

	// 플레이어객체 존재 체크
	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
	{
		// _mapPlayer에 플레이어객체가 없으므로 세션만 끊는다.
		CNetServer::Disconnect(sessionId);
		_monitor.disconnByLoginFail++; // 모니터링

		// 로그인실패 응답 발송
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)0 << accountNo;
		SendUnicast(sessionId, sendPacket);
		sendPacket.SubUseCount();
		LOGGING(LOGGING_LEVEL_DEBUG, L"send login failed. session:%lld, accountNo:%lld\n", sessionId, accountNo);
		return;
	}
	pPlayer->SetLogin();

	// redis 세션key get 작업을 비동기로 요청한다. (테스트해본결과 동기로 redis get 하는데 평균 234us 걸림, 비동기 redis get은 평균 60us)
	// 비동기 get이 완료되면 CompleteUnfinishedLogin 함수가 APC queue에 삽입된다.
	PROFILE_BLOCK_BEGIN("CChatServer::ThreadChatServer::RedisGet");
	StAPCData* pAPCData = _poolAPCData.Alloc();
	pAPCData->pChatServer = this;
	pAPCData->pPacket = &packet;
	pAPCData->sessionId = sessionId;
	pAPCData->accountNo = accountNo;
	packet.AddUseCount();
	std::string redisKey = std::to_string(accountNo);
	CChatServer* pChatServer = this;
	_pRedisClient->get(redisKey, [pChatServer, pAPCData](cpp_redis::reply& reply) {
		if (reply.is_null() == true)
		{
			pAPCData->isNull = true;
		}
		else
		{
			pAPCData->isNull = false;
			memcpy(pAPCData->sessionKey, reply.as_string().c_str(), 64);
		}
		DWORD ret = QueueUserAPC(pChatServer->CompleteUnfinishedLogin
			, pChatServer->_thChatServer.handle
			, (ULONG_PTR)pAPCData);
		if (ret == 0)
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"failed to queue asynchronous redis get user APC. error:%u, session:%lld, accountNo:%lld\n"
				, GetLastError(), pAPCData->sessionId, pAPCData->accountNo);
		}
		});
	_pRedisClient->commit();
	PROFILE_BLOCK_END;
	LOGGING(LOGGING_LEVEL_DEBUG, L"request asynchronous redis get. session:%lld, accountNo:%lld\n", sessionId, accountNo);
}

void CChatServer::PacketProc_SectorMoveRequest(__int64 sessionId, CPacket& packet)
{
	PROFILE_BEGIN("CChatServer::ThreadChatServer::en_PACKET_CS_CHAT_REQ_SECTOR_MOVE");

	INT64 accountNo;
	WORD  sectorX;
	WORD  sectorY;
	packet >> accountNo >> sectorX >> sectorY;

	// 플레이어 객체 얻기
	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
		return; //Crash();

	if (pPlayer->_bDisconnect == true)
		return;

	// 데이터 검증
	if (pPlayer->_accountNo != accountNo)
	{
		LOGGING(LOGGING_LEVEL_DEBUG, L"receive sector move. account number is invalid!! session:%lld, accountNo (origin:%lld, recved:%lld), sector from (%d,%d) to (%d,%d)\n"
			, pPlayer->_sessionId, pPlayer->_accountNo, accountNo, pPlayer->_sectorX, pPlayer->_sectorY, sectorX, sectorY);
		DisconnectPlayer(pPlayer);
		_monitor.disconnByInvalidAccountNo++;
		return;
	}
	if (sectorX < 0 || sectorX >= SECTOR_MAX_X || sectorY < 0 || sectorY >= SECTOR_MAX_Y)
	{
		LOGGING(LOGGING_LEVEL_DEBUG, L"receive sector move. sector coordinate is invalid!! session:%lld, accountNo:%lld, sector from (%d,%d) to (%d,%d)\n"
			, pPlayer->_sessionId, pPlayer->_accountNo, pPlayer->_sectorX, pPlayer->_sectorY, sectorX, sectorY);
		DisconnectPlayer(pPlayer);
		_monitor.disconnByInvalidSector++;
		return;
	}

	// 섹터 이동
	LOGGING(LOGGING_LEVEL_DEBUG, L"receive sector move. session:%lld, accountNo:%lld, sector from (%d,%d) to (%d,%d)\n"
		, sessionId, accountNo, pPlayer->_sectorX, pPlayer->_sectorY, sectorX, sectorY);
	MoveSector(pPlayer, sectorX, sectorY);
	pPlayer->SetHeartBeatTime();

	// 채팅서버 섹터 이동 결과 발송
	CPacket& sendPacket = AllocPacket();
	sendPacket << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << accountNo << pPlayer->_sectorX << pPlayer->_sectorY;
	SendUnicast(pPlayer, sendPacket);
	sendPacket.SubUseCount();
	LOGGING(LOGGING_LEVEL_DEBUG, L"send sector move. session:%lld, accountNo:%lld, sector to (%d,%d)\n"
		, sessionId, accountNo, pPlayer->_sectorX, pPlayer->_sectorY);
}

void CChatServer::PacketProc_ChatRequest(__int64 sessionId, CPacket& packet)
{
	PROFILE_BEGIN("CChatServer::ThreadChatServer::en_PACKET_CS_CHAT_REQ_MESSAGE");

	INT64 accountNo;
	WORD  messageLen;
	WCHAR* message;
	packet >> accountNo >> messageLen;
	message = (WCHAR*)packet.GetFrontPtr();

	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
		return; //Crash();

	if (pPlayer->_bDisconnect == true)
		return;

	// 데이터 검증
	if (pPlayer->_accountNo != accountNo)
	{
		LOGGING(LOGGING_LEVEL_DEBUG, L"receive chat message. account number is invalid!! session:%lld, accountNo (origin:%lld, recved:%lld)\n"
			, pPlayer->_sessionId, pPlayer->_accountNo, accountNo);
		DisconnectPlayer(pPlayer);
		_monitor.disconnByInvalidAccountNo++; // 모니터링
		return;
	}
	pPlayer->SetHeartBeatTime();

	// 채팅서버 채팅보내기 응답
	LOGGING(LOGGING_LEVEL_DEBUG, L"receive chat message. session:%lld, accountNo:%lld, messageLen:%d\n", sessionId, accountNo, messageLen);
	CPacket& sendPacket = AllocPacket();
	sendPacket << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE << accountNo;
	sendPacket.PutData((char*)pPlayer->_id, sizeof(pPlayer->_id));
	sendPacket.PutData((char*)pPlayer->_nickname, sizeof(pPlayer->_nickname));
	sendPacket << messageLen;
	sendPacket.PutData((char*)message, messageLen);
	int sendCount = SendAroundSector(pPlayer, sendPacket, nullptr);
	sendPacket.SubUseCount();
	LOGGING(LOGGING_LEVEL_DEBUG, L"send chat message. to %d players, session:%lld, accountNo:%lld, messageLen:%d, sector:(%d,%d)\n"
		, sendCount, sessionId, accountNo, messageLen, pPlayer->_sectorX, pPlayer->_sectorY);
}

void CChatServer::PacketProc_HeartBeat(__int64 sessionId, CPacket& packet)
{
	PROFILE_BEGIN("CChatServer::ThreadChatServer::en_PACKET_CS_CHAT_REQ_HEARTBEAT");

	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
		return;
	pPlayer->SetHeartBeatTime();
	LOGGING(LOGGING_LEVEL_DEBUG, L"receive heart beat. session:%lld, accountNo:%lld\n", sessionId, pPlayer->_accountNo);
}

void CChatServer::PacketProc_Default(__int64 sessionId, CPacket& packet, WORD packetType)
{
	PROFILE_BEGIN("CChatServer::ThreadChatServer::DEFAULT");

	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	LOGGING(LOGGING_LEVEL_DEBUG, L"received invalid packet type. session:%lld, accountNo:%lld, packet type:%d\n"
		, sessionId, pPlayer == nullptr ? -1 : pPlayer->_accountNo, packetType);

	if (pPlayer == nullptr)
	{
		CNetServer::Disconnect(sessionId);
	}
	else
	{
		DisconnectPlayer(pPlayer);
	}

	_monitor.disconnByInvalidMessageType++;
}


void CChatServer::MsgProc_JoinPlayer(__int64 sessionId)
{
	PROFILE_BEGIN("CChatServer::ThreadChatServer::MSG_JOIN_PLAYER");

	CPlayer_t pPlayer = AllocPlayer(sessionId);
	_mapPlayer.insert(std::make_pair(sessionId, pPlayer));
	LOGGING(LOGGING_LEVEL_DEBUG, L"join player. sessionId:%lld\n", sessionId);
}

void CChatServer::MsgProc_LeavePlayer(__int64 sessionId)
{
	PROFILE_BEGIN("CChatServer::ThreadChatServer::MSG_LEAVE_PLAYER");

	LOGGING(LOGGING_LEVEL_DEBUG, L"message leave player. sessionId:%lld\n", sessionId);
	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);

	LOGGING(LOGGING_LEVEL_DEBUG, L"leave player. sessionId:%lld, accountNo:%lld\n", sessionId, pPlayer->_bLogin ? pPlayer->_accountNo : -1);

	// 플레이어의 DB status를 로그아웃으로 업데이트 (현재 DB 업데이트는 하지않고있음)
	//bool retDB = _pDBConn->PostQueryRequest(
	//	L" UPDATE `accountdb`.`status`"
	//	L" SET `status` = 0"
	//	L" WHERE `accountno` = %lld"
	//	, pPlayer->_accountNo);
	//if (retDB == false)
	//{
	//	LOGGING(LOGGING_LEVEL_ERROR, L"posting DB status update request failed. session:%lld, accountNo:%lld\n", sessionId, pPlayer->_accountNo);
	//}

	// 플레이어 객체 삭제
	DeletePlayer(sessionId);
}

void CChatServer::MsgProc_CheckLoginTimeout()
{
	PROFILE_BEGIN("CChatServer::ThreadChatServer::MSG_CHECK_LOGIN_TIMEOUT");

	LOGGING(LOGGING_LEVEL_DEBUG, L"message check login timeout\n");
	ULONGLONG currentTime;
	for (auto iter = _mapPlayer.begin(); iter != _mapPlayer.end(); ++iter)
	{
		currentTime = GetTickCount64();
		CPlayer_t& pPlayer = iter->second;
		if (pPlayer->_bLogin == false && pPlayer->_bDisconnect == false && currentTime - pPlayer->_lastHeartBeatTime > _timeoutLogin)
		{
			LOGGING(LOGGING_LEVEL_DEBUG, L"timeout login. sessionId:%lld\n", pPlayer->_sessionId);
			DisconnectPlayer(pPlayer);
			_monitor.disconnByLoginTimeout++;
		}
	}
}

void CChatServer::MsgProc_CheckHeartBeatTimeout()
{
	LOGGING(LOGGING_LEVEL_DEBUG, L"message check heart beat timeout\n");
	ULONGLONG currentTime = GetTickCount64();
	for (auto iter = _mapPlayer.begin(); iter != _mapPlayer.end(); ++iter)
	{
		CPlayer_t& pPlayer = iter->second;
		if (pPlayer->_bDisconnect == false && currentTime - pPlayer->_lastHeartBeatTime > _timeoutHeartBeat)
		{
			LOGGING(LOGGING_LEVEL_DEBUG, L"timeout heart beat. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
			DisconnectPlayer(pPlayer);
			_monitor.disconnByHeartBeatTimeout++;
		}
	}
}

void CChatServer::MsgProc_Shutdown()
{
	LOGGING(LOGGING_LEVEL_DEBUG, L"message shutdown\n");
	// 모든 플레이어의 연결을 끊고 스레드를 종료한다.
	for (auto iter = _mapPlayer.begin(); iter != _mapPlayer.end(); ++iter)
	{
		CPlayer_t& pPlayer = iter->second;
		if (pPlayer->_bDisconnect == false)
		{
			DisconnectPlayer(pPlayer);
		}
	}
}



/* (static) 채팅서버 스레드 */
unsigned WINAPI CChatServer::ThreadChatServer(PVOID pParam)
{
	wprintf(L"begin chat server\n");
	CChatServer& chatServer = *(CChatServer*)pParam;

	chatServer.RunChatServer();

	wprintf(L"end chat server\n");
	return 0;
}

void CChatServer::RunChatServer()
{
	while (true)
	{
		DWORD retWait = WaitForSingleObjectEx(_hEventMsg, INFINITE, TRUE);
		if (retWait != WAIT_OBJECT_0 && retWait != WAIT_IO_COMPLETION)
		{
			LOGGING(LOGGING_LEVEL_FATAL, L"Wait for event failed!!, error:%d\n", GetLastError());
			InterlockedExchange8((char*)&_bEventSetFlag, false);
			return;
		}
		InterlockedExchange8((char*)&_bEventSetFlag, false);

		MsgChatServer* pMsg;
		while (_msgQ.Dequeue(pMsg))
		{
			// 클라이언트에게 받은 메시지일 경우
			if (pMsg->msgFrom == MSG_FROM_CLIENT)
			{
				WORD packetType;

				CPacket& recvPacket = *pMsg->pPacket;
				recvPacket >> packetType;

				// 패킷 타입에 따른 메시지 처리
				switch (packetType)
				{
				// 채팅서버 로그인 요청
				case en_PACKET_CS_CHAT_REQ_LOGIN:
					PacketProc_LoginRequest(pMsg->sessionId, recvPacket);
					break;
				// 채팅서버 섹터 이동 요청
				case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
					PacketProc_SectorMoveRequest(pMsg->sessionId, recvPacket);
					break;
				// 채팅서버 채팅보내기 요청
				case en_PACKET_CS_CHAT_REQ_MESSAGE:
					PacketProc_ChatRequest(pMsg->sessionId, recvPacket);
					break;
				// 하트비트
				case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
					PacketProc_HeartBeat(pMsg->sessionId, recvPacket);
					break;

				default:
					PacketProc_Default(pMsg->sessionId, recvPacket, packetType);
					break;
				} // end of switch(packetType)

				// 패킷의 사용카운트 감소
				recvPacket.SubUseCount();
			}

			// 채팅서버 내부 메시지인 경우
			else if (pMsg->msgFrom == MSG_FROM_SERVER)
			{
				switch (pMsg->eServerMsgType)
				{
				// 플레이어 생성 메시지 
				case EServerMsgType::MSG_JOIN_PLAYER:
					MsgProc_JoinPlayer(pMsg->sessionId);
					break;
				// 플레이어 삭제 메시지 
				case EServerMsgType::MSG_LEAVE_PLAYER:
					MsgProc_LeavePlayer(pMsg->sessionId);
					break;
				// 로그인 타임아웃 확인
				case EServerMsgType::MSG_CHECK_LOGIN_TIMEOUT:
					MsgProc_CheckLoginTimeout();
					break;
				// 하트비트 타임아웃 확인
				case EServerMsgType::MSG_CHECK_HEART_BEAT_TIMEOUT:
					MsgProc_CheckHeartBeatTimeout();
					break;
				// shutdown
				case EServerMsgType::MSG_SHUTDOWN:
					MsgProc_Shutdown();
					break;

				default:
					LOGGING(LOGGING_LEVEL_ERROR, L"invalid server message type. type:%d\n", pMsg->eServerMsgType);
					break;
				} // end of switch(pMsg->eServerMsgType)

			}

			else
			{
				LOGGING(LOGGING_LEVEL_ERROR, L"invalid message type:%d\n", pMsg->msgFrom);
			}

			// free 메시지
			_poolMsg.Free(pMsg);

			_monitor.msgHandleCount++;  // 모니터링

		} // end of while (_msgQ.Dequeue(pMsg))

	} // end of while (true)
}



/* (static) timeout 확인 메시지 발생 스레드 */
unsigned WINAPI CChatServer::ThreadMsgGenerator(PVOID pParam)
{
	wprintf(L"begin message generator\n");
	CChatServer& chatServer = *(CChatServer*)pParam;

	chatServer.RunMsgGenerator();

	wprintf(L"end message generator\n");
	return 0;
}

void CChatServer::RunMsgGenerator()
{
	constexpr int numMessage = 2;       // 메시지 타입 수
	int msgPeriod[numMessage] = { _timeoutLoginCheckPeriod, _timeoutHeartBeatCheckPeriod };   // 메시지 타입별 발생주기
	EServerMsgType msg[numMessage] = { EServerMsgType::MSG_CHECK_LOGIN_TIMEOUT, EServerMsgType::MSG_CHECK_HEART_BEAT_TIMEOUT };  // 메시지 타입
	ULONGLONG lastMsgTime[numMessage] = { 0, 0 };  // 마지막으로 해당 타입 메시지를 보낸 시간

	ULONGLONG currentTime;  // 현재 시간
	int nextMsgIdx;         // 다음에 발송할 메시지 타입
	ULONGLONG nextSendTime; // 다음 메시지 발송 시간
	while (true)
	{
		currentTime = GetTickCount64();

		// 다음에 보낼 메시지를 선택한다.
		nextMsgIdx = -1;
		nextSendTime = UINT64_MAX;
		for (int i = 0; i < numMessage; i++)
		{
			// 다음 메시지 발송시간(마지막으로 메시지를 보낸 시간 + 메시지 발생 주기)이 currentTime보다 작으면 바로 메시지 발송
			ULONGLONG t = lastMsgTime[i] + msgPeriod[i];
			if (t <= currentTime)
			{
				nextMsgIdx = i;
				nextSendTime = currentTime;
				break;
			}
			// 그렇지 않으면 기다려야하는 최소시간을 찾음
			else
			{
				if (t < nextSendTime)
				{
					nextMsgIdx = i;
					nextSendTime = t;
				}
			}
		}

		if (nextMsgIdx < 0 || nextMsgIdx >= numMessage)
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"Message generator is trying to generate an invalid message. message index: %d\n", nextMsgIdx);
			Crash();
			return;
		}

		if (_bShutdown == true)
			break;
		// 다음 메시지 발생 주기까지 기다림
		if (nextSendTime > currentTime)
		{
			Sleep((DWORD)(nextSendTime - currentTime));
		}
		if (_bShutdown == true)
			break;

		// 타입에 따른 메시지 생성
		MsgChatServer* pMsg = _poolMsg.Alloc();
		pMsg->msgFrom = MSG_FROM_SERVER;
		switch (msg[nextMsgIdx])
		{

		case EServerMsgType::MSG_CHECK_LOGIN_TIMEOUT:
		{
			pMsg->sessionId = 0;
			pMsg->pPacket = nullptr;
			pMsg->eServerMsgType = EServerMsgType::MSG_CHECK_LOGIN_TIMEOUT;
			break;
		}

		case EServerMsgType::MSG_CHECK_HEART_BEAT_TIMEOUT:
		{
			pMsg->sessionId = 0;
			pMsg->pPacket = nullptr;
			pMsg->eServerMsgType = EServerMsgType::MSG_CHECK_HEART_BEAT_TIMEOUT;
			break;
		}
		}

		// 메시지큐에 Enqueue
		_msgQ.Enqueue(pMsg);

		// 마지막으로 메시지 보낸시간 업데이트
		lastMsgTime[nextMsgIdx] = nextSendTime;

		// 이벤트 set
		SetEvent(_hEventMsg);
	}
}



/* (static) 모니터링 데이터 수집 스레드 */
unsigned WINAPI CChatServer::ThreadMonitoringCollector(PVOID pParam)
{
	wprintf(L"begin monitoring collector thread\n");
	CChatServer& chatServer = *(CChatServer*)pParam;

	chatServer.RunMonitoringCollector();

	wprintf(L"end monitoring collector thread\n");
	return 0;
}

void CChatServer::RunMonitoringCollector()
{
	PDHCount pdhCount;
	LARGE_INTEGER liFrequency;
	LARGE_INTEGER liStartTime;
	LARGE_INTEGER liEndTime;
	time_t collectTime;
	__int64 spentTime;
	__int64 sleepTime;
	DWORD dwSleepTime;

	__int64 prevMsgHandleCount = 0;
	__int64 currMsgHandleCount = 0;

	QueryPerformanceFrequency(&liFrequency);

	// 최초 update
	_pCPUUsage->UpdateCpuTime();
	_pPDH->Update();

	// 최초 sleep
	Sleep(990);

	// 1초마다 모니터링 데이터를 수집하여 모니터링 서버에게 데이터를 보냄
	QueryPerformanceCounter(&liStartTime);
	while (_bShutdown == false)
	{
		// 데이터 수집
		time(&collectTime);
		_pCPUUsage->UpdateCpuTime();
		_pPDH->Update();
		pdhCount = _pPDH->GetPDHCount();
		currMsgHandleCount = _monitor.GetMsgHandleCount();

		// 모니터링 서버에 send
		lanlib::CPacket& packet = _pLANClientMonitoring->AllocPacket();
		packet << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN << (int)1 << (int)collectTime; // 에이전트 ChatServer 실행 여부 ON / OFF
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU << (int)_pCPUUsage->ProcessTotal() << (int)collectTime; // 에이전트 ChatServer CPU 사용률
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM << (int)((double)pdhCount.processPrivateBytes / 1048576.0) << (int)collectTime; // 에이전트 ChatServer 메모리 사용 MByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SESSION << GetNumSession() << (int)collectTime; // 채팅서버 세션 수 (컨넥션 수)
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_PLAYER << GetNumAccount() << (int)collectTime; // 채팅서버 인증성공 사용자 수 (실제 접속자)
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS << (int)(currMsgHandleCount - prevMsgHandleCount) << (int)collectTime; // 채팅서버 UPDATE 스레드 초당 처리 횟수
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL << GetPacketAllocCount() << (int)collectTime;    // 채팅서버 패킷풀 사용량
		//packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL << GetMsgAllocCount() << (int)collectTime; // 채팅서버 UPDATE MSG 풀 사용량
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL << GetUnhandeledMsgCount() << (int)collectTime; // 채팅서버 UPDATE MSG 풀 사용량
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL << (int)_pCPUUsage->ProcessorTotal() << (int)collectTime; // 서버컴퓨터 CPU 전체 사용률
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY << (int)((double)pdhCount.systemNonpagedPoolBytes / 1048576.0) << (int)collectTime; // 서버컴퓨터 논페이지 메모리 MByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV << (int)(pdhCount.networkRecvBytes / 1024.0) << (int)collectTime; // 서버컴퓨터 네트워크 수신량 KByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND << (int)(pdhCount.networkSendBytes / 1024.0) << (int)collectTime; // 서버컴퓨터 네트워크 송신량 KByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY << (int)((double)pdhCount.systemAvailableBytes / 1048576.0) << (int)collectTime; // 서버컴퓨터 사용가능 메모리 MByte

		prevMsgHandleCount = currMsgHandleCount;

		_pLANClientMonitoring->SendPacket(packet);
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
}



/* crash */
void CChatServer::Crash()
{
	int* p = 0;
	*p = 0;
}





/* 네트워크 라이브러리 callback 함수 구현 */
bool CChatServer::OnRecv(__int64 sessionId, CPacket& packet)
{
	PROFILE_BEGIN("CChatServer::OnRecv");

	// 메시지 생성
	packet.AddUseCount();
	MsgChatServer* pMsg = _poolMsg.Alloc();
	pMsg->msgFrom = MSG_FROM_CLIENT;
	pMsg->sessionId = sessionId;
	pMsg->pPacket = &packet;
	
	// 메시지큐에 Enqueue
	_msgQ.Enqueue(pMsg);

	// 이벤트 set
	if (InterlockedExchange8((char*)&_bEventSetFlag, true) == false)
		SetEvent(_hEventMsg);

	return true;
}

bool CChatServer::OnConnectionRequest(unsigned long IP, unsigned short port) 
{ 
	if (GetNumPlayer() >= _config.numMaxPlayer)
	{
		InterlockedIncrement64(&_monitor.disconnByPlayerLimit); // 모니터링
		return false;
	}

	return true; 
}


bool CChatServer::OnClientJoin(__int64 sessionId)
{
	PROFILE_BEGIN("CChatServer::OnClientJoin");

	// Create player 메시지 생성
	MsgChatServer* pMsg = _poolMsg.Alloc();
	pMsg->msgFrom = MSG_FROM_SERVER;
	pMsg->sessionId = sessionId;
	pMsg->pPacket = nullptr;
	pMsg->eServerMsgType = EServerMsgType::MSG_JOIN_PLAYER;

	// 메시지큐에 Enqueue
	_msgQ.Enqueue(pMsg);

	// 이벤트 set
	if (InterlockedExchange8((char*)&_bEventSetFlag, true) == false)
		SetEvent(_hEventMsg);

	return true;
}


bool CChatServer::OnClientLeave(__int64 sessionId) 
{ 
	PROFILE_BEGIN("CChatServer::OnClientLeave");

	// Delete player 메시지 생성
	MsgChatServer* pMsg = _poolMsg.Alloc();
	pMsg->msgFrom = MSG_FROM_SERVER;
	pMsg->sessionId = sessionId;
	pMsg->pPacket = nullptr;
	pMsg->eServerMsgType = EServerMsgType::MSG_LEAVE_PLAYER;

	// 메시지큐에 Enqueue
	_msgQ.Enqueue(pMsg);

	// 이벤트 set
	if (InterlockedExchange8((char*)&_bEventSetFlag, true) == false)
		SetEvent(_hEventMsg);

	return true; 
}


void CChatServer::OnError(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_ERROR, szError, vaList);
	va_end(vaList);
}


void CChatServer::OnOutputDebug(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_DEBUG, szError, vaList);
	va_end(vaList);
}

void CChatServer::OnOutputSystem(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_INFO, szError, vaList);
	va_end(vaList);
}

