#pragma once

#include "netserver/CNetServer.h"
#include "defineMonitoringServer.h"
#include "CDBConnector.h"
#include "CMemoryPool.h"


#include <sstream>

class CUser;
class CDBConnector;

class alignas(64) CMonitoringServer : public netserver::CNetServer
{
private:
	/* ��� */
	WCHAR _szCurrentPath[MAX_PATH];     // ���� ���丮 ���
	WCHAR _szConfigFilePath[MAX_PATH];  // config ���� ���

	/* config */
	wchar_t _szBindIP[20];
	unsigned short _portNumber;
	int _numMaxSession;
	int _numMaxUser;
	bool _bUseNagle;
	bool _bUsePacketEncryption;  // ��Ŷ ��ȣȭ ��� ����

	BYTE _packetCode;
	BYTE _packetKey;
	int _maxPacketSize;


	/* ���� */
	std::unordered_map<__int64, CUser*> _mapUser;
	std::unordered_map<__int64, CUser*> _mapWANUser;
	CMemoryPool<CUser> _poolUser;

	/* ���� */
	char _loginKey[32];          // login key    
	volatile bool _bShutdown;    // shutdown ����
	volatile bool _bTerminated;  // ����� ����
	int _timeoutLogin;           // �α��� Ÿ�Ӿƿ� ���ð�

	/* DB */
	CDBConnector* _pDBConn;
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
	MonitoringDataSet* _arrMonitoringDataSet;



	/* ����͸� */
	volatile __int64 _msgHandleCount = 0;              // �޽��� ó�� Ƚ��
	volatile __int64 _receivedInvalidDatatype = 0;     // �߸��� ����͸� ������ Ÿ���� ����
	volatile __int64 _disconnByUserLimit = 0;          // ������ �������� ����
	volatile __int64 _disconnByInvalidMessageType = 0; // �޽��� Ÿ���� �߸���
	volatile __int64 _disconnByLoginKeyMismatch = 0;   // �α��� Ű�� �߸���
	volatile __int64 _disconnByWANUserUpdateRequest = 0;  // WAN ������ ������ ������Ʈ ��û�� ����
	volatile __int64 _DBErrorCount = 0;                // DB ���� Ƚ��

public:
	CMonitoringServer();
	~CMonitoringServer();

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


	/* Get ����͸� */
	__int64 GetMsgHandleCount() { return _msgHandleCount; }
	__int64 GetReceivedInvalidDatatype() { return _receivedInvalidDatatype; }
	__int64 GetDisconnByUserLimit() { return _disconnByUserLimit; }
	__int64 GetDisconnByInvalidMessageType() { return _disconnByInvalidMessageType; }
	__int64 GetDisconnByLoginKeyMismatch() { return _disconnByLoginKeyMismatch; }
	__int64 GetDisconnByWANUserUpdateRequest() { return _disconnByWANUserUpdateRequest; }
	__int64 GetDBErrorCount() { return _DBErrorCount; }

	/* dynamic alloc */
	// 64byte aligned ��ü ������ ���� new, delete overriding
	void* operator new(size_t size);
	void operator delete(void* p);

private:
	/* ��Ʈ��ũ ���� */
	int SendUnicast(__int64 sessionId, netserver::CPacket* pPacket);

	/* ���� */
	CUser* AllocUser(__int64 sessionId);
	void FreeUser(CUser* pUser);
	CUser* GetUserBySessionId(__int64 sessionId);
	void DisconnectSession(__int64 sessionId);
	void DisconnectSession(CUser* pUser);

	/* Thread */
	static unsigned WINAPI ThreadJobGenerator(PVOID pParam);

	/* crash */
	void Crash();

private:
	// ��Ʈ��ũ ���̺귯�� �Լ� ����
	virtual bool OnRecv(__int64 sessionId, netserver::CPacket& packet);
	virtual bool OnInnerRequest(netserver::CPacket& packet);
	virtual bool OnConnectionRequest(unsigned long IP, unsigned short port);
	virtual bool OnClientJoin(__int64 sessionId);
	virtual bool OnClientLeave(__int64 sessionId);
	virtual void OnError(const wchar_t* szError, ...);
	virtual void OnOutputDebug(const wchar_t* szError, ...);
	virtual void OnOutputSystem(const wchar_t* szError, ...);
};

