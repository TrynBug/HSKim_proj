#pragma once

#include "netserver/CNetServer.h"
#include "CLANClientMonitoring.h"

#include "defineLoginServer.h"
#include "CMemoryPoolTLS.h"
#include "CDBConnectorTLSManager.h"

class CClient;
class CDBConnectorTLSManager;
class CCpuUsage;
class CPDH;

namespace cpp_redis
{
	class client;
}

class CLoginServer : public netserver::CNetServer
{
private:
	/* ��� */
	WCHAR _szCurrentPath[MAX_PATH];     // ���� ���丮 ���
	WCHAR _szConfigFilePath[MAX_PATH];  // config ���� ���

	/* config */
	wchar_t _szBindIP[20];
	unsigned short _portNumber;
	wchar_t _szGameServerIP[20];
	wchar_t _szGameServerDummy1IP[20];
	wchar_t _szGameServerDummy2IP[20];
	unsigned short _gameServerPort;
	wchar_t _szChatServerIP[20];
	wchar_t _szChatServerDummy1IP[20];
	wchar_t _szChatServerDummy2IP[20];
	unsigned short _chatServerPort;
	int _numConcurrentThread;
	int _numWorkerThread;
	int _numMaxSession;
	int _numMaxClient;
	bool _bUseNagle;

	BYTE _packetCode;
	BYTE _packetKey;
	int _maxPacketSize;

	unsigned short _monitoringServerPortNumber;
	BYTE _monitoringServerPacketCode;
	BYTE _monitoringServerPacketKey;

	/* server */
	int _serverNo;                         // ������ȣ(�α��μ���:3)
	HANDLE _hThreadPeriodicWorker;         // �ֱ��� �۾� ������
	unsigned int _threadPeriodicWorkerId;
	HANDLE _hThreadMonitoringCollector;    // ����͸� ������ ���� ������
	unsigned int _threadMonitoringCollectorId;

	volatile bool _bShutdown;               // shutdown ����
	volatile bool _bTerminated;             // ����� ����
	LARGE_INTEGER _performanceFrequency;


	/* timeout */
	int _timeoutLogin;
	int _timeoutLoginCheckPeriod;

	/* TLS DBConnector */
	CDBConnectorTLSManager* _tlsDBConnector;

	/* Client */
	std::unordered_map<__int64, CClient*> _mapClient;
	SRWLOCK _srwlMapClient;

	/* account */
	std::unordered_map<__int64, __int64> _mapAccountToSession;
	SRWLOCK _srwlMapAccountToSession;

	/* pool */
	CMemoryPoolTLS<CClient> _poolClient;      // Ŭ���̾�Ʈ Ǯ

	/* redis */
	cpp_redis::client* _pRedisClient;

	/* ����͸� */
	CLANClientMonitoring* _pLANClientMonitoring;       // ����͸� ������ ������ LAN Ŭ���̾�Ʈ
	CCpuUsage* _pCPUUsage;
	CPDH* _pPDH;

	volatile __int64 _loginCount = 0;                  // �α��� ���� Ƚ��
	volatile __int64 _disconnByClientLimit = 0;        // Ŭ���̾�Ʈ�� �������� ����
	volatile __int64 _disconnByNoClient = 0;           // ���ǹ�ȣ�� ���� Ŭ���̾�Ʈ ��ü�� ã������
	volatile __int64 _disconnByDBDataError = 0;        // DB ������ ����
	volatile __int64 _disconnByNoAccount = 0;          // account ���̺��� account ��ȣ�� ã�� ����
	volatile __int64 _disconnByDupAccount = 0;         // ���� account�� �ٸ������忡�� �α����� ��������
	volatile __int64 _disconnByInvalidMessageType = 0; // �޽��� Ÿ���� �߸���
	volatile __int64 _disconnByLoginTimeout = 0;       // �α��� Ÿ�Ӿƿ����� ����
	volatile __int64 _disconnByHeartBeatTimeout = 0;   // ��Ʈ��Ʈ Ÿ�Ӿƿ����� ����

public:
	CLoginServer();
	virtual ~CLoginServer();
	
public:
	/* StartUp */
	bool StartUp();

	/* Shutdown */
	bool Shutdown();

	/* ���� ���� */
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


	/* Get ����͸� */
	__int64 GetLoginCount() { return _loginCount; }
	__int64 GetDisconnByClientLimit() { return _disconnByClientLimit; }
	__int64 GetDisconnByNoClient() { return _disconnByNoClient; }
	__int64 GetDisconnByDBDataError() { return _disconnByDBDataError; }
	__int64 GetDisconnByNoAccount() { return _disconnByNoAccount; }
	__int64 GetDisconnByDupAccount() { return _disconnByDupAccount; }
	__int64 GetDisconnByInvalidMessageType() { return _disconnByInvalidMessageType; }
	__int64 GetDisconnByLoginTimeout() { return _disconnByLoginTimeout; }
	__int64 GetDisconnByHeartBeatTimeout() { return _disconnByHeartBeatTimeout; }


private:
	/* Get */
	CClient* GetClientBySessionId(__int64 sessionId);   // ����ID�� Ŭ���̾�Ʈ ��ü�� ��´�. �����Ұ�� null�� ��ȯ�Ѵ�.
	
	/* Client */
	void InsertClientToMap(__int64 sessionId, CClient* pClient);   // ����ID-Ŭ���̾�Ʈ map�� Ŭ���̾�Ʈ�� ����Ѵ�. 
	void DeleteClientFromMap(__int64 sessionId);                   // ����ID-Ŭ���̾�Ʈ map���� Ŭ���̾�Ʈ�� �����Ѵ�.
	void DisconnectSession(__int64 sessionId);                     // Ŭ���̾�Ʈ�� ������ ���´�.
	void DeleteClient(CClient* pClient);                           // Ŭ���̾�Ʈ�� map���� �����ϰ� free �Ѵ�.

	CClient* AllocClient(__int64 sessionId);
	void FreeClient(CClient* pClient);

	/* account */
	bool InsertAccountToMap(__int64 accountNo, __int64 sessionId);     // account��ȣ�� account��ȣ-����ID map�� ����Ѵ�. ���� accountNo�� �ش��ϴ� �����Ͱ� �̹� ������ ��� �����ϸ� false�� ��ȯ�Ѵ�.
	void DeleteAccountFromMap(__int64 accountNo);   // account��ȣ�� account��ȣ-����ID map���� �����Ѵ�.

	/* ��Ʈ��ũ ���� */
	int SendUnicast(__int64 sessionId, netserver::CPacket* pPacket);
	int SendUnicastAndDisconnect(__int64 sessionId, netserver::CPacket* pPacket);

	/* ������ */
	static unsigned WINAPI ThreadPeriodicWorker(PVOID pParam);       // �ֱ��� �۾� ó�� ������
	static unsigned WINAPI ThreadMonitoringCollector(PVOID pParam);

	/* crash */
	void Crash();


private:
	// ��Ʈ��ũ ���̺귯�� callback �Լ� ����
	virtual bool OnRecv(__int64 sessionId, netserver::CPacket& packet);
	virtual bool OnConnectionRequest(unsigned long IP, unsigned short port);
	virtual bool OnClientJoin(__int64 sessionId);
	virtual bool OnClientLeave(__int64 sessionId);
	virtual void OnError(const wchar_t* szError, ...);
	virtual void OnOutputDebug(const wchar_t* szError, ...);
	virtual void OnOutputSystem(const wchar_t* szError, ...);
};

