#pragma once

#include "../NetLib/CNetServer.h"
#include "defineMonitoringServer.h"
#include "../utils/CDBConnector.h"
#include "../utils/CMemoryPool.h"

#include <sstream>

class CUser;
class CDBConnector;

namespace monitorserver
{
	class alignas(64) CMonitoringServer : public netlib::CNetServer
	{
	public:
		class Monitor;

	public:
		CMonitoringServer();
		~CMonitoringServer();

		CMonitoringServer(const CMonitoringServer&) = delete;
		CMonitoringServer(CMonitoringServer&&) = delete;

		/* dynamic alloc */
		// 64byte aligned ��ü ������ ���� new, delete overriding
		void* operator new(size_t size);
		void operator delete(void* p);

	public:
		/* StartUp */
		bool StartUp();

		/* Shutdown */
		bool Shutdown();

		/* Get �������� */
		int IsTerminated() { return _bTerminated; }
		int GetNumUser() { return (int)_mapUser.size(); }
		int GetNumWANUser() { return (int)_mapWANUser.size(); }
		int GetUserPoolSize() { return _poolUser.GetPoolSize(); };
		int GetUserAllocCount() { return _poolUser.GetUseCount(); }
		int GetUserFreeCount() { return _poolUser.GetFreeCount(); }
		__int64 GetQueryRunCount() { return _pDBConn->GetQueryRunCount(); }
		float GetMaxQueryRunTime() { return _pDBConn->GetMaxQueryRunTime(); }
		float GetMinQueryRunTime() { return _pDBConn->GetMinQueryRunTime(); }
		float GetAvgQueryRunTime() { return _pDBConn->GetAvgQueryRunTime(); }

		/* Get ����͸� count */
		const Monitor& GetMonitor() { return _monitor; }
		const CNetServer::Monitor& GetNetworkMonitor() { return CNetServer::_monitor; }

	private:
		/* ��Ʈ��ũ ���� */
		int SendUnicast(__int64 sessionId, CPacket& packet);

		/* ���� */
		CUser_t AllocUser(__int64 sessionId);
		CUser_t GetUserBySessionId(__int64 sessionId);
		void DisconnectSession(__int64 sessionId);
		void DisconnectSession(CUser_t pUser);

		/* Thread */
		static unsigned WINAPI ThreadJobGenerator(PVOID pParam);

		/* crash */
		void Crash();

	private:
		// ��Ʈ��ũ ���̺귯�� �Լ� ����
		virtual bool OnRecv(__int64 sessionId, CPacket& packet);
		virtual bool OnInnerRequest(CPacket& packet);
		virtual bool OnConnectionRequest(unsigned long IP, unsigned short port);
		virtual bool OnClientJoin(__int64 sessionId);
		virtual bool OnClientLeave(__int64 sessionId);
		virtual void OnError(const wchar_t* szError, ...);
		virtual void OnOutputDebug(const wchar_t* szError, ...);
		virtual void OnOutputSystem(const wchar_t* szError, ...);


	private:
		/* ��� */
		std::wstring _sCurrentPath;     // ���� ���丮 ���
		std::wstring _sConfigFilePath;  // config ���� ���

		/* config */
		struct Config
		{
			wchar_t szBindIP[20];
			unsigned short portNumber;
			int numMaxSession;
			int numMaxUser;
			bool bUseNagle;
			bool bUsePacketEncryption;  // ��Ŷ ��ȣȭ ��� ����

			BYTE packetCode;
			BYTE packetKey;
			int maxPacketSize;

			Config();
			void LoadConfigFile(std::wstring& sFilePath);
		};
		Config _config;

		/* ���� */
		std::unordered_map<__int64, CUser_t> _mapUser;
		std::unordered_map<__int64, CUser_t> _mapWANUser;
		CMemoryPool<CUser> _poolUser;

		/* ���� */
		char _loginKey[32];          // login key    
		volatile bool _bShutdown;    // shutdown ����
		volatile bool _bTerminated;  // ����� ����
		std::mutex _mtxShutdown;
		int _timeoutLogin;           // �α��� Ÿ�Ӿƿ� ���ð�

		/* DB */
		std::unique_ptr<CDBConnector> _pDBConn;
		std::wostringstream _ossQuery;

		/* ������ */
		HANDLE _hThreadJobGenerator;
		unsigned int _idThreadJobGenerator;

		/* �����κ��� ������ ����͸� ������ */
		struct MonitoringDataSet
		{
			int dataCollectCount = 0;
			int arrValue5m[300] = { 0, };
		};
		std::vector<MonitoringDataSet> _vecMonitoringDataSet;


	public:
		/* ����͸� counter */
		class Monitor
		{
			friend class CMonitoringServer;
		private:
			volatile __int64 msgHandleCount = 0;              // �޽��� ó�� Ƚ��
			volatile __int64 receivedInvalidDatatype = 0;     // �߸��� ����͸� ������ Ÿ���� ����
			volatile __int64 disconnByUserLimit = 0;          // ������ �������� ����
			volatile __int64 disconnByInvalidMessageType = 0; // �޽��� Ÿ���� �߸���
			volatile __int64 disconnByLoginKeyMismatch = 0;   // �α��� Ű�� �߸���
			volatile __int64 disconnByWANUserUpdateRequest = 0;  // WAN ������ ������ ������Ʈ ��û�� ����
			volatile __int64 DBErrorCount = 0;                // DB ���� Ƚ��

		public:
			/* Get ����͸� */
			__int64 GetMsgHandleCount() const { return msgHandleCount; }
			__int64 GetReceivedInvalidDatatype() const { return receivedInvalidDatatype; }
			__int64 GetDisconnByUserLimit() const { return disconnByUserLimit; }
			__int64 GetDisconnByInvalidMessageType() const { return disconnByInvalidMessageType; }
			__int64 GetDisconnByLoginKeyMismatch() const { return disconnByLoginKeyMismatch; }
			__int64 GetDisconnByWANUserUpdateRequest() const { return disconnByWANUserUpdateRequest; }
			__int64 GetDBErrorCount() const { return DBErrorCount; }
		};
	private:
		Monitor _monitor;


	};

}