#include "CommonProtocol.h"
#include "CChatServer.h"
#include "CSector.h"
#include "CPlayer.h"
#include "CDBAsyncWriter.h"

#include "cpp_redis/cpp_redis.h"
#pragma comment (lib, "NetServer.lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")

#include "profiler.h"
#include "logger.h"
#include "CJsonParser.h"
#include "CCpuUsage.h"
#include "CPDH.h"

CChatServer::CChatServer()
	: _szCurrentPath{ 0, }, _szConfigFilePath{ 0, }, _szBindIP{ 0, }, _portNumber(0)
	, _numConcurrentThread(0), _numWorkerThread(0), _numMaxSession(0), _numMaxPlayer(0), _bUseNagle(true)
	, _packetCode(0), _packetKey(0), _maxPacketSize(0)
	, _monitoringServerPortNumber(0), _monitoringServerPacketCode(0), _monitoringServerPacketKey(0)
	, _pRedisClient(nullptr), _pDBConn(nullptr), _sector(nullptr)
	, _hThreadChatServer(0), _threadChatServerId(0), _hThreadMsgGenerator(0), _threadMsgGeneratorId(0)
	, _hThreadMonitoringCollector(0), _threadMonitoringCollectorId(0), _hEventMsg(0), _bEventSetFlag(false)
	, _bShutdown(false), _bTerminated(false)
	, _poolMsg(0, false, 100), _poolPlayer(0, false, 100), _poolAPCData(0, false, 100)  // �޸�Ǯ �ʱ�ȭ
	, _pLANClientMonitoring(nullptr), _pCPUUsage(nullptr), _pPDH(nullptr)
{
	// DB Connector ����
	_pDBConn = new CDBAsyncWriter();

	_serverNo = 1;  // ������ȣ(ä�ü���:1)

	_timeoutLogin = 10000000;
	_timeoutHeartBeat = 10000000;
	_timeoutLoginCheckPeriod = 10000;
	_timeoutHeartBeatCheckPeriod = 30000;

	QueryPerformanceFrequency(&_performanceFrequency);
}

CChatServer::~CChatServer() 
{


}


bool CChatServer::StartUp()
{
	// config ���� �б�
	// ���� ��ο� config ���� ��θ� ����
	GetCurrentDirectory(MAX_PATH, _szCurrentPath);
	swprintf_s(_szConfigFilePath, MAX_PATH, L"%s\\chat_server_config.json", _szCurrentPath);

	// config parse
	CJsonParser jsonParser;
	jsonParser.ParseJsonFile(_szConfigFilePath);

	const wchar_t* serverBindIP = jsonParser[L"ServerBindIP"].Str().c_str();
	wcscpy_s(_szBindIP, wcslen(serverBindIP) + 1, serverBindIP);
	_portNumber = jsonParser[L"ChatServerPort"].Int();

	_numConcurrentThread = jsonParser[L"NetRunningWorkerThread"].Int();
	_numWorkerThread = jsonParser[L"NetWorkerThread"].Int();

	_bUseNagle = (bool)jsonParser[L"EnableNagle"].Int();
	_numMaxSession = jsonParser[L"SessionLimit"].Int();
	_numMaxPlayer = jsonParser[L"PlayerLimit"].Int();
	_packetCode = jsonParser[L"PacketHeaderCode"].Int();
	_packetKey = jsonParser[L"PacketEncodeKey"].Int();
	_maxPacketSize = jsonParser[L"MaximumPacketSize"].Int();

	_monitoringServerPortNumber = jsonParser[L"MonitoringServerPort"].Int();
	_monitoringServerPacketCode = jsonParser[L"MonitoringServerPacketHeaderCode"].Int();
	_monitoringServerPacketKey = jsonParser[L"MonitoringServerPacketEncodeKey"].Int();


	// map �ʱ�ũ�� ����
	_mapPlayer.max_load_factor(1.0f);
	_mapPlayer.rehash(_numMaxPlayer * 4);
	_mapPlayerAccountNo.max_load_factor(1.0f);
	_mapPlayerAccountNo.rehash(_numMaxPlayer * 4);


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
		, _szConfigFilePath
		, _szBindIP
		, _portNumber
		, _monitoringServerPortNumber
		, _numConcurrentThread
		, _numWorkerThread
		, _numMaxSession
		, _numMaxPlayer
		, _bUseNagle ? L"Yes" : L"No"
		, _packetCode
		, _packetKey
		, _maxPacketSize
		, _monitoringServerPacketCode
		, _monitoringServerPacketKey);


	// sector �迭 �޸� �Ҵ�
	_sector = new CSector * [dfSECTOR_MAX_Y];
	_sector[0] = (CSector*)malloc(sizeof(CSector) * dfSECTOR_MAX_Y * dfSECTOR_MAX_X);
	for (int y = 0; y < dfSECTOR_MAX_Y; y++)
		_sector[y] = _sector[0] + (y * dfSECTOR_MAX_X);

	// sector ������ ȣ��
	for (int y = 0; y < dfSECTOR_MAX_Y; y++)
	{
		for (int x = 0; x < dfSECTOR_MAX_X; x++)
		{
			new (&_sector[y][x]) CSector(x, y);
		}
	}

	// sector�� �ֺ� sector ���
	for (int y = 0; y < dfSECTOR_MAX_Y; y++)
	{
		for (int x = 0; x < dfSECTOR_MAX_X; x++)
		{
			for (int aroundY = y - 1; aroundY < y + 2; aroundY++)
			{
				for (int aroundX = x - 1; aroundX < x + 2; aroundX++)
				{
					if (aroundY < 0 || aroundY >= dfSECTOR_MAX_Y || aroundX < 0 || aroundX >= dfSECTOR_MAX_X)
					{
						_sector[y][x].AddAroundSector(aroundX, aroundY, CSector::GetDummySector());
					}
					else
					{
						_sector[y][x].AddAroundSector(aroundX, aroundY, &_sector[aroundY][aroundX]);
					}
				}
			}
		}
	}
	
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
	_hThreadChatServer = (HANDLE)_beginthreadex(NULL, 0, ThreadChatServer, (PVOID)this, 0, &_threadChatServerId);
	if (_hThreadChatServer == 0)
	{
		wprintf(L"failed to start chat thread. error:%u\n", GetLastError());
		return false;
	}


	// �޽��� ���� ������ start
	_hThreadMsgGenerator = (HANDLE)_beginthreadex(NULL, 0, ThreadMsgGenerator, (PVOID)this, 0, &_threadMsgGeneratorId);
	if (_hThreadMsgGenerator == 0)
	{
		wprintf(L"failed to start message generator thread. error:%u\n", GetLastError());
		return false;
	}

	// DB Connect �� DB Writer ������ start
	if (_pDBConn->ConnectAndRunThread("127.0.0.1", "root", "vmfhzkepal!", "accountdb", 3306) == false)
	{
		wprintf(L"DB Connector start failed!!\n");
		return false;
	}

	// ����͸� ������ �����ϴ� Ŭ���̾�Ʈ start
	_pLANClientMonitoring = new CLANClientMonitoring;
	if (_pLANClientMonitoring->StartUp(L"127.0.0.1", _monitoringServerPortNumber, true, _monitoringServerPacketCode, _monitoringServerPacketKey, 10000, true) == false)
	{
		wprintf(L"LAN client start failed\n");
		//return false;
	}

	// ����͸� ������ ����
	CPacket* pPacket = _pLANClientMonitoring->AllocPacket();
	*pPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN << _serverNo;
	_pLANClientMonitoring->SendPacket(pPacket);
	pPacket->SubUseCount();

	// ����͸� ������ ���� ������ start
	_pCPUUsage = new CCpuUsage;
	_pPDH = new CPDH;
	_pPDH->Init();
	_hThreadMonitoringCollector = (HANDLE)_beginthreadex(NULL, 0, ThreadMonitoringCollector, (PVOID)this, 0, &_threadMonitoringCollectorId);
	if (_hThreadMonitoringCollector == 0)
	{
		wprintf(L"failed to start monitoring collector thread. error:%u\n", GetLastError());
		return false;
	}


	// ��Ʈ��ũ start
	bool retNetStart = CNetServer::StartUp(_szBindIP, _portNumber, _numConcurrentThread, _numWorkerThread, _bUseNagle, _numMaxSession, _packetCode, _packetKey, _maxPacketSize, true, true);
	if (retNetStart == false)
	{
		wprintf(L"Network Library Start failed\n");
		return false;
	}

	// redis start
	_pRedisClient = new cpp_redis::client;
	_pRedisClient->connect();

	return true;

}


/* Shutdown */
bool CChatServer::Shutdown()
{
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
	retWait = WaitForSingleObject(_hThreadChatServer, (DWORD)timeout);
	if (retWait != WAIT_OBJECT_0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[chat server] terminate chat thread timeout. error:%u\n", GetLastError());
		TerminateThread(_hThreadChatServer, 0);
	}
	timeout = timeout - GetTickCount64() - tick;
	timeout = timeout < 1 ? 1 : timeout;
	retWait = WaitForSingleObject(_hThreadMsgGenerator, (DWORD)timeout);
	if (retWait != WAIT_OBJECT_0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[chat server] terminate message generator thread timeout. error:%u\n", GetLastError());
		TerminateThread(_hThreadMsgGenerator, 0);
	}

	// DB ����
	_pDBConn->Close();
	return true;
}


/* DB ���� */
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
CPlayer* CChatServer::GetPlayerBySessionId(__int64 sessionId)
{
	auto iter = _mapPlayer.find(sessionId);
	if (iter == _mapPlayer.end())
		return nullptr;
	else
		return iter->second;
}

CPlayer* CChatServer::GetPlayerByAccountNo(__int64 accountNo)
{
	auto iter = _mapPlayerAccountNo.find(accountNo);
	if (iter == _mapPlayerAccountNo.end())
		return nullptr;
	else
		return iter->second;
}

void CChatServer::ReplacePlayerByAccountNo(__int64 accountNo, CPlayer* replacePlayer)
{
	auto iter = _mapPlayerAccountNo.find(accountNo);
	iter->second = replacePlayer;
}


/* ��Ʈ��ũ ���� */
int CChatServer::SendUnicast(__int64 sessionId, netserver::CPacket* pPacket)
{
	//return SendPacket(sessionId, pPacket);
	return SendPacketAsync(sessionId, pPacket);
}

int CChatServer::SendUnicast(CPlayer* pPlayer, netserver::CPacket* pPacket)
{
	//return SendPacket(pPlayer->_sessionId, pPacket);
	return SendPacketAsync(pPlayer->_sessionId, pPacket);
}

int CChatServer::SendBroadcast(netserver::CPacket* pPacket)
{
	int sendCount = 0;
	for (const auto& iter : _mapPlayer)
	{
		//sendCount += SendPacket(iter.second->_sessionId, pPacket);
		sendCount += SendPacketAsync(iter.second->_sessionId, pPacket);
	}

	return sendCount;
}

int CChatServer::SendOneSector(CPlayer* pPlayer, netserver::CPacket* pPacket, CPlayer* except)
{
	int sendCount = 0;
	CSector* pSector = &_sector[pPlayer->_sectorY][pPlayer->_sectorX];
	for (const auto& player : pSector->GetPlayerVector())
	{
		if (player == except)
			continue;
		//sendCount += SendPacket(player->_sessionId, pPacket);
		sendCount += SendPacketAsync(player->_sessionId, pPacket);
	}

	return sendCount;
}

int CChatServer::SendAroundSector(CPlayer* pPlayer, netserver::CPacket* pPacket, CPlayer* except)
{
	int sendCount = 0;
	CSector** pArrSector = _sector[pPlayer->_sectorY][pPlayer->_sectorX].GetAllAroundSector();
	int numAroundSector = _sector[pPlayer->_sectorY][pPlayer->_sectorX].GetNumOfAroundSector();
	for (int i = 0; i < numAroundSector; i++)
	{
		for (const auto& player : pArrSector[i]->GetPlayerVector())
		{
			if (player == except)
				continue;
			//sendCount += SendPacket(player->_sessionId, pPacket);
			sendCount += SendPacketAsync(player->_sessionId, pPacket);
		}
	}

	return sendCount;
}


/* player */
void CChatServer::MoveSector(CPlayer* pPlayer, WORD x, WORD y)
{
	if (pPlayer->_sectorX == x && pPlayer->_sectorY == y)
		return;

	if (pPlayer->_sectorX >= 0 && pPlayer->_sectorX < dfSECTOR_MAX_X
		&& pPlayer->_sectorY >= 0 && pPlayer->_sectorY < dfSECTOR_MAX_Y)
	{
		_sector[pPlayer->_sectorY][pPlayer->_sectorX].RemovePlayer(pPlayer);
	}
	
	pPlayer->_sectorX = min(max(x, 0), dfSECTOR_MAX_X - 1);
	pPlayer->_sectorY = min(max(y, 0), dfSECTOR_MAX_Y - 1);
	_sector[pPlayer->_sectorY][pPlayer->_sectorX].AddPlayer(pPlayer);
}

void CChatServer::DisconnectPlayer(CPlayer* pPlayer)
{
	if (pPlayer->_bDisconnect == false)
	{
		pPlayer->_bDisconnect = true;
		CNetServer::Disconnect(pPlayer->_sessionId);
	}
}


void CChatServer::DeletePlayer(__int64 sessionId)
{
	auto iter = _mapPlayer.find(sessionId);
	if (iter == _mapPlayer.end())
		return;

	CPlayer* pPlayer = iter->second;
	if (pPlayer->_sectorX != SECTOR_NOT_SET && pPlayer->_sectorY != SECTOR_NOT_SET)
	{
		_sector[pPlayer->_sectorY][pPlayer->_sectorX].RemovePlayer(pPlayer);
	}

	auto iterAccountNo = _mapPlayerAccountNo.find(pPlayer->_accountNo);
	if(iterAccountNo != _mapPlayerAccountNo.end() 
		&& iterAccountNo->second->_sessionId == sessionId)  // �ߺ��α����� �߻��ϸ� map�� player ��ü�� ��ü�Ǿ� �����ּҰ� �ٸ� �� ����. �� ��� �����ϸ� �ȵ�
		_mapPlayerAccountNo.erase(iterAccountNo);

	_mapPlayer.erase(iter);
	_poolPlayer.Free(pPlayer);
}

std::unordered_map<__int64, CPlayer*>::iterator CChatServer::DeletePlayer(std::unordered_map<__int64, CPlayer*>::iterator& iterPlayer)
{
	CPlayer* pPlayer = iterPlayer->second;
	if (pPlayer->_sectorX != SECTOR_NOT_SET && pPlayer->_sectorY != SECTOR_NOT_SET)
	{
		_sector[pPlayer->_sectorY][pPlayer->_sectorX].RemovePlayer(pPlayer);
	}

	auto iterAccountNo = _mapPlayerAccountNo.find(pPlayer->_accountNo);
	if (iterAccountNo != _mapPlayerAccountNo.end()
		&& iterAccountNo->second->_sessionId == iterPlayer->second->_sessionId)  // �ߺ��α����� �߻��ϸ� map�� player ��ü�� ��ü�Ǿ� �����ּҰ� �ٸ� �� ����. �� ��� �����ϸ� �ȵ�
		_mapPlayerAccountNo.erase(iterAccountNo);

	auto iterNext = _mapPlayer.erase(iterPlayer);
	_poolPlayer.Free(pPlayer);
	return iterNext;
}


/* (static) �α��� ������ ó�� APC �Լ� */
void CChatServer::CompleteUnfinishedLogin(ULONG_PTR pStAPCData)
{
	CChatServer& chatServer = *((StAPCData*)pStAPCData)->pChatServer;
	netserver::CPacket* pRecvPacket = ((StAPCData*)pStAPCData)->pPacket;
	__int64 sessionId = ((StAPCData*)pStAPCData)->sessionId;
	__int64 accountNo = ((StAPCData*)pStAPCData)->accountNo;
	bool isNull = ((StAPCData*)pStAPCData)->isNull;
	char* redisSessionKey = ((StAPCData*)pStAPCData)->sessionKey;

	LOGGING(LOGGING_LEVEL_DEBUG, L"CompleteUnfinishedLogin start. session:%lld, accountNo:%lld\n", sessionId, accountNo);

	netserver::CPacket* pSendPacket = chatServer.AllocPacket();
	do  // do..while(0)
	{
		if (isNull == true)
		{
			// redis�� ����key�� �����Ƿ� �α��� ����
			// �α��ν��� ���� �߼�
			*pSendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)0 << accountNo;
			chatServer.SendUnicast(sessionId, pSendPacket);
			chatServer._disconnByNoSessionKey++; // ����͸�
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
			*pSendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)0 << accountNo;
			chatServer.SendUnicast(sessionId, pSendPacket);
			chatServer._disconnByInvalidSessionKey++; // ����͸�
			LOGGING(LOGGING_LEVEL_DEBUG, L"login failed by invalid session key. session:%lld, accountNo:%lld\n", sessionId, accountNo);

			// �������
			chatServer.Disconnect(sessionId);
			break;
		}

		// �÷��̾� ���
		CPlayer* pPlayer = chatServer.GetPlayerBySessionId(sessionId);
		if (pPlayer == nullptr)   // �÷��̾ ã�����ߴٸ� APCť�� ��û�� ���Եȴ��� leave�� �÷��̾���
			break;

		// �÷��̾� �ߺ��α��� üũ
		CPlayer* pPlayerDup = chatServer.GetPlayerByAccountNo(accountNo);
		if (pPlayerDup != nullptr)
		{
			// �ߺ� �α����� ��� ���� �÷��̾ ���´�.
			chatServer.DisconnectPlayer(pPlayerDup);
			chatServer._disconnByDupPlayer++; // ����͸�

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
		*pSendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)1 << accountNo;
		chatServer.SendUnicast(pPlayer, pSendPacket);
		LOGGING(LOGGING_LEVEL_DEBUG, L"send login succeed. session:%lld, accountNo:%lld\n", sessionId, accountNo);

		break;
	} while (0);


	chatServer._poolAPCData.Free((StAPCData*)pStAPCData);
	pRecvPacket->SubUseCount();
	pSendPacket->SubUseCount();
}


/* (static) ä�ü��� ������ */
unsigned WINAPI CChatServer::ThreadChatServer(PVOID pParam)
{
	wprintf(L"begin chat server\n");
	CChatServer& chatServer = *(CChatServer*)pParam;
	MsgChatServer* pMsg;
	netserver::CPacket* pRecvPacket;
	WORD packetType;
	netserver::CPacket* pSendPacket;
	CPlayer* pPlayer;
	LARGE_INTEGER liEventWaitStart;
	LARGE_INTEGER liEventWaitEnd;
	while (true)
	{
		QueryPerformanceCounter(&liEventWaitStart);
		DWORD retWait = WaitForSingleObjectEx(chatServer._hEventMsg, INFINITE, TRUE);
		if (retWait != WAIT_OBJECT_0 && retWait != WAIT_IO_COMPLETION)
		{
			LOGGING(LOGGING_LEVEL_FATAL, L"Wait for event failed!!, error:%d\n", GetLastError());
			InterlockedExchange8((char*)&chatServer._bEventSetFlag, false);
			return 0;
		}
		InterlockedExchange8((char*)&chatServer._bEventSetFlag, false);
		QueryPerformanceCounter(&liEventWaitEnd);
		chatServer._eventWaitTime += (liEventWaitEnd.QuadPart - liEventWaitStart.QuadPart);  // ����͸�

		while (chatServer._msgQ.Dequeue(pMsg))
		{
			// Ŭ���̾�Ʈ���� ���� �޽����� ���
			if (pMsg->msgFrom == MSG_FROM_CLIENT)
			{
				pRecvPacket = pMsg->pPacket;
				*pRecvPacket >> packetType;

				pSendPacket = chatServer.AllocPacket();  // send�� ��Ŷ alloc (switch�� ���� �� free��)

				// ��Ŷ Ÿ�Կ� ���� �޽��� ó��
				switch (packetType)
				{
				// ä�ü��� �α��� ��û
				case en_PACKET_CS_CHAT_REQ_LOGIN:
				{
					PROFILE_BEGIN("CChatServer::ThreadChatServer::en_PACKET_CS_CHAT_REQ_LOGIN");

					INT64 accountNo;
					*pRecvPacket >> accountNo;
					LOGGING(LOGGING_LEVEL_DEBUG, L"receive login. session:%lld, accountNo:%lld\n", pMsg->sessionId, accountNo);

					// �÷��̾ü ���� üũ
					pPlayer = chatServer.GetPlayerBySessionId(pMsg->sessionId);
					if (pPlayer == nullptr)
					{
						// _mapPlayer�� �÷��̾ü�� �����Ƿ� ���Ǹ� ���´�.
						chatServer.CNetServer::Disconnect(pMsg->sessionId);
						chatServer._disconnByLoginFail++; // ����͸�

						// �α��ν��� ���� �߼�
						*pSendPacket << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << (BYTE)0 << accountNo;
						chatServer.SendUnicast(pMsg->sessionId, pSendPacket);
						LOGGING(LOGGING_LEVEL_DEBUG, L"send login failed. session:%lld, accountNo:%lld\n", pMsg->sessionId, accountNo);
						break;
					}
					pPlayer->SetLogin();

					// redis ����key get �۾��� �񵿱�� ��û�Ѵ�. (������ �׽�Ʈ�غ���� ����� redis get �ϴµ� ��� 234us �ɸ�, �񵿱� redis get�� ��� 60us)
					// �񵿱� get�� �Ϸ�Ǹ� CompleteUnfinishedLogin �Լ��� APC queue�� ���Եȴ�.
					PROFILE_BLOCK_BEGIN("CChatServer::ThreadChatServer::RedisGet");
					StAPCData* pAPCData = chatServer._poolAPCData.Alloc();
					pAPCData->pChatServer = &chatServer;
					pAPCData->pPacket = pRecvPacket;
					pAPCData->sessionId = pMsg->sessionId;
					pAPCData->accountNo = accountNo;
					pRecvPacket->AddUseCount();
					std::string redisKey = std::to_string(accountNo);
					CChatServer* pChatServer = &chatServer;
					chatServer._pRedisClient->get(redisKey, [pChatServer, pAPCData](cpp_redis::reply& reply) {
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
							, pChatServer->_hThreadChatServer
							, (ULONG_PTR)pAPCData);
						if (ret == 0)
						{
							LOGGING(LOGGING_LEVEL_ERROR, L"failed to queue asynchronous redis get user APC. error:%u, session:%lld, accountNo:%lld\n"
								, GetLastError(), pAPCData->sessionId, pAPCData->accountNo);
						}
						});
					chatServer._pRedisClient->commit();
					PROFILE_BLOCK_END;
					LOGGING(LOGGING_LEVEL_DEBUG, L"request asynchronous redis get. session:%lld, accountNo:%lld\n", pMsg->sessionId, accountNo);
					break;
				}

				// ä�ü��� ���� �̵� ��û
				case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
				{
					PROFILE_BEGIN("CChatServer::ThreadChatServer::en_PACKET_CS_CHAT_REQ_SECTOR_MOVE");

					INT64 accountNo;
					WORD  sectorX;
					WORD  sectorY;
					*pRecvPacket >> accountNo >> sectorX >> sectorY;

					// �÷��̾� ��ü ���
					pPlayer = chatServer.GetPlayerBySessionId(pMsg->sessionId);
					if (pPlayer == nullptr)
						break; //chatServer.Crash();

					if (pPlayer->_bDisconnect == true)
						break;

					// ������ ����
					if (pPlayer->_accountNo != accountNo)
					{
						LOGGING(LOGGING_LEVEL_DEBUG, L"receive sector move. account number is invalid!! session:%lld, accountNo (origin:%lld, recved:%lld), sector from (%d,%d) to (%d,%d)\n"
							, pPlayer->_sessionId, pPlayer->_accountNo, accountNo, pPlayer->_sectorX, pPlayer->_sectorY, sectorX, sectorY);
						chatServer.DisconnectPlayer(pPlayer);
						chatServer._disconnByInvalidAccountNo++; // ����͸�
						break;
					}
					if (sectorX < 0 || sectorX >= dfSECTOR_MAX_X || sectorY < 0 || sectorY >= dfSECTOR_MAX_Y)
					{
						LOGGING(LOGGING_LEVEL_DEBUG, L"receive sector move. sector coordinate is invalid!! session:%lld, accountNo:%lld, sector from (%d,%d) to (%d,%d)\n"
							, pPlayer->_sessionId, pPlayer->_accountNo, pPlayer->_sectorX, pPlayer->_sectorY, sectorX, sectorY);
						chatServer.DisconnectPlayer(pPlayer);
						chatServer._disconnByInvalidSector++; // ����͸�
						break;
					}


					// ���� �̵�
					LOGGING(LOGGING_LEVEL_DEBUG, L"receive sector move. session:%lld, accountNo:%lld, sector from (%d,%d) to (%d,%d)\n"
						, pMsg->sessionId, accountNo, pPlayer->_sectorX, pPlayer->_sectorY, sectorX, sectorY);
					chatServer.MoveSector(pPlayer, sectorX, sectorY);
					pPlayer->SetHeartBeatTime();

					// ä�ü��� ���� �̵� ��� �߼�
					*pSendPacket << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << accountNo << pPlayer->_sectorX << pPlayer->_sectorY;
					chatServer.SendUnicast(pPlayer, pSendPacket);
					LOGGING(LOGGING_LEVEL_DEBUG, L"send sector move. session:%lld, accountNo:%lld, sector to (%d,%d)\n"
						, pMsg->sessionId, accountNo, pPlayer->_sectorX, pPlayer->_sectorY);
					break;
				}


				// ä�ü��� ä�ú����� ��û
				case en_PACKET_CS_CHAT_REQ_MESSAGE:
				{
					PROFILE_BEGIN("CChatServer::ThreadChatServer::en_PACKET_CS_CHAT_REQ_MESSAGE");

					INT64 accountNo;
					WORD  messageLen;
					WCHAR* message;
					*pRecvPacket >> accountNo >> messageLen;
					message = (WCHAR*)pRecvPacket->GetFrontPtr();

					pPlayer = chatServer.GetPlayerBySessionId(pMsg->sessionId);
					if (pPlayer == nullptr)
						break; //chatServer.Crash();

					if (pPlayer->_bDisconnect == true)
						break;

					// ������ ����
					if (pPlayer->_accountNo != accountNo)
					{
						LOGGING(LOGGING_LEVEL_DEBUG, L"receive chat message. account number is invalid!! session:%lld, accountNo (origin:%lld, recved:%lld)\n"
							, pPlayer->_sessionId, pPlayer->_accountNo, accountNo);
						chatServer.DisconnectPlayer(pPlayer);
						chatServer._disconnByInvalidAccountNo++; // ����͸�
						break;
					}
					pPlayer->SetHeartBeatTime();

					// ä�ü��� ä�ú����� ����
					LOGGING(LOGGING_LEVEL_DEBUG, L"receive chat message. session:%lld, accountNo:%lld, messageLen:%d\n", pMsg->sessionId, accountNo, messageLen);
					*pSendPacket << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE << accountNo;
					pSendPacket->PutData((char*)pPlayer->_id, sizeof(pPlayer->_id));
					pSendPacket->PutData((char*)pPlayer->_nickname, sizeof(pPlayer->_nickname));
					*pSendPacket << messageLen;
					pSendPacket->PutData((char*)message, messageLen);
					int sendCount = chatServer.SendAroundSector(pPlayer, pSendPacket, nullptr);
					LOGGING(LOGGING_LEVEL_DEBUG, L"send chat message. to %d players, session:%lld, accountNo:%lld, messageLen:%d, sector:(%d,%d)\n"
						, sendCount, pMsg->sessionId, accountNo, messageLen, pPlayer->_sectorX, pPlayer->_sectorY);
					break;
				}

				// ��Ʈ��Ʈ
				case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
				{
					PROFILE_BEGIN("CChatServer::ThreadChatServer::en_PACKET_CS_CHAT_REQ_HEARTBEAT");

					pPlayer = chatServer.GetPlayerBySessionId(pMsg->sessionId);
					if (pPlayer == nullptr)
						break;
					pPlayer->SetHeartBeatTime();
					LOGGING(LOGGING_LEVEL_DEBUG, L"receive heart beat. session:%lld, accountNo:%lld\n", pMsg->sessionId, pPlayer->_accountNo);
					break;
				}


				default:
				{
					PROFILE_BEGIN("CChatServer::ThreadChatServer::DEFAULT");

					pPlayer = chatServer.GetPlayerBySessionId(pMsg->sessionId);
					LOGGING(LOGGING_LEVEL_DEBUG, L"received invalid packet type. session:%lld, accountNo:%lld, packet type:%d\n"
						, pMsg->sessionId, pPlayer == nullptr ? -1 : pPlayer->_accountNo, packetType);
					
					if (pPlayer == nullptr)
					{
						chatServer.CNetServer::Disconnect(pMsg->sessionId);
					}
					else
					{
						chatServer.DisconnectPlayer(pPlayer);
					}

					chatServer._disconnByInvalidMessageType++; // ����͸�
					break;
				}
				} // end of switch(packetType)

				// ��Ŷ�� ���ī��Ʈ ����
				pRecvPacket->SubUseCount();
				pSendPacket->SubUseCount();
			}

			// ä�ü��� ���� �޽����� ���
			else if (pMsg->msgFrom == MSG_FROM_SERVER)
			{
				switch (pMsg->eServerMsgType)
				{
				// �÷��̾� ���� �޽��� 
				case EServerMsgType::MSG_JOIN_PLAYER:
				{
					PROFILE_BEGIN("CChatServer::ThreadChatServer::MSG_JOIN_PLAYER");

					pPlayer = chatServer._poolPlayer.Alloc();
					pPlayer->Init(pMsg->sessionId);
					chatServer._mapPlayer.insert(std::make_pair(pMsg->sessionId, pPlayer));
					LOGGING(LOGGING_LEVEL_DEBUG, L"join player. sessionId:%lld\n", pMsg->sessionId);

					break;
				}

				// �÷��̾� ���� �޽��� 
				case EServerMsgType::MSG_LEAVE_PLAYER:
				{
					PROFILE_BEGIN("CChatServer::ThreadChatServer::MSG_LEAVE_PLAYER");

					LOGGING(LOGGING_LEVEL_DEBUG, L"message leave player. sessionId:%lld\n", pMsg->sessionId);
					auto iter = chatServer._mapPlayer.find(pMsg->sessionId);
					if (iter == chatServer._mapPlayer.end())
						break;
					pPlayer = iter->second;

					LOGGING(LOGGING_LEVEL_DEBUG, L"leave player. sessionId:%lld, accountNo:%lld\n", pMsg->sessionId, pPlayer->_bLogin ? pPlayer->_accountNo : -1);

					// �÷��̾��� DB status�� �α׾ƿ����� ������Ʈ (���� DB ������Ʈ�� �����ʰ�����)
					//bool retDB = chatServer._pDBConn->PostQueryRequest(
					//	L" UPDATE `accountdb`.`status`"
					//	L" SET `status` = 0"
					//	L" WHERE `accountno` = %lld"
					//	, pPlayer->_accountNo);
					//if (retDB == false)
					//{
					//	LOGGING(LOGGING_LEVEL_ERROR, L"posting DB status update request failed. session:%lld, accountNo:%lld\n", pMsg->sessionId, pPlayer->_accountNo);
					//}

					// �÷��̾� ��ü ����
					chatServer.DeletePlayer(iter);

					break;
				}

				// �α��� Ÿ�Ӿƿ� Ȯ��
				case EServerMsgType::MSG_CHECK_LOGIN_TIMEOUT:
				{
					PROFILE_BEGIN("CChatServer::ThreadChatServer::MSG_CHECK_LOGIN_TIMEOUT");

					LOGGING(LOGGING_LEVEL_DEBUG, L"message check login timeout\n");
					ULONGLONG currentTime;
					for (auto iter = chatServer._mapPlayer.begin(); iter != chatServer._mapPlayer.end(); ++iter)
					{
						currentTime = GetTickCount64();
						pPlayer = iter->second;
						if (pPlayer->_bLogin == false && pPlayer->_bDisconnect == false && currentTime - pPlayer->_lastHeartBeatTime > chatServer._timeoutLogin)
						{
							LOGGING(LOGGING_LEVEL_DEBUG, L"timeout login. sessionId:%lld\n", pPlayer->_sessionId);
							chatServer.DisconnectPlayer(pPlayer);
							chatServer._disconnByLoginTimeout++; // ����͸�
						}
					}
					break;
				}

				// ��Ʈ��Ʈ Ÿ�Ӿƿ� Ȯ��
				case EServerMsgType::MSG_CHECK_HEART_BEAT_TIMEOUT:
				{
					LOGGING(LOGGING_LEVEL_DEBUG, L"message check heart beat timeout\n");
					ULONGLONG currentTime = GetTickCount64();
					for (auto iter = chatServer._mapPlayer.begin(); iter != chatServer._mapPlayer.end(); ++iter)
					{
						pPlayer = iter->second;
						if (pPlayer->_bDisconnect == false && currentTime - pPlayer->_lastHeartBeatTime > chatServer._timeoutHeartBeat)
						{
							LOGGING(LOGGING_LEVEL_DEBUG, L"timeout heart beat. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
							chatServer.DisconnectPlayer(pPlayer);
							chatServer._disconnByHeartBeatTimeout++; // ����͸�
						}
					}
					break;
				}

				// shutdown
				case EServerMsgType::MSG_SHUTDOWN:
				{
					LOGGING(LOGGING_LEVEL_DEBUG, L"message shutdown\n");
					// ��� �÷��̾��� ������ ���� �����带 �����Ѵ�.
					for (auto iter = chatServer._mapPlayer.begin(); iter != chatServer._mapPlayer.end(); ++iter)
					{
						pPlayer = iter->second;
						if (pPlayer->_bDisconnect == false)
						{
							chatServer.DisconnectPlayer(pPlayer);
						}
					}

					wprintf(L"end chat server\n");
					return 0;
					break;
				}

				default:
				{
					LOGGING(LOGGING_LEVEL_ERROR, L"invalid server message type. type:%d\n", pMsg->eServerMsgType);
					break;
				}
				} // end of switch(pMsg->eServerMsgType)

			}

			else
			{
				LOGGING(LOGGING_LEVEL_ERROR, L"invalid message type:%d\n", pMsg->msgFrom);
			}

			// free �޽���
			chatServer._poolMsg.Free(pMsg);

			chatServer._msgHandleCount++;  // ����͸�

		} // end of while (chatServer._msgQ.Dequeue(pMsg))

	} // end of while (true)


	wprintf(L"end chat server\n");
	return 0;
}




/* (static) timeout Ȯ�� �޽��� �߻� ������ */
unsigned WINAPI CChatServer::ThreadMsgGenerator(PVOID pParam)
{
	wprintf(L"begin message generator\n");
	CChatServer& chatServer = *(CChatServer*)pParam;

	const int numMessage = 2;       // �޽��� Ÿ�� ��
	int msgPeriod[numMessage] = { chatServer._timeoutLoginCheckPeriod, chatServer._timeoutHeartBeatCheckPeriod };   // �޽��� Ÿ�Ժ� �߻��ֱ�
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
			chatServer.Crash();
			return 0;
		}

		if (chatServer._bShutdown == true)
			break;
		// ���� �޽��� �߻� �ֱ���� ��ٸ�
		if (nextSendTime > currentTime)
		{
			Sleep((DWORD)(nextSendTime - currentTime));
		}
		if (chatServer._bShutdown == true)
			break;

		// Ÿ�Կ� ���� �޽��� ����
		MsgChatServer* pMsg = chatServer._poolMsg.Alloc();
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
		chatServer._msgQ.Enqueue(pMsg);

		// ���������� �޽��� �����ð� ������Ʈ
		lastMsgTime[nextMsgIdx] = nextSendTime;

		// �̺�Ʈ set
		SetEvent(chatServer._hEventMsg);
	}

	wprintf(L"end message generator\n");
	return 0;
}



/* (static) ����͸� ������ ���� ������ */
unsigned WINAPI CChatServer::ThreadMonitoringCollector(PVOID pParam)
{
	wprintf(L"begin monitoring collector thread\n");
	CChatServer& chatServer = *(CChatServer*)pParam;

	CPacket* pPacket;
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
	chatServer._pCPUUsage->UpdateCpuTime();
	chatServer._pPDH->Update();

	// ���� sleep
	Sleep(990);

	// 1�ʸ��� ����͸� �����͸� �����Ͽ� ����͸� �������� �����͸� ����
	QueryPerformanceCounter(&liStartTime);
	while (chatServer._bShutdown == false)
	{
		// ������ ����
		time(&collectTime);
		chatServer._pCPUUsage->UpdateCpuTime();
		chatServer._pPDH->Update();
		pdhCount = chatServer._pPDH->GetPDHCount();
		currMsgHandleCount = chatServer.GetMsgHandleCount();

		// ����͸� ������ send
		pPacket = chatServer._pLANClientMonitoring->AllocPacket();
		*pPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN << (int)1 << (int)collectTime; // ������Ʈ ChatServer ���� ���� ON / OFF
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU << (int)chatServer._pCPUUsage->ProcessTotal() << (int)collectTime; // ������Ʈ ChatServer CPU ����
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM << (int)((double)pdhCount.processPrivateBytes / 1048576.0) << (int)collectTime; // ������Ʈ ChatServer �޸� ��� MByte
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_CHAT_SESSION << chatServer.GetNumSession() << (int)collectTime; // ä�ü��� ���� �� (���ؼ� ��)
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_CHAT_PLAYER << chatServer.GetNumAccount() << (int)collectTime; // ä�ü��� �������� ����� �� (���� ������)
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS << (int)(currMsgHandleCount - prevMsgHandleCount) << (int)collectTime; // ä�ü��� UPDATE ������ �ʴ� ó�� Ƚ��
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL << chatServer.GetPacketAllocCount() << (int)collectTime;    // ä�ü��� ��ŶǮ ��뷮
		//*pPacket << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL << chatServer.GetMsgAllocCount() << (int)collectTime; // ä�ü��� UPDATE MSG Ǯ ��뷮
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL << chatServer.GetUnhandeledMsgCount() << (int)collectTime; // ä�ü��� UPDATE MSG Ǯ ��뷮
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL << (int)chatServer._pCPUUsage->ProcessorTotal() << (int)collectTime; // ������ǻ�� CPU ��ü ����
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY << (int)((double)pdhCount.systemNonpagedPoolBytes / 1048576.0) << (int)collectTime; // ������ǻ�� �������� �޸� MByte
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV << (int)(pdhCount.networkRecvBytes / 1024.0) << (int)collectTime; // ������ǻ�� ��Ʈ��ũ ���ŷ� KByte
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND << (int)(pdhCount.networkSendBytes / 1024.0) << (int)collectTime; // ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte
		*pPacket << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY << (int)((double)pdhCount.systemAvailableBytes / 1048576.0) << (int)collectTime; // ������ǻ�� ��밡�� �޸� MByte

		prevMsgHandleCount = currMsgHandleCount;

		chatServer._pLANClientMonitoring->SendPacket(pPacket);
		pPacket->SubUseCount();


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

	wprintf(L"end monitoring collector thread\n");
	return 0;
}




/* crash */
void CChatServer::Crash()
{
	int* p = 0;
	*p = 0;
}





/* ��Ʈ��ũ ���̺귯�� callback �Լ� ���� */
bool CChatServer::OnRecv(__int64 sessionId, netserver::CPacket& packet)
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
	if (GetNumPlayer() >= _numMaxPlayer)
	{
		InterlockedIncrement64(&_disconnByPlayerLimit); // ����͸�
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

