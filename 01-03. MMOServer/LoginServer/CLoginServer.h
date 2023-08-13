#pragma once

#include "../Netlib/CNetServer.h"
#include "CLANClientMonitoring.h"
#include "defineLoginServer.h"
#include "CClient.h"

#include "../utils/CMemoryPoolTLS.h"
#include "../utils/CDBConnectorTLSManager.h"

#include <shared_mutex>

class CDBConnectorTLSManager;
class CCpuUsage;
class CPDH;


namespace cpp_redis
{
	class client;
}

namespace loginserver
{
	using CPacket = netlib::CPacket;
	using CClient_t = std::shared_ptr<CClient>;


	class CLoginServer : public netlib::CNetServer
	{
	public:
		class Monitor;

	public:
		CLoginServer();
		virtual ~CLoginServer();

		CLoginServer(const CLoginServer&) = delete;
		CLoginServer(CLoginServer&&) = delete;

	public:
		/* StartUp */
		bool StartUp();

		/* Shutdown */
		bool Shutdown();

	private:
		/* Get */
		CClient_t GetClientBySessionId(__int64 sessionId);   // ����ID�� Ŭ���̾�Ʈ ��ü�� ��´�. �����Ұ�� null�� ��ȯ�Ѵ�.

		/* Client */
		void InsertClientToMap(__int64 sessionId, CClient_t pClient);   // ����ID-Ŭ���̾�Ʈ map�� Ŭ���̾�Ʈ�� ����Ѵ�. 
		void DeleteClientFromMap(__int64 sessionId);                   // ����ID-Ŭ���̾�Ʈ map���� Ŭ���̾�Ʈ�� �����Ѵ�.
		void DisconnectSession(__int64 sessionId);                     // Ŭ���̾�Ʈ�� ������ ���´�.
		void DeleteClient(CClient_t& pClient);                           // Ŭ���̾�Ʈ�� map���� �����ϰ� free �Ѵ�.

		CClient_t AllocClient(__int64 sessionId);

		/* account */
		bool InsertAccountToMap(__int64 accountNo, __int64 sessionId);     // account��ȣ�� account��ȣ-����ID map�� ����Ѵ�. ���� accountNo�� �ش��ϴ� �����Ͱ� �̹� ������ ��� �����ϸ� false�� ��ȯ�Ѵ�.
		void DeleteAccountFromMap(__int64 accountNo);   // account��ȣ�� account��ȣ-����ID map���� �����Ѵ�.

		/* ��Ʈ��ũ ���� */
		int SendUnicast(__int64 sessionId, CPacket& packet);
		int SendUnicastAndDisconnect(__int64 sessionId, CPacket& packet);

		/* ������ */
		static unsigned WINAPI ThreadPeriodicWorker(PVOID pParam);       // �ֱ��� �۾� ó�� ������
		static unsigned WINAPI ThreadMonitoringCollector(PVOID pParam);

		/* crash */
		void Crash();

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
		/* ���� ���� */
		int GetNumClient() const { return (int)_mapClient.size(); }
		int GetSizeAccountMap() const { return (int)_mapAccountToSession.size(); }
		int GetClientPoolSize() const { return _poolClient.GetPoolSize(); };
		int GetClientAllocCount() const { return _poolClient.GetAllocCount(); }
		int GetClientActualAllocCount() const { return _poolClient.GetActualAllocCount(); }
		int GetClientFreeCount() const { return _poolClient.GetFreeCount(); }
		int GetNumDBConnection() const { return _tlsDBConnector->GetNumConnection(); }
		__int64 GetQueryRunCount() const { return _tlsDBConnector->GetQueryRunCount(); }
		float GetMaxQueryRunTime() const { return _tlsDBConnector->GetMaxQueryRunTime(); }
		float GetMinQueryRunTime() const { return _tlsDBConnector->GetMinQueryRunTime(); }
		float GetAvgQueryRunTime() const { return _tlsDBConnector->GetAvgQueryRunTime(); }
		bool IsShutdown() const { return _bShutdown; }
		bool IsTerminated() const { return _bTerminated; }

		/* Get ����͸� count */
		const Monitor& GetMonitor() { return _monitor; }
		const CNetServer::Monitor& GetNetworkMonitor() { return CNetServer::_monitor; }


	private:
		/* packet processing :: ��Ŷ ó�� */
		void PacketProc_LoginRequest(__int64 sessionId, CPacket& packet);
		/* packet processing :: ��Ŷ�� ������ �Է� */
		void Msg_LoginResponseFail(CPacket& packet, __int64 accountNo);
		void Msg_LoginResponseAccountMiss(CPacket& packet, __int64 accountNo);
		void Msg_LoginResponseSucceed(CPacket& packet, CClient_t& client);
		/* database processing */
		void DB_UpdateClientStatus(CClient_t& pClient);



	private:
		/* ��� */
		std::wstring _sCurrentPath;     // ���� ���丮 ���
		std::wstring _sConfigFilePath;  // config ���� ���

		/* config */
		struct Config
		{
			wchar_t szBindIP[20];
			unsigned short portNumber;
			wchar_t szGameServerIP[20];
			wchar_t szGameServerDummy1IP[20];
			wchar_t szGameServerDummy2IP[20];
			unsigned short gameServerPort;
			wchar_t szChatServerIP[20];
			wchar_t szChatServerDummy1IP[20];
			wchar_t szChatServerDummy2IP[20];
			unsigned short chatServerPort;
			int numConcurrentThread;
			int numWorkerThread;
			int numMaxSession;
			int numMaxClient;
			bool bUseNagle;

			BYTE packetCode;
			BYTE packetKey;
			int maxPacketSize;

			unsigned short monitoringServerPortNumber;
			BYTE monitoringServerPacketCode;
			BYTE monitoringServerPacketKey;

		public:
			Config();
			void LoadConfigFile(std::wstring& sFilePath);
		};
		Config _config;



		/* server */
		int _serverNo;                         // ������ȣ(�α��μ���:3)
		struct ThreadInfo
		{
			HANDLE handle;
			unsigned int id;
			ThreadInfo() :handle(NULL), id(0) {}
		};
		ThreadInfo _thPeriodicWorker;          // �ֱ��� �۾� ������
		ThreadInfo _thMonitoringCollector;     // ����͸� ������ ���� ������

		volatile bool _bShutdown;               // shutdown ����
		volatile bool _bTerminated;             // ����� ����
		std::mutex _mtxShutdown;
		LARGE_INTEGER _performanceFrequency;


		/* timeout */
		int _timeoutLogin;
		int _timeoutLoginCheckPeriod;

		/* TLS DBConnector */
		std::unique_ptr<CDBConnectorTLSManager> _tlsDBConnector;

		/* Client */
		std::unordered_map<__int64, CClient_t> _mapClient;
		std::shared_mutex _smtxMapClient;

		/* account */
		std::unordered_map<__int64, __int64> _mapAccountToSession;
		std::shared_mutex _smtxMapAccountToSession;

		/* pool */
		CMemoryPoolTLS<CClient> _poolClient;      // Ŭ���̾�Ʈ Ǯ

		/* redis */
		std::unique_ptr<cpp_redis::client> _pRedisClient;

		/* ����͸� Ŭ���̾�Ʈ */
		std::unique_ptr<CLANClientMonitoring> _pLANClientMonitoring;       // ����͸� ������ ������ LAN Ŭ���̾�Ʈ
		std::unique_ptr<CCpuUsage> _pCPUUsage;
		std::unique_ptr<CPDH> _pPDH;

		/* ����͸� counter */
	public:
		class Monitor
		{
			friend class CLoginServer;

			std::atomic<__int64> loginCount = 0;                  // �α��� ���� Ƚ��
			std::atomic<__int64> disconnByClientLimit = 0;        // Ŭ���̾�Ʈ�� �������� ����
			std::atomic<__int64> disconnByNoClient = 0;           // ���ǹ�ȣ�� ���� Ŭ���̾�Ʈ ��ü�� ã������
			std::atomic<__int64> disconnByDBDataError = 0;        // DB ������ ����
			std::atomic<__int64> disconnByNoAccount = 0;          // account ���̺��� account ��ȣ�� ã�� ����
			std::atomic<__int64> disconnByDupAccount = 0;         // ���� account�� �ٸ������忡�� �α����� ��������
			std::atomic<__int64> disconnByInvalidMessageType = 0; // �޽��� Ÿ���� �߸���
			std::atomic<__int64> disconnByLoginTimeout = 0;       // �α��� Ÿ�Ӿƿ����� ����
			std::atomic<__int64> disconnByHeartBeatTimeout = 0;   // ��Ʈ��Ʈ Ÿ�Ӿƿ����� ����

		public:
			__int64 GetLoginCount() const { return loginCount; }
			__int64 GetDisconnByClientLimit() const { return disconnByClientLimit; }
			__int64 GetDisconnByNoClient() const { return disconnByNoClient; }
			__int64 GetDisconnByDBDataError() const { return disconnByDBDataError; }
			__int64 GetDisconnByNoAccount() const { return disconnByNoAccount; }
			__int64 GetDisconnByDupAccount() const { return disconnByDupAccount; }
			__int64 GetDisconnByInvalidMessageType() const { return disconnByInvalidMessageType; }
			__int64 GetDisconnByLoginTimeout() const { return disconnByLoginTimeout; }
			__int64 GetDisconnByHeartBeatTimeout() const { return disconnByHeartBeatTimeout; }
		};
	private:
		Monitor _monitor;

	};

}