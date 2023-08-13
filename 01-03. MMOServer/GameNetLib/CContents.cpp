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

/* ������ */
// ������ ����
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

// ������ ����
void CContents::Shutdown()
{
	// shutdown �޽��� ����
	MsgContents& msgShutdown = _pNetAccessor->AllocMsg();
	msgShutdown.type = EMsgContentsType::SHUTDOWN;
	msgShutdown.pSession = nullptr;
	msgShutdown.data = nullptr;
	InsertMessage(msgShutdown);

	// �����尡 ����Ǹ� _bTerminated = true�� ������
}

// ������ �޽���ť�� �޽��� ����
void CContents::InsertMessage(MsgContents& msg)
{
	_thread.msgQ.Enqueue(&msg);
}

// ���� ����ð��� FPS�� �����. ������� �������� ������. �ִ� FPS�� ������ ������ �����ȴ�.
void CContents::SyncLogicTime()
{
	QueryPerformanceCounter(&_calcFrame.logicEndTime);
	__int64 spentTime = max(0, _calcFrame.logicEndTime.QuadPart - _calcFrame.logicStartTime.QuadPart);
	__int64 sleepTime = _calcFrame.oneFrameTime - spentTime;

	DWORD dwSleepTime = 0;
	// sleep �ؾ��� �ð��� �ִ� ���
	if (sleepTime > 0)
	{
		// sleep �ð��� ms ������ ��ȯ�Ͽ� sleep �Ѵ�.
		dwSleepTime = (DWORD)round((double)(sleepTime * 1000) / (double)_calcFrame.performanceFrequency.QuadPart);
		Sleep(dwSleepTime);
	}

	// ���� ������ ���۽ð��� [���� ���� ����ð� + sleep�� �ð�] �̴�.
	_calcFrame.logicStartTime.QuadPart = _calcFrame.logicEndTime.QuadPart + (dwSleepTime * (_calcFrame.performanceFrequency.QuadPart / 1000));
}


// ���� ����ð��� FPS�� �����. ���� ������� �������� ���� ��� �Ѱ��Ҷ� ������´�. �ִ� FPS�� ������ ������ ������ �� �ִ�.
void CContents::SyncLogicTimeAndCatchUp()
{
	QueryPerformanceCounter(&_calcFrame.logicEndTime);
	__int64 spentTime = max(0, _calcFrame.logicEndTime.QuadPart - _calcFrame.logicStartTime.QuadPart);
	__int64 sleepTime = _calcFrame.oneFrameTime - spentTime;

	// sleep �ؾ��� �ð��� �ִ� ���
	if (sleepTime > 0)
	{
		// ������ƾ��� �ð��� sleep �ð����� ũ�ٸ� ������ƾ��� �ð����� sleep �ð��� �����Ѵ�.
		if (_calcFrame.catchUpTime > sleepTime)
		{
			_calcFrame.catchUpTime -= sleepTime;
			sleepTime = 0;
		}
		// ������ƾ��� �ð��� sleep �ð����� �۴ٸ� sleep �ð����� ������ƾ��� �ð��� �����Ѵ�.
		else
		{
			sleepTime -= _calcFrame.catchUpTime;
			_calcFrame.catchUpTime = 0;
		}
	}
	// ���� ����ð��� 1�����Ӵ� �ҿ�ð����� �� �ɷ��� ���
	else
	{
		// ������ƾ��� �ð��� �� �ɸ� �ð��� �߰��Ѵ�.
		_calcFrame.catchUpTime += -sleepTime;
		sleepTime = 0;
	}

	__int64 sleepTimeRemainder = 0;
	DWORD dwSleepTime = 0;
	// sleep �ؾ��� �ð��� �ִ� ���
	if (sleepTime > 0)
	{
		// sleep �ð��� ms ������ �����ϰ�, �������� ������ƾ��� �ð��� �߰��Ѵ�.
		sleepTimeRemainder = sleepTime % (_calcFrame.performanceFrequency.QuadPart / 1000);
		sleepTime -= sleepTimeRemainder;
		_calcFrame.catchUpTime += sleepTimeRemainder;

		if (sleepTime > 0)
		{
			// sleep �ð��� ms ������ ��ȯ�Ͽ� sleep �Ѵ�.
			dwSleepTime = (DWORD)(sleepTime * 1000 / _calcFrame.performanceFrequency.QuadPart);
			Sleep(dwSleepTime);
		}
	}

	// ���� ������ ���۽ð��� [���� ���� ����ð� + sleep�� �ð�] �̴�.
	// �� �� 0.5�� ��ŭ�� count�� ���� ���۽ð��� �����ִ� ������ �ؾ� FPS�� ��Ȯ������(������ ��;;).
	_calcFrame.logicStartTime.QuadPart = _calcFrame.logicEndTime.QuadPart + sleepTime + (_calcFrame.performanceFrequency.QuadPart / 1000 / 2);
}


/* ���� (private) */
// ������ ������ ���´�. ���ǿ� ���� �޽����� ���̻� ó������ �ʴ´�.
void CContents::DisconnectSession(CSession* pSession)
{
	_pNetAccessor->Disconnect(pSession);
}

// ���� ���� ������ ��Ŷ ���� ���� ��Ŷ�� �����Ѵ�.
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


// ���� ���� ������ ��Ŷ ���� ���� ��Ŷ�� �����ϰ� ������ ���´�.
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




/* ���� */
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



// ������ destination �������� �ű��. ���ǿ� ���� �޽����� ���� �����忡���� ���̻� ó������ �ʴ´�.
void CContents::TransferSession(__int64 sessionId, std::shared_ptr<CContents> destination, PVOID data)
{
	auto iter = _mapSession.find(sessionId);
	CSession* pSession = iter->second;
	pSession->_transfer.bOnTransfer = true;
	pSession->_transfer.spDestination = destination;
	pSession->_transfer.pData = data;
}


// ������ ������ ���´�. ���ǿ� ���� �޽����� ���̻� ó������ �ʴ´�.
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




/* ��Ʈ��Ʈ enable */
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
// 64byte aligned ��ü ������ ���� new, delete overriding
void* CContents::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}

void CContents::operator delete(void* p)
{
	_aligned_free(p);
}


/* ������ */
// ������ ������ �Լ�
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

	// time manager �ʱ�ȭ
	_spTimeMgr->Init();
	_lastHeartBeatCheckTime = GetTickCount64();

	_calcFrame.SetFrameStartTime();

	// ������ ���� ����
	while (true)
	{
		// �޽���ť ���� ��� �޽��� ó��
		MsgContents* pMsg;
		while (_thread.msgQ.Dequeue(pMsg))
		{
			MsgContents& msg = *pMsg;
			switch (msg.type)
			{
				// �����忡 ���� ������ ������ ���� ���
			case EMsgContentsType::INITIAL_JOIN_SESSION:
			{
				pSession = msg.pSession;
				_mapSession.insert(std::make_pair(pSession->_sessionId, pSession));
				OnSessionJoin(pSession->_sessionId, nullptr);  // �� �Լ������� �÷��̾ �����ϰ� �÷��̾�ʿ� ����Ѵ�.
				break;
			}
			// �����忡 ������ ���� ���
			// ������ map�� �߰��ϰ�, OnSessionJoin �Լ��� ȣ���Ѵ�. �� �� �÷��̾� ��ü �ּҵ� ���� �����Ѵ�.
			case EMsgContentsType::JOIN_SESSION:
			{
				pSession = msg.pSession;
				pSession->_transfer.bOnTransfer = false;
				_mapSession.insert(std::make_pair(pSession->_sessionId, pSession));
				OnSessionJoin(pSession->_sessionId, msg.data);  // �� �Լ������� �÷��̾ �÷��̾�ʿ� ����Ѵ�.
				break;
			}
			// ������ ���� �޽����� ����
			case EMsgContentsType::SHUTDOWN:
			{
				// ��� ������ ������ ���´�.
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

				// �޽���ť�� ��� �޽����� �����Ѵ�.
				while (_thread.msgQ.Dequeue(pMsg))
				{
					_pNetAccessor->FreeMsg(*pMsg);
				}

				// ������ ����
				_thread.bTerminated = true;
				wprintf(L"[Contents] End Contents Thread. id:%u\n", GetCurrentThreadId());
				return;
				break;
			}
			// �߸��� ������ �޽����� ���� ��� �����޽��� ���
			default:
			{
				OnError(L"[Contents] Contents thread got a invalid internal message. type:%d, session:%p, sessionId:%lld, data:%p\n"
					, msg.type, msg.pSession, msg.pSession == nullptr ? 0 : msg.pSession->_sessionId, msg.data);
				break;
			}
			}

			// �޽��� free
			_pNetAccessor->FreeMsg(msg);
			_monitor.internalMsgCount++;
		}

		// ��� ���ǳ��� ��� �޽��� ó��
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
				// ��Ʈ��ũ ���̺귯������, �Ǵ� ���� OnUpdate �Լ������� ������ ����
				if (pSession->_isClosed == true)
				{
					isSessionClosed = true;
					break;
				}
				// ���� OnUpdate �Լ������� ������ �̵�������
				if (pSession->_transfer.bOnTransfer == true)
				{
					isSessionOnTransfer = true;
					break;
				}

				// ���ǳ��� ��� �޽��� ó��
				CPacket* pRecvPacket;
				while (pSession->_msgQ->Dequeue(pRecvPacket))
				{
					isSessionReceivedMsg = true;
					OnRecv(pSession->_sessionId, *pRecvPacket);
					pRecvPacket->SubUseCount();  // ��Ŷ�� ���ī��Ʈ�� ���ҽ�Ų��.
					_monitor.sessionMsgCount++;
					// OnRecv �Լ� ������ ������ �̵���Ŵ
					if (pSession->_transfer.bOnTransfer == true)
					{
						isSessionOnTransfer = true;
						break;
					}
					// OnRecv �Լ� ������ DisconnectSession �Լ��� ȣ���, �Ǵ� ��Ʈ��ũ ���̺귯������ ������ ����
					if (pSession->_isClosed == true)
					{
						isSessionClosed = true;
						break;
					}
				}

				// ������ ���� �޽����� ���� ��� ��Ʈ��Ʈ ����
				if (isSessionReceivedMsg == true)
				{
					pSession->SetHeartBeatTime();
				}
			} while (0);

			// ���� �̵��� ȣ��Ǿ��� ������ map���� �����ϰ�, ������ contents�� ���� join �޽����� ������.
			// (����ڰ� ������ �̵����״ٸ� �÷��̾� ��ü�� map���� �������� ���̴�. ������ �÷��̾� ��ü�� ���� free������ �ʾҴ�.)
			// (�÷��̾� ��ü�� free�Ǵ°��� �����ϱ� ���� ����ڰ� ���� �̵��� ��û�ߴٸ� �ݵ�� �̵��Ǿ�� �Ѵ�. �׷��� ���� ���� ���躸�� �̵���û�� ���� ó���ؾ� �Ѵ�.)
			// (���� �÷��̾� ��ü�� map���� �����ߴµ� �÷��̾� ��ü�� �̵����� �ʰ� OnSessionDisconnected �Լ��� ȣ��ȴٸ� �÷��̾� ��ü�� free���� ���� ���̴�.)
			if (isSessionOnTransfer == true)
			{
				if (!(_thread.mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY))
				{
					// ��Ŷ ��� ���� ��尡 �ƴ� ��� ���� ��Ŷ���۳��� ��Ŷ�� ��� �����Ѵ�.
					_sendContentsPacket(pSession);
				}

				iter = _mapSession.erase(iter);

				MsgContents& msg = _pNetAccessor->AllocMsg();
				msg.type = EMsgContentsType::JOIN_SESSION;
				msg.pSession = pSession;
				msg.data = pSession->_transfer.pData;
				pSession->_transfer.spDestination->InsertMessage(msg);
			}
			// ���� ������ �����Ǿ��� ��� ������ map���� �����ϰ� IoCount�� ���ҽ�Ų��. �׷����� OnSessionDisconnected�� ȣ���Ѵ�.
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

		// ��Ʈ��Ʈ Ÿ�Ӿƿ� üũ(_bEnableHeartBeatTimeout == true �� ���)
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
					// ��Ʈ��Ʈ Ÿ�Ӿƿ� ����� ��� ������ map���� �����ϰ� ������ ���´�.
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

		// ��Ŷ ������� ��尡 �ƴ� ���
		if (!(_thread.mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY))
		{
			// ������ �ڵ忡�� ���� ��û�� ��Ŷ�� �����Ѵ�.
			for (auto iter = _mapSession.begin(); iter != _mapSession.end(); ++iter)
			{
				pSession = iter->second;

				// ���� ���ǿ����� ��������� �Լ��� ȣ��Ǿ��ٸ� ��Ʈ��ũ�� ��������� �Լ��� ȣ���Ѵ�.
				// ���� ��Ŷ�� ��� ������ ���ܾ� �ϱ� ������ ��������� �Լ��� �ݵ�� ȣ���Ѵ�.
				if (pSession->_bContentsWaitToDisconnect == true)
				{
					_sendContentsPacketAndDisconnect(pSession);
				}
				// ������ ��Ŷ�� ��� �����Ѵ�.
				else if (pSession->_vecContentsPacket.size() > 0)
				{
					_sendContentsPacket(pSession);
				}
			}
		}

		// ���� ����ð��� FPS�� ���߱� ���� sleep�Ѵ�.
		SyncLogicTime();

		// time manager update
		_spTimeMgr->Update();
	}
}





