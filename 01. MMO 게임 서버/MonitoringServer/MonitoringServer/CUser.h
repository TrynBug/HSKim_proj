#pragma once


class CUser
{
	friend class CMonitoringServer;

	__int64 _sessionId;
	int	    _serverNo;      // ���� ��ȣ. �ܺ� Ŭ���̾�Ʈ�� ��� -1
	char    _loginKey[32];  // �α��� Ű. �ܺ� Ŭ���̾�Ʈ�� ����

	bool _bLogin;                  // �α��ο���
	bool _bDisconnect;             // ������� ����
	ULONGLONG _lastHeartBeatTime;  // ���������� heart beat ���� �ð�

public:
	CUser();
	~CUser();

public:
	void Init(__int64 sessionId);

	bool IsWANUser() { return _serverNo == -1; }

	void SetLANUserInfo(int serverNo);
	void SetWANUserInfo(char loginKey[32]);
	void SetLogin();
	void SetHeartBeatTime() { _lastHeartBeatTime = GetTickCount64(); }
};

