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

		/* Get DB 모니터링 */
		virtual __int64 GetQueryRunCount() const override;
		virtual float GetMaxQueryRunTime() const override;
		virtual float GetMinQueryRunTime() const override;
		virtual float GetAvgQueryRunTime() const override;

		/* Set */
		void SetSessionTransferLimit(int limit) { _sessionTransferLimit = limit; }

	private:
		/* 컨텐츠 클래스 callback 함수 구현 */
		virtual void OnUpdate() override;                                     // 주기적으로 호출됨
		virtual void OnRecv(__int64 sessionId, CPacket& packet) override;     // 컨텐츠가 관리하는 세션의 메시지큐에 메시지가 들어왔을 때 호출됨
		virtual void OnSessionDisconnected(__int64 sessionId) override;       // 스레드 내에서 세션의 연결이 끊겼을때 호출됨
		virtual void OnError(const wchar_t* szError, ...) override;           // 오류 발생
		virtual void OnInitialSessionJoin(__int64 sessionId) override;        // 스레드에 최초접속 세션이 들어왔을 때 호출됨
		virtual void OnPlayerJoin(__int64 sessionId, CPlayer_t pPlayer) override;  // 스레드에 플레이어과 들어왔을 때 호출됨(다른 컨텐츠로부터 이동해옴)


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