#pragma once

class CDBConnector;

namespace gameserver
{

	class CGameContentsAuth : public CGameContents
	{
	public:
		CGameContentsAuth(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS);
		virtual ~CGameContentsAuth();

	public:
		void Init();

		/* Get DB ����͸� */
		virtual __int64 GetQueryRunCount() const override;
		virtual float GetMaxQueryRunTime() const override;
		virtual float GetMinQueryRunTime() const override;
		virtual float GetAvgQueryRunTime() const override;

		/* Set */
		void SetSessionTransferLimit(int limit) { _sessionTransferLimit = limit; }

	private:
		/* ������ Ŭ���� callback �Լ� ���� */
		virtual void OnUpdate() override;                                     // �ֱ������� ȣ���
		virtual void OnRecv(__int64 sessionId, CPacket& packet) override;     // �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
		virtual void OnSessionDisconnected(__int64 sessionId) override;       // ������ ������ ������ ������ �������� ȣ���
		virtual void OnError(const wchar_t* szError, ...) override;           // ���� �߻�
		virtual void OnInitialSessionJoin(__int64 sessionId) override;        // �����忡 �������� ������ ������ �� ȣ���
		virtual void OnPlayerJoin(__int64 sessionId, CPlayer_t pPlayer) override;  // �����忡 �÷��̾�� ������ �� ȣ���(�ٸ� �������κ��� �̵��ؿ�)


	private:
		/* packet processing */
		void PacketProc_LoginRequest(__int64 sessionId, CPacket& packet);
		void PacketProc_CharacterSelect(__int64 sessionId, CPacket& packet);
		void PacketProc_HeartBeat(__int64 sessionId, CPacket& packet);
		void PacketProc_Default(__int64 sessionId, CPacket& packet, WORD msgType);

	private:
		int _sessionTransferLimit;

		/* DB */
		std::unique_ptr<CDBConnector> _pDBConn;
	};

}