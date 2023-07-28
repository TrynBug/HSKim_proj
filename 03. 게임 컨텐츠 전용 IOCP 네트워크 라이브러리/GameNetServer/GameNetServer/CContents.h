#pragma once

#define CONTENTS_MODE_SEND_PACKET_IMMEDIATELY   0x1

namespace game_netserver
{

class CNetServer;
class CPacket;
class CTimeMgr;

class alignas(64) CContents
{
	friend class CNetServer;

private:
	/* ��Ʈ��ũ */
	CNetServer* _pNetServer;       // ��Ʈ��ũ ���� ������. ������ Ŭ���� ��ü�� ��Ʈ��ũ ���̺귯���� attach�� �� �����ȴ�.

	/* ������ */
	HANDLE _hThreadContents;                  // ������ ������ �ڵ�
	CLockfreeQueue<MsgContents*> _msgQ;       // ������ �޽���ť
	bool _bTerminated;                        // ������ ����� ����
	DWORD _mode;                              // ���

	/* ���� */
	std::unordered_map<__int64, CSession*> _mapSession;     // ���� ��
	int _sessionPacketProcessLimit;                         // �ϳ��� ���Ǵ� �� �����ӿ� ó���� �� �ִ� �ִ� ��Ŷ ��

	/* ��Ʈ��Ʈ */
	bool _bEnableHeartBeatTimeout;
	ULONGLONG _timeoutHeartBeat;
	int _timeoutHeartBeatCheckPeriod;
	ULONGLONG _lastHeartBeatCheckTime;

	/* ������ */
	int _FPS;                            // FPS
	__int64 _oneFrameTime;               // performance counter ������ 1�����Ӵ� count
	LARGE_INTEGER _logicStartTime;       // ���� ���� performance count
	LARGE_INTEGER _logicEndTime;         // ���� ���� performance count
	LARGE_INTEGER _performanceFrequency; // frequency
	__int64 _catchUpTime;                // FPS�� �����ϱ����� ������ƾ��� performance count. �� ����ŭ �� sleep �ؾ��Ѵ�.

	/* Time */
	CTimeMgr* _pTimeMgr;    // ������ ������ ���� �� �ʱ�ȭ�Ѵ�.

	/* ����͸� */
	volatile __int64 _updateCount = 0;
	volatile __int64 _internalMsgCount = 0;
	volatile __int64 _sessionMsgCount = 0;
	volatile __int64 _disconnByHeartBeat = 0;

public:
	CContents(int FPS);
	CContents(int FPS, DWORD mode);
	virtual ~CContents();

private:
	/* ������ */
	bool StartUp();     // ������ ����
	void Shutdown();    // ������ ����
	void InsertMessage(MsgContents* pMsg);      // ������ �޽���ť�� �޽��� ����
	void SyncLogicTime();                       // ���� ����ð��� FPS�� �����. ������� �������� ������. �ִ� FPS�� ������ ������ �����ȴ�.
	void SyncLogicTimeAndCatchUp();             // ���� ����ð��� FPS�� �����. ���� ������� �������� ���� ��� �Ѱ��Ҷ� ������´�. �ִ� FPS�� ������ ������ ������ �� �ִ�.


	/* ������ */
	static unsigned WINAPI ThreadContents(PVOID pParam);    // ������ ������ �Լ�

	/* ����� ���� callback �Լ� */
	virtual void OnUpdate() = 0;                                     // �ֱ������� ȣ���
	virtual void OnRecv(__int64 sessionId, CPacket* pPacket) = 0;    // �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
	virtual void OnSessionJoin(__int64 sessionId, PVOID data) = 0;   // �����忡 ������ ������ �� ȣ���. ���� ���� ������ ������ ���� ��� data�� null��
	virtual void OnSessionDisconnected(__int64 sessionId) = 0;       // �����尡 �����ϴ� ������ ������ �������� ȣ���

	/* ���� (private) */
	void _sendContentsPacket(CSession* pSession);               // ���� ���� ������ ��Ŷ ���� ���� ��Ŷ�� �����Ѵ�.
	void _sendContentsPacketAndDisconnect(CSession* pSession);  // ���� ���� ������ ��Ŷ ���� ���� ��Ŷ�� �����ϰ� ������ ���´�.
	void DisconnectSession(CSession* pSession);                // ������ ������ ���´�. 

public:
	/* ���� */
	const wchar_t* GetSessionIP(__int64 sessionId);
	unsigned short GetSessionPort(__int64 sessionId);
	void TransferSession(__int64 sessionId, CContents* destination, PVOID data);   // ������ destination �������� �ű��. ���ǿ� ���� �޽����� ���� �����忡���� ���̻� ó������ �ʴ´�.
	void DisconnectSession(__int64 sessionId);   // ������ ������ ���´�. 


	/* packet */
	CPacket* AllocPacket();
	bool SendPacket(__int64 sessionId, CPacket* pPacket);                 // ������ ������ ��Ŷ ���Ϳ� ��Ŷ�� �����Ѵ�.
	bool SendPacket(__int64 sessionId, CPacket** arrPtrPacket, int numPacket);  // ������ ������ ��Ŷ ���Ϳ� ��Ŷ�� �����Ѵ�.
	bool SendPacketAndDisconnect(__int64 sessionId, CPacket* pPacket);    // ������ ������ ��Ŷ ���Ϳ� ��Ŷ�� �����Ѵ�. ��Ŷ�� �������� ������ �����.


	/* Set */
	void SetSessionPacketProcessLimit(int limit) { _sessionPacketProcessLimit = limit; }   // �ϳ��� ���Ǵ� �� �����ӿ� ó���� �� �ִ� �ִ� ��Ŷ �� ����
	void EnableHeartBeatTimeout(int timeoutCriteria);   // ��Ʈ��Ʈ Ÿ�Ӿƿ� ���ؽð� ����
	void DisableHeartBeatTimeout();                     // ��Ʈ��Ʈ Ÿ�Ӿƿ� ��Ȱ��ȭ

	/* Time */
	double GetDT() const;
	float GetfDT() const;
	double GetAvgDT1s() const;
	double GetMinDT1st() const;
	double GetMinDT2nd() const;
	double GetMaxDT1st() const;
	double GetMaxDT2nd() const;
	int GetFPS() const;
	float GetAvgFPS1m() const;
	int GetMinFPS1st() const;
	int GetMinFPS2nd() const;
	int GetMaxFPS1st() const;
	int GetMaxFPS2nd() const;
	bool OneSecondTimer() const;

	/* Get ����͸� */
	__int64 GetUpdateCount() const { return _updateCount; }
	__int64 GetInternalMsgCount() const { return _internalMsgCount; }
	__int64 GetSessionMsgCount() const { return _sessionMsgCount; }
	__int64 GetDisconnByHeartBeat() const { return _disconnByHeartBeat; }

	/* dynamic alloc */
	// 64byte aligned ��ü ������ ���� new, delete overriding
	void* operator new(size_t size);
	void operator delete(void* p);

};


}