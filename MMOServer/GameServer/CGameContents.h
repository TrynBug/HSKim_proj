#pragma once

namespace gameserver
{

	class CGameContents : public netlib_game::CContents
	{
		friend class CGameServer;

	public:
		CGameContents(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS);
		CGameContents(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS, DWORD sendMode);
		virtual ~CGameContents();

		/* Get */
		EContentsThread GetMyContentsType() const { return _eMyContentsType; }
		int GetNumPlayer() const { return (int)_mapPlayer.size(); }
		const CGameServer::Config& GetConfig() { return _pServerAccessor->GetConfig(); }
		cpp_redis::client& GetRedisClient() { return _pServerAccessor->GetRedisClient(); }

		/* Get ����͸� count */
		__int64 GetDisconnByInvalidMsgType() const { return _disconnByInvalidMessageType; }


		/* Get DB ����͸� */
		virtual int GetQueryRequestPoolSize() const { return 0; }
		virtual int GetQueryRequestAllocCount() const { return 0; }
		virtual int GetQueryRequestFreeCount() const { return 0; }
		virtual __int64 GetQueryRunCount() const { return 0; }
		virtual float GetMaxQueryRunTime() const { return 0; }
		virtual float GetMinQueryRunTime() const { return 0; }
		virtual float GetAvgQueryRunTime() const { return 0; }
		virtual int GetRemainQueryCount() const { return 0; }


		/* session */
		void TransferSession(__int64 sessionId, EContentsThread destination, CPlayer_t pPlayer);   // player�� destination �������� �ű��. �ش� ���ǿ� ���� �޽����� ���� �����忡���� ���̻� ó������ �ʴ´�.
		void DisconnectSession(__int64 sessionId) { CContents::DisconnectSession(sessionId); }

		/* �÷��̾� */
		CPlayer_t GetPlayerBySessionId(__int64 sessionId);
		void InsertPlayerToMap(__int64 sessionId, CPlayer_t pPlayer);
		void ErasePlayerFromMap(__int64 sessionId);
		CPlayer_t AllocPlayer();

	private:
		/* ���̺귯�� callback �Լ� */
		virtual void OnUpdate() = 0;                                     // �ֱ������� ȣ���
		virtual void OnRecv(__int64 sessionId, netlib_game::CPacket& packet) = 0;    // �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
		virtual void OnSessionDisconnected(__int64 sessionId) = 0;       // ������ ������ ������ ������ �������� ȣ���
		virtual void OnError(const wchar_t* szError, ...) = 0;           // ���� �߻�
		virtual void OnSessionJoin(__int64 sessionId, PVOID data) final;  // �����忡 ������ ������ �� ȣ���(final)

		/* ���������� callback �Լ� */
		virtual void OnInitialSessionJoin(__int64 sessionId) = 0;             // ���� �������� ���ʷ� ������ ������ ������ �� ȣ���(���ǿ� �ش��ϴ� �÷��̾� ��ü�� ����)
		virtual void OnPlayerJoin(__int64 sessionId, CPlayer_t pPlayer) = 0;  // ���� �������� �ٸ� ���������� �̵��� �÷��̾ ������ �� ȣ���
		


	private:
		EContentsThread _eMyContentsType;                        // �� ������ Ÿ��
		std::unique_ptr<CGameServerAccessor> _pServerAccessor;   // ���� ���� ������
		int _FPS;                           // FPS

	protected:
		std::unordered_map<__int64, CPlayer_t> _mapPlayer;  // �÷��̾� ��

		/* ����͸� */
		volatile __int64 _disconnByInvalidMessageType = 0;
	};

}