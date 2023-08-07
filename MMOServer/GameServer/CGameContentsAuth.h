#pragma once

class CDBConnector;

class CGameContentsAuth : public CGameContents
{
	using CPacket = netlib_game::CPacket;

public:
	CGameContentsAuth(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS);
	virtual ~CGameContentsAuth();

	/* Get DB 모니터링 */
	virtual __int64 GetQueryRunCount() const override;
	virtual float GetMaxQueryRunTime() const override;
	virtual float GetMinQueryRunTime() const override;
	virtual float GetAvgQueryRunTime() const override;

	/* Set */
	void SetSessionTransferLimit(int limit) { _sessionTransferLimit = limit; }

private:
	/* 컨텐츠 클래스 callback 함수 구현 */
	virtual void OnUpdate();                                     // 주기적으로 호출됨
	virtual void OnRecv(__int64 sessionId, CPacket& packet);    // 컨텐츠가 관리하는 세션의 메시지큐에 메시지가 들어왔을 때 호출됨
	virtual void OnSessionJoin(__int64 sessionId, PVOID data);   // 스레드에 세션이 들어왔을 때 호출됨
	virtual void OnSessionDisconnected(__int64 sessionId);       // 스레드 내에서 세션의 연결이 끊겼을때 호출됨
	virtual void OnError(const wchar_t* szError, ...);           // 오류 발생

private:
	int _sessionTransferLimit;

	/* DB */
	std::unique_ptr<CDBConnector> _pDBConn;
};

