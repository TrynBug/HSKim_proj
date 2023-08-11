#include "CMonitoringServer.h"
#pragma comment (lib, "NetLib.lib")
#pragma comment (lib, "ws2_32.lib")

#include <iomanip>
#include "../common/CommonProtocol.h"
#include "CUser.h"

#include "../utils/logger.h"
#include "../utils/CJsonParser.h"

using namespace monitorserver;

CMonitoringServer::CMonitoringServer()
	: _poolUser(0, true, false)
	, _bShutdown(false)
	, _bTerminated(false)
	, _hThreadJobGenerator(NULL)
	, _idThreadJobGenerator(0)
{
	// ��ŶǮ �缳��
	CPacket::RegeneratePacketPool(1000, 0, 100);

	// timeout
	_timeoutLogin = 5000;

	// login key ����
	memcpy(_loginKey, "ajfw@!cv980dSZ[fje#@fdj123948djf", 32);

	// DB Connector ����
	_pDBConn = std::make_unique<CDBConnector>();


	for (int i = 0; i < dfMONITOR_DATA_TYPE_END; i++)
		_vecMonitoringDataSet.push_back(MonitoringDataSet());
}


CMonitoringServer::~CMonitoringServer()
{
	Shutdown();
}

CMonitoringServer::Config::Config()
	: szBindIP{ 0, }
	, portNumber(0)
	, numMaxSession(0)
	, numMaxUser(0)
	, bUseNagle(true)
	, bUsePacketEncryption(true)
	, packetCode(0)
	, packetKey(0)
	, maxPacketSize(0)
{
}

void CMonitoringServer::Config::LoadConfigFile(std::wstring& sFilePath)
{
	// config parse
	CJsonParser jsonParser;
	jsonParser.ParseJsonFile(sFilePath.c_str());

	const wchar_t* serverBindIP = jsonParser[L"ServerBindIP"].Str().c_str();
	wcscpy_s(szBindIP, wcslen(serverBindIP) + 1, serverBindIP);
	portNumber = jsonParser[L"MonitoringServerPort"].Int();
	bUseNagle = (bool)jsonParser[L"EnableNagle"].Int();
	numMaxSession = jsonParser[L"SessionLimit"].Int();
	numMaxUser = jsonParser[L"UserLimit"].Int();
	packetCode = jsonParser[L"PacketHeaderCode"].Int();
	packetKey = jsonParser[L"PacketEncodeKey"].Int();
	maxPacketSize = jsonParser[L"MaximumPacketSize"].Int();
}


bool CMonitoringServer::StartUp()
{
	// config ���� �б�
	// ���� ��ο� config ���� ��θ� ����
	WCHAR szPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szPath);
	_sCurrentPath = szPath;
	_sConfigFilePath = _sCurrentPath + L"\\monitoring_server_config.json";

	// config ���� �ε�
	_config.LoadConfigFile(_sConfigFilePath);


	// logger �ʱ�ȭ
	logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_INFO);
	//logger::ex_Logger.SetConsoleLoggingLevel(LOGGING_LEVEL_DEBUG);

	logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_INFO);
	//logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_DEBUG);
	//logger::ex_Logger.SetFileLoggingLevel(LOGGING_LEVEL_ERROR);

	//CNetServer::SetOutputDebug(true);
	//CNetServer::SetOutputSystem(false);

	LOGGING(LOGGING_LEVEL_INFO, L"\n********** StartUp Monitoring Server ************\n"
		L"Config File: %s\n"
		L"Server Bind IP: %s\n"
		L"Monitoring Server Port: %d\n"
		L"Number of Network Worker Thread: %d\n"
		L"Number of Network Running Worker Thread: %d\n"
		L"Number of Maximum Session: %d\n"
		L"Number of Maximum User: %d\n"
		L"Enable Nagle: %s\n"
		L"Packet Header Code: 0x%x\n"
		L"Packet Encode Key: 0x%x\n"
		L"Maximum Packet Size: %d\n"
		L"*******************************************************************\n\n"
		, _sConfigFilePath.c_str()
		, _config.szBindIP
		, _config.portNumber
		, 1
		, 1
		, _config.numMaxSession
		, _config.numMaxUser
		, _config.bUseNagle ? L"Yes" : L"No"
		, _config.packetCode
		, _config.packetKey
		, _config.maxPacketSize);


	// �����ͺ��̽� ����
	if (_pDBConn->Connect("127.0.0.1", "root", "vmfhzkepal!", "logdb", 3306) == false)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[Monitor] failed to connect database. error:%s\n", _pDBConn->GetMysqlError());
		Crash();
		return false;
	}

	// ��Ʈ��ũ start
	bool retNetStart = CNetServer::StartUp(_config.szBindIP, _config.portNumber, 1, 1, _config.bUseNagle, _config.numMaxSession, _config.packetCode, _config.packetKey, _config.maxPacketSize, true, false);
	if (retNetStart == false)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[Monitor] Network Library Start failed\n");
		Crash();
		return false;
	}

	// job generator ������ ����
	_hThreadJobGenerator = (HANDLE)_beginthreadex(NULL, 0, ThreadJobGenerator, (PVOID)this, 0, &_idThreadJobGenerator);
	if (_hThreadJobGenerator == 0)
	{
		LOGGING(LOGGING_LEVEL_FATAL, L"[Monitor] failed to start job generator thread. error:%u\n", GetLastError());
		Crash();
		return false;
	}

	return true;
}



/* Shutdown */
bool CMonitoringServer::Shutdown()
{
	std::lock_guard<std::mutex> lock_guard(_mtxShutdown);
	if (_bShutdown == true)
		return true;

	// accept ����
	CNetServer::StopAccept();
	Sleep(10);

	_bShutdown = true;

	// ��� ������ ������ ���´�.
	// ���� ���Ⱑ worker �����忡�� ó���ǵ��� ���������� ��û�� ����
	CPacket& shutdownPacket = AllocPacket();
	shutdownPacket << (int)EMonitoringServerJob::SHUTDOWN;
	PostInnerRequest(shutdownPacket);

	// ��Ʈ��ũ ���̺귯������ ��� �÷��̾��� ������ ���������� ��� ��ٸ���.
	Sleep(1000);

	// ��Ʈ��ũ shutdown
	CNetServer::Shutdown();

	// DB ����
	_pDBConn->Disconnect();

	// ��ü ����
	CloseHandle(_hThreadJobGenerator);

	// ���Ῡ�� ����
	_bTerminated = true;
	return true;
}




/* dynamic alloc */
// 64byte aligned ��ü ������ ���� new, delete overriding
void* CMonitoringServer::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}

void CMonitoringServer::operator delete(void* p)
{
	_aligned_free(p);
}




/* User */
// ����ID�� ���� ��ü�� ��´�. �����Ұ�� null�� ��ȯ�Ѵ�.
CUser_t CMonitoringServer::GetUserBySessionId(__int64 sessionId)
{
	const auto& iter = _mapUser.find(sessionId);
	if (iter == _mapUser.end())
		return nullptr;
	else
		return iter->second;
}


// ������ ������ ���´�.
void CMonitoringServer::DisconnectSession(__int64 sessionId)
{
	bool bDisconnect = false;
	CUser_t pUser = GetUserBySessionId(sessionId);
	if (pUser == nullptr)
		return;

	CNetServer::Disconnect(sessionId);
}

void CMonitoringServer::DisconnectSession(CUser_t pUser)
{
	CNetServer::Disconnect(pUser->_sessionId);
}


CUser_t CMonitoringServer::AllocUser(__int64 sessionId)
{
	auto Deleter = [this](CUser* pUser) {
		this->_poolUser.Free(pUser);
	};
	std::shared_ptr<CUser> pUser(_poolUser.Alloc(), Deleter);
	pUser->Init(sessionId);
	return pUser;
}





/* ��Ʈ��ũ ���� */
int CMonitoringServer::SendUnicast(__int64 sessionId, CPacket& packet)
{
	return SendPacket(sessionId, packet);
}



/* (static) timeout Ȯ�� �޽��� �߻� ������ */
unsigned WINAPI CMonitoringServer::ThreadJobGenerator(PVOID pParam)
{
	wprintf(L"[Monitor] begin job generator thread\n");
	CMonitoringServer& server = *(CMonitoringServer*)pParam;
	while (server._bShutdown == false)
	{
		Sleep(2000);
		CPacket& packet = server.AllocPacket();
		packet << (int)EMonitoringServerJob::CHECK_LOGIN_TIMEOUT;
		server.PostInnerRequest(packet);
	}

	wprintf(L"[Monitor] end job generator thread\n");
	return 0;
}


/* crash */
void CMonitoringServer::Crash()
{
	int* p = 0;
	*p = 0;
}















bool CMonitoringServer::OnConnectionRequest(unsigned long IP, unsigned short port)
{
	if (GetNumUser() >= _config.numMaxUser)
	{
		_monitor.disconnByUserLimit++;
		return false;
	}

	return true;
}


bool CMonitoringServer::OnClientJoin(__int64 sessionId)
{
	// ���� ���� �� ���
	CUser_t pUser = AllocUser(sessionId);
	_mapUser.insert(std::make_pair(sessionId, pUser));
	LOGGING(LOGGING_LEVEL_DEBUG, L"OnClientJoin. sessionId:%lld\n", sessionId);

	return true;
}




bool CMonitoringServer::OnRecv(__int64 sessionId, CPacket& packet)
{
	WORD packetType;

	// ��Ŷ Ÿ�Կ� ���� �޽��� ó��
	packet >> packetType;
	switch (packetType)
	{
	// ���� ������ �α��� ��û
	case en_PACKET_SS_MONITOR_LOGIN:
	{
		int serverNo;
		packet >> serverNo;
		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv login request from internal server. session:%lld, serverNo:%lld\n", sessionId, serverNo);
		CUser_t pUser = GetUserBySessionId(sessionId);
		pUser->SetLANUserInfo(serverNo);
		pUser->SetLogin();
		break;
	}

	// ����͸� ������ ������Ʈ ��û
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
	{
		CUser_t pUser = GetUserBySessionId(sessionId);
		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv monitor data update request. session:%lld, server no:%d\n", sessionId, pUser->_serverNo);

		// �ܺ� ����(Ŭ���̾�Ʈ ��)�� ��� ����
		if (pUser->IsWANUser())
		{
			LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv received monitor data update request from wrong external user. session:%lld\n", sessionId);
			DisconnectSession(pUser);
			_monitor.disconnByWANUserUpdateRequest++;
			break;
		}

		// ����͸� ������ ������Ʈ �� DB BULK INSERT ���� ����
		BYTE type;
		int value;
		int collectTime;
		
		// BULK INSERT ���� ����
		SYSTEMTIME systime;;
		GetLocalTime(&systime);
		_ossQuery.str(L"");
		auto ossQueryOldFlags = _ossQuery.flags();
		_ossQuery << L"INSERT `logdb`.`monitorlog_" << systime.wYear << std::setfill(L'0') << std::setw(2) << systime.wMonth;
		_ossQuery.flags(ossQueryOldFlags);
		_ossQuery << L"` (`logtime`, `serverno`, `type`, `avg1m`, `avg5m`, `min1m`, `min5m`, `max1m`, `max5m`) VALUES ";

		// ���� ����͸� ������ ����ŭ loop
		int numDBInsertData = 0;
		while(packet.GetFrontPtr() < packet.GetRearPtr())
		{
			packet >> type >> value >> collectTime;
			if (type >= dfMONITOR_DATA_TYPE_END)  // type ���� �迭 �ε����� ������ error
			{
				LOGGING(LOGGING_LEVEL_ERROR, L"OnRecv monitor data update request, received invalid data type. session:%lld, server no:%d, data type:%d\n", sessionId, pUser->_serverNo, type);
				_monitor.receivedInvalidDatatype++;
				break;
			}

			// ���� type�� ����͸� ������set�� ������
			MonitoringDataSet& dataset = _vecMonitoringDataSet[type];
			
			// ������ ����
			int lastIndex = dataset.dataCollectCount % 300;
			dataset.arrValue5m[lastIndex] = value;
			dataset.dataCollectCount++;

			// 60��° ������ �������� ��� DB insert ���� ����
			if (dataset.dataCollectCount > 0 && dataset.dataCollectCount % 60 == 0)
			{
				numDBInsertData++;
				
				// 1�� ���, min, max ���
				__int64 avg1m = 0;
				int min1m = INT_MAX;
				int max1m = INT_MIN;
				for (int i = 300 + lastIndex + 1 - 60; i < 300 + lastIndex + 1; i++)
				{
					int val = dataset.arrValue5m[i % 300];
					avg1m += val;
					if (val < min1m)
						min1m = val;
					if (val > max1m)
						max1m = val;
				}
				avg1m = (__int64)((double)avg1m / 60.0);

				// 5�� ���, min, max ���
				__int64 avg5m = 0;
				int min5m = INT_MAX;
				int max5m = INT_MIN;
				for (int i = 0; i < 300; i++)
				{
					int val = dataset.arrValue5m[i];
					avg5m += val;
					if (val < min5m)
						min5m = val;
					if (val > max5m)
						max5m = val;
				}
				avg5m = (__int64)((double)avg5m / 300.0);


				// BULK INSERT ���� ����
				_ossQuery << L"(from_unixtime(" << collectTime << L")," << pUser->_serverNo << L"," << type << L",";
				_ossQuery << (int)avg1m << L"," << (int)avg5m << L",";
				_ossQuery << min1m << L"," << min5m << L"," << max1m << L"," << max5m << L"), ";
			}

			// �ܺ� ����(Ŭ���̾�Ʈ ��)���� ����͸� ������ ����
			CPacket& sendPacket = AllocPacket();
			sendPacket << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << (BYTE)pUser->_serverNo << type << value << collectTime;
			for (const auto& iter : _mapWANUser)
			{
				if(iter.second->_bLogin == true)
					SendPacket(iter.second->_sessionId, sendPacket);
			}
			sendPacket.SubUseCount();

		}
		_ossQuery.seekp(-2, std::ios_base::end); // ������ ", " ���ڿ� ����
		_ossQuery << L";";


		// DB�� insert�� �����Ͱ� �ִٸ� insert
		if (numDBInsertData > 0)
		{
			int numAffectedRows = _pDBConn->ExecuteQuery(_ossQuery.str().c_str());
			if (numAffectedRows == -1)
			{
				// ���̺� ���� ������ ��� ���̺� ������ insert
				if (_pDBConn->GetMysqlErrorNo() == 1146)
				{
					std::wostringstream oss;
					oss << L"CREATE TABLE `logdb`.`monitorlog_" << systime.wYear << systime.wMonth << L"` LIKE `logdb`.`monitorlog`;";
					_pDBConn->ExecuteQuery(oss.str().c_str());
					if (_pDBConn->ExecuteQuery(_ossQuery.str().c_str()) == -1)
					{
						LOGGING(LOGGING_LEVEL_ERROR, L"OnRecv monitor data update request, failed to insert to DB. session:%lld, server no:%d, num of data:%d\n", sessionId, pUser->_serverNo, numDBInsertData);
						_monitor.DBErrorCount++;
					}
				}
				else
				{
					LOGGING(LOGGING_LEVEL_ERROR, L"OnRecv monitor data update request, failed to insert to DB. session:%lld, server no:%d, num of data:%d\n", sessionId, pUser->_serverNo, numDBInsertData);
					_monitor.DBErrorCount++;
				}
			}
			else
			{
				LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv monitor data update request, insert to DB. session:%lld, server no:%d, num of data:%d, affected rows:%d\n", sessionId, pUser->_serverNo, numDBInsertData, numAffectedRows);
			}
		}


		break;
	}


	// ����͸� Ŭ���̾�Ʈ(��)�� �α��� ��û
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
	{
		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv login request from client tool. session:%lld\n", sessionId);

		CUser_t pUser = GetUserBySessionId(sessionId);
		char loginKey[32];
		packet.TakeData(loginKey, 32);

		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
		// �α��� Ű �˻� �� �α��� ����
		if (memcmp(_loginKey, loginKey, 32) == 0)
		{
			// �α��� ����
			pUser->SetWANUserInfo(loginKey);
			pUser->SetLogin();
			_mapWANUser.insert(std::make_pair(sessionId, pUser));
			sendPacket << (BYTE)dfMONITOR_TOOL_LOGIN_OK;
			SendPacket(sessionId, sendPacket);
			sendPacket.SubUseCount();
			LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv send login success. session:%lld\n", sessionId);
		}
		else
		{
			// �α��� ����
			CPacket& sendPacket = AllocPacket();
			sendPacket << (BYTE)dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;
			SendPacket(sessionId, sendPacket);
			sendPacket.SubUseCount();
			_monitor.disconnByLoginKeyMismatch++;
			LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv send login fail. session:%lld\n", sessionId);
			DisconnectSession(pUser);
		}
		break;
	}


	default:
	{
		DisconnectSession(sessionId);
		LOGGING(LOGGING_LEVEL_DEBUG, L"OnRecv received invalid packet type. session:%lld, packet type:%d\n", sessionId, packetType);

		_monitor.disconnByInvalidMessageType++;
		break;
	}
	}

	_monitor.msgHandleCount++;
	return true;
}



bool CMonitoringServer::OnInnerRequest(CPacket& packet)
{
	int job;
	packet >> job;

	switch ((EMonitoringServerJob)job)
	{
		// shutdown job�� ����
	case EMonitoringServerJob::SHUTDOWN:
	{
		LOGGING(LOGGING_LEVEL_DEBUG, L"OnInnerRequest receive shutdown job.\n");
		// ��� Ŭ���̾�Ʈ�� ������ ���´�.
		for (auto& iter : _mapUser)
			DisconnectSession(iter.second);
		break;
	}

		// login timeout üũ job�� ����
	case EMonitoringServerJob::CHECK_LOGIN_TIMEOUT:
	{
		LOGGING(LOGGING_LEVEL_DEBUG, L"OnInnerRequest receive check login timeout job.\n");
		ULONGLONG currentTime = GetTickCount64();
		for (auto& iter : _mapUser)
		{
			CUser_t pUser = iter.second;
			if (pUser->_bLogin == false && pUser->_bDisconnect == false && currentTime - pUser->_lastHeartBeatTime > _timeoutLogin)
				DisconnectSession(pUser);
		}
	}
		break;
	}
	
	

	packet.SubUseCount();
	return true;
}


bool CMonitoringServer::OnClientLeave(__int64 sessionId)
{
	LOGGING(LOGGING_LEVEL_DEBUG, L"OnClientLeave. sessionId:%lld\n", sessionId);

	auto iter = _mapUser.find(sessionId);
	_mapUser.erase(iter);

	auto iterEx = _mapWANUser.find(sessionId);
	if (iterEx != _mapWANUser.end())
		_mapWANUser.erase(iterEx);

	return true;
}



void CMonitoringServer::OnError(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_ERROR, szError, vaList);
	va_end(vaList);
}


void CMonitoringServer::OnOutputDebug(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_DEBUG, szError, vaList);
	va_end(vaList);
}

void CMonitoringServer::OnOutputSystem(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_INFO, szError, vaList);
	va_end(vaList);
}



