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
	_serverNo = 3;       // ������ȣ(�α��μ���:3)

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

	// ���� ��ο� config ���� ��θ� ����
	WCHAR szPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szPath);
	_sCurrentPath = szPath;
	_sConfigFilePath = _sCurrentPath + L"\\login_server_config.json";

	// config ���� �ε�
	_config.LoadConfigFile(_sConfigFilePath);


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
		LOGGING(LOGGING_LEVEL_FATAL, L"[Login Server] periodic worker thread failed to start. error:%u\n", GetLastError());
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


	// ��Ʈ��ũ start
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

	// accept ����
	CNetServer::StopAccept();

	// ��Ʈ��ũ shutdown
	CNetServer::Shutdown();

	// �α��μ��� ��������� ����Ǳ⸦ 10�ʰ� ��ٸ�
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

	// DB ����
	_tlsDBConnector->DisconnectAll();

	_bTerminated = true;
	return true;
}







/* Get */
// ����ID�� Ŭ���̾�Ʈ ��ü�� ��´�. �����Ұ�� null�� ��ȯ�Ѵ�.
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
// ����ID-Ŭ���̾�Ʈ map�� Ŭ���̾�Ʈ�� ����Ѵ�. 
void CLoginServer::InsertClientToMap(__int64 sessionId, CClient_t pClient)
{
	_smtxMapClient.lock();
	_mapClient.insert(std::make_pair(sessionId, pClient));
	_smtxMapClient.unlock();
}

// ����ID-Ŭ���̾�Ʈ map���� Ŭ���̾�Ʈ�� �����Ѵ�.
void CLoginServer::DeleteClientFromMap(__int64 sessionId)
{
	_smtxMapClient.lock();
	const auto& iter = _mapClient.find(sessionId);
	if (iter != _mapClient.end())
		_mapClient.erase(iter);
	_smtxMapClient.unlock();
}

// Ŭ���̾�Ʈ�� ������ ���´�.
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

// Ŭ���̾�Ʈ�� map���� �����Ѵ�.
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
// account��ȣ�� account��ȣ-����ID map�� ����Ѵ�. ���� accountNo�� �ش��ϴ� �����Ͱ� �̹� ������ ��� �����ϸ� false�� ��ȯ�Ѵ�.
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

// account��ȣ�� account��ȣ-����ID map���� �����Ѵ�.
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
	CClient_t pClient = AllocClient(sessionId);
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

	// ��Ŷ Ÿ�Կ� ���� �޽��� ó��
	packet >> packetType;
	switch (packetType)
	{
	// �α��μ��� �α��� ��û
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
	// OnClientLeave �Լ��� ���ǿ� �ɸ� IO�� �ϳ��� ��� ������ ��ȯ�� �ڿ� ȣ��ȴ�.
	// OnRecv �Լ��� ����ǰ� �ִ� �߿��� ������ ���� ��ȯ���� �ʱ� ������ ���� ���ǿ� ���� OnRecv �Լ��� OnClientLeave �Լ��� ���ÿ� ����� ���� ����.

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


	// Ŭ���̾�Ʈ��ü ���� üũ
	CClient_t pClient = GetClientBySessionId(sessionId);
	if (pClient == nullptr)
	{
		// �α��ν��� ���� �߼�
		CPacket& sendPacket = AllocPacket();
		Msg_LoginResponseFail(sendPacket, accountNo);
		SendUnicast(sessionId, sendPacket);
		sendPacket.SubUseCount();

		// ����ID-�÷��̾� map�� �÷��̾ü�� �����Ƿ� ���Ǹ� ���´�.
		CNetServer::Disconnect(sessionId);
		_monitor.disconnByNoClient++;

		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv send login failed. session:%lld, accountNo:%lld\n", sessionId, accountNo);
		return;
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
		CPacket& sendPacket = AllocPacket();
		Msg_LoginResponseFail(sendPacket, accountNo);
		SendUnicast(sessionId, sendPacket);
		sendPacket.SubUseCount();

		DisconnectSession(sessionId);
		_monitor.disconnByDupAccount++;

		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv another session is handling same account number. session:%lld, accountNo:%lld\n", sessionId, accountNo);
		return;
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

	// Ŭ���̾�Ʈ ���� ����
	pClient->LockExclusive();  // lock
	pClient->SetClientInfo(accountNo, userId, userNick, sessionKey);
	pClient->SetLogin();
	pClient->UnlockExclusive();  // unlock

	// status�� 0�̸� 1�� ������Ʈ �Ѵ�. (���� DB ������Ʈ�� �����ʰ�����)
	//if (status == 0)
	//{
	//	DB_UpdateClientStatus(pClient);
	//}

	// redis�� ����key�� ������
	std::string redisKey = std::to_string(accountNo);
	_pRedisClient->set(redisKey, sessionKey);
	_pRedisClient->sync_commit();


	// Ŭ���̾�Ʈ���� �α��μ��� ���� �߼�
	CPacket& sendPacket = AllocPacket();
	Msg_LoginResponseSucceed(sendPacket, pClient);
	SendUnicast(sessionId, sendPacket);
	//SendUnicastAndDisconnect(sessionId, sendPacket);  // ������ �������
	sendPacket.SubUseCount();


	// account ��ȣ�� map���� ����
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
	// numAffectedRow �� update�� �� ������ʹ� �־ ����Ǵ� �����Ͱ� ������ 0�� ���ϵ�
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