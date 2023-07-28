#pragma once

class CGameContents : public game_netserver::CContents
{
	friend class CGameServer;

private:
	EContentsThread _eMyContentsType;   // �� ������ Ÿ��
	CGameServer* _pGameServer;          // ���� ���� ������
	int _FPS;                           // FPS

protected:
	std::unordered_map<__int64, CPlayer*> _mapPlayer;  // �÷��̾� ��

	/* ����͸� */
	volatile __int64 _disconnByInvalidMessageType = 0;

public:
	CGameContents(EContentsThread myContentsType, CGameServer* pGameServer, int FPS);
	CGameContents(EContentsThread myContentsType, CGameServer* pGameServer, int FPS, DWORD sendMode);
	virtual ~CGameContents();

	/* Get */
	EContentsThread GetMyContentsType() const { return _eMyContentsType; }
	int GetNumPlayer() const { return (int)_mapPlayer.size(); }

	/* Get ����͸� */
	__int64 GetDisconnByInvalidMsgType() const { return _disconnByInvalidMessageType; }


	/* Get ����͸� (�����Լ�) */
	virtual __int64 GetQueryRunCount() const { return 0; }
	virtual float GetMaxQueryRunTime() const { return 0; }
	virtual float GetMinQueryRunTime() const { return 0; }
	virtual float GetAvgQueryRunTime() const { return 0; }
	virtual int GetRemainQueryCount() const { return 0; }


	/* config.json */
	CJsonParser* GetConfigJsonParser() { return _pGameServer->_configJsonParser; }

	/* redis */
	cpp_redis::client* GetRedisClient() { return _pGameServer->_pRedisClient; }



	/* session */
	void TransferSession(__int64 sessionId, EContentsThread destination, PVOID data);   // ������ destination �������� �ű��. ���ǿ� ���� �޽����� ���� �����忡���� ���̻� ó������ �ʴ´�.
	void DisconnectSession(__int64 sessionId) { CContents::DisconnectSession(sessionId); }

	/* �÷��̾� */
	CPlayer* AllocPlayer() { return _pGameServer->_poolPlayer.Alloc(); }
	void FreePlayer(CPlayer* pPlayer);

private:
	/* ������ Ŭ���� callback �Լ� */
	virtual void OnUpdate() = 0;                                     // �ֱ������� ȣ���
	virtual void OnRecv(__int64 sessionId, game_netserver::CPacket* pPacket) = 0;    // �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
	virtual void OnSessionJoin(__int64 sessionId, PVOID data) = 0;   // �����忡 ������ ������ �� ȣ���
	virtual void OnSessionDisconnected(__int64 sessionId) = 0;       // ������ ������ ������ ������ �������� ȣ���
};