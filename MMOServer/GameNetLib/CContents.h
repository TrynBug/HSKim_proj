#pragma once

constexpr int CONTENTS_MODE_SEND_PACKET_IMMEDIATELY = 0x1;

namespace netlib_game
{

	class CNetServer;
	class CNetServerAccessor;
	class CPacket;
	class CTimeMgr;

	class alignas(64) CContents
	{
	public:
		friend class CNetServer;
		struct Monitor;

	public:
		CContents(int FPS);
		CContents(int FPS, DWORD mode);
		virtual ~CContents();

		/* dynamic alloc */
		// 64byte aligned 객체 생성을 위한 new, delete overriding
		void* operator new(size_t size);
		void operator delete(void* p);

	private:
		/* 컨텐츠 */
		bool StartUp();     // 스레드 시작
		void Shutdown();    // 스레드 종료
		void InsertMessage(MsgContents& msg);      // 스레드 메시지큐에 메시지 삽입
		void SyncLogicTime();                       // 로직 실행시간을 FPS에 맞춘다. 수행못한 프레임은 버린다. 최대 FPS가 정해진 값으로 유지된다.
		void SyncLogicTimeAndCatchUp();             // 로직 실행시간을 FPS에 맞춘다. 만약 수행못한 프레임이 있을 경우 한가할때 따라잡는다. 최대 FPS가 정해진 값보다 높아질 수 있다.

		/* 사용자 구현 callback 함수 */
		virtual void OnUpdate() = 0;                                     // 주기적으로 호출됨
		virtual void OnRecv(__int64 sessionId, CPacket& packet) = 0;    // 컨텐츠가 관리하는 세션의 메시지큐에 메시지가 들어왔을 때 호출됨
		virtual void OnSessionJoin(__int64 sessionId, PVOID data) = 0;   // 스레드에 세션이 들어왔을 때 호출됨. 만약 최초 접속한 세션이 들어온 경우 data가 null임
		virtual void OnSessionDisconnected(__int64 sessionId) = 0;       // 스레드가 관리하는 세션의 연결이 끊겼을때 호출됨
		virtual void OnError(const wchar_t* szError, ...) = 0;           // 오류 발생

		/* 세션 (private) */
		void _sendContentsPacket(CSession* pSession);               // 세션 내의 컨텐츠 패킷 벡터 내의 패킷을 전송한다.
		void _sendContentsPacketAndDisconnect(CSession* pSession);  // 세션 내의 컨텐츠 패킷 벡터 내의 패킷을 전송하고 연결을 끊는다.
		void DisconnectSession(CSession* pSession);                // 세션의 연결을 끊는다. 

	public:
		/* 세션 */
		const wchar_t* GetSessionIP(__int64 sessionId) const;
		unsigned short GetSessionPort(__int64 sessionId) const;
		void TransferSession(__int64 sessionId, std::shared_ptr<CContents> destination, PVOID data);   // 세션을 destination 컨텐츠로 옮긴다. 세션에 대한 메시지는 현재 스레드에서는 더이상 처리되지 않는다.
		void DisconnectSession(__int64 sessionId);   // 세션의 연결을 끊는다. 

		/* packet */
		CPacket& AllocPacket();
		bool SendPacket(__int64 sessionId, CPacket& packet);                 // 세션의 컨텐츠 패킷 벡터에 패킷을 삽입한다.
		bool SendPacket(__int64 sessionId, const std::vector<CPacket*> vecPacket);  // 세션의 컨텐츠 패킷 벡터에 패킷을 삽입한다.
		bool SendPacketAndDisconnect(__int64 sessionId, CPacket& packet);    // 세션의 컨텐츠 패킷 벡터에 패킷을 삽입한다. 패킷을 보낸다음 연결이 끊긴다.


		/* Set */
		void SetSessionPacketProcessLimit(int limit) { _sessionPacketProcessLimit = limit; }   // 하나의 세션당 한 프레임에 처리할 수 있는 최대 패킷 수 설정
		void EnableHeartBeatTimeout(int timeoutCriteria);   // 하트비트 타임아웃 기준시간 설정
		void DisableHeartBeatTimeout();                     // 하트비트 타임아웃 비활성화

		/* Get */
		int GetFPS();
		const std::shared_ptr<CTimeMgr> GetTimeMgr() { return _spTimeMgr; }
		const Monitor GetMonitor() { return _monitor; }

	private:
		/* 스레드 */
		static unsigned WINAPI ThreadContents(PVOID pParam);    // 컨텐츠 스레드 함수
		void RunContentsThread();

	private:
		/* 네트워크 */
		std::unique_ptr<CNetServerAccessor> _pNetAccessor;   // 네트워크 서버 접근자 포인터. 컨텐츠 클래스 객체를 네트워크 라이브러리에 attach할 때 설정된다.

		/* 컨텐츠 스레드 */
		struct Thread
		{
			HANDLE handle; 
			CLockfreeQueue<MsgContents*> msgQ;       // 메시지큐
			bool bTerminated;                        // 종료됨 여부
			DWORD mode;                              // 모드
			Thread() : handle(0), bTerminated(false), mode(0) {}
		};
		Thread _thread;

		/* 세션 */
		std::unordered_map<__int64, CSession*> _mapSession;     // 세션 맵
		int _sessionPacketProcessLimit;                         // 하나의 세션당 한 프레임에 처리할 수 있는 최대 패킷 수

		/* 하트비트 */
		bool _bEnableHeartBeatTimeout;
		ULONGLONG _timeoutHeartBeat;
		int _timeoutHeartBeatCheckPeriod;
		ULONGLONG _lastHeartBeatCheckTime;

		/* 프레임 */
		int _FPS;                  // FPS
		struct CalcFrame           // 프레임 시간 계산을 위한 값들
		{
			__int64 oneFrameTime;               // performance counter 기준의 1프레임당 count
			LARGE_INTEGER logicStartTime;       // 로직 시작 performance count
			LARGE_INTEGER logicEndTime;         // 로직 종료 performance count
			LARGE_INTEGER performanceFrequency; // frequency
			__int64 catchUpTime;                // FPS를 유지하기위해 따라잡아야할 performance count. 이 값만큼 덜 sleep 해야한다.
			CalcFrame();
			void SetFrameStartTime();
		};
		CalcFrame _calcFrame;


		/* Time */
		std::shared_ptr<CTimeMgr> _spTimeMgr;    // 타이머 매니저. 컨텐츠 스레드 시작 시 초기화한다.

		/* 모니터링 */
	public:
		struct Monitor
		{
			friend class CContents;
		private:
			volatile __int64 updateCount = 0;
			volatile __int64 internalMsgCount = 0;
			volatile __int64 sessionMsgCount = 0;
			volatile __int64 disconnByHeartBeat = 0;

		public:
			__int64 GetUpdateCount() const { return updateCount; }
			__int64 GetInternalMsgCount() const { return internalMsgCount; }
			__int64 GetSessionMsgCount() const { return sessionMsgCount; }
			__int64 GetDisconnByHeartBeat() const { return disconnByHeartBeat; }
		};
		Monitor _monitor;

	};


}