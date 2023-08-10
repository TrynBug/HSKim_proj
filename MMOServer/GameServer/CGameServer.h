#pragma once

#include "../GameNetLib/CNetServer.h"

#include "../utils/cpp_redis/cpp_redis.h"
#include "../utils/CMemoryPool.h"

class CJsonParser;
class CLANClientMonitoring;
class CCpuUsage;
class CPDH;

namespace gameserver
{

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
		bool IsTerminated() { return _bTerminated; }   // 종료됨 여부

		/* Get */
		int GetPlayerPoolSize() const { return _poolPlayer.GetPoolSize(); }
		//int GetPlayerAllocCount() const  { return _poolPlayer.GetAllocCount(); }
		//int GetPlayerActualAllocCount() const  { return _poolPlayer.GetActualAllocCount(); }
		int GetPlayerAllocCount() const { return _poolPlayer.GetUseCount(); }
		int GetPlayerActualAllocCount() const { return _poolPlayer.GetUseCount(); }
		int GetPlayerFreeCount() const { return _poolPlayer.GetFreeCount(); }

		/* 컨텐츠 */
		const std::shared_ptr<CGameContents> GetGameContents(EContentsThread contents);

	private:
		/* 컨텐츠 */
		std::shared_ptr<netlib_game::CContents> GetContents(EContentsThread contents);

		/* accessor */
		std::unique_ptr<CGameServerAccessor> CreateAccessor();

		/* object pool */
		CPlayer_t AllocPlayer();
		_TransferPlayerData* AllocTransferPlayerData(CPlayer_t pPlayer);
		void FreeTransferPlayerData(_TransferPlayerData* pData);

		/* 네트워크 callback 함수 구현 */
		virtual bool OnConnectionRequest(unsigned long IP, unsigned short port);
		virtual void OnError(const wchar_t* szError, ...);
		virtual void OnOutputDebug(const wchar_t* szError, ...);
		virtual void OnOutputSystem(const wchar_t* szError, ...);

		/* 스레드 */
		static unsigned WINAPI ThreadMonitoringCollector(PVOID pParam);  // 모니터링 스레드

		/* crash */
		void Crash();

	private:
		/* 경로 */
		std::wstring _sCurrentPath;     // 현재 디렉토리 경로
		std::wstring _sConfigFilePath;  // config 파일 경로

		/* config */
		struct Config
		{
			std::unique_ptr<CJsonParser> jsonParser;    // config.json 파일 parser
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
			void LoadConfigFile(std::wstring& sFilePath);
			const CJsonParser& GetJsonParser() const;
		};
		Config _config;

		/* game server */
		int _serverNo;                          // 서버번호(게임서버:2)
		volatile bool _bShutdown;               // shutdown 여부
		volatile bool _bTerminated;             // 종료됨 여부

		/* redis */
		std::unique_ptr<cpp_redis::client> _pRedisClient;

		/* 컨텐츠 */
		std::unordered_map<EContentsThread, std::shared_ptr<netlib_game::CContents>> _mapContents;

		/* pool */
		CMemoryPool<CPlayer> _poolPlayer;
		CMemoryPool<_TransferPlayerData> _poolTransferPlayerData;

		/* 모니터링 */
		std::unique_ptr<CLANClientMonitoring> _pLANClientMonitoring;       // 모니터링 서버와 연결할 LAN 클라이언트
		struct ThreadInfo
		{
			HANDLE handle;                // 모니터링 데이터 수집 스레드
			unsigned int id;
			ThreadInfo() : handle(0), id(0) {}
		};
		ThreadInfo _thMonitoring;

		/* utils */
		std::unique_ptr<CCpuUsage> _pCPUUsage;
		std::unique_ptr<CPDH> _pPDH;

		volatile __int64 _disconnByPlayerLimit = 0;        // 플레이어수 제한으로 끊김
	};






	// CGameServer의 일부 기능에만 접근가능하도록 만든 클래스. CGameContents 클래스에서 사용한다.
	class CGameServerAccessor
	{
		friend class CGameServer;

	private:
		CGameServerAccessor(CGameServer& gameServer);   // 이 클래스의 인스턴스는 CGameServer 로부터만 생성될 수 있다.
	public:
		~CGameServerAccessor();

	public:
		const CGameServer::Config& GetConfig() { return _gameServer._config; }
		cpp_redis::client& GetRedisClient() { return *_gameServer._pRedisClient; }
		std::shared_ptr<netlib_game::CContents> GetContents(EContentsThread contents) { return _gameServer.GetContents(contents); }

		CPlayer_t AllocPlayer() { return _gameServer.AllocPlayer(); }
		_TransferPlayerData* AllocTransferPlayerData(CPlayer_t pPlayer) { return _gameServer.AllocTransferPlayerData(std::move(pPlayer)); }
		void FreeTransferPlayerData(_TransferPlayerData* pData) { _gameServer.FreeTransferPlayerData(pData); }

	private:
		CGameServer& _gameServer;
	};


}