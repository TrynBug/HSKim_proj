#pragma once

class CDBConnector;

class CGameContentsAuth : public CGameContents
{
	using CPacket = netlib_game::CPacket;

public:
	CGameContentsAuth(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS);
	virtual ~CGameContentsAuth();

	/* Get DB ����͸� */
	virtual __int64 GetQueryRunCount() const override;
	virtual float GetMaxQueryRunTime() const override;
	virtual float GetMinQueryRunTime() const override;
	virtual float GetAvgQueryRunTime() const override;

	/* Set */
	void SetSessionTransferLimit(int limit) { _sessionTransferLimit = limit; }

private:
	/* ������ Ŭ���� callback �Լ� ���� */
	virtual void OnUpdate();                                     // �ֱ������� ȣ���
	virtual void OnRecv(__int64 sessionId, CPacket& packet);    // �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
	virtual void OnSessionJoin(__int64 sessionId, PVOID data);   // �����忡 ������ ������ �� ȣ���
	virtual void OnSessionDisconnected(__int64 sessionId);       // ������ ������ ������ ������ �������� ȣ���
	virtual void OnError(const wchar_t* szError, ...);           // ���� �߻�

private:
	int _sessionTransferLimit;

	/* DB */
	std::unique_ptr<CDBConnector> _pDBConn;
};

