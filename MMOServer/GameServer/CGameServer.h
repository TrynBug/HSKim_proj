#pragma once

#include "../GameNetLib/CNetServer.h"
#include "defineGameServer.h"
#include "CPlayer.h"

#include "../utils/cpp_redis/cpp_redis.h"
#include "../utils/CMemoryPool.h"

class CJsonParser;
class CLANClientMonitoring;
class CCpuUsage;
class CPDH;
class CGameServerAccessor;

class CGameServer : public netlib_game::CNetServer
{
	friend class CGameContents;
	friend class CGameServerAccessor;

public:
	CGameServer();
	virtual ~CGameServer();

public:
	/* game server */
	bool StartUp();
	void Shutdown();
	bool IsTerminated() { return _bTerminated; }   // ����� ����

	/* Get */
	int GetPlayerPoolSize() const { return _poolPlayer.GetPoolSize(); }
	//int GetPlayerAllocCount() const  { return _poolPlayer.GetAllocCount(); }
	//int GetPlayerActualAllocCount() const  { return _poolPlayer.GetActualAllocCount(); }
	int GetPlayerAllocCount() const { return _poolPlayer.GetUseCount(); }
	int GetPlayerActualAllocCount() const { return _poolPlayer.GetUseCount(); }
	int GetPlayerFreeCount() const { return _poolPlayer.GetFreeCount(); }

	/* ������ */
	const std::shared_ptr<CGameContents> GetGameContents(EContentsThread contents);

private:
	/* ������ */
	std::shared_ptr<netlib_game::CContents> GetContents(EContentsThread contents);

	/* accessor */
	std::unique_ptr<CGameServerAccessor> CreateAccessor();

	/* object pool */
	CPlayer& AllocPlayer() { return *_poolPlayer.Alloc(); }
	void FreePlayer(CPlayer& player);

	/* ��Ʈ��ũ callback �Լ� ���� */
	virtual bool OnConnectionRequest(unsigned long IP, unsigned short port); 
	virtual void OnError(const wchar_t* szError, ...);
	virtual void OnOutputDebug(const wchar_t* szError, ...);
	virtual void OnOutputSystem(const wchar_t* szError, ...);

	/* ������ */
	static unsigned WINAPI ThreadMonitoringCollector(PVOID pParam);  // ����͸� ������

private:
	/* ��� */
	std::wstring _sCurrentPath;     // ���� ���丮 ���
	std::wstring _sConfigFilePath;  // config ���� ���

	/* config */
	struct Config
	{
		std::unique_ptr<CJsonParser> jsonParser;    // config.json ���� parser
		wchar_t szBindIP[20];
		unsigned short portNumber;
		int numConcurrentThread;
		int numWorkerThread;
		int numMaxSession;
		int numMaxPlayer;
		bool bUseNagle;
		BYTE packetCode;
		BYTE packetKey;
		int maxPacketSize;
		unsigned short monitoringServerPortNumber;
		BYTE monitoringServerPacketCode;
		BYTE monitoringServerPacketKey;

		Config();
		const CJsonParser& GetJsonParser() const;
	};
	Config _config;

	/* game server */
	int _serverNo;                          // ������ȣ(���Ӽ���:2)
	volatile bool _bShutdown;               // shutdown ����
	volatile bool _bTerminated;             // ����� ����

	/* redis */
	std::unique_ptr<cpp_redis::client> _pRedisClient;

	/* ������ */
	std::unordered_map<EContentsThread, std::shared_ptr<netlib_game::CContents>> _mapContents;

	/* �÷��̾� */
	//CMemoryPoolTLS<CPlayer> _poolPlayer;               // �÷��̾� Ǯ
	CMemoryPool<CPlayer> _poolPlayer;

	/* ����͸� */
	std::unique_ptr<CLANClientMonitoring> _pLANClientMonitoring;       // ����͸� ������ ������ LAN Ŭ���̾�Ʈ
	struct ThreadInfo
	{
		HANDLE handle;                // ����͸� ������ ���� ������
		unsigned int id;
		ThreadInfo() : handle(0), id(0) {}
	};
	ThreadInfo _thMonitoring;

	/* utils */
	std::unique_ptr<CCpuUsage> _pCPUUsage;
	std::unique_ptr<CPDH> _pPDH;

	volatile __int64 _disconnByPlayerLimit = 0;        // �÷��̾�� �������� ����
};






// CGameServer�� �Ϻ� ��ɿ��� ���ٰ����ϵ��� ���� Ŭ����. CGameContents Ŭ�������� ����Ѵ�.
class CGameServerAccessor
{
	friend class CGameServer;

private:
	CGameServerAccessor(CGameServer& gameServer);   // �� Ŭ������ �ν��Ͻ��� CGameServer �κ��͸� ������ �� �ִ�.
public:
	~CGameServerAccessor();

public:
	const CGameServer::Config& GetConfig() { return _gameServer._config; }
	cpp_redis::client& GetRedisClient() { return *_gameServer._pRedisClient; }
	std::shared_ptr<netlib_game::CContents> GetContents(EContentsThread contents) { return _gameServer.GetContents(contents); }

	CPlayer& AllocPlayer() { return _gameServer.AllocPlayer(); }
	void FreePlayer(CPlayer& player) { _gameServer.FreePlayer(player); }

private:
	CGameServer& _gameServer;
};