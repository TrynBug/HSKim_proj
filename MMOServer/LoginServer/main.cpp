
#include <iostream>
#include <vector>
#include <list>
#include <unordered_map>
#include <string>
#include <sstream>

#include "CLoginServer.h"

#include "../utils/profiler.h"
#include "../utils/CCrashDump.h"
#include "../utils/logger.h"
#include "../utils/CCpuUsage.h"
#include "../utils/CPDH.h"


using namespace loginserver;
void ConsoleOutServerState(std::shared_ptr<CLoginServer> server);


int main()
{
	std::shared_ptr<CLoginServer> server(new CLoginServer);
	bool retStart = server->StartUp();
	ConsoleOutServerState(server);

	return 0;
}



void ConsoleOutServerState(std::shared_ptr<CLoginServer> server)
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
	__int64 prevLoginCount = 0;
	__int64 prevQueryRunCount = 0;

	while (true)
	{
		Sleep(50);

		if (GetConsoleWindow() == GetForegroundWindow())
		{
			if (GetAsyncKeyState('E') || GetAsyncKeyState('e'))
			{
				printf("[server] terminate chat server\n");
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
				printf("reset profiler\n");
			}
		}

		// 1초마다 로그 출력
		if (GetTickCount64() - tick < 1000)
			continue;
		tick += 1000;
		CPUTime.UpdateCpuTime();
		pdh.Update();

		const netlib::CNetServer::Monitor& netMonitor = server->GetNetworkMonitor();
		const CLoginServer::Monitor& monitor = server->GetMonitor();
		__int64 currAcceptCount = netMonitor.GetAcceptCount();                     // accept 횟수
		__int64 currConnectCount = netMonitor.GetConnectCount();                   // connect 횟수 (accept 후 connect 승인된 횟수)
		__int64 currDisconnectCount = netMonitor.GetDisconnectCount();             // disconnect 횟수 (세션 release 횟수)
		__int64 currRecvCount = netMonitor.GetRecvCount();                         // WSARecv 함수 호출 횟수      
		__int64 currSendCount = netMonitor.GetSendCount();                         // WSASend 함수 호출 횟수
		__int64 currRecvCompletionCount = netMonitor.GetRecvCompletionCount();     // recv 완료통지 처리횟수
		__int64 currSendCompletionCount = netMonitor.GetSendCompletionCount();     // send 완료통지 처리횟수
		__int64 currLoginCount = monitor.GetLoginCount();                          // 로그인 성공 횟수
		__int64 currQueryRunCount = server->GetQueryRunCount();                    // 쿼리 실행횟수


		// 현재 세션 수, WSASend 호출횟수, WSARecv 호출횟수, recv 완료통지 처리 횟수, send 완료통지 처리횟수 출력
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [network]  session:%d,  accept:%lld (%lld/s),  conn:%lld (%lld/s),  disconn:%lld (%lld/s),  recv:%lld/s,  send:%lld/s,  recvComp:%lld/s,  sendComp:%lld/s\n"
			, whileCount, server->GetNumSession()
			, currAcceptCount, currAcceptCount - prevAcceptCount
			, currConnectCount, currConnectCount - prevConnectCount
			, currDisconnectCount, currDisconnectCount - prevDisconnectCount
			, currRecvCount - prevRecvCount, currSendCount - prevSendCount
			, currRecvCompletionCount - prevRecvCompletionCount, currSendCompletionCount - prevSendCompletionCount);

		// 로그인서버
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [server ]  client:%d,  account:%d,  login:%lld/s"
			",  CPU usage [T:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]  [Server:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]\n"
			, whileCount
			, server->GetNumClient(), server->GetSizeAccountMap(), currLoginCount - prevLoginCount
			, CPUTime.ProcessorTotal(), CPUTime.ProcessorKernel(), CPUTime.ProcessorUser(), CPUTime.ProcessTotal(), CPUTime.ProcessKernel(), CPUTime.ProcessUser());

		// 연결끊김 사유
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  sess lim:%lld,  client lim:%lld,  recv io err (known:%lld,  121:%lld,  unknown:%lld)"
			",  packet (code:%lld,  len:%lld,  decode:%lld,  type:%lld)\n"
			, whileCount, netMonitor.GetDisconnBySessionLimit(), monitor.GetDisconnByClientLimit()
			, netMonitor.GetDisconnByKnownIoError(), netMonitor.GetDisconnBy121RecvIoError(), netMonitor.GetDisconnByUnknownIoError()
			, netMonitor.GetDisconnByPacketCode(), netMonitor.GetDisconnByPacketLength(), netMonitor.GetDisconnByPacketDecode(), monitor.GetDisconnByInvalidMessageType());

		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  no client:%lld,  DB error:%lld,  DB no account:%lld,  dup login:%lld"
			",  timeout (login:%lld,  heart:%lld)\n"
			, whileCount
			, monitor.GetDisconnByNoClient(), monitor.GetDisconnByDBDataError(), monitor.GetDisconnByNoAccount(), monitor.GetDisconnByDupAccount()
			, monitor.GetDisconnByLoginTimeout(), monitor.GetDisconnByHeartBeatTimeout());

		// 메모리풀. 패킷, 메시지, 플레이어
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [pool   ]  packet:%d (alloc:%d, used:%d, free:%d),  client:%d (alloc:%d, used:%d, free:%d)\n"
			, whileCount
			, server->GetPacketPoolSize(), server->GetPacketAllocCount(), server->GetPacketActualAllocCount(), server->GetPacketFreeCount()
			, server->GetClientPoolSize(), server->GetClientAllocCount(), server->GetClientActualAllocCount(), server->GetClientFreeCount());

		// DB
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [DB     ]  connection:%d,  query count:%d/s,  query run time (avg:%.1fms , max:%.1fms , min:%.1fms)\n"
			, whileCount, server->GetNumDBConnection(), currQueryRunCount - prevQueryRunCount
			, server->GetAvgQueryRunTime(), server->GetMaxQueryRunTime()
			, server->GetMinQueryRunTime() == FLT_MAX ? 0.f : server->GetMinQueryRunTime());

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
		oss << whileCount << L" [etc    ]  traffic control:" << netMonitor.GetTrafficCongestionControlCount();
		oss << L",  deferred closesocket:" << netMonitor.GetDeferredDisconnectCount() << L" (msgQ:" << server->GetSizeMsgQDeferredCloseSocket();
		oss << L",  error:" << netMonitor.GetOtherErrorCount() << L")\n\n";
		LOGGING(LOGGING_LEVEL_INFO, oss.str().c_str());

		prevAcceptCount = currAcceptCount;
		prevConnectCount = currConnectCount;
		prevDisconnectCount = currDisconnectCount;
		prevRecvCount = currRecvCount;
		prevSendCount = currSendCount;
		prevRecvCompletionCount = currRecvCompletionCount;
		prevSendCompletionCount = currSendCompletionCount;
		prevLoginCount = currLoginCount;
		prevQueryRunCount = currQueryRunCount;

		whileCount++;
	}
}