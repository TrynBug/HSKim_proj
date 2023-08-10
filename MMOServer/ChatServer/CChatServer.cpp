
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
	, _poolMsg(0, false, 100)      // �޸�Ǯ �ʱ�ȭ
	, _poolPlayer(0, false, 100)   // �޸�Ǯ �ʱ�ȭ
	, _poolAPCData(0, false, 100)  // �޸�Ǯ �ʱ�ȭ
	, _pLANClientMonitoring(nullptr)
	, _pCPUUsage(nullptr)
	, _pPDH(nullptr)
{
	// DB Connector ����
	_pDBConn = std::make_unique<CDBAsyncWriter>();

	_serverNo = 1;  // ������ȣ(ä�ü���:1)

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
	// config ���� �б�
	// ���� ��ο� config ���� ��θ� ����
	WCHAR szPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szPath);
	_sCurrentPath = szPath;
	_sConfigFilePath = _sCurrentPath + L"\\chat_server_config.json";

	// config ���� �ε�
	_config.LoadConfigFile(_sConfigFilePath);


	// map �ʱ�ũ�� ����
	_mapPlayer.max_load_factor(1.0f);
	_mapPlayer.rehash(_config.numMaxPlayer * 4);
	_mapPlayerAccountNo.max_load_factor(1.0f);
	_mapPlayerAccountNo.rehash(_config.numMaxPlayer * 4);


	// logger �ʱ�ȭ
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
	
	// event ��ü ����
	_hEventMsg = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (_hEventMsg == NULL)
	{
		wprintf(L"CreateEvent failed!!, error:%d\n", GetLastError());
		return false;
	}

	// �޸� ���� üũ
	if ((unsigned long long)this % 64 != 0)
		LOGGING(LOGGING_LEVEL_ERROR, L"[chat server] chat server object is not aligned as 64\n");
	if ((unsigned long long) & _msgQ % 64 != 0)
		LOGGING(LOGGING_LEVEL_ERROR, L"[chat server] message queue object is not aligned as 64\n");


	// ä�ü��� ������ start
	_thChatServer.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadChatServer, (PVOID)this, 0, &_thChatServer.id);
	if (_thChatServer.handle == 0)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[chat server] failed to start chat thread. error:%u\n", GetLastError());
		Crash();
		return false;
	}


	// �޽��� ���� ������ start
	_thMsgGenerator.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadMsgGenerator, (PVOID)this, 0, &_thMsgGenerator.id);
	if (_thMsgGenerator.handle == 0)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[chat server] failed to start message generator thread. error:%u\n", GetLastError());
		Crash();
		return false;
	}

	// DB Connect �� DB Writer ������ start
	if (_pDBConn->ConnectAndRunThread("127.0.0.1", "root", "vmfhzkepal!", "accountdb", 3306) == false)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[chat server] DB Connector start failed!!\n");
		Crash();
		return false;
	}

	// ����͸� ������ �����ϴ� Ŭ���̾�Ʈ start
	_pLANClientMonitoring = std::make_unique<CLANClientMonitoring>(_serverNo, L"127.0.0.1", _config.monitoringServerPortNumber, true, _config.monitoringServerPacketCode, _config.monitoringServerPacketKey, 10000, true);
	if (_pLANClientMonitoring->StartUp() == false)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[Login Server] LAN client start failed. error:%u\n", GetLastError());
		//Crash();
		//return false;
	}
	_pLANClientMonitoring->ConnectToServer();

	// ����͸� ������ ���� ������ start
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

	// ��Ʈ��ũ start
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

	// ThreadMsgGenerator �� �� ���� true�� �Ǹ� �����
	_bShutdown = true;  

	// ä�ü��� �����忡�� shutdown �޽��� ����
	MsgChatServer* pMsg = _poolMsg.Alloc();
	pMsg->msgFrom = MSG_FROM_SERVER;
	pMsg->sessionId = 0;
	pMsg->pPacket = nullptr;
	pMsg->eServerMsgType = EServerMsgType::MSG_SHUTDOWN;
	_msgQ.Enqueue(pMsg);
	SetEvent(_hEventMsg);

	// accept ����
	CNetServer::StopAccept();



	// ��Ʈ��ũ shutdown
	CNetServer::Shutdown();

	// ä�ü��� ��������� ����Ǳ⸦ 60�ʰ� ��ٸ�
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

	// DB ����
	_pDBConn->Close();

	_bTerminated = true;
	return true;
}


/* Get DB ���� */
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


/* ��Ʈ��ũ ���� */
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
		&& iterAccountNo->second->_sessionId == sessionId)  // �ߺ��α����� �߻��ϸ� map�� player ��ü�� ��ü�Ǿ� �����ּҰ� �ٸ� �� ����. �� ��� �����ϸ� �ȵ�
		_mapPlayerAccountNo.erase(iterAccountNo);

	_mapPlayer.erase(iter);
}


/* (static) �α��� ������ ó�� APC �Լ� */
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
			// redis�� ����key�� �����Ƿ� �α��� ����
			// �α��ν��� ���� �߼�
			sendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)0 << accountNo;
			chatServer.SendUnicast(sessionId, sendPacket);
			chatServer._monitor.disconnByNoSessionKey++; // ����͸�
			LOGGING(LOGGING_LEVEL_DEBUG, L"login failed by no session key. session:%lld, accountNo:%lld\n", sessionId, accountNo);

			// ���� ����
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
			// ����key�� �ٸ��� ������ �α��� ����
			// �α��ν��� ���� �߼�
			sendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)0 << accountNo;
			chatServer.SendUnicast(sessionId, sendPacket);
			chatServer._monitor.disconnByInvalidSessionKey++; // ����͸�
			LOGGING(LOGGING_LEVEL_DEBUG, L"login failed by invalid session key. session:%lld, accountNo:%lld\n", sessionId, accountNo);

			// �������
			chatServer.Disconnect(sessionId);
			break;
		}

		// �÷��̾� ���
		CPlayer_t pPlayer = chatServer.GetPlayerBySessionId(sessionId);
		if (pPlayer == nullptr)   // �÷��̾ ã�����ߴٸ� APCť�� ��û�� ���Եȴ��� leave�� �÷��̾���
			break;

		// �÷��̾� �ߺ��α��� üũ
		CPlayer_t pPlayerDup = chatServer.GetPlayerByAccountNo(accountNo);
		if (pPlayerDup != nullptr)
		{
			// �ߺ� �α����� ��� ���� �÷��̾ ���´�.
			chatServer.DisconnectPlayer(pPlayerDup);
			chatServer._monitor.disconnByDupPlayer++; // ����͸�

			// accountNo-player map���� �÷��̾� ��ü�� ��ü�Ѵ�.
			chatServer.ReplacePlayerByAccountNo(accountNo, pPlayer);
		}
		else
		{
			// �ߺ� �α����� �ƴ� ��� accountNo-player �ʿ� ���
			chatServer._mapPlayerAccountNo.insert(std::make_pair(accountNo, pPlayer));
		}


		// Ŭ���̾�Ʈ�� �α��� status ������Ʈ (���� DB ������Ʈ�� �����ʰ�����)
		//bool retDB = chatServer._pDBConn->PostQueryRequest(
		//	L" UPDATE `accountdb`.`status`"
		//	L" SET `status` = 2"
		//	L" WHERE `accountno` = %lld"
		//	, accountNo);
		//if (retDB == false)
		//{
		//	LOGGING(LOGGING_LEVEL_ERROR, L"posting DB status update request failed. session:%lld, accountNo:%lld\n", sessionId, accountNo);
		//}

		// �÷��̾� ���� ����
		pPlayer->SetPlayerInfo(accountNo, id, nickname, sessionKey);
		pPlayer->SetLogin();

		// Ŭ���̾�Ʈ���� �α��� ���� ��Ŷ �߼�
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

	// �÷��̾ü ���� üũ
	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
	{
		// _mapPlayer�� �÷��̾ü�� �����Ƿ� ���Ǹ� ���´�.
		CNetServer::Disconnect(sessionId);
		_monitor.disconnByLoginFail++; // ����͸�

		// �α��ν��� ���� �߼�
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)0 << accountNo;
		SendUnicast(sessionId, sendPacket);
		sendPacket.SubUseCount();
		LOGGING(LOGGING_LEVEL_DEBUG, L"send login failed. session:%lld, accountNo:%lld\n", sessionId, accountNo);
		return;
	}
	pPlayer->SetLogin();

	// redis ����key get �۾��� �񵿱�� ��û�Ѵ�. (�׽�Ʈ�غ���� ����� redis get �ϴµ� ��� 234us �ɸ�, �񵿱� redis get�� ��� 60us)
	// �񵿱� get�� �Ϸ�Ǹ� CompleteUnfinishedLogin �Լ��� APC queue�� ���Եȴ�.
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

	// �÷��̾� ��ü ���
	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
		return; //Crash();

	if (pPlayer->_bDisconnect == true)
		return;

	// ������ ����
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

	// ���� �̵�
	LOGGING(LOGGING_LEVEL_DEBUG, L"receive sector move. session:%lld, accountNo:%lld, sector from (%d,%d) to (%d,%d)\n"
		, sessionId, accountNo, pPlayer->_sectorX, pPlayer->_sectorY, sectorX, sectorY);
	MoveSector(pPlayer, sectorX, sectorY);
	pPlayer->SetHeartBeatTime();

	// ä�ü��� ���� �̵� ��� �߼�
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

	// ������ ����
	if (pPlayer->_accountNo != accountNo)
	{
		LOGGING(LOGGING_LEVEL_DEBUG, L"receive chat message. account number is invalid!! session:%lld, accountNo (origin:%lld, recved:%lld)\n"
			, pPlayer->_sessionId, pPlayer->_accountNo, accountNo);
		DisconnectPlayer(pPlayer);
		_monitor.disconnByInvalidAccountNo++; // ����͸�
		return;
	}
	pPlayer->SetHeartBeatTime();

	// ä�ü��� ä�ú����� ����
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

	// �÷��̾��� DB status�� �α׾ƿ����� ������Ʈ (���� DB ������Ʈ�� �����ʰ�����)
	//bool retDB = _pDBConn->PostQueryRequest(
	//	L" UPDATE `accountdb`.`status`"
	//	L" SET `status` = 0"
	//	L" WHERE `accountno` = %lld"
	//	, pPlayer->_accountNo);
	//if (retDB == false)
	//{
	//	LOGGING(LOGGING_LEVEL_ERROR, L"posting DB status update request failed. session:%lld, accountNo:%lld\n", sessionId, pPlayer->_accountNo);
	//}

	// �÷��̾� ��ü ����
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
	// ��� �÷��̾��� ������ ���� �����带 �����Ѵ�.
	for (auto iter = _mapPlayer.begin(); iter != _mapPlayer.end(); ++iter)
	{
		CPlayer_t& pPlayer = iter->second;
		if (pPlayer->_bDisconnect == false)
		{
			DisconnectPlayer(pPlayer);
		}
	}
}



/* (static) ä�ü��� ������ */
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
			// Ŭ���̾�Ʈ���� ���� �޽����� ���
			if (pMsg->msgFrom == MSG_FROM_CLIENT)
			{
				WORD packetType;

				CPacket& recvPacket = *pMsg->pPacket;
				recvPacket >> packetType;

				// ��Ŷ Ÿ�Կ� ���� �޽��� ó��
				switch (packetType)
				{
				// ä�ü��� �α��� ��û
				case en_PACKET_CS_CHAT_REQ_LOGIN:
					PacketProc_LoginRequest(pMsg->sessionId, recvPacket);
					break;
				// ä�ü��� ���� �̵� ��û
				case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
					PacketProc_SectorMoveRequest(pMsg->sessionId, recvPacket);
					break;
				// ä�ü��� ä�ú����� ��û
				case en_PACKET_CS_CHAT_REQ_MESSAGE:
					PacketProc_ChatRequest(pMsg->sessionId, recvPacket);
					break;
				// ��Ʈ��Ʈ
				case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
					PacketProc_HeartBeat(pMsg->sessionId, recvPacket);
					break;

				default:
					PacketProc_Default(pMsg->sessionId, recvPacket, packetType);
					break;
				} // end of switch(packetType)

				// ��Ŷ�� ���ī��Ʈ ����
				recvPacket.SubUseCount();
			}

			// ä�ü��� ���� �޽����� ���
			else if (pMsg->msgFrom == MSG_FROM_SERVER)
			{
				switch (pMsg->eServerMsgType)
				{
				// �÷��̾� ���� �޽��� 
				case EServerMsgType::MSG_JOIN_PLAYER:
					MsgProc_JoinPlayer(pMsg->sessionId);
					break;
				// �÷��̾� ���� �޽��� 
				case EServerMsgType::MSG_LEAVE_PLAYER:
					MsgProc_LeavePlayer(pMsg->sessionId);
					break;
				// �α��� Ÿ�Ӿƿ� Ȯ��
				case EServerMsgType::MSG_CHECK_LOGIN_TIMEOUT:
					MsgProc_CheckLoginTimeout();
					break;
				// ��Ʈ��Ʈ Ÿ�Ӿƿ� Ȯ��
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

			// free �޽���
			_poolMsg.Free(pMsg);

			_monitor.msgHandleCount++;  // ����͸�

		} // end of while (_msgQ.Dequeue(pMsg))

	} // end of while (true)
}



/* (static) timeout Ȯ�� �޽��� �߻� ������ */
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
	constexpr int numMessage = 2;       // �޽��� Ÿ�� ��
	int msgPeriod[numMessage] = { _timeoutLoginCheckPeriod, _timeoutHeartBeatCheckPeriod };   // �޽��� Ÿ�Ժ� �߻��ֱ�
	EServerMsgType msg[numMessage] = { EServerMsgType::MSG_CHECK_LOGIN_TIMEOUT, EServerMsgType::MSG_CHECK_HEART_BEAT_TIMEOUT };  // �޽��� Ÿ��
	ULONGLONG lastMsgTime[numMessage] = { 0, 0 };  // ���������� �ش� Ÿ�� �޽����� ���� �ð�

	ULONGLONG currentTime;  // ���� �ð�
	int nextMsgIdx;         // ������ �߼��� �޽��� Ÿ��
	ULONGLONG nextSendTime; // ���� �޽��� �߼� �ð�
	while (true)
	{
		currentTime = GetTickCount64();

		// ������ ���� �޽����� �����Ѵ�.
		nextMsgIdx = -1;
		nextSendTime = UINT64_MAX;
		for (int i = 0; i < numMessage; i++)
		{
			// ���� �޽��� �߼۽ð�(���������� �޽����� ���� �ð� + �޽��� �߻� �ֱ�)�� currentTime���� ������ �ٷ� �޽��� �߼�
			ULONGLONG t = lastMsgTime[i] + msgPeriod[i];
			if (t <= currentTime)
			{
				nextMsgIdx = i;
				nextSendTime = currentTime;
				break;
			}
			// �׷��� ������ ��ٷ����ϴ� �ּҽð��� ã��
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
		// ���� �޽��� �߻� �ֱ���� ��ٸ�
		if (nextSendTime > currentTime)
		{
			Sleep((DWORD)(nextSendTime - currentTime));
		}
		if (_bShutdown == true)
			break;

		// Ÿ�Կ� ���� �޽��� ����
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

		// �޽���ť�� Enqueue
		_msgQ.Enqueue(pMsg);

		// ���������� �޽��� �����ð� ������Ʈ
		lastMsgTime[nextMsgIdx] = nextSendTime;

		// �̺�Ʈ set
		SetEvent(_hEventMsg);
	}
}



/* (static) ����͸� ������ ���� ������ */
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

	// ���� update
	_pCPUUsage->UpdateCpuTime();
	_pPDH->Update();

	// ���� sleep
	Sleep(990);

	// 1�ʸ��� ����͸� �����͸� �����Ͽ� ����͸� �������� �����͸� ����
	QueryPerformanceCounter(&liStartTime);
	while (_bShutdown == false)
	{
		// ������ ����
		time(&collectTime);
		_pCPUUsage->UpdateCpuTime();
		_pPDH->Update();
		pdhCount = _pPDH->GetPDHCount();
		currMsgHandleCount = _monitor.GetMsgHandleCount();

		// ����͸� ������ send
		lanlib::CPacket& packet = _pLANClientMonitoring->AllocPacket();
		packet << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN << (int)1 << (int)collectTime; // ������Ʈ ChatServer ���� ���� ON / OFF
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU << (int)_pCPUUsage->ProcessTotal() << (int)collectTime; // ������Ʈ ChatServer CPU ����
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM << (int)((double)pdhCount.processPrivateBytes / 1048576.0) << (int)collectTime; // ������Ʈ ChatServer �޸� ��� MByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SESSION << GetNumSession() << (int)collectTime; // ä�ü��� ���� �� (���ؼ� ��)
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_PLAYER << GetNumAccount() << (int)collectTime; // ä�ü��� �������� ����� �� (���� ������)
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS << (int)(currMsgHandleCount - prevMsgHandleCount) << (int)collectTime; // ä�ü��� UPDATE ������ �ʴ� ó�� Ƚ��
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL << GetPacketAllocCount() << (int)collectTime;    // ä�ü��� ��ŶǮ ��뷮
		//packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL << GetMsgAllocCount() << (int)collectTime; // ä�ü��� UPDATE MSG Ǯ ��뷮
		packet << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL << GetUnhandeledMsgCount() << (int)collectTime; // ä�ü��� UPDATE MSG Ǯ ��뷮
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL << (int)_pCPUUsage->ProcessorTotal() << (int)collectTime; // ������ǻ�� CPU ��ü ����
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY << (int)((double)pdhCount.systemNonpagedPoolBytes / 1048576.0) << (int)collectTime; // ������ǻ�� �������� �޸� MByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV << (int)(pdhCount.networkRecvBytes / 1024.0) << (int)collectTime; // ������ǻ�� ��Ʈ��ũ ���ŷ� KByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND << (int)(pdhCount.networkSendBytes / 1024.0) << (int)collectTime; // ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY << (int)((double)pdhCount.systemAvailableBytes / 1048576.0) << (int)collectTime; // ������ǻ�� ��밡�� �޸� MByte

		prevMsgHandleCount = currMsgHandleCount;

		_pLANClientMonitoring->SendPacket(packet);
		packet.SubUseCount();


		// ������ sleep�� �ð��� ����Ѵ�.
		QueryPerformanceCounter(&liEndTime);
		spentTime = max(0, liEndTime.QuadPart - liStartTime.QuadPart);
		sleepTime = liFrequency.QuadPart - spentTime;  // performance counter ������ sleep �ð�

		// sleep �ð��� ms ������ ��ȯ�Ͽ� sleep �Ѵ�.
		dwSleepTime = 0;
		if (sleepTime > 0)
		{
			dwSleepTime = (DWORD)round((double)(sleepTime * 1000) / (double)liFrequency.QuadPart);
			Sleep(dwSleepTime);
		}

		// ���� ������ ���۽ð��� [���� ���� ����ð� + sleep�� �ð�] �̴�.
		liStartTime.QuadPart = liEndTime.QuadPart + (dwSleepTime * (liFrequency.QuadPart / 1000));

	}
}



/* crash */
void CChatServer::Crash()
{
	int* p = 0;
	*p = 0;
}





/* ��Ʈ��ũ ���̺귯�� callback �Լ� ���� */
bool CChatServer::OnRecv(__int64 sessionId, CPacket& packet)
{
	PROFILE_BEGIN("CChatServer::OnRecv");

	// �޽��� ����
	packet.AddUseCount();
	MsgChatServer* pMsg = _poolMsg.Alloc();
	pMsg->msgFrom = MSG_FROM_CLIENT;
	pMsg->sessionId = sessionId;
	pMsg->pPacket = &packet;
	
	// �޽���ť�� Enqueue
	_msgQ.Enqueue(pMsg);

	// �̺�Ʈ set
	if (InterlockedExchange8((char*)&_bEventSetFlag, true) == false)
		SetEvent(_hEventMsg);

	return true;
}

bool CChatServer::OnConnectionRequest(unsigned long IP, unsigned short port) 
{ 
	if (GetNumPlayer() >= _config.numMaxPlayer)
	{
		InterlockedIncrement64(&_monitor.disconnByPlayerLimit); // ����͸�
		return false;
	}

	return true; 
}


bool CChatServer::OnClientJoin(__int64 sessionId)
{
	PROFILE_BEGIN("CChatServer::OnClientJoin");

	// Create player �޽��� ����
	MsgChatServer* pMsg = _poolMsg.Alloc();
	pMsg->msgFrom = MSG_FROM_SERVER;
	pMsg->sessionId = sessionId;
	pMsg->pPacket = nullptr;
	pMsg->eServerMsgType = EServerMsgType::MSG_JOIN_PLAYER;

	// �޽���ť�� Enqueue
	_msgQ.Enqueue(pMsg);

	// �̺�Ʈ set
	if (InterlockedExchange8((char*)&_bEventSetFlag, true) == false)
		SetEvent(_hEventMsg);

	return true;
}


bool CChatServer::OnClientLeave(__int64 sessionId) 
{ 
	PROFILE_BEGIN("CChatServer::OnClientLeave");

	// Delete player �޽��� ����
	MsgChatServer* pMsg = _poolMsg.Alloc();
	pMsg->msgFrom = MSG_FROM_SERVER;
	pMsg->sessionId = sessionId;
	pMsg->pPacket = nullptr;
	pMsg->eServerMsgType = EServerMsgType::MSG_LEAVE_PLAYER;

	// �޽���ť�� Enqueue
	_msgQ.Enqueue(pMsg);

	// �̺�Ʈ set
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

