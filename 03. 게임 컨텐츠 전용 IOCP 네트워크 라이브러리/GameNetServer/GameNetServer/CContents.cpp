#include "CNetServer.h"
#include "CTimeMgr.h"

using namespace game_netserver;

CContents::CContents(int FPS)
	:_pNetServer(nullptr), _hThreadContents(0), _bTerminated(false)
	, _sessionPacketProcessLimit(1000000)
	, _bEnableHeartBeatTimeout(false), _timeoutHeartBeat(0), _timeoutHeartBeatCheckPeriod(0), _lastHeartBeatCheckTime(0)
	, _FPS(FPS), _logicStartTime{ 0, }, _logicEndTime{ 0, }, _performanceFrequency{ 0, }, _catchUpTime(0)
	, _pTimeMgr(nullptr)
{
	QueryPerformanceFrequency(&_performanceFrequency);
	_oneFrameTime = _performanceFrequency.QuadPart / _FPS;
	_pTimeMgr = new CTimeMgr;

	_mode = 0;
}

CContents::CContents(int FPS, DWORD mode)
	:_pNetServer(nullptr), _hThreadContents(0), _bTerminated(false)
	, _sessionPacketProcessLimit(1000000)
	, _bEnableHeartBeatTimeout(false), _timeoutHeartBeat(0), _timeoutHeartBeatCheckPeriod(0), _lastHeartBeatCheckTime(0)
	, _FPS(FPS), _logicStartTime{ 0, }, _logicEndTime{ 0, }, _performanceFrequency{ 0, }, _catchUpTime(0)
	, _pTimeMgr(nullptr)
{
	QueryPerformanceFrequency(&_performanceFrequency);
	_oneFrameTime = _performanceFrequency.QuadPart / _FPS;
	_pTimeMgr = new CTimeMgr;

	_mode = mode;
}


CContents::~CContents()
{


	
}



/* ������ */
// ������ ����
bool CContents::StartUp()
{
	_hThreadContents = (HANDLE)_beginthreadex(NULL, 0, ThreadContents, this, 0, NULL);
	if (_hThreadContents == NULL)
	{
		_pNetServer->OnError(L"[Contents] An error occurred when starting the contents thread. error:%u\n", GetLastError());
		return false;
	}

	return true;
}

// ������ ����
void CContents::Shutdown()
{
	// shutdown �޽��� ����
	MsgContents* pMsgShutdown = _pNetServer->AllocMsg();
	pMsgShutdown->type = EMsgContentsType::SHUTDOWN;
	pMsgShutdown->pSession = nullptr;
	pMsgShutdown->data = nullptr;
	InsertMessage(pMsgShutdown);

	// �����尡 ����Ǹ� _bTerminated = true�� ������
}

// ������ �޽���ť�� �޽��� ����
void CContents::InsertMessage(MsgContents* pMsg)
{
	_msgQ.Enqueue(pMsg);
}

// ���� ����ð��� FPS�� �����. ������� �������� ������. �ִ� FPS�� ������ ������ �����ȴ�.
void CContents::SyncLogicTime()
{
	QueryPerformanceCounter(&_logicEndTime);
	__int64 spentTime = max(0, _logicEndTime.QuadPart - _logicStartTime.QuadPart);
	__int64 sleepTime = _oneFrameTime - spentTime;

	DWORD dwSleepTime = 0;
	// sleep �ؾ��� �ð��� �ִ� ���
	if (sleepTime > 0)
	{
		// sleep �ð��� ms ������ ��ȯ�Ͽ� sleep �Ѵ�.
		dwSleepTime = (DWORD)round((double)(sleepTime * 1000) / (double)_performanceFrequency.QuadPart);
		Sleep(dwSleepTime);
	}

	// ���� ������ ���۽ð��� [���� ���� ����ð� + sleep�� �ð�] �̴�.
	_logicStartTime.QuadPart = _logicEndTime.QuadPart + (dwSleepTime * (_performanceFrequency.QuadPart / 1000));
}


// ���� ����ð��� FPS�� �����. ���� ������� �������� ���� ��� �Ѱ��Ҷ� ������´�. �ִ� FPS�� ������ ������ ������ �� �ִ�.
void CContents::SyncLogicTimeAndCatchUp()
{
	QueryPerformanceCounter(&_logicEndTime);
	__int64 spentTime = max(0, _logicEndTime.QuadPart - _logicStartTime.QuadPart);
	__int64 sleepTime = _oneFrameTime - spentTime;

	// sleep �ؾ��� �ð��� �ִ� ���
	if (sleepTime > 0)
	{
		// ������ƾ��� �ð��� sleep �ð����� ũ�ٸ� ������ƾ��� �ð����� sleep �ð��� �����Ѵ�.
		if (_catchUpTime > sleepTime)
		{
			_catchUpTime -= sleepTime;
			sleepTime = 0;
		}
		// ������ƾ��� �ð��� sleep �ð����� �۴ٸ� sleep �ð����� ������ƾ��� �ð��� �����Ѵ�.
		else
		{
			sleepTime -= _catchUpTime;
			_catchUpTime = 0;
		}
	}
	// ���� ����ð��� 1�����Ӵ� �ҿ�ð����� �� �ɷ��� ���
	else
	{
		// ������ƾ��� �ð��� �� �ɸ� �ð��� �߰��Ѵ�.
		_catchUpTime += -sleepTime;
		sleepTime = 0;
	}

	__int64 sleepTimeRemainder = 0;
	DWORD dwSleepTime = 0;
	// sleep �ؾ��� �ð��� �ִ� ���
	if (sleepTime > 0)
	{
		// sleep �ð��� ms ������ �����ϰ�, �������� ������ƾ��� �ð��� �߰��Ѵ�.
		sleepTimeRemainder = sleepTime % (_performanceFrequency.QuadPart / 1000);  
		sleepTime -= sleepTimeRemainder;
		_catchUpTime += sleepTimeRemainder;

		if (sleepTime > 0)
		{
			// sleep �ð��� ms ������ ��ȯ�Ͽ� sleep �Ѵ�.
			dwSleepTime = (DWORD)(sleepTime * 1000 / _performanceFrequency.QuadPart);
			Sleep(dwSleepTime);
		}
	}

	// ���� ������ ���۽ð��� [���� ���� ����ð� + sleep�� �ð�] �̴�.
	// �� �� 0.5�� ��ŭ�� count�� ���� ���۽ð��� �����ִ� ������ �ؾ� FPS�� ��Ȯ������(������ ��;;).
	_logicStartTime.QuadPart = _logicEndTime.QuadPart + sleepTime + (_performanceFrequency.QuadPart / 1000 / 2);
}


/* ���� (private) */
// ������ ������ ���´�. ���ǿ� ���� �޽����� ���̻� ó������ �ʴ´�.
void CContents::DisconnectSession(CSession* pSession)
{
	_pNetServer->Disconnect(pSession);
}

// ���� ���� ������ ��Ŷ ���� ���� ��Ŷ�� �����Ѵ�.
void CContents::_sendContentsPacket(CSession* pSession)
{	const wchar_t* GetSessionIP();
	unsigned short GetSessionPort();
	size_t numPacket = pSession->_vecContentsPacket.size();
	_pNetServer->SendPacketAsync(pSession->_sessionId, pSession->_vecContentsPacket.data(), numPacket);
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
	_pNetServer->SendPacketAsyncAndDisconnect(pSession->_sessionId, pSession->_vecContentsPacket.data(), numPacket);
	for (size_t i = 0; i < numPacket; i++)
	{
		pSession->_vecContentsPacket[i]->SubUseCount();
	}
	pSession->_vecContentsPacket.clear();
}




/* ���� */
const wchar_t* CContents::GetSessionIP(__int64 sessionId)
{
	auto iter = _mapSession.find(sessionId);
	if (iter == _mapSession.end())
		return nullptr;

	return (iter->second)->GetIP();
}

unsigned short CContents::GetSessionPort(__int64 sessionId)
{
	auto iter = _mapSession.find(sessionId);
	if (iter == _mapSession.end())
		return 0;

	return (iter->second)->GetPort();
}



// ������ destination �������� �ű��. ���ǿ� ���� �޽����� ���� �����忡���� ���̻� ó������ �ʴ´�.
void CContents::TransferSession(__int64 sessionId, CContents* destination, PVOID data)
{
	auto iter = _mapSession.find(sessionId);
	CSession* pSession = iter->second;
	pSession->_bOnTransfer = true;
	pSession->_pDestinationContents = destination;
	pSession->_pTransferData = data;
}


// ������ ������ ���´�. ���ǿ� ���� �޽����� ���̻� ó������ �ʴ´�.
void CContents::DisconnectSession(__int64 sessionId)
{
	auto iter = _mapSession.find(sessionId);
	CSession* pSession = iter->second;
	_pNetServer->Disconnect(pSession);
}



/* packet */
CPacket* CContents::AllocPacket()
{ 
	return _pNetServer->AllocPacket(); 
}

bool CContents::SendPacket(__int64 sessionId, CPacket* pPacket)
{
	if (_mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	{
		_pNetServer->SendPacketAsync(sessionId, pPacket);
	}
	else
	{
		auto iter = _mapSession.find(sessionId);
		if (iter == _mapSession.end())
			return false;

		CSession* pSession = iter->second;
		pPacket->AddUseCount();
		pSession->_vecContentsPacket.push_back(pPacket);
	}

	return true;
}


bool CContents::SendPacket(__int64 sessionId, CPacket** arrPtrPacket, int numPacket)
{
	if (_mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	{
		_pNetServer->SendPacketAsync(sessionId, arrPtrPacket, numPacket);
	}
	else
	{
		auto iter = _mapSession.find(sessionId);
		if (iter == _mapSession.end())
			return false;

		CSession* pSession = iter->second;
		for (int i = 0; i < numPacket; i++)
		{
			arrPtrPacket[i]->AddUseCount();
			pSession->_vecContentsPacket.push_back(arrPtrPacket[i]);
		}
	}

	return true;
}

bool CContents::SendPacketAndDisconnect(__int64 sessionId, CPacket* pPacket)
{
	if (_mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	{
		_pNetServer->SendPacketAsyncAndDisconnect(sessionId, pPacket);
	}
	else
	{
		auto iter = _mapSession.find(sessionId);
		if (iter == _mapSession.end())
			return false;

		CSession* pSession = iter->second;
		pPacket->AddUseCount();
		pSession->_vecContentsPacket.push_back(pPacket);
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



/* Time */
double CContents::GetDT() const { return _pTimeMgr->GetDT(); }
float CContents::GetfDT() const { return _pTimeMgr->GetfDT(); }
double CContents::GetAvgDT1s() const { return _pTimeMgr->GetAvgDT1s(); }
double CContents::GetMinDT1st() const { return _pTimeMgr->GetMinDT1st(); }
double CContents::GetMinDT2nd() const { return _pTimeMgr->GetMinDT2nd(); }
double CContents::GetMaxDT1st() const { return _pTimeMgr->GetMaxDT1st(); }
double CContents::GetMaxDT2nd() const { return _pTimeMgr->GetMaxDT2nd(); }
int CContents::GetFPS() const { return _pTimeMgr->GetFPS(); }
float CContents::GetAvgFPS1m() const { return _pTimeMgr->GetAvgFPS1m(); }
int CContents::GetMinFPS1st() const { return _pTimeMgr->GetMinFPS1st(); }
int CContents::GetMinFPS2nd() const { return _pTimeMgr->GetMinFPS2nd(); }
int CContents::GetMaxFPS1st() const { return _pTimeMgr->GetMaxFPS1st(); }
int CContents::GetMaxFPS2nd() const { return _pTimeMgr->GetMaxFPS2nd(); }
bool CContents::OneSecondTimer() const { return _pTimeMgr->Is1sTimerSet(); }

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

	CContents& contents = *(CContents*)pParam;
	MsgContents* pMsg;
	CPacket* pRecvPacket;
	CSession* pSession;
	__int64 sessionId;

	// time manager �ʱ�ȭ
	contents._pTimeMgr->Init();
	contents._lastHeartBeatCheckTime = GetTickCount64();

	// ������ ���� ����
	QueryPerformanceCounter(&contents._logicStartTime);
	while (true)
	{
		// �޽���ť ���� ��� �޽��� ó��
		while (contents._msgQ.Dequeue(pMsg))
		{
			switch (pMsg->type)
			{
			// �����忡 ���� ������ ������ ���� ���
			case EMsgContentsType::INITIAL_JOIN_SESSION:
			{
				pSession = pMsg->pSession;
				contents._mapSession.insert(std::make_pair(pSession->_sessionId, pSession));
				contents.OnSessionJoin(pSession->_sessionId, nullptr);  // �� �Լ������� �÷��̾ �����ϰ� �÷��̾�ʿ� ����Ѵ�.
				break;
			}
			// �����忡 ������ ���� ���
			// ������ map�� �߰��ϰ�, OnSessionJoin �Լ��� ȣ���Ѵ�. �� �� �÷��̾� ��ü �ּҵ� ���� �����Ѵ�.
			case EMsgContentsType::JOIN_SESSION:
			{
				pSession = pMsg->pSession;
				pSession->_bOnTransfer = false;
				contents._mapSession.insert(std::make_pair(pSession->_sessionId, pSession));
				contents.OnSessionJoin(pSession->_sessionId, pMsg->data);  // �� �Լ������� �÷��̾ �÷��̾�ʿ� ����Ѵ�.
				break;
			}
			// ������ ���� �޽����� ����
			case EMsgContentsType::SHUTDOWN:
			{
				// ��� ������ ������ ���´�.
				int disconnCount = 0;
				for (auto iter = contents._mapSession.begin(); iter != contents._mapSession.end(); ++iter)
				{
					pSession = iter->second;
					sessionId = pSession->_sessionId;
					contents.DisconnectSession(pSession);
					contents._pNetServer->DecreaseIoCount(pSession);
					contents.OnSessionDisconnected(sessionId);
					disconnCount++;
					if (disconnCount > 100)
					{
						Sleep(50);
						disconnCount = 0;
					}
				}
				contents._mapSession.clear();
				contents._pNetServer->_poolMsgContents.Free(pMsg);

				// �޽���ť�� ��� �޽����� �����Ѵ�.
				MsgContents* pMsg;
				while (contents._msgQ.Dequeue(pMsg))
				{
					contents._pNetServer->_poolMsgContents.Free(pMsg);
				}

				// ������ ����
				contents._bTerminated = true;
				wprintf(L"[Contents] End Contents Thread. id:%u\n", GetCurrentThreadId());
				return 0;
				break;
			}
			// �߸��� ������ �޽����� ���� ��� �����޽��� ���
			default:
			{
				contents._pNetServer->OnError(L"[Contents] Contents thread got a invalid internal message. type:%d, session:%p, sessionId:%lld, data:%p\n"
					, pMsg->type, pMsg->pSession, pMsg->pSession == nullptr ? 0 : pMsg->pSession->_sessionId, pMsg->data);
				break;
			}
			}

			// �޽��� free
			contents._pNetServer->_poolMsgContents.Free(pMsg);
			contents._internalMsgCount++;  // ����͸�
		}

		// ��� ���ǳ��� ��� �޽��� ó��
		bool isSessionClosed;
		bool isSessionOnTransfer;
		bool isSessionReceivedMsg;
		for (auto iter = contents._mapSession.begin(); iter != contents._mapSession.end();)
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
				if (pSession->_bOnTransfer == true)
				{
					isSessionOnTransfer = true;
					break;
				}

				// ���ǳ��� ��� �޽��� ó��
				while (pSession->_msgQ->Dequeue(pRecvPacket))
				{
					isSessionReceivedMsg = true;
					contents.OnRecv(pSession->_sessionId, pRecvPacket);
					pRecvPacket->SubUseCount();  // ��Ŷ�� ���ī��Ʈ�� ���ҽ�Ų��.
					contents._sessionMsgCount++;  // ����͸�
					// OnRecv �Լ� ������ ������ �̵���Ŵ
					if (pSession->_bOnTransfer == true)
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
				if (!(contents._mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY))
				{
					// ��Ŷ ��� ���� ��尡 �ƴ� ��� ���� ��Ŷ���۳��� ��Ŷ�� ��� �����Ѵ�.
					contents._sendContentsPacket(pSession);
				}

				iter = contents._mapSession.erase(iter);

				MsgContents* pMsg = contents._pNetServer->_poolMsgContents.Alloc();
				pMsg->type = EMsgContentsType::JOIN_SESSION;
				pMsg->pSession = pSession;
				pMsg->data = pSession->_pTransferData;
				pSession->_pDestinationContents->InsertMessage(pMsg);
			}
			// ���� ������ �����Ǿ��� ��� ������ map���� �����ϰ� IoCount�� ���ҽ�Ų��. �׷����� OnSessionDisconnected�� ȣ���Ѵ�.
			else if (isSessionClosed == true)
			{
				iter = contents._mapSession.erase(iter);
				sessionId = pSession->_sessionId;
				contents._pNetServer->DecreaseIoCount(pSession);
				contents.OnSessionDisconnected(sessionId);
			}
			else
			{
				++iter;
			}
		}

		// ��Ʈ��Ʈ Ÿ�Ӿƿ� üũ(_bEnableHeartBeatTimeout == true �� ���)
		ULONGLONG currentTime = GetTickCount64();
		if (contents._bEnableHeartBeatTimeout == true
			&& currentTime > contents._lastHeartBeatCheckTime + contents._timeoutHeartBeatCheckPeriod)
		{
			contents._lastHeartBeatCheckTime = currentTime;
			for (auto iter = contents._mapSession.begin(); iter != contents._mapSession.end();)
			{
				pSession = iter->second;
				if (pSession->_lastHeartBeatTime + contents._timeoutHeartBeat < currentTime)
				{
					// ��Ʈ��Ʈ Ÿ�Ӿƿ� ����� ��� ������ map���� �����ϰ� ������ ���´�.
					iter = contents._mapSession.erase(iter);
					sessionId = pSession->_sessionId;
					contents.DisconnectSession(pSession);
					contents._pNetServer->DecreaseIoCount(pSession);
					contents.OnSessionDisconnected(sessionId);
					contents._disconnByHeartBeat++; // ����͸�
				}
				else
				{
					++iter;
				}
			}
		}

		// OnUpdate
		contents._updateCount++;    // ����͸�
		contents.OnUpdate();

		// ��Ŷ ������� ��尡 �ƴ� ���
		if (!(contents._mode & CONTENTS_MODE_SEND_PACKET_IMMEDIATELY))
		{
			// ������ �ڵ忡�� ���� ��û�� ��Ŷ�� �����Ѵ�.
			for (auto iter = contents._mapSession.begin(); iter != contents._mapSession.end(); ++iter)
			{
				pSession = iter->second;

				// ���� ���ǿ����� ��������� �Լ��� ȣ��Ǿ��ٸ� ��Ʈ��ũ�� ��������� �Լ��� ȣ���Ѵ�.
				// ���� ��Ŷ�� ��� ������ ���ܾ� �ϱ� ������ ��������� �Լ��� �ݵ�� ȣ���Ѵ�.
				if (pSession->_bContentsWaitToDisconnect == true)
				{
					contents._sendContentsPacketAndDisconnect(pSession);
				}
				// ������ ��Ŷ�� ��� �����Ѵ�.
				else if (pSession->_vecContentsPacket.size() > 0)
				{
					contents._sendContentsPacket(pSession);
				}
			}
		}

		// ���� ����ð��� FPS�� ���߱� ���� sleep�Ѵ�.
		contents.SyncLogicTime();

		// time manager update
		contents._pTimeMgr->Update();
	}


	wprintf(L"[Contents] End Contents Thread. id:%u\n", GetCurrentThreadId());

	return 0;
}






