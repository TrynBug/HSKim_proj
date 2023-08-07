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
		// 64byte aligned ��ü ������ ���� new, delete overriding
		void* operator new(size_t size);
		void operator delete(void* p);

	private:
		/* ������ */
		bool StartUp();     // ������ ����
		void Shutdown();    // ������ ����
		void InsertMessage(MsgContents& msg);      // ������ �޽���ť�� �޽��� ����
		void SyncLogicTime();                       // ���� ����ð��� FPS�� �����. ������� �������� ������. �ִ� FPS�� ������ ������ �����ȴ�.
		void SyncLogicTimeAndCatchUp();             // ���� ����ð��� FPS�� �����. ���� ������� �������� ���� ��� �Ѱ��Ҷ� ������´�. �ִ� FPS�� ������ ������ ������ �� �ִ�.

		/* ����� ���� callback �Լ� */
		virtual void OnUpdate() = 0;                                     // �ֱ������� ȣ���
		virtual void OnRecv(__int64 sessionId, CPacket& packet) = 0;    // �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
		virtual void OnSessionJoin(__int64 sessionId, PVOID data) = 0;   // �����忡 ������ ������ �� ȣ���. ���� ���� ������ ������ ���� ��� data�� null��
		virtual void OnSessionDisconnected(__int64 sessionId) = 0;       // �����尡 �����ϴ� ������ ������ �������� ȣ���
		virtual void OnError(const wchar_t* szError, ...) = 0;           // ���� �߻�

		/* ���� (private) */
		void _sendContentsPacket(CSession* pSession);               // ���� ���� ������ ��Ŷ ���� ���� ��Ŷ�� �����Ѵ�.
		void _sendContentsPacketAndDisconnect(CSession* pSession);  // ���� ���� ������ ��Ŷ ���� ���� ��Ŷ�� �����ϰ� ������ ���´�.
		void DisconnectSession(CSession* pSession);                // ������ ������ ���´�. 

	public:
		/* ���� */
		const wchar_t* GetSessionIP(__int64 sessionId) const;
		unsigned short GetSessionPort(__int64 sessionId) const;
		void TransferSession(__int64 sessionId, std::shared_ptr<CContents> destination, PVOID data);   // ������ destination �������� �ű��. ���ǿ� ���� �޽����� ���� �����忡���� ���̻� ó������ �ʴ´�.
		void DisconnectSession(__int64 sessionId);   // ������ ������ ���´�. 

		/* packet */
		CPacket& AllocPacket();
		bool SendPacket(__int64 sessionId, CPacket& packet);                 // ������ ������ ��Ŷ ���Ϳ� ��Ŷ�� �����Ѵ�.
		bool SendPacket(__int64 sessionId, const std::vector<CPacket*> vecPacket);  // ������ ������ ��Ŷ ���Ϳ� ��Ŷ�� �����Ѵ�.
		bool SendPacketAndDisconnect(__int64 sessionId, CPacket& packet);    // ������ ������ ��Ŷ ���Ϳ� ��Ŷ�� �����Ѵ�. ��Ŷ�� �������� ������ �����.


		/* Set */
		void SetSessionPacketProcessLimit(int limit) { _sessionPacketProcessLimit = limit; }   // �ϳ��� ���Ǵ� �� �����ӿ� ó���� �� �ִ� �ִ� ��Ŷ �� ����
		void EnableHeartBeatTimeout(int timeoutCriteria);   // ��Ʈ��Ʈ Ÿ�Ӿƿ� ���ؽð� ����
		void DisableHeartBeatTimeout();                     // ��Ʈ��Ʈ Ÿ�Ӿƿ� ��Ȱ��ȭ

		/* Get */
		int GetFPS();
		const std::shared_ptr<CTimeMgr> GetTimeMgr() { return _spTimeMgr; }
		const Monitor GetMonitor() { return _monitor; }

	private:
		/* ������ */
		static unsigned WINAPI ThreadContents(PVOID pParam);    // ������ ������ �Լ�
		void RunContentsThread();

	private:
		/* ��Ʈ��ũ */
		std::unique_ptr<CNetServerAccessor> _pNetAccessor;   // ��Ʈ��ũ ���� ������ ������. ������ Ŭ���� ��ü�� ��Ʈ��ũ ���̺귯���� attach�� �� �����ȴ�.

		/* ������ ������ */
		struct Thread
		{
			HANDLE handle; 
			CLockfreeQueue<MsgContents*> msgQ;       // �޽���ť
			bool bTerminated;                        // ����� ����
			DWORD mode;                              // ���
			Thread() : handle(0), bTerminated(false), mode(0) {}
		};
		Thread _thread;

		/* ���� */
		std::unordered_map<__int64, CSession*> _mapSession;     // ���� ��
		int _sessionPacketProcessLimit;                         // �ϳ��� ���Ǵ� �� �����ӿ� ó���� �� �ִ� �ִ� ��Ŷ ��

		/* ��Ʈ��Ʈ */
		bool _bEnableHeartBeatTimeout;
		ULONGLONG _timeoutHeartBeat;
		int _timeoutHeartBeatCheckPeriod;
		ULONGLONG _lastHeartBeatCheckTime;

		/* ������ */
		int _FPS;                  // FPS
		struct CalcFrame           // ������ �ð� ����� ���� ����
		{
			__int64 oneFrameTime;               // performance counter ������ 1�����Ӵ� count
			LARGE_INTEGER logicStartTime;       // ���� ���� performance count
			LARGE_INTEGER logicEndTime;         // ���� ���� performance count
			LARGE_INTEGER performanceFrequency; // frequency
			__int64 catchUpTime;                // FPS�� �����ϱ����� ������ƾ��� performance count. �� ����ŭ �� sleep �ؾ��Ѵ�.
			CalcFrame();
			void SetFrameStartTime();
		};
		CalcFrame _calcFrame;


		/* Time */
		std::shared_ptr<CTimeMgr> _spTimeMgr;    // Ÿ�̸� �Ŵ���. ������ ������ ���� �� �ʱ�ȭ�Ѵ�.

		/* ����͸� */
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