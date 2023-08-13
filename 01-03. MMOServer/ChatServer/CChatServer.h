#pragma once

#include "../NetLib/CNetServer.h"
#include "CLANClientMonitoring.h"
#include "../utils/CMemoryPoolTLS.h"
#include "../utils/CLockfreeQueue.h"

#include "CSector.h"

class CDBAsyncWriter;
class CCpuUsage;
class CPDH;
namespace cpp_redis
{
	class client;
}

namespace chatserver
{
	class CChatServer : public netlib::CNetServer
	{
	public:
		class Monitor;

	public:
		CChatServer();
		virtual ~CChatServer();

		CChatServer(const CChatServer&) = delete;
		CChatServer(CChatServer&&) = delete;

	public:
		/* StartUp */
		bool StartUp();

		/* Shutdown */
		bool Shutdown();

	private:
		/* Get */
		CPlayer_t GetPlayerBySessionId(__int64 sessionId);
		CPlayer_t GetPlayerByAccountNo(__int64 accountNo);

		/* ��Ʈ��ũ ���� */
		int SendUnicast(__int64 sessionId, CPacket& packet);
		int SendUnicast(const CPlayer_t& pPlayer, CPacket& packet);
		int SendBroadcast(CPacket& packet);
		int SendOneSector(const CPlayer_t& pPlayer, CPacket& packet, const CPlayer_t& except);
		int SendAroundSector(const CPlayer_t& pPlayer, CPacket& packet, const CPlayer_t& except);

		/* player */
		CPlayer_t AllocPlayer(__int64 sessionId);
		void MoveSector(CPlayer_t& pPlayer, WORD x, WORD y);
		void DisconnectPlayer(CPlayer_t& pPlayer);
		void DeletePlayer(__int64 sessionId);
		void ReplacePlayerByAccountNo(__int64 accountNo, CPlayer_t& replacePlayer);

		/* Thread */
		static unsigned WINAPI ThreadChatServer(PVOID pParam);
		static unsigned WINAPI ThreadMsgGenerator(PVOID pParam);
		static unsigned WINAPI ThreadMonitoringCollector(PVOID pParam);
		void RunChatServer();
		void RunMsgGenerator();
		void RunMonitoringCollector();

		/* crash */
		void Crash();

		/* �α��� ������ ó�� APC �Լ� */
		static void CompleteUnfinishedLogin(ULONG_PTR pStAPCData);
		

	private:
		// ��Ʈ��ũ ���̺귯�� callback �Լ� ����
		virtual bool OnRecv(__int64 sessionId, CPacket& packet);
		virtual bool OnConnectionRequest(unsigned long IP, unsigned short port);
		virtual bool OnClientJoin(__int64 sessionId);
		virtual bool OnClientLeave(__int64 sessionId);
		virtual void OnError(const wchar_t* szError, ...);
		virtual void OnOutputDebug(const wchar_t* szError, ...);
		virtual void OnOutputSystem(const wchar_t* szError, ...);


	public:
		/* Get �������� */
		int GetMsgPoolSize() { return _poolMsg.GetPoolSize(); };
		int GetMsgAllocCount() { return _poolMsg.GetAllocCount(); }
		int GetMsgActualAllocCount() { return _poolMsg.GetActualAllocCount(); }
		int GetMsgFreeCount() { return _poolMsg.GetFreeCount(); }
		int GetUnhandeledMsgCount() { return _msgQ.Size(); }
		int GetNumPlayer() { return (int)_mapPlayer.size(); }
		int GetNumAccount() { return (int)_mapPlayerAccountNo.size(); }
		int GetPlayerPoolSize() { return _poolPlayer.GetPoolSize(); };
		int GetPlayerAllocCount() { return _poolPlayer.GetAllocCount(); }
		int GetPlayerActualAllocCount() { return _poolPlayer.GetActualAllocCount(); }
		int GetPlayerFreeCount() { return _poolPlayer.GetFreeCount(); }
		bool IsTerminated() { return _bTerminated; }   // ����� ����

		/* Get ����͸� count */
		const Monitor& GetMonitor() { return _monitor; }
		const CNetServer::Monitor& GetNetworkMonitor() { return CNetServer::_monitor; }

		/* DB ���� */
		int GetUnprocessedQueryCount();
		__int64 GetQueryRunCount();
		float GetMaxQueryRunTime();
		float Get2ndMaxQueryRunTime();
		float GetMinQueryRunTime();
		float Get2ndMinQueryRunTime();
		float GetAvgQueryRunTime();
		int GetQueryRequestPoolSize();
		int GetQueryRequestAllocCount();
		int GetQueryRequestFreeCount();

		
	private:
		/* packet processing */
		void PacketProc_LoginRequest(__int64 sessionId, CPacket& packet);
		void PacketProc_SectorMoveRequest(__int64 sessionId, CPacket& packet);
		void PacketProc_ChatRequest(__int64 sessionId, CPacket& packet);
		void PacketProc_HeartBeat(__int64 sessionId, CPacket& packet);
		void PacketProc_Default(__int64 sessionId, CPacket& packet, WORD packetType);
		/* server message processing */
		void MsgProc_JoinPlayer(__int64 sessionId);
		void MsgProc_LeavePlayer(__int64 sessionId);
		void MsgProc_CheckLoginTimeout();
		void MsgProc_CheckHeartBeatTimeout();
		void MsgProc_Shutdown();



	private:
		/* ��� */
		std::wstring _sCurrentPath;     // ���� ���丮 ���
		std::wstring _sConfigFilePath;  // config ���� ���

		/* config */
		struct Config
		{
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
		};
		Config _config;


		/* redis */
		std::unique_ptr<cpp_redis::client> _pRedisClient;

		/* Async DB Writer */
		std::unique_ptr<CDBAsyncWriter> _pDBConn;

		/* sector */
		CSectorGrid _sector;

		/* �÷��̾� */
		std::unordered_map<__int64, CPlayer_t> _mapPlayer;
		std::unordered_map<__int64, CPlayer_t> _mapPlayerAccountNo;
		CMemoryPoolTLS<CPlayer> _poolPlayer;

		/* ä�ü��� */
		int _serverNo;                         // ������ȣ(ä�ü���:1)
		struct ThreadInfo
		{
			HANDLE handle;
			unsigned int id;
			ThreadInfo() :handle(NULL), id(0) {}
		};
		ThreadInfo _thChatServer;
		ThreadInfo _thMsgGenerator;
		ThreadInfo _thMonitoringCollector;

		volatile bool _bShutdown;               // shutdown ����
		volatile bool _bTerminated;             // ����� ����
		std::mutex _mtxShutdown;
		LARGE_INTEGER _performanceFrequency;

		/* �޽��� */
		CLockfreeQueue<MsgChatServer*> _msgQ;
		CMemoryPoolTLS<MsgChatServer>  _poolMsg;
		CMemoryPoolTLS<StAPCData> _poolAPCData;     // redis APC ��û�� ���� ������ Ǯ
		HANDLE _hEventMsg;
		bool _bEventSetFlag;

		/* timeout */
		int _timeoutLogin;
		int _timeoutHeartBeat;
		int _timeoutLoginCheckPeriod;
		int _timeoutHeartBeatCheckPeriod;

		/* ����͸� Ŭ���̾�Ʈ */
		std::unique_ptr<CLANClientMonitoring> _pLANClientMonitoring;       // ����͸� ������ ������ LAN Ŭ���̾�Ʈ
		std::unique_ptr<CCpuUsage> _pCPUUsage;
		std::unique_ptr<CPDH> _pPDH;



		/* ����͸� counter */
	public:
		class Monitor
		{
			friend class CChatServer;
		private:
			volatile __int64 eventWaitTime = 0;              // �̺�Ʈ�� ��ٸ� Ƚ��
			volatile __int64 msgHandleCount = 0;              // �޽��� ó�� Ƚ��
			volatile __int64 disconnByPlayerLimit = 0;        // �÷��̾�� �������� ����
			volatile __int64 disconnByLoginFail = 0;          // �α��� ���з� ����
			volatile __int64 disconnByNoSessionKey = 0;       // redis�� ����key�� ����
			volatile __int64 disconnByInvalidSessionKey = 0;  // redis�� ����key�� �ٸ�
			volatile __int64 disconnByDupPlayer = 0;          // �ߺ��α������� ����
			volatile __int64 disconnByInvalidAccountNo = 0;   // accountNo�� �߸���
			volatile __int64 disconnByInvalidSector = 0;      // Sector�� �߸���
			volatile __int64 disconnByInvalidMessageType = 0; // �޽��� Ÿ���� �߸���
			volatile __int64 disconnByLoginTimeout = 0;       // �α��� Ÿ�Ӿƿ����� ����
			volatile __int64 disconnByHeartBeatTimeout = 0;   // ��Ʈ��Ʈ Ÿ�Ӿƿ����� ����

		public:
			__int64 GetEventWaitTime() const { return eventWaitTime; }
			__int64 GetMsgHandleCount() const { return msgHandleCount; }
			__int64 GetDisconnByPlayerLimit() const { return disconnByPlayerLimit; }
			__int64 GetDisconnByLoginFail() const { return disconnByLoginFail; }
			__int64 GetDisconnByNoSessionKey() const { return disconnByNoSessionKey; }
			__int64 GetDisconnByInvalidSessionKey() const { return disconnByInvalidSessionKey; }
			__int64 GetDisconnByDupPlayer() const { return disconnByDupPlayer; }
			__int64 GetDisconnByInvalidAccountNo() const { return disconnByInvalidAccountNo; }
			__int64 GetDisconnByInvalidSector() const { return disconnByInvalidSector; }
			__int64 GetDisconnByInvalidMessageType() const { return disconnByInvalidMessageType; }
			__int64 GetDisconnByLoginTimeout() const { return disconnByLoginTimeout; }
			__int64 GetDisconnByHeartBeatTimeout() const { return disconnByHeartBeatTimeout; }
		};
	private:
		Monitor _monitor;

	};

};