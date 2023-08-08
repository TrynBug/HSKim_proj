#include "CLoginServer.h"

#include "../utils/cpp_redis/cpp_redis.h"
#pragma comment (lib, "NetLib.lib")
#pragma comment (lib, "LANClient.lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")

#include "CommonProtocol.h"
#include "CClient.h"

#include "../utils/profiler.h"
#include "../utils/logger.h"
#include "../utils/CJsonParser.h"
#include "../utils/CCpuUsage.h"
#include "../utils/CPDH.h"


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
	_serverNo = 3;       // ������ȣ(�α��μ���:3)

	QueryPerformanceFrequency(&_performanceFrequency);

	// timeout
	_timeoutLogin = 15000;
	_timeoutLoginCheckPeriod = 3000;


	// SRWLock �ʱ�ȭ
	InitializeSRWLock(&_srwlMapClient);
	InitializeSRWLock(&_srwlMapAccountToSession);
}


CLoginServer::~CLoginServer()
{
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


bool CLoginServer::StartUp()
{
	// config ���� �б�
	// ���� ��ο� config ���� ��θ� ����
	WCHAR szPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szPath);
	_sCurrentPath = szPath;
	_sConfigFilePath = _sCurrentPath + L"\\login_server_config.json";

	// config parse
	CJsonParser jsonParser;
	jsonParser.ParseJsonFile(_sConfigFilePath.c_str());

	const wchar_t* serverBindIP = jsonParser[L"ServerBindIP"].Str().c_str();
	wcscpy_s(_config.szBindIP, wcslen(serverBindIP) + 1, serverBindIP);
	_config.portNumber = jsonParser[L"LoginServerPort"].Int();
	const wchar_t* gameServerIP = jsonParser[L"GameServerIP"].Str().c_str();
	wcscpy_s(_config.szGameServerIP, wcslen(gameServerIP) + 1, gameServerIP);
	_config.gameServerPort = jsonParser[L"GameServerPort"].Int();
	const wchar_t* chatServerIP = jsonParser[L"ChatServerIP"].Str().c_str();
	wcscpy_s(_config.szChatServerIP, wcslen(chatServerIP) + 1, chatServerIP);
	_config.chatServerPort = jsonParser[L"ChatServerPort"].Int();


	const wchar_t* gameServerDummy1IP = jsonParser[L"GameServerDummy1IP"].Str().c_str();
	wcscpy_s(_config.szGameServerDummy1IP, wcslen(gameServerDummy1IP) + 1, gameServerDummy1IP);
	const wchar_t* gameServerDummy2IP = jsonParser[L"GameServerDummy2IP"].Str().c_str();
	wcscpy_s(_config.szGameServerDummy2IP, wcslen(gameServerDummy2IP) + 1, gameServerDummy2IP);
	const wchar_t* chatServerDummy1IP = jsonParser[L"ChatServerDummy1IP"].Str().c_str();
	wcscpy_s(_config.szChatServerDummy1IP, wcslen(chatServerDummy1IP) + 1, chatServerDummy1IP);
	const wchar_t* chatServerDummy2IP = jsonParser[L"ChatServerDummy2IP"].Str().c_str();
	wcscpy_s(_config.szChatServerDummy2IP, wcslen(chatServerDummy2IP) + 1, chatServerDummy2IP);


	_config.numConcurrentThread = jsonParser[L"NetRunningWorkerThread"].Int();
	_config.numWorkerThread = jsonParser[L"NetWorkerThread"].Int();

	_config.bUseNagle = (bool)jsonParser[L"EnableNagle"].Int();
	_config.numMaxSession = jsonParser[L"SessionLimit"].Int();
	_config.numMaxClient = jsonParser[L"ClientLimit"].Int();
	_config.packetCode = jsonParser[L"PacketHeaderCode"].Int();
	_config.packetKey = jsonParser[L"PacketEncodeKey"].Int();
	_config.maxPacketSize = jsonParser[L"MaximumPacketSize"].Int();

	_config.monitoringServerPortNumber = jsonParser[L"MonitoringServerPort"].Int();
	_config.monitoringServerPacketCode = jsonParser[L"MonitoringServerPacketHeaderCode"].Int();
	_config.monitoringServerPacketKey = jsonParser[L"MonitoringServerPacketEncodeKey"].Int();

	// logger �ʱ�ȭ
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

	// TLS DB Connector ����
	_tlsDBConnector = std::make_unique<CDBConnectorTLSManager>("127.0.0.1", "root", "vmfhzkepal!", "accountdb", 3306);

	// �ֱ��� job ó�� ������ start
	_thPeriodicWorker.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadPeriodicWorker, (PVOID)this, 0, &_thPeriodicWorker.id);
	if (_thPeriodicWorker.handle == 0 || _thPeriodicWorker.handle == INVALID_HANDLE_VALUE)
	{
		wprintf(L"[Login Server] periodic worker thread failed to start. error:%u\n", GetLastError());
		return false;
	}


	// ����͸� ������ �����ϴ� Ŭ���̾�Ʈ start
	_pLANClientMonitoring = std::make_unique<CLANClientMonitoring>();
	if (_pLANClientMonitoring->StartUp(L"127.0.0.1", _config.monitoringServerPortNumber, true, _config.monitoringServerPacketCode, _config.monitoringServerPacketKey, 10000, true) == false)
	{
		wprintf(L"[Login Server] LAN client start failed\n");
		//return false;
	}

	// ����͸� ������ ����
	lanlib::CPacket& packet = _pLANClientMonitoring->AllocPacket();
	packet << (WORD)en_PACKET_SS_MONITOR_LOGIN << _serverNo;
	_pLANClientMonitoring->SendPacket(packet);
	packet.SubUseCount();

	// ����͸� ������ ���� ������ start
	_pCPUUsage = std::make_unique<CCpuUsage>();
	_pPDH = std::make_unique<CPDH>();
	_pPDH->Init();
	_thMonitoringCollector.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadMonitoringCollector, (PVOID)this, 0, &_thMonitoringCollector.id);
	if (_thMonitoringCollector.handle == 0)
	{
		wprintf(L"[Login Server] failed to start monitoring collector thread. error:%u\n", GetLastError());
		return false;
	}


	// ��Ʈ��ũ start
	bool retNetStart = CNetServer::StartUp(_config.szBindIP, _config.portNumber, _config.numConcurrentThread, _config.numWorkerThread, _config.bUseNagle, _config.numMaxSession, _config.packetCode, _config.packetKey, _config.maxPacketSize, true, true);
	if (retNetStart == false)
	{
		wprintf(L"[Login Server] Network Library Start failed\n");
		return false;
	}

	// redis start
	_pRedisClient = std::make_unique<cpp_redis::client>();
	_pRedisClient->connect();

	return true;
}


/* Shutdown */
bool CLoginServer::Shutdown()
{
	return true;
}







/* Get */
// ����ID�� Ŭ���̾�Ʈ ��ü�� ��´�. �����Ұ�� null�� ��ȯ�Ѵ�.
CClient* CLoginServer::GetClientBySessionId(__int64 sessionId)
{
	CClient* pClient;
	AcquireSRWLockShared(&_srwlMapClient);
	auto iter = _mapClient.find(sessionId);
	if (iter == _mapClient.end())
		pClient = nullptr;
	else
		pClient = iter->second;
	ReleaseSRWLockShared(&_srwlMapClient);
	
	return pClient;
}


/* Client */
// ����ID-Ŭ���̾�Ʈ map�� Ŭ���̾�Ʈ�� ����Ѵ�. 
void CLoginServer::InsertClientToMap(__int64 sessionId, CClient* pClient)
{
	AcquireSRWLockExclusive(&_srwlMapClient);
	_mapClient.insert(std::make_pair(sessionId, pClient));
	ReleaseSRWLockExclusive(&_srwlMapClient);
}

// ����ID-Ŭ���̾�Ʈ map���� Ŭ���̾�Ʈ�� �����Ѵ�.
void CLoginServer::DeleteClientFromMap(__int64 sessionId)
{
	AcquireSRWLockExclusive(&_srwlMapClient);
	auto iter = _mapClient.find(sessionId);
	if (iter != _mapClient.end())
		_mapClient.erase(iter);
	ReleaseSRWLockExclusive(&_srwlMapClient);
}

// Ŭ���̾�Ʈ�� ������ ���´�.
void CLoginServer::DisconnectSession(__int64 sessionId)
{
	PROFILE_BEGIN("CLoginServer::DisconnectSession");

	bool bDisconnect = false;
	CClient* pClient = GetClientBySessionId(sessionId);
	if (pClient == nullptr)
	{
		bDisconnect = true;
	}
	else
	{
		// Periodic Worker �����尡 �α��� Ÿ�Ӿƿ� ��� ���ǿ� DisconnectSession�� ȣ���ϰ� Client ��ü�� ���� ����,
		// �ش� ������ �ٸ� �����忡�� ������ ����� Client ��ü�� ���Ҵ�� �� �ִ�. 
		// �׷��� Client ��ü�� ����ID�� ���Ͽ� ��ġ�ϴ� ��쿡�� ������ ������ ���´�.
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

// Ŭ���̾�Ʈ�� map���� �����ϰ� free �Ѵ�.
void CLoginServer::DeleteClient(CClient* pClient)
{
	DeleteClientFromMap(pClient->_sessionId);
	_poolClient.Free(pClient);
}


CClient* CLoginServer::AllocClient(__int64 sessionId)
{
	CClient* pClient = _poolClient.Alloc();
	pClient->Init(sessionId);
	return pClient;
}


void CLoginServer::FreeClient(CClient* pClient)
{
	_poolClient.Free(pClient);
}



/* account */
// account��ȣ�� account��ȣ-����ID map�� ����Ѵ�. ���� accountNo�� �ش��ϴ� �����Ͱ� �̹� ������ ��� �����ϸ� false�� ��ȯ�Ѵ�.
bool CLoginServer::InsertAccountToMap(__int64 accountNo, __int64 sessionId)
{
	bool bInserted = false;
	AcquireSRWLockExclusive(&_srwlMapAccountToSession);
	auto iter = _mapAccountToSession.find(accountNo);
	if (iter == _mapAccountToSession.end())
	{
		_mapAccountToSession.insert(std::make_pair(accountNo, sessionId));
		bInserted = true;
	}
	ReleaseSRWLockExclusive(&_srwlMapAccountToSession);
	return bInserted;
}

// account��ȣ�� account��ȣ-����ID map���� �����Ѵ�.
void CLoginServer::DeleteAccountFromMap(__int64 accountNo)
{
	AcquireSRWLockExclusive(&_srwlMapAccountToSession);
	auto iter = _mapAccountToSession.find(accountNo);
	if (iter != _mapAccountToSession.end())
	{
		_mapAccountToSession.erase(accountNo);
	}
	ReleaseSRWLockExclusive(&_srwlMapAccountToSession);
}




/* ��Ʈ��ũ ���� */
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







// ��Ʈ��ũ ���̺귯�� callback �Լ� ����
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
	// OnClientJoin �Լ��� �Ϸ�Ǳ� �� ������ ���� ���ǿ� recv�� �ɸ��� �ʴ´�.
	// �׷��� ���� ���ǿ� ���� OnRecv�� OnClientLeave �Լ��� ȣ����� �ʴ´�.

	PROFILE_BEGIN("CLoginServer::OnClientJoin");

	// Ŭ���̾�Ʈ ���� �� ���
	CClient* pClient = AllocClient(sessionId);
	InsertClientToMap(sessionId, pClient);
	LOGGING(LOGGING_LEVEL_DEBUG, L"OnClientJoin. sessionId:%lld\n", sessionId);

	return true;
}




bool CLoginServer::OnRecv(__int64 sessionId, CPacket& packet)
{
	// OnRecv �Լ��� ����ǰ� �ִ� �߿��� ������ ���� ��ȯ���� �ʴ´�.
	// �׷��� ���� ���ǿ� ���� OnClientLeave �Լ��� ȣ��� ���� ����, ���� Ŭ���̾�Ʈ ��ü�� ��ȯ�� �ϵ� ����.
	// �׷��ϱ� ���� ����, Ŭ���̾�Ʈ�� ���� OnRecv �Լ��� OnClientLeave �Լ��� ���ÿ� ȣ��� ���� ���ٴ� ���̴�.
	// �׷��� map���� Ŭ���̾�Ʈ ��ü�� ���� ���� Ŭ���̾�Ʈ ��ü�� lock �Ͽ��� ���� �ʴ´�.
	// OnRecv �Լ� ��ü�� ���� ����, Ŭ���̾�Ʈ�� ���� ���ÿ� ȣ��� �� �ִ�.

	WORD packetType;
	CClient* pClient;

	// ��Ŷ Ÿ�Կ� ���� �޽��� ó��
	packet >> packetType;
	CPacket& sendPacket = AllocPacket();  // Ŭ���̾�Ʈ send�� ��Ŷ �Ҵ�. switch���� ������ free�ȴ�.
	switch (packetType)
	{
	// �α��μ��� �α��� ��û
	case en_PACKET_CS_LOGIN_REQ_LOGIN:
	{
		PROFILE_BEGIN("CLoginServer::OnRecv::en_PACKET_CS_LOGIN_REQ_LOGIN");

		INT64 accountNo;
		char  sessionKey[65];
		packet >> accountNo;
		packet.TakeData(sessionKey, sizeof(sessionKey) - 1);
		sessionKey[64] = '\0';
		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv login request. session:%lld, accountNo:%lld\n", sessionId, accountNo);

		// Ŭ���̾�Ʈ��ü ���� üũ
		pClient = GetClientBySessionId(sessionId);
		if (pClient == nullptr)
		{
			// �α��ν��� ���� �߼�
			sendPacket << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN << accountNo << (BYTE)en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_FAIL;
			SendUnicast(sessionId, sendPacket);

			// ����ID-�÷��̾� map�� �÷��̾ü�� �����Ƿ� ���Ǹ� ���´�.
			CNetServer::Disconnect(sessionId);
			_monitor.disconnByNoClient++;

			LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv send login failed. session:%lld, accountNo:%lld\n", sessionId, accountNo);
			break;
		}
		pClient->SetLogin();

		// ���� account�� ������ �α����� �õ��ϰ��ִ��� üũ
		/* # ���δٸ� Ŭ�� ���� account�� �α��ο�û�� ���ÿ� ������ ���
			status�� ��ȸ�غ��� 0�̶� �������� �α��� ���μ����� 2�� �����忡�� ���ÿ� �����
			1�� �����忡�� redis�� 1�� Ŭ���� ����key�� ����ϰ�, Ŭ�� ä�ü����� �α��ο�û�� �ϰ�, ä�ü����� ����key�� ���� �α����� �����Ŵ
			�׷����� 2�� �����忡�� redis�� 2�� Ŭ���� ����key�� ����ϰ�, Ŭ�� ä�ü����� �α��ο�û�� �ϰ�, ä�ü����� key�� ���� �α����� �����Ŵ
			�̷��ԵǸ� �ߺ� �α����� �߻��ϹǷ� ���� �α���ó���� �����ϰ� �ִ� account��ȣ�� set�� ����س���, ���� set�� account��ȣ�� �̹� �����Ѵٸ� �α����� ������.
			�α���ó���� �Ϸ��ߴٸ� account��ȣ�� set���� ������ */
		bool bInserted = InsertAccountToMap(accountNo, sessionId);
		if (bInserted == false)
		{
			// ���� account��ȣ�� ������ ���� �ٸ� �����忡�� �α����� �����ϰ� �ִٸ� �α��� ������Ŷ�� ����
			sendPacket << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN << accountNo << (BYTE)en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_FAIL;

			SendUnicast(sessionId, sendPacket);
			DisconnectSession(sessionId);
			_monitor.disconnByDupAccount++;

			LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv another session is handling same account number. session:%lld, accountNo:%lld\n", sessionId, accountNo);
			break;
		}

		// DB���� ���� ����, status ��ȸ
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
			// account ��ȣ�� ���� �����Ͱ� �˻����� ���� ��� �α��� ����
			LOGGING(LOGGING_LEVEL_DEBUG, L"Can't find account data from account table. accountNo:%lld\n", accountNo);
			sendPacket << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN << accountNo << (BYTE)en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_ACCOUNT_MISS;
			SendUnicast(sessionId, sendPacket);
			DisconnectSession(sessionId);
			DeleteAccountFromMap(accountNo);
			pDBConn->FreeResult(myRes);
			_monitor.disconnByNoAccount++;
			break;
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

		// Ŭ���̾�Ʈ ���� ����
		pClient->LockExclusive();  // lock
		pClient->SetClientInfo(accountNo, userId, userNick, sessionKey);
		pClient->SetLogin();
		pClient->UnlockExclusive();  // unlock

		// status�� 0�̸� 1�� ������Ʈ �Ѵ�. (���� DB ������Ʈ�� �����ʰ�����)
		//int numAffectedRow = 0;
		//if (status == 0)
		//{
		//	numAffectedRow = pDBConn->ExecuteQuery(
		//		L" UPDATE `accountdb`.`status`"
		//		L" SET `status` = 1"
		//		L" WHERE `accountno` = %lld"
		//		, accountNo);
		//	// numAffectedRow �� update�� �� ������ʹ� �־ ����Ǵ� �����Ͱ� ������ 0�� ���ϵ�
		//}
		
		// redis�� ����key�� ������
		std::string redisKey = std::to_string(accountNo);
		_pRedisClient->set(redisKey, sessionKey);
		_pRedisClient->sync_commit();



		// Ŭ���̾�Ʈ���� �α��μ��� ���� �߼�
		sendPacket << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN << accountNo << (BYTE)en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK;
		sendPacket.PutData((char*)userId, sizeof(userId));
		sendPacket.PutData((char*)userNick, sizeof(userNick));
		if (accountNo < 50000)
		{
			sendPacket.PutData((char*)_config.szGameServerDummy1IP, 32);
			sendPacket << _config.gameServerPort;
			sendPacket.PutData((char*)_config.szChatServerDummy1IP, 32);
			sendPacket << _config.chatServerPort;
		}
		else if (accountNo < 100000)
		{
			sendPacket.PutData((char*)_config.szGameServerDummy2IP, 32);
			sendPacket << _config.gameServerPort;
			sendPacket.PutData((char*)_config.szChatServerDummy2IP, 32);
			sendPacket << _config.chatServerPort;
		}
		else
		{
			sendPacket.PutData((char*)_config.szGameServerIP, 32);
			sendPacket << _config.gameServerPort;
			sendPacket.PutData((char*)_config.szChatServerIP, 32);
			sendPacket << _config.chatServerPort;
		}
		SendUnicast(sessionId, sendPacket);
		//SendUnicastAndDisconnect(sessionId, sendPacket);  // ������ �������

		// account ��ȣ�� map���� ����
		DeleteAccountFromMap(accountNo);

		_monitor.loginCount++;

		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv send login succeed. session:%lld, accountNo:%lld\n", sessionId, accountNo);
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

	sendPacket.SubUseCount();

	return true;
}




bool CLoginServer::OnClientLeave(__int64 sessionId)
{
	// OnClientLeave �Լ��� ���ǿ� �ɸ� IO�� �ϳ��� ��� ������ ��ȯ�� �ڿ� ȣ��ȴ�.
	// OnRecv �Լ��� ����ǰ� �ִ� �߿��� ������ ���� ��ȯ���� �ʱ� ������ ���� ���ǿ� ���� OnRecv �Լ��� OnClientLeave �Լ��� ���ÿ� ����� ���� ����.

	PROFILE_BEGIN("CLoginServer::OnClientLeave");
	LOGGING(LOGGING_LEVEL_DEBUG, L"OnClientLeave. sessionId:%lld\n", sessionId);

	CClient* pClient = GetClientBySessionId(sessionId);
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







/* �ֱ��� �۾� ó�� ������ */
unsigned WINAPI CLoginServer::ThreadPeriodicWorker(PVOID pParam)
{
	wprintf(L"[Login Server] begin periodic worker thread. id:%u\n", GetCurrentThreadId());
	CLoginServer& server = *(CLoginServer*)pParam;

	const int numJob = 1;       // job Ÿ�� ��
	int jogPeriod[numJob] = { server._timeoutLoginCheckPeriod };  // job �߻� �ֱ�
	EServerJobType job[numJob] = { EServerJobType::JOB_CHECK_LOGIN_TIMEOUT }; // job Ÿ��
	ULONGLONG lastJobTime[numJob] = { 0 };  // ���������� �ش� Ÿ�� job�� ó���� �ð�

	ULONGLONG currentTime;  // ���� �ð�
	int nextJobIdx;         // ������ ó���� job Ÿ��
	ULONGLONG nextJobTime;  // ���� job ó�� �ð�
	while (true)
	{
		currentTime = GetTickCount64();

		// ������ ó���� job�� �����Ѵ�.
		nextJobIdx = -1;
		nextJobTime = UINT64_MAX;
		for (int i = 0; i < numJob; i++)
		{
			// ���� job ó���ð�(���������� job�� ó���� �ð� + job ó�� �ֱ�)�� currentTime���� ������ �ٷ� job ó��
			ULONGLONG t = lastJobTime[i] + jogPeriod[i];
			if (t <= currentTime)
			{
				nextJobIdx = i;
				nextJobTime = currentTime;
				break;
			}
			// �׷��� ������ ��ٷ����ϴ� �ּҽð��� ã��
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
		// ���� job ó���ð����� ��ٸ�
		if (nextJobTime > currentTime)
		{
			Sleep((DWORD)(nextJobTime - currentTime));
		}
		if (server._bShutdown == true)
			break;

		// ���������� job�� ó���� �ð� ������Ʈ
		lastJobTime[nextJobIdx] = nextJobTime;

		// Ÿ�Կ� ���� job ó��
		switch (job[nextJobIdx])
		{
			// �α��� Ÿ�Ӿƿ� ó��
		case EServerJobType::JOB_CHECK_LOGIN_TIMEOUT:
		{
			PROFILE_BEGIN("CLoginServer::ThreadPeriodicJobHandler::JOB_CHECK_LOGIN_TIMEOUT");

			LOGGING(LOGGING_LEVEL_DEBUG, L"job check login timeout\n");

			// �α��� Ÿ�Ӿƿ� ��� ����ID�� ���Ϳ� �����Ѵ�.
			std::vector<__int64> vecTimeoutSessionId;
			vecTimeoutSessionId.reserve(100);
			CClient* pClient;
			ULONGLONG currentTime;
			AcquireSRWLockExclusive(&server._srwlMapClient);
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
			ReleaseSRWLockExclusive(&server._srwlMapClient);

			server._monitor.disconnByLoginTimeout += vecTimeoutSessionId.size();
			// �α��� Ÿ�Ӿƿ� ��� ������ ������ ���´�.
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





/* (static) ����͸� ������ ���� ������ */
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

	// ���� update
	server._pCPUUsage->UpdateCpuTime();
	server._pPDH->Update();

	// ���� sleep
	Sleep(990);

	// 1�ʸ��� ����͸� �����͸� �����Ͽ� ����͸� �������� �����͸� ����
	QueryPerformanceCounter(&liStartTime);
	while (server._bShutdown == false)
	{
		// ������ ����
		time(&collectTime);
		server._pCPUUsage->UpdateCpuTime();
		server._pPDH->Update();
		pdhCount = server._pPDH->GetPDHCount();
		currLoginCount = server._monitor.GetLoginCount();

		// ����͸� ������ send
		lanlib::CPacket& packet = server._pLANClientMonitoring->AllocPacket();
		packet << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN << (int)1 << (int)collectTime; // �α��μ��� ���࿩�� ON / OFF
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU << (int)server._pCPUUsage->ProcessTotal() << (int)collectTime; // �α��μ��� CPU ����
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM << (int)((double)pdhCount.processPrivateBytes / 1048576.0) << (int)collectTime; // �α��μ��� �޸� ��� MByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_SESSION << server.GetNumSession() << (int)collectTime; // �α��μ��� ���� �� (���ؼ� ��)
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS << (int)(currLoginCount - prevLoginCount) << (int)collectTime; // �α��μ��� ���� ó�� �ʴ� Ƚ��
		packet << (BYTE)dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL << server.GetPacketAllocCount() << (int)collectTime; // �α��μ��� ��ŶǮ ��뷮

		prevLoginCount = currLoginCount;

		server._pLANClientMonitoring->SendPacket(packet);
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

	wprintf(L"end monitoring collector thread\n");
	return 0;
}