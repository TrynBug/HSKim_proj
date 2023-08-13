#include "CNetServer.h"
#include "CTimeMgr.h"

using namespace netlib_game;

CContents::CContents(int FPS)
	: _pNetAccessor(nullptr)
	, _sessionPacketProcessLimit(1000000)
	, _bEnableHeartBeatTimeout(false)
	, _timeoutHeartBeat(0)
	, _timeoutHeartBeatCheckPeriod(0)
	, _lastHeartBeatCheckTime(0)
	, _FPS(FPS)
	, _spTimeMgr(nullptr)
{
	_calcFrame.oneFrameTime = _calcFrame.performanceFrequency.QuadPart / _FPS;
	_spTimeMgr = std::make_shared<CTimeMgr>();
}

CContents::CContents(int FPS, DWORD mode)
	: _pNetAccessor(nullptr)
	, _sessionPacketProcessLimit(1000000)
	, _bEnableHeartBeatTimeout(false)
	, _timeoutHeartBeat(0)
	, _timeoutHeartBeatCheckPeriod(0)
	, _lastHeartBeatCheckTime(0)
	, _FPS(FPS)
	, _spTimeMgr(nullptr)
{
	_calcFrame.oneFrameTime = _calcFrame.performanceFrequency.QuadPart / _FPS;
	_spTimeMgr = std::make_shared<CTimeMgr>();

	_thread.mode = mode;
}

CContents::~CContents()
{
}


CContents::CalcFrame::CalcFrame()
	: oneFrameTime(0)
	, logicStartTime{0,}
	, logicEndTime{0,}
	, performanceFrequency{0,}
	, catchUpTime(0) 
{
	QueryPerformanceFrequency(&performanceFrequency);
}

void CContents::CalcFrame::SetFrameStartTime()
{
	QueryPerformanceCounter(&logicStartTime);
}

/* 컨텐츠 */
// 스레드 시작
bool CContents::StartUp()
{
	_thread.handle = (HANDLE)_beginthreadex(NULL, 0, ThreadContents, this, 0, NULL);
	if (_thread.handle == NULL)
	{
		OnError(L"[Contents] An error occurred when starting the contents thread. error:%u\n", GetLastError());
		return false;
	}

	return true;
}

// 스레드 종료
void CContents::Shutdown()
{
	// shutdown 메시지 삽입
	MsgContents& msgShutdown = _pNetAccessor->AllocMsg();
	msgShutdown.type = EMsgContentsType::SHUTDOWN;
	msgShutdown.pSession = nullptr;
	msgShutdown.data = nullptr;
	InsertMessage(msgShutdown);

	// 스레드가 종료되면 _bTerminated = true로 설정됨
}

// 스레드 메시지큐에 메시지 삽입
void CContents::InsertMessage(MsgContents& msg)
{
	_thread.msgQ.Enqueue(&msg);
}

// 로직 실행시간을 FPS에 맞춘다. 수행못한 프레임은 버린다. 최대 FPS가 지정한 값으로 유지된다.
void CContents::SyncLogicTime()
{
	QueryPerformanceCounter(&_calcFrame.logicEndTime);
	__int64 spentTime = max(0, _calcFrame.logicEndTime.QuadPart - _calcFrame.logicStartTime.QuadPart);
	__int64 sleepTime = _calcFrame.oneFrameTime - spentTime;

	DWORD dwSleepTime = 0;
	// sleep 해야할 시간이 있는 경우
	if (sleepTime > 0)
	{
		// sleep 시간을 ms 단위로 변환하여 sleep 한다.
		dwSleepTime = (DWORD)round((double)(sleepTime * 1000) / (double)_calcFrame.performanceFrequency.QuadPart);
		Sleep(dwSleepTime);
	}

	// 다음 로직의 시작시간은 [현재 로직 종료시간 + sleep한 시간] 이다.
	_calcFrame.logicStartTime.QuadPart = _calcFrame.logicEndTime.QuadPart + (dwSleepTime * (_calcFrame.performanceFrequency.QuadPart / 1000));
}


// 로직 실행시간을 FPS에 맞춘다. 만약 수행못한 프레임이 있을 경우 한가할때 따라잡는다. 최대 FPS가 지정한 값보다 높아질 수 있다.
void CContents::SyncLogicTimeAndCatchUp()
{
	QueryPerformanceCounter(&_calcFrame.logicEndTime);
	__int64 spentTime = max(0, _calcFrame.logicEndTime.QuadPart - _calcFrame.logicStartTime.QuadPart);
	__int64 sleepTime = _calcFrame.oneFrameTime - spentTime;

	// sleep 해야할 시간이 있는 경우
	if (sleepTime > 0)
	{
		// 따라잡아야할 시간이 sleep 시간보다 크다면 따라잡아야할 시간에서 sleep 시간을 차감한다.
		if (_calcFrame.catchUpTime > sleepTime)
		{
			_calcFrame.catchUpTime -= sleepTime;
			sleepTime = 0;
		}
		// 따라잡아야할 시간이 sleep 시간보다 작다면 sleep 시간에서 따라잡아야할 시간을 차감한다.
		else
		{
			sleepTime -= _calcFrame.catchUpTime;
			_calcFrame.catchUpTime = 0;
		}
	}
	// 로직 수행시간이 1프레임당 소요시간보다 더 걸렸을 경우
	else
	{
		// 따라잡아야할 시간에 더 걸린 시간을 추가한다.
		_calcFrame.catchUpTime += -sleepTime;
		sleepTime = 0;
	}

	__int64 sleepTimeRemainder = 0;
	DWORD dwSleepTime = 0;
	// sleep 해야할 시간이 있는 경우
	if (sleepTime > 0)
	{
		// sleep 시간을 ms 단위로 절삭하고, 나머지는 따라잡아야할 시간에 추가한다.
		sleepTimeRemainder = sleepTime % (_calcFrame.performanceFrequency.QuadPart / 1000);
		sleepTime -= sleepTimeRemainder;
		_calcFrame.catchUpTime += sleepTimeRemainder;

		if (sleepTime > 0)
		{
			// sleep 시간을 ms 단위로 변환하여 sleep 한다.
			dwSleepTime = (DWORD)(sleepTime * 1000 / _calcFrame.performanceFrequency.QuadPart);
			Sleep(dwSleepTime);
		}
	}

	// 다음 로직의 시작시간은 [현재 로직 종료시간 + sleep한 시간] 이다.
	// 이 때 0.5초 만큼의 count를 로직 시작시간에 더해주는 보정을 해야 FPS가 정확해진다(이유는 모름;;).
	_calcFrame.logicStartTime.QuadPart = _calcFrame.logicEndTime.QuadPart + sleepTime + (_calcFrame.performanceFrequency.QuadPart / 1000 / 2);
}


/* 세션 (private) */
// 세션의 연결을 끊는다. 세션에 대한 메시지는 더이상 처리되지 않는다.
void CContents::DisconnectSession(CSession* pSession)
{
	_pNetAccessor->Disconnect(pSession);
}

// 세션 내의 컨텐츠 패킷 벡터 내의 패킷을 전송한다.
void CContents::_sendContentsPacket(CSession* pSession)
{	
	size_t numPacket = pSession->_vecContentsPacket.size();
	_pNetAccessor->SendPacketAsync(pSession->_sessionId, pSession->_vecContentsPacket);
	for (size_t i = 0; i < numPacket; i++)
	{
		pSession->_vecContentsPacket[i]->SubUseCount();
	}
	pSession->_vecContentsPacket.clear();
}


// 세션 내의 컨텐츠 패킷 벡터 내의 패킷을 전송하고 연결을 끊는다.
void CContents::_sendContentsPacketAndDisconnect(CSession* pSession)
{
	size_t numPacket = pSession->_vecContentsPacket.size();
	_pNetAccessor->SendPacketAsyncAndDisconnect(pSession->_sessionId, pSession->_vecContentsPacket);
	for (size_t i = 0; i < numPacket; i++)
	{
		pSession->_vecContentsPacket[i]->SubUseCount();
	}
	pSession->_vecContentsPacket.clear();
}




/* 세션 */
const wchar_t* CContents::GetSessionIP(__int64 sessionId) const
{
	auto iter = _mapSession.find(sessionId);
	if (iter == _mapSession.end())
		return nullptr;

	return (iter->second)->GetIP();
}

unsigned short CContents::GetSessionPort(__int64 sessionId) const
{
	auto iter = _mapSession.find(sessionId);
	if (iter == _mapSession.end())
		return 0;

	return (iter->second)->GetPort();
}



// 세션을 destination 컨텐츠로 옮긴다. 세션에 대한 메시지는 현재 스레드에서는 더이상 처리되지 않는다.
void CContents::TransferSession(__int64 sessionId, std::shared_ptr<CContents> destination, PVOID data)
{
	auto iter = _mapSession.find(sessionId);
	CSession* pSession = iter->second;
	pSession->_transfer.bOnTransfer = true;
	pSession->_transfer.spDestination = destination;
	pSession->_transfer.pData = data;
}


// 세션의 연결을 끊는다. 세션에 대한 메시지는 더이상 처리되지 않는다.
void CContents::DisconnectSession(__int64 sessionId)
{
	auto iter = _mapSession.find(sessionId);
	CSession* pSession = iter->second;
	_pNetAccessor->Disconnect(pSession);
}



/* packet */
CPacket& CContents::AllocPacket()
{ 
	return _pNetAccessor->AllocPacket(); 
}

bool CContents::SendPacket(__int64 sessionId, CPacket& packet)
{
	if (_thread.mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	{
		_pNetAccessor->SendPacketAsync(sessionId, packet);
	}
	else
	{
		auto iter = _mapSession.find(sessionId);
		if (iter == _mapSession.end())
			return false;

		CSession* pSession = iter->second;
		packet.AddUseCount();
		pSession->_vecContentsPacket.push_back(&packet);
	}

	return true;
}


bool CContents::SendPacket(__int64 sessionId, const std::vector<CPacket*> vecPacket)
{
	if (_thread.mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	{
		_pNetAccessor->SendPacketAsync(sessionId, vecPacket);
	}
	else
	{
		auto iter = _mapSession.find(sessionId);
		if (iter == _mapSession.end())
			return false;

		CSession* pSession = iter->second;
		for (int i = 0; i < vecPacket.size(); i++)
		{
			vecPacket[i]->AddUseCount();
			pSession->_vecContentsPacket.push_back(vecPacket[i]);
		}
	}

	return true;
}

bool CContents::SendPacketAndDisconnect(__int64 sessionId, CPacket& packet)
{
	if (_thread.mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	{
		_pNetAccessor->SendPacketAsyncAndDisconnect(sessionId, packet);
	}
	else
	{
		auto iter = _mapSession.find(sessionId);
		if (iter == _mapSession.end())
			return false;

		CSession* pSession = iter->second;
		packet.AddUseCount();
		pSession->_vecContentsPacket.push_back(&packet);
		pSession->_bContentsWaitToDisconnect = true;
	}
	

	return true;
}




/* 하트비트 enable */
void CContents::EnableHeartBeatTimeout(int timeoutCriteria)
{
	_bEnableHeartBeatTimeout = true;
	_timeoutHeartBeat = timeoutCriteria;
	_timeoutHeartBeatCheckPeriod = timeoutCriteria / 2;
	_lastHeartBeatCheckTime = GetTickCount64();
}

void CContents::DisableHeartBeatTimeout()
{
	_bEnableHeartBeatTimeout = false;
}


int CContents::GetFPS()
{
	return _spTimeMgr->GetFPS();
}



/* dynamic alloc */
// 64byte aligned 객체 생성을 위한 new, delete overriding
void* CContents::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}

void CContents::operator delete(void* p)
{
	_aligned_free(p);
}


/* 스레드 */
// 컨텐츠 스레드 함수
unsigned WINAPI CContents::ThreadContents(PVOID pParam)
{
	wprintf(L"[Contents] Run Contents Thread. id:%u\n", GetCurrentThreadId());

	CContents& contents = *reinterpret_cast<CContents*>(pParam);
	contents.RunContentsThread();

	wprintf(L"[Contents] End Contents Thread. id:%u\n", GetCurrentThreadId());

	return 0;
}


void CContents::RunContentsThread()
{
	CSession* pSession;
	__int64 sessionId;

	// time manager 초기화
	_spTimeMgr->Init();
	_lastHeartBeatCheckTime = GetTickCount64();

	_calcFrame.SetFrameStartTime();

	// 스레드 로직 시작
	while (true)
	{
		// 메시지큐 내의 모든 메시지 처리
		MsgContents* pMsg;
		while (_thread.msgQ.Dequeue(pMsg))
		{
			MsgContents& msg = *pMsg;
			switch (msg.type)
			{
				// 스레드에 최초 생성된 세션이 들어온 경우
			case EMsgContentsType::INITIAL_JOIN_SESSION:
			{
				pSession = msg.pSession;
				_mapSession.insert(std::make_pair(pSession->_sessionId, pSession));
				OnSessionJoin(pSession->_sessionId, nullptr);  // 이 함수내에서 플레이어를 생성하고 플레이어맵에 등록한다.
				break;
			}
			// 스레드에 세션이 들어온 경우
			// 세션을 map에 추가하고, OnSessionJoin 함수를 호출한다. 이 때 플레이어 객체 주소도 같이 전달한다.
			case EMsgContentsType::JOIN_SESSION:
			{
				pSession = msg.pSession;
				pSession->_transfer.bOnTransfer = false;
				_mapSession.insert(std::make_pair(pSession->_sessionId, pSession));
				OnSessionJoin(pSession->_sessionId, msg.data);  // 이 함수내에서 플레이어를 플레이어맵에 등록한다.
				break;
			}
			// 스레드 종료 메시지를 받음
			case EMsgContentsType::SHUTDOWN:
			{
				// 모든 세션의 연결을 끊는다.
				int disconnCount = 0;
				for (auto iter = _mapSession.begin(); iter != _mapSession.end(); ++iter)
				{
					pSession = iter->second;
					sessionId = pSession->_sessionId;
					DisconnectSession(pSession);
					_pNetAccessor->DecreaseIoCount(pSession);
					OnSessionDisconnected(sessionId);
					disconnCount++;
					if (disconnCount > 100)
					{
						Sleep(50);
						disconnCount = 0;
					}
				}
				_mapSession.clear();
				_pNetAccessor->FreeMsg(msg);

				// 메시지큐의 모든 메시지를 삭제한다.
				while (_thread.msgQ.Dequeue(pMsg))
				{
					_pNetAccessor->FreeMsg(*pMsg);
				}

				// 스레드 종료
				_thread.bTerminated = true;
				wprintf(L"[Contents] End Contents Thread. id:%u\n", GetCurrentThreadId());
				return;
				break;
			}
			// 잘못된 스레드 메시지를 받은 경우 오류메시지 출력
			default:
			{
				OnError(L"[Contents] Contents thread got a invalid internal message. type:%d, session:%p, sessionId:%lld, data:%p\n"
					, msg.type, msg.pSession, msg.pSession == nullptr ? 0 : msg.pSession->_sessionId, msg.data);
				break;
			}
			}

			// 메시지 free
			_pNetAccessor->FreeMsg(msg);
			_monitor.internalMsgCount++;
		}

		// 모든 세션내의 모든 메시지 처리
		bool isSessionClosed;
		bool isSessionOnTransfer;
		bool isSessionReceivedMsg;
		for (auto iter = _mapSession.begin(); iter != _mapSession.end();)
		{
			isSessionClosed = false;
			isSessionOnTransfer = false;
			isSessionReceivedMsg = false;
			do {
				pSession = iter->second;
				// 네트워크 라이브러리에서, 또는 이전 OnUpdate 함수내에서 세션을 끊음
				if (pSession->_isClosed == true)
				{
					isSessionClosed = true;
					break;
				}
				// 이전 OnUpdate 함수내에서 세션을 이동시켰음
				if (pSession->_transfer.bOnTransfer == true)
				{
					isSessionOnTransfer = true;
					break;
				}

				// 세션내의 모든 메시지 처리
				CPacket* pRecvPacket;
				while (pSession->_msgQ->Dequeue(pRecvPacket))
				{
					isSessionReceivedMsg = true;
					OnRecv(pSession->_sessionId, *pRecvPacket);
					pRecvPacket->SubUseCount();  // 패킷의 사용카운트를 감소시킨다.
					_monitor.sessionMsgCount++;
					// OnRecv 함수 내에서 세션을 이동시킴
					if (pSession->_transfer.bOnTransfer == true)
					{
						isSessionOnTransfer = true;
						break;
					}
					// OnRecv 함수 내에서 DisconnectSession 함수가 호출됨, 또는 네트워크 라이브러리에서 세션을 끊음
					if (pSession->_isClosed == true)
					{
						isSessionClosed = true;
						break;
					}
				}

				// 세션이 받은 메시지가 있을 경우 하트비트 설정
				if (isSessionReceivedMsg == true)
				{
					pSession->SetHeartBeatTime();
				}
			} while (0);

			// 세션 이동이 호출되었을 세션을 map에서 제거하고, 목적지 contents에 세션 join 메시지를 보낸다.
			// (사용자가 세션을 이동시켰다면 플레이어 객체를 map에서 제거했을 것이다. 하지만 플레이어 객체가 아직 free되지는 않았다.)
			// (플레이어 객체가 free되는것을 보장하기 위해 사용자가 세션 이동을 요청했다면 반드시 이동되어야 한다. 그래서 세션 연결 끊김보다 이동요청을 먼저 처리해야 한다.)
			// (만약 플레이어 객체를 map에서 제거했는데 플레이어 객체가 이동되지 않고 OnSessionDisconnected 함수가 호출된다면 플레이어 객체는 free되지 않을 것이다.)
			if (isSessionOnTransfer == true)
			{
				if (!(_thread.mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY))
				{
					// 패킷 즉시 전송 모드가 아닐 경우 세션 패킷버퍼내의 패킷을 모두 전송한다.
					_sendContentsPacket(pSession);
				}

				iter = _mapSession.erase(iter);

				MsgContents& msg = _pNetAccessor->AllocMsg();
				msg.type = EMsgContentsType::JOIN_SESSION;
				msg.pSession = pSession;
				msg.data = pSession->_transfer.pData;
				pSession->_transfer.spDestination->InsertMessage(msg);
			}
			// 세션 끊김이 감지되었을 경우 세션을 map에서 제거하고 IoCount를 감소시킨다. 그런다음 OnSessionDisconnected를 호출한다.
			else if (isSessionClosed == true)
			{
				iter = _mapSession.erase(iter);
				sessionId = pSession->_sessionId;
				_pNetAccessor->DecreaseIoCount(pSession);
				OnSessionDisconnected(sessionId);
			}
			else
			{
				++iter;
			}
		}

		// 하트비트 타임아웃 체크(_bEnableHeartBeatTimeout == true 일 경우)
		ULONGLONG currentTime = GetTickCount64();
		if (_bEnableHeartBeatTimeout == true
			&& currentTime > _lastHeartBeatCheckTime + _timeoutHeartBeatCheckPeriod)
		{
			_lastHeartBeatCheckTime = currentTime;
			for (auto iter = _mapSession.begin(); iter != _mapSession.end();)
			{
				pSession = iter->second;
				if (pSession->_lastHeartBeatTime + _timeoutHeartBeat < currentTime)
				{
					// 하트비트 타임아웃 대상일 경우 세션을 map에서 제거하고 연결을 끊는다.
					iter = _mapSession.erase(iter);
					sessionId = pSession->_sessionId;
					DisconnectSession(pSession);
					_pNetAccessor->DecreaseIoCount(pSession);
					OnSessionDisconnected(sessionId);
					_monitor.disconnByHeartBeat++;
				}
				else
				{
					++iter;
				}
			}
		}

		// OnUpdate
		_monitor.updateCount++;
		OnUpdate();

		// 패킷 즉시전송 모드가 아닐 경우
		if (!(_thread.mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY))
		{
			// 컨텐츠 코드에서 전송 요청한 패킷을 전송한다.
			for (auto iter = _mapSession.begin(); iter != _mapSession.end(); ++iter)
			{
				pSession = iter->second;

				// 만약 세션에대해 보내고끊기 함수가 호출되었다면 네트워크의 보내고끊기 함수를 호출한다.
				// 보낼 패킷이 없어도 연결이 끊겨야 하기 때문에 보내고끊기 함수를 반드시 호출한다.
				if (pSession->_bContentsWaitToDisconnect == true)
				{
					_sendContentsPacketAndDisconnect(pSession);
				}
				// 세션의 패킷을 모두 전송한다.
				else if (pSession->_vecContentsPacket.size() > 0)
				{
					_sendContentsPacket(pSession);
				}
			}
		}

		// 로직 실행시간을 FPS에 맞추기 위해 sleep한다.
		SyncLogicTime();

		// time manager update
		_spTimeMgr->Update();
	}
}





