/*
게임 서버용 네트워크 라이브러리 사용
인증스레드1, 필드스레드1
*/
#include "stdafx.h"

#include "CGameServer.h"
#include "CGameContents.h"
#include "CGameContentsField.h"

#include "../utils/profiler.h"
#include "../utils/CCrashDump.h"
#include "../utils/logger.h"
#include "../utils/CCpuUsage.h"
#include "../utils/CPDH.h"

using namespace gameserver;
void ConsoleOutServerState(std::shared_ptr<CGameServer> server);

int main()
{
	std::shared_ptr<CGameServer> server(new CGameServer);
	bool retStart = server->StartUp();
	if (retStart == false)
	{
		wprintf(L"[main] failed to start game server\n");
		return 0;
	}
	ConsoleOutServerState(server);

	return 0;
}

void ConsoleOutServerState(std::shared_ptr<CGameServer> server)
{
	CCrashDump::Init();
	CCpuUsage CPUTime;

	CPDH pdh;
	pdh.Init();

	int numWorkerThread = server->GetNumWorkerThread();
	LARGE_INTEGER liFrequency;
	QueryPerformanceFrequency(&liFrequency);
	ULONGLONG tick = GetTickCount64();
	__int64 whileCount = 0;
	__int64 prevAcceptCount = 0;
	__int64 prevConnectCount = 0;
	__int64 prevDisconnectCount = 0;
	__int64 prevRecvCount = 0;
	__int64 prevSendCount = 0;
	__int64 prevRecvCompletionCount = 0;
	__int64 prevSendCompletionCount = 0;
	__int64 prevMsgHandleCount = 0;
	__int64 prevQueryRunCount = 0;
	__int64 prevAuthClientMsgCount = 0;
	__int64 prevAuthInternalMsgCount = 0;
	__int64 prevAuthQueryCount = 0;
	__int64 prevFieldClientMsgCount = 0;
	__int64 prevFieldInternalMsgCount = 0;
	__int64 prevFieldQueryCount = 0;

	Sleep(1000);
	while (true)
	{
		Sleep(50);

		if (GetConsoleWindow() == GetForegroundWindow())
		{
			if (GetAsyncKeyState('E') || GetAsyncKeyState('e'))
			{
				printf("[main] terminate game server\n");
				server->Shutdown();
				while (server->IsTerminated() == false)
				{
					Sleep(100);
					continue;
				}
				break;
			}
			else if (GetAsyncKeyState('C') || GetAsyncKeyState('c'))
			{
				int* p = 0;
				*p = 0;
				break;
			}
			else if (GetAsyncKeyState('P') || GetAsyncKeyState('p'))
			{
				profiler::ResetProfilingData();
				wprintf(L"[main] reset profiler\n");
			}
			else if (GetAsyncKeyState('O') || GetAsyncKeyState('o'))
			{
				profiler::OutputProfilingData();
				printf("[main] output profiler\n");
			}
		}

		// 1초마다 로그 출력
		if (GetTickCount64() - tick < 1000)
			continue;
		tick += 1000;
		CPUTime.UpdateCpuTime();
		pdh.Update();

		auto& monitor = server->GetMonitor();
		auto& netMonitor = server->GetNetworkMonitor();
		__int64 currAcceptCount = netMonitor.GetAcceptCount();                     // accept 횟수
		__int64 currConnectCount = netMonitor.GetConnectCount();                   // connect 횟수 (accept 후 connect 승인된 횟수)
		__int64 currDisconnectCount = netMonitor.GetDisconnectCount();             // disconnect 횟수 (세션 release 횟수)
		__int64 currRecvCount = netMonitor.GetRecvCount();                         // WSARecv 함수 호출 횟수      
		__int64 currSendCount = netMonitor.GetSendCount();                         // WSASend 함수 호출 횟수
		__int64 currRecvCompletionCount = netMonitor.GetRecvCompletionCount();     // recv 완료통지 처리횟수
		__int64 currSendCompletionCount = netMonitor.GetSendCompletionCount();     // send 완료통지 처리횟수
		__int64 currMsgHandleCount = server->GetContentsSessionMsgCount();         // 컨텐츠 스레드 세션 메시지 처리 횟수
		//__int64 currQueryRunCount = server->GetQueryRunCount();                  // 쿼리 실행횟수

		const std::shared_ptr<CGameContents> pAuth = server->GetGameContents(EContentsThread::AUTH);
		const std::shared_ptr<CGameContents> pField = server->GetGameContents(EContentsThread::FIELD);
		auto& monitorAuth = pAuth->GetMonitor();
		auto& monitorField = pField->GetMonitor();
		auto timeAuth = pAuth->GetTimeMgr();
		auto timeField = pField->GetTimeMgr();
		__int64 currAuthClientMsgCount = monitorAuth.GetSessionMsgCount();
		__int64 currAuthInternalMsgCount = monitorAuth.GetInternalMsgCount();
		__int64 currAuthQueryCount = pAuth->GetQueryRunCount();
		__int64 currFieldClientMsgCount = monitorField.GetSessionMsgCount();
		__int64 currFieldInternalMsgCount = monitorField.GetInternalMsgCount();
		__int64 currFieldQueryCount = pField->GetQueryRunCount();

		// 현재 세션 수, WSASend 호출횟수, WSARecv 호출횟수, recv 완료통지 처리 횟수, send 완료통지 처리횟수 출력
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [network]  session:%d,  accept:%lld (%lld/s),  conn:%lld (%lld/s),  disconn:%lld (%lld/s),  recv:%lld/s,  send:%lld/s,  recvComp:%lld/s,  sendComp:%lld/s\n"
			, whileCount, server->GetNumSession()
			, currAcceptCount, currAcceptCount - prevAcceptCount
			, currConnectCount, currConnectCount - prevConnectCount
			, currDisconnectCount, currDisconnectCount - prevDisconnectCount
			, currRecvCount - prevRecvCount, currSendCount - prevSendCount
			, currRecvCompletionCount - prevRecvCompletionCount, currSendCompletionCount - prevSendCompletionCount);

		// 게임서버
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [game  ]  CPU usage [T:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]  [Server:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]\n"
			, whileCount
			, CPUTime.ProcessorTotal(), CPUTime.ProcessorKernel(), CPUTime.ProcessorUser(), CPUTime.ProcessTotal(), CPUTime.ProcessKernel(), CPUTime.ProcessUser());

		// 인증컨텐츠
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [auth  ]  player:%d, msg proc:%lld/s (internal:%lld/s),  query: (run:%d/s, time:(avg:%.1fms , max:%.1fms , min:%.1fms)),  FPS:%d (avg1m:%d, max:%d, min:%d), DT:%.3f (max:%.3f, min:%.3f)\n"
			, whileCount
			, pAuth->GetNumPlayer(), currAuthClientMsgCount - prevAuthClientMsgCount, currAuthInternalMsgCount - prevAuthInternalMsgCount
			, currAuthQueryCount - prevAuthQueryCount, pAuth->GetAvgQueryRunTime(), pAuth->GetMaxQueryRunTime(), pAuth->GetMinQueryRunTime()
			, pAuth->GetFPS(), (int)timeAuth->GetAvgFPS1m(), timeAuth->GetMaxFPS1st(), timeAuth->GetMinFPS1st()
			, timeAuth->GetAvgDT1s(), timeAuth->GetMaxDT1st(), timeAuth->GetMinDT1st());

		// 필드컨텐츠
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [field ]  player:%d, msg proc:%lld/s (internal:%lld/s), query: (run:%d/s, remain:%d, time:(avg:%.1fms , max:%.1fms , min:%.1fms)),  FPS:%d (avg1m:%d, max:%d, min:%d), DT:%.3f (max:%.3f, min:%.3f)\n"
			, whileCount
			, pField->GetNumPlayer(), currFieldClientMsgCount - prevFieldClientMsgCount, currFieldInternalMsgCount - prevFieldInternalMsgCount
			, currFieldQueryCount - prevFieldQueryCount, pField->GetRemainQueryCount(), pField->GetAvgQueryRunTime(), pField->GetMaxQueryRunTime(), pField->GetMinQueryRunTime()
			, pField->GetFPS(), (int)timeField->GetAvgFPS1m(), timeField->GetMaxFPS1st(), timeField->GetMinFPS1st()
			, timeField->GetAvgDT1s(), timeField->GetMaxDT1st(), timeField->GetMinDT1st());

		// 연결끊김 사유
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  sess lim:%lld,  iocp assoc:%lld,  recv io err (known:%lld,  121:%lld,  unknown:%lld),  by normal:%lld"
			",  packet (code:%lld,  len:%lld,  decode:%lld)\n"
			, whileCount, netMonitor.GetDisconnBySessionLimit(), netMonitor.GetDisconnByIOCPAssociation()
			, netMonitor.GetDisconnByKnownIoError(), netMonitor.GetDisconnBy121RecvIoError(), netMonitor.GetDisconnByUnknownIoError(), netMonitor.GetDisconnByNormalProcess()
			, netMonitor.GetDisconnByPacketCode(), netMonitor.GetDisconnByPacketLength(), netMonitor.GetDisconnByPacketDecode());

		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  plyr lim:%lld,  invalid msg (auth:%lld, field:%lld),  heart beat timeout (auth:%lld, field:%lld)\n"
			, whileCount
			, 0//server->_disconnByPlayerLimit
			, pAuth->GetDisconnByInvalidMsgType(), pField->GetDisconnByInvalidMsgType()
			, monitorAuth.GetDisconnByHeartBeat(), monitorField.GetDisconnByHeartBeat());

		// 메모리풀. 패킷, 메시지, 플레이어, field컨텐츠 DB
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [pool   ]  packet:%d (alloc:%d, used:%d, free:%d),  message:%d (alloc:%d, used:%d, free:%d),  player:%d (alloc:%d, used:%d, free:%d),  field DB:%d (alloc:%d, free:%d)\n"
			, whileCount
			, server->GetPacketPoolSize(), server->GetPacketAllocCount(), server->GetPacketActualAllocCount(), server->GetPacketFreeCount()
			, server->GetMsgPoolSize(), server->GetMsgAllocCount(), server->GetMsgActualAllocCount(), server->GetMsgFreeCount()
			, server->GetPlayerPoolSize(), server->GetPlayerAllocCount(), server->GetPlayerActualAllocCount(), server->GetPlayerFreeCount()
			, pField->GetQueryRequestPoolSize(), pField->GetQueryRequestAllocCount(), pField->GetQueryRequestFreeCount());

		// DB
		//LOGGING(LOGGING_LEVEL_INFO, L"%lld [DB     ]  query count:%d/s,  remain query:%d,  query run time (avg:%.1fms , max:%.1fms , min:%.1fms)\n\n"
		//	, whileCount, currQueryRunCount - prevQueryRunCount
		//	, server->GetUnprocessedQueryCount(), server->GetAvgQueryRunTime(), server->GetMaxQueryRunTime()
		//	, server->GetMinQueryRunTime() == FLT_MAX ? 0.f : server->GetMinQueryRunTime());

		// 시스템
		const PDHCount& pdhCount = pdh.GetPDHCount();
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [system ]  TCP (segment:%lld/s, retrans:%lld/s, recv:%.2fMB/s, send:%.2fMB/s),  memory(MB) [System C:%.2f, P:%.2f, NP:%.2f] [Process C:%.2f, P:%.2f, NP:%.2f]\n"
			, whileCount
			, pdhCount.TCPSegmentsSent, pdhCount.TCPSegmentsRetransmitted, pdhCount.networkRecvBytes / 1048576.0, pdhCount.networkSendBytes / 1048576.0
			, (double)pdhCount.systemComittedBytes / 1048576.0, (double)pdhCount.systemPagedPoolBytes / 1048576.0, (double)pdhCount.systemNonpagedPoolBytes / 1048576.0
			, (double)pdhCount.processPrivateBytes / 1048576.0, (double)pdhCount.processPagedPoolBytes / 1048576.0, (double)pdhCount.processNonpagedPoolBytes / 1048576.0);

		// worker 스레드별 GQCS wait 시간, 트래픽 혼잡제어 횟수
		std::wostringstream oss;
		oss.str(L"");
		oss << whileCount << L" [etc    ] traffic control:" << netMonitor.GetTrafficCongestionControlCount();
		oss << L",  error:" << netMonitor.GetOtherErrorCount() << L"\n\n";
		LOGGING(LOGGING_LEVEL_INFO, oss.str().c_str());


		prevAcceptCount = currAcceptCount;
		prevConnectCount = currConnectCount;
		prevDisconnectCount = currDisconnectCount;
		prevRecvCount = currRecvCount;
		prevSendCount = currSendCount;
		prevRecvCompletionCount = currRecvCompletionCount;
		prevSendCompletionCount = currSendCompletionCount;
		prevMsgHandleCount = currMsgHandleCount;
		//prevQueryRunCount = currQueryRunCount;
		prevAuthClientMsgCount = currAuthClientMsgCount;
		prevAuthInternalMsgCount = currAuthInternalMsgCount;
		prevAuthQueryCount = currAuthQueryCount;
		prevFieldClientMsgCount = currFieldClientMsgCount;
		prevFieldInternalMsgCount = currFieldInternalMsgCount;
		prevFieldQueryCount = currFieldQueryCount;

		whileCount++;
	}

}
