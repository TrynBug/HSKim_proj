#pragma once

class CPlayer
{
	friend class CChatServer;

	__int64 _sessionId;
	INT64	_accountNo;
	WCHAR	_id[20];			// null ����
	WCHAR	_nickname[20];		// null ����
	char	_sessionKey[64];	// ������ū

	bool _bLogin;                  // �α��ο���
	bool _bDisconnect;             // ������� ����
	ULONGLONG _lastHeartBeatTime;  // ���������� heart beat ���� �ð�

	WORD _sectorX;
	WORD _sectorY;

public:
	CPlayer();
	~CPlayer();

public:
	void Init(__int64 sessionId);

	void SetPlayerInfo(INT64 accountNo, WCHAR id[20], WCHAR nickname[20], char sessionKey[64]);
	void SetLogin();
	void SetHeartBeatTime() { _lastHeartBeatTime = GetTickCount64(); }
};

