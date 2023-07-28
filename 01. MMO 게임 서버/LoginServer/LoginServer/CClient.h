#pragma once

class CClient
{
	friend class CLoginServer;

	__int64 _sessionId;
	INT64	_accountNo;
	WCHAR	_id[20];			// null ����
	WCHAR	_nickname[20];		// null ����
	char	_sessionKey[64];	// ������ū

	bool _bLogin;                  // �α��ο���
	bool _bDisconnect;             // ������� ����
	ULONGLONG _lastHeartBeatTime;  // ���������� heart beat ���� �ð�

	SRWLOCK _srwlClient;

public:
	CClient();
	~CClient();

public:
	void Init(__int64 sessionId);

	void SetClientInfo(INT64 accountNo, WCHAR id[20], WCHAR nickname[20], char sessionKey[64]);
	void SetLogin();
	void SetHeartBeatTime() { _lastHeartBeatTime = GetTickCount64(); }

	void LockExclusive() { AcquireSRWLockExclusive(&_srwlClient); }
	void UnlockExclusive() { ReleaseSRWLockExclusive(&_srwlClient); }
	void LockShared() { AcquireSRWLockShared(&_srwlClient); }
	void UnlockShared() { ReleaseSRWLockShared(&_srwlClient); }
};

