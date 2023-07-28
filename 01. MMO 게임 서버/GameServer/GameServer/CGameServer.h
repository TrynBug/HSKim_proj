#pragma once

#include "game_netserver/CNetServer.h"
#include "defineGameServer.h"
#include "CPlayer.h"

#include "cpp_redis.h"

#include "CMemoryPool.h"

class CJsonParser;
class CLANClientMonitoring;
class CCpuUsage;
class CPDH;

class CGameServer : public game_netserver::CNetServer
{
	friend class CGameContents;

private:
	/* ��� */
	WCHAR _szCurrentPath[MAX_PATH];     // ���� ���丮 ���
	WCHAR _szConfigFilePath[MAX_PATH];  // config ���� ���

	/* json */
	CJsonParser* _configJsonParser;    // config.json ���� parser

	/* config */
	wchar_t _szBindIP[20];
	unsigned short _portNumber;
	int _numConcurrentThread;
	int _numWorkerThread;
	int _numMaxSession;
	int _numMaxPlayer;
	bool _bUseNagle;

	BYTE _packetCode;
	BYTE _packetKey;
	int _maxPacketSize;

	unsigned short _monitoringServerPortNumber;
	BYTE _monitoringServerPacketCode;
	BYTE _monitoringServerPacketKey;

	/* game server */
	int _serverNo;                          // ������ȣ(���Ӽ���:2)
	volatile bool _bShutdown;               // shutdown ����
	volatile bool _bTerminated;             // ����� ����

	/* redis */
	cpp_redis::client* _pRedisClient;

	/* ������ */
	game_netserver::CContents* _arrContentsPtr[(UINT)EContentsThread::END];   // ������ Ŭ���� �����͵��� ���� �迭

	/* �÷��̾� */
	//CMemoryPoolTLS<CPlayer> _poolPlayer;               // �÷��̾� Ǯ
	CMemoryPool<CPlayer> _poolPlayer;

	/* ����͸� */
	CLANClientMonitoring* _pLANClientMonitoring;       // ����͸� ������ ������ LAN Ŭ���̾�Ʈ
	HANDLE _hThreadMonitoringCollector;                // ����͸� ������ ���� ������
	unsigned int _threadMonitoringCollectorId;
	CCpuUsage* _pCPUUsage;
	CPDH* _pPDH;

	volatile __int64 _disconnByPlayerLimit = 0;        // �÷��̾�� �������� ����

public:
	CGameServer();
	virtual ~CGameServer();

public:
	/* game server */
	bool StartUp();
	void Shutdown();
	bool IsTerminated() { return _bTerminated; }   // ����� ����

	/* Get */
	int GetPlayerPoolSize() { return _poolPlayer.GetPoolSize(); }
	//int GetPlayerAllocCount() { return _poolPlayer.GetAllocCount(); }
	//int GetPlayerActualAllocCount() { return _poolPlayer.GetActualAllocCount(); }
	int GetPlayerAllocCount() { return _poolPlayer.GetUseCount(); }
	int GetPlayerActualAllocCount() { return _poolPlayer.GetUseCount(); }
	int GetPlayerFreeCount() { return _poolPlayer.GetFreeCount(); }

	/* ������ */
	const CGameContents* GetGameContentsPtr(EContentsThread contents);

	/* ������ */
	static unsigned WINAPI ThreadMonitoringCollector(PVOID pParam);  // ����͸� ������

private:
	/* ������ */
	game_netserver::CContents* GetContentsPtr(EContentsThread contents);

	/* ��Ʈ��ũ callback �Լ� ���� */
	virtual bool OnConnectionRequest(unsigned long IP, unsigned short port); 
	virtual void OnError(const wchar_t* szError, ...);
	virtual void OnOutputDebug(const wchar_t* szError, ...);
	virtual void OnOutputSystem(const wchar_t* szError, ...);
};



