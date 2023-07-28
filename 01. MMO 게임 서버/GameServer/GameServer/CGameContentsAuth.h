#pragma once

class CDBConnector;

class CGameContentsAuth : public CGameContents
{
private:
	int _sessionTransferLimit;

	/* DB */
	CDBConnector* _pDBConn;

public:
	CGameContentsAuth(EContentsThread myContentsType, CGameServer* pGameServer, int FPS);
	virtual ~CGameContentsAuth();

	/* Get (�����Լ�) */
	// @Override
	virtual __int64 GetQueryRunCount() const;
	virtual float GetMaxQueryRunTime() const;
	virtual float GetMinQueryRunTime() const;
	virtual float GetAvgQueryRunTime() const;

	/* Set */
	void SetSessionTransferLimit(int limit) { _sessionTransferLimit = limit; }

private:
	/* ������ Ŭ���� callback �Լ� ���� */
	virtual void OnUpdate();                                     // �ֱ������� ȣ���
	virtual void OnRecv(__int64 sessionId, game_netserver::CPacket* pPacket);    // �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
	virtual void OnSessionJoin(__int64 sessionId, PVOID data);   // �����忡 ������ ������ �� ȣ���
	virtual void OnSessionDisconnected(__int64 sessionId);       // ������ ������ ������ ������ �������� ȣ���

};

