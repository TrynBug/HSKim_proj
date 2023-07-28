#pragma once

#include "netserver/CNetServer.h"
#include "CLANClientMonitoring.h"

#include "defineChatServer.h"
#include "CMemoryPoolTLS.h"
#include "CLockfreeQueue.h"


class CSector;
class CPlayer;
class CDBAsyncWriter;
class CCpuUsage;
class CPDH;

namespace cpp_redis
{
	class client;
}


class CChatServer : public netserver::CNetServer
{
private:
	/* ��� */
	WCHAR _szCurrentPath[MAX_PATH];     // ���� ���丮 ���
	WCHAR _szConfigFilePath[MAX_PATH];  // config ���� ���

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

	/* redis */
	cpp_redis::client* _pRedisClient;

	/* Async DB Writer */
	CDBAsyncWriter* _pDBConn;

	/* sector */
	CSector** _sector;

	/* �÷��̾� */
	std::unordered_map<__int64, CPlayer*> _mapPlayer;
	std::unordered_map<__int64, CPlayer*> _mapPlayerAccountNo;
	CMemoryPoolTLS<CPlayer> _poolPlayer;

	/* ä�ü��� */
	int _serverNo;                         // ������ȣ(ä�ü���:1)
	HANDLE _hThreadChatServer;             // ä�� ������
	unsigned int _threadChatServerId;
	HANDLE _hThreadMsgGenerator;           // �ֱ��� �۾� �޽��� �߻� ������
	unsigned int _threadMsgGeneratorId;
	HANDLE _hThreadMonitoringCollector;    // ����͸� ������ ���� ������
	unsigned int _threadMonitoringCollectorId;

	volatile bool _bShutdown;               // shutdown ����
	volatile bool _bTerminated;             // ����� ����
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
	
	/* ����͸� */
	CLANClientMonitoring* _pLANClientMonitoring;       // ����͸� ������ ������ LAN Ŭ���̾�Ʈ
	CCpuUsage* _pCPUUsage;
	CPDH* _pPDH;

	volatile __int64 _eventWaitTime = 0;              // �̺�Ʈ�� ��ٸ� Ƚ��
	volatile __int64 _msgHandleCount = 0;              // �޽��� ó�� Ƚ��
	volatile __int64 _disconnByPlayerLimit = 0;        // �÷��̾�� �������� ����
	volatile __int64 _disconnByLoginFail = 0;          // �α��� ���з� ����
	volatile __int64 _disconnByNoSessionKey = 0;       // redis�� ����key�� ����
	volatile __int64 _disconnByInvalidSessionKey = 0;  // redis�� ����key�� �ٸ�
	volatile __int64 _disconnByDupPlayer = 0;          // �ߺ��α������� ����
	volatile __int64 _disconnByInvalidAccountNo = 0;   // accountNo�� �߸���
	volatile __int64 _disconnByInvalidSector = 0;      // Sector�� �߸���
	volatile __int64 _disconnByInvalidMessageType = 0; // �޽��� Ÿ���� �߸���
	volatile __int64 _disconnByLoginTimeout = 0;       // �α��� Ÿ�Ӿƿ����� ����
	volatile __int64 _disconnByHeartBeatTimeout = 0;   // ��Ʈ��Ʈ Ÿ�Ӿƿ����� ����
	
	

public:
	CChatServer();
	virtual ~CChatServer();

public:
	/* StartUp */
	bool StartUp();

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

	/* Get ����͸� */
	__int64 GetEventWaitTime() { return _eventWaitTime; }
	__int64 GetMsgHandleCount() { return _msgHandleCount; }
	__int64 GetDisconnByPlayerLimit() { return _disconnByPlayerLimit; }
	__int64 GetDisconnByLoginFail() { return _disconnByLoginFail; }
	__int64 GetDisconnByNoSessionKey() { return _disconnByNoSessionKey; }
	__int64 GetDisconnByInvalidSessionKey() { return _disconnByInvalidSessionKey; }
	__int64 GetDisconnByDupPlayer() { return _disconnByDupPlayer; }
	__int64 GetDisconnByInvalidAccountNo() { return _disconnByInvalidAccountNo; }
	__int64 GetDisconnByInvalidSector() { return _disconnByInvalidSector; }
	__int64 GetDisconnByInvalidMessageType() { return _disconnByInvalidMessageType; }
	__int64 GetDisconnByLoginTimeout() { return _disconnByLoginTimeout; }
	__int64 GetDisconnByHeartBeatTimeout() { return _disconnByHeartBeatTimeout; }


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

	/* Shutdown */
	bool Shutdown();

private:
	/* Get */
	CPlayer* GetPlayerBySessionId(__int64 sessionId);
	CPlayer* GetPlayerByAccountNo(__int64 accountNo);

	/* ��Ʈ��ũ ���� */
	int SendUnicast(__int64 sessionId, netserver::CPacket* pPacket);
	int SendUnicast(CPlayer* pPlayer, netserver::CPacket* pPacket);
	int SendBroadcast(netserver::CPacket* pPacket);
	int SendOneSector(CPlayer* pPlayer, netserver::CPacket* pPacket, CPlayer* except);
	int SendAroundSector(CPlayer* pPlayer, netserver::CPacket* pPacket, CPlayer* except);

	/* player */
	void MoveSector(CPlayer* pPlayer, WORD x, WORD y);
	void DisconnectPlayer(CPlayer* pPlayer);
	void DeletePlayer(__int64 sessionId);
	std::unordered_map<__int64, CPlayer*>::iterator DeletePlayer(std::unordered_map<__int64, CPlayer*>::iterator& iterPlayer);
	void ReplacePlayerByAccountNo(__int64 accountNo, CPlayer* replacePlayer);

	/* Thread */
	static unsigned WINAPI ThreadChatServer(PVOID pParam);
	static unsigned WINAPI ThreadMsgGenerator(PVOID pParam);
	static unsigned WINAPI ThreadMonitoringCollector(PVOID pParam);

	/* crash */
	void Crash();

	/* �α��� ������ ó�� APC �Լ� */
	static void CompleteUnfinishedLogin(ULONG_PTR pStAPCData);

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

