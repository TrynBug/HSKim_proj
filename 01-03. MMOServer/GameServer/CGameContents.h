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

		/* Get 모니터링 count */
		__int64 GetDisconnByInvalidMsgType() const { return _disconnByInvalidMessageType; }


		/* Get DB 모니터링 */
		virtual int GetQueryRequestPoolSize() const { return 0; }
		virtual int GetQueryRequestAllocCount() const { return 0; }
		virtual int GetQueryRequestFreeCount() const { return 0; }
		virtual __int64 GetQueryRunCount() const { return 0; }
		virtual float GetMaxQueryRunTime() const { return 0; }
		virtual float GetMinQueryRunTime() const { return 0; }
		virtual float GetAvgQueryRunTime() const { return 0; }
		virtual int GetRemainQueryCount() const { return 0; }


		/* session */
		void TransferSession(__int64 sessionId, EContentsThread destination, CPlayer_t pPlayer);   // player를 destination 컨텐츠로 옮긴다. 해당 세션에 대한 메시지는 현재 스레드에서는 더이상 처리되지 않는다.
		void DisconnectSession(__int64 sessionId) { CContents::DisconnectSession(sessionId); }

		/* 플레이어 */
		CPlayer_t GetPlayerBySessionId(__int64 sessionId);
		void InsertPlayerToMap(__int64 sessionId, CPlayer_t pPlayer);
		void ErasePlayerFromMap(__int64 sessionId);
		CPlayer_t AllocPlayer();

	private:
		/* 라이브러리 callback 함수 */
		virtual void OnUpdate() = 0;                                     // 주기적으로 호출됨
		virtual void OnRecv(__int64 sessionId, netlib_game::CPacket& packet) = 0;    // 컨텐츠가 관리하는 세션의 메시지큐에 메시지가 들어왔을 때 호출됨
		virtual void OnSessionDisconnected(__int64 sessionId) = 0;       // 스레드 내에서 세션의 연결이 끊겼을때 호출됨
		virtual void OnError(const wchar_t* szError, ...) = 0;           // 오류 발생
		virtual void OnSessionJoin(__int64 sessionId, PVOID data) final;  // 스레드에 세션이 들어왔을 때 호출됨(final)

		/* 게임컨텐츠 callback 함수 */
		virtual void OnInitialSessionJoin(__int64 sessionId) = 0;             // 현재 컨텐츠에 최초로 접속한 세션이 들어왔을 때 호출됨(세션에 해당하는 플레이어 객체가 없음)
		virtual void OnPlayerJoin(__int64 sessionId, CPlayer_t pPlayer) = 0;  // 현재 컨텐츠에 다른 컨텐츠에서 이동한 플레이어가 들어왔을 때 호출됨
		


	private:
		EContentsThread _eMyContentsType;                        // 내 컨텐츠 타입
		std::unique_ptr<CGameServerAccessor> _pServerAccessor;   // 게임 서버 포인터
		int _FPS;                           // FPS

	protected:
		std::unordered_map<__int64, CPlayer_t> _mapPlayer;  // 플레이어 맵

		/* 모니터링 */
		volatile __int64 _disconnByInvalidMessageType = 0;
	};

}