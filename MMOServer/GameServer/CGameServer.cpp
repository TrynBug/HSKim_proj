#include "stdafx.h"
#pragma comment (lib, "GameNetLib.lib")
#pragma comment (lib, "LANClient.lib")
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")

#include "CGameServer.h"
#include "CGameContents.h"

#include "CObject.h"
#include "CPlayer.h"

#include "CGameContentsAuth.h"
#include "CGameContentsField.h"

#include "../common/CommonProtocol.h"
#include "CLANClientMonitoring.h"
#include "../utils/logger.h"
#include "../utils/CJsonParser.h"
#include "../utils/CCpuUsage.h"
#include "../utils/CPDH.h"

using namespace gameserver;

CGameServer::CGameServer()
	: _bShutdown(false)
	, _bTerminated(false)
	, _pRedisClient(nullptr)
	, _poolPlayer(10000, true, false)
	, _poolTransferPlayerData(100, 0, false)
	, _pLANClientMonitoring(nullptr)
	, _pCPUUsage(nullptr)
	, _pPDH(nullptr)
{
	_serverNo = 2;   // ������ȣ(���Ӽ���:2)
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
	jsonParser = std::make_unique<CJsonParser>();
}

void CGameServer::Config::LoadConfigFile(std::wstring& sFilePath)
{
	// config parse
	jsonParser->ParseJsonFile(sFilePath.c_str());
	CJsonParser& json = *jsonParser;

	// ��Ʈ��ũ config
	const wchar_t* serverBindIP = json[L"ServerBindIP"].Str().c_str();
	wcscpy_s(szBindIP, wcslen(serverBindIP) + 1, serverBindIP);
	portNumber = json[L"GameServerPort"].Int();
	numConcurrentThread = json[L"NetRunningWorkerThread"].Int();
	numWorkerThread = json[L"NetWorkerThread"].Int();
	bUseNagle = (bool)json[L"EnableNagle"].Int();
	numMaxSession = json[L"SessionLimit"].Int();
	numMaxPlayer = json[L"PlayerLimit"].Int();
	packetCode = json[L"PacketHeaderCode"].Int();
	packetKey = json[L"PacketEncodeKey"].Int();
	maxPacketSize = json[L"MaximumPacketSize"].Int();

	// ����͸� ���� config
	monitoringServerPortNumber = json[L"MonitoringServerPort"].Int();
	monitoringServerPacketCode = json[L"MonitoringServerPacketHeaderCode"].Int();
	monitoringServerPacketKey = json[L"MonitoringServerPacketEncodeKey"].Int();
}

const CJsonParser& CGameServer::Config::GetJsonParser() const
{
	return *jsonParser;
}


bool CGameServer::StartUp()
{
	// ��ŶǮ ����
	netlib_game::CPacket::RegeneratePacketPool(10000, 0, 1000);

	// config ���� �б�
	// ���� ��ο� config ���� ��θ� ����
	WCHAR szPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szPath);
	_sCurrentPath = szPath;
	_sConfigFilePath = _sCurrentPath + L"\\config_game_server.json";
	_config.LoadConfigFile(_sConfigFilePath);

	// logger �ʱ�ȭ
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


	// ������ Ŭ���� ���� �� ����
	CJsonParser& json = *_config.jsonParser;
	int AuthFPS = json[L"ContentsAuthentication"][L"FPS"].Int();
	std::shared_ptr<CGameContentsAuth> pAuth(new CGameContentsAuth(EContentsThread::AUTH, CreateAccessor(), AuthFPS));
	_mapContents.insert(std::make_pair(EContentsThread::AUTH, pAuth));
	pAuth->Init();

	int FieldFPS = json[L"ContentsField"][L"FPS"].Int();
	std::shared_ptr<CGameContentsField> pField(new CGameContentsField(EContentsThread::FIELD, CreateAccessor(), FieldFPS));
	_mapContents.insert(std::make_pair(EContentsThread::FIELD, pField));
	pField->Init();


	// ���� ���� ������ ����
	CNetServer::SetInitialContents(pAuth);



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
	_thMonitoring.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadMonitoringCollector, (PVOID)this, 0, &_thMonitoring.id);
	if (_thMonitoring.handle == 0)
	{
		wprintf(L"failed to start monitoring collector thread. error:%u\n", GetLastError());
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
		LOGGING(LOGGING_LEVEL_ERROR, L"[GameServer] Network Library Start failed\n");
		return false;
	}

	// ������ ������ attach �� start
	if (CNetServer::AttachContents(pAuth) == false)
		LOGGING(LOGGING_LEVEL_ERROR, L"[GameServer] Field thread start failed\n");
	if(CNetServer::AttachContents(pField) == false)
		LOGGING(LOGGING_LEVEL_ERROR, L"[GameServer] Authentication thread start failed\n");


	return true;
}


void CGameServer::Shutdown()
{
	_bShutdown = true;

	// ��Ʈ��ũ shutdown
	CNetServer::Shutdown();

	// DB ���� ����
	//_pDBConn->Close();

	// ������ ��ü ����
	Sleep(1000);
	_mapContents.clear();

	_bTerminated = true;
}


/* ������ */
const std::shared_ptr<CGameContents> CGameServer::GetGameContents(EContentsThread contents)
{
	auto iter = _mapContents.find(contents);
	if (iter == _mapContents.end())
		return nullptr;

	return std::dynamic_pointer_cast<CGameContents>(iter->second);
}



/* ������ */
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
CPlayer_t CGameServer::AllocPlayer()
{
	auto Deleter = [this](CPlayer* pPlayer) {
		for (int i = 0; i < pPlayer->_vecPacketBuffer.size(); i++)
		{
			pPlayer->_vecPacketBuffer[i]->SubUseCount();
		}
		pPlayer->_vecPacketBuffer.clear();
		this->_poolPlayer.Free(pPlayer);
	};
	std::shared_ptr<CPlayer> pPlayer(_poolPlayer.Alloc(), Deleter);
	return pPlayer;
}

_TransferPlayerData* CGameServer::AllocTransferPlayerData(CPlayer_t pPlayer)
{
	_TransferPlayerData* pData = _poolTransferPlayerData.Alloc();
	pData->pPlayer = std::move(pPlayer);
	return pData;
}

void CGameServer::FreeTransferPlayerData(_TransferPlayerData* pData)
{
	pData->pPlayer = nullptr;
	_poolTransferPlayerData.Free(pData);
}


/* (static) ����͸� ������ ���� ������ */
unsigned WINAPI CGameServer::ThreadMonitoringCollector(PVOID pParam)
{
	wprintf(L"begin monitoring collector thread\n");
	CGameServer& server = *(CGameServer*)pParam;

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

	// ���� update
	server._pCPUUsage->UpdateCpuTime();
	server._pPDH->Update();

	// ���� sleep
	Sleep(990);

	// 1�ʸ��� ����͸� �����͸� �����Ͽ� ����͸� �������� �����͸� ����
	QueryPerformanceCounter(&liStartTime);
	while (server._bShutdown == false)
	{
		std::shared_ptr<CGameContents> pContentsAuth = server.GetGameContents(EContentsThread::AUTH);
		std::shared_ptr<CGameContents> pContentsField = server.GetGameContents(EContentsThread::FIELD);

		// ������ ����
		time(&collectTime);
		server._pCPUUsage->UpdateCpuTime();
		server._pPDH->Update();
		pdhCount = server._pPDH->GetPDHCount();
		currAcceptCount = server._monitor.GetAcceptCount();
		currRecvCount = server._monitor.GetRecvCount();
		currSendCount = server._monitor.GetSendCount();
		currDBQueryCount = pContentsField->GetQueryRunCount();

		// ����͸� ������ send
		lanlib::CPacket& packet = server._pLANClientMonitoring->AllocPacket();
		packet << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_SERVER_RUN << (int)1 << (int)collectTime; // GameServer ���� ���� ON / OFF
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_SERVER_CPU << (int)server._pCPUUsage->ProcessTotal() << (int)collectTime; // GameServer CPU ����
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_SERVER_MEM << (int)((double)pdhCount.processPrivateBytes / 1048576.0) << (int)collectTime; // GameServer �޸� ��� MByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_SESSION << (int)server.GetNumSession() << (int)collectTime; // ���Ӽ��� ���� �� (���ؼ� ��)
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER << (int)pContentsAuth->GetNumPlayer() << (int)collectTime; // ���Ӽ��� AUTH MODE �÷��̾� ��
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER << (int)pContentsField->GetNumPlayer() << (int)collectTime; // ���Ӽ��� GAME MODE �÷��̾� ��
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS << (int)(currAcceptCount - prevAcceptCount) << (int)collectTime; // ���Ӽ��� Accept ó�� �ʴ� Ƚ��
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS << (int)(currRecvCount - prevRecvCount) << (int)collectTime; // ���Ӽ��� ��Ŷó�� �ʴ� Ƚ��
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS << (int)(currSendCount - prevSendCount) << (int)collectTime; // ���Ӽ��� ��Ŷ ������ �ʴ� �Ϸ� Ƚ��
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS << (int)(currDBQueryCount - prevDBQueryCount) << (int)collectTime; // ���Ӽ��� DB ���� �޽��� �ʴ� ó�� Ƚ��
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG << (int)pContentsField->GetRemainQueryCount() << (int)collectTime; // ���Ӽ��� DB ���� �޽��� ť ���� (���� ��)
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS << (int)pContentsAuth->GetFPS() << (int)collectTime; // ���Ӽ��� AUTH ������ �ʴ� ������ �� (���� ��)
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS << (int)pContentsField->GetFPS() << (int)collectTime; // ���Ӽ��� GAME ������ �ʴ� ������ �� (���� ��)
		packet << (BYTE)dfMONITOR_DATA_TYPE_GAME_PACKET_POOL << (int)server.GetPacketActualAllocCount() << (int)collectTime; // ���Ӽ��� ��ŶǮ ��뷮

		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL << (int)server._pCPUUsage->ProcessorTotal() << (int)collectTime; // ������ǻ�� CPU ��ü ����
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY << (int)((double)pdhCount.systemNonpagedPoolBytes / 1048576.0) << (int)collectTime; // ������ǻ�� �������� �޸� MByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV << (int)(pdhCount.networkRecvBytes / 1024.0) << (int)collectTime; // ������ǻ�� ��Ʈ��ũ ���ŷ� KByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND << (int)(pdhCount.networkSendBytes / 1024.0) << (int)collectTime; // ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte
		packet << (BYTE)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY << (int)((double)pdhCount.systemAvailableBytes / 1048576.0) << (int)collectTime; // ������ǻ�� ��밡�� �޸� MByte

		prevAcceptCount = currAcceptCount;
		prevRecvCount = currRecvCount;
		prevSendCount = currSendCount;
		prevDBQueryCount = currDBQueryCount;

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









/* ��Ʈ��ũ callback �Լ� ���� */
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


/* crash */
void CGameServer::Crash()
{
	int* p = 0;
	*p = 0;
}





CGameServerAccessor::CGameServerAccessor(CGameServer& gameServer)
	: _gameServer(gameServer)
{
}

CGameServerAccessor::~CGameServerAccessor()
{
}