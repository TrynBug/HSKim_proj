/*
�̱۽����� ä�ü���
*/
#include "stdafx.h"
#include "CChatServer.h"

#include "../utils/CCrashDump.h"
#include "../utils/profiler.h"
#include "../utils/logger.h"
#include "../utils/CCpuUsage.h"
#include "../utils/CPDH.h"

using namespace chatserver;
void ConsoleOutServerState(std::shared_ptr<CChatServer> server);

int main()
{
	std::shared_ptr<CChatServer> server(new CChatServer);
	bool retStart = server->StartUp();
	ConsoleOutServerState(server);

	return 0;
}

void ConsoleOutServerState(std::shared_ptr<CChatServer> server)
{
	CCrashDump::Init();
	timeBeginPeriod(1);
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
	__int64 prevEventWaitTime = 0;

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
			else if (GetAsyncKeyState('O') || GetAsyncKeyState('o'))
			{
				profiler::OutputProfilingData();
				printf("output profiler\n");
			}
		}

		// 1�ʸ��� �α� ���
		if (GetTickCount64() - tick < 1000)
			continue;
		tick += 1000;
		CPUTime.UpdateCpuTime();
		pdh.Update();

		const netlib::CNetServer::Monitor& netMonitor = server->GetNetworkMonitor();
		const CChatServer::Monitor& monitor = server->GetMonitor();
		__int64 currAcceptCount = netMonitor.GetAcceptCount();                     // accept Ƚ��
		__int64 currConnectCount = netMonitor.GetConnectCount();                   // connect Ƚ�� (accept �� connect ���ε� Ƚ��)
		__int64 currDisconnectCount = netMonitor.GetDisconnectCount();             // disconnect Ƚ�� (���� release Ƚ��)
		__int64 currRecvCount = netMonitor.GetRecvCount();                         // WSARecv �Լ� ȣ�� Ƚ��      
		__int64 currSendCount = netMonitor.GetSendCount();                         // WSASend �Լ� ȣ�� Ƚ��
		__int64 currRecvCompletionCount = netMonitor.GetRecvCompletionCount();     // recv �Ϸ����� ó��Ƚ��
		__int64 currSendCompletionCount = netMonitor.GetSendCompletionCount();     // send �Ϸ����� ó��Ƚ��
		__int64 currMsgHandleCount = monitor.GetMsgHandleCount();               // ä�ü��� ������ �޽��� ó�� Ƚ��
		__int64 currEventWaitTime = monitor.GetEventWaitTime();                 // ä�ü��� WaitForSingleObject �Լ� ȣ��Ƚ��

		// ���� ���� ��, WSASend ȣ��Ƚ��, WSARecv ȣ��Ƚ��, recv �Ϸ����� ó�� Ƚ��, send �Ϸ����� ó��Ƚ�� ���
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [network]  session:%d,  accept:%lld (%lld/s),  conn:%lld (%lld/s),  disconn:%lld (%lld/s),  recv:%lld/s,  send:%lld/s,  recvComp:%lld/s,  sendComp:%lld/s\n"
			, whileCount, server->GetNumSession()
			, currAcceptCount, currAcceptCount - prevAcceptCount
			, currConnectCount, currConnectCount - prevConnectCount
			, currDisconnectCount, currDisconnectCount - prevDisconnectCount
			, currRecvCount - prevRecvCount, currSendCount - prevSendCount
			, currRecvCompletionCount - prevRecvCompletionCount, currSendCompletionCount - prevSendCompletionCount);

		// ä�ü���
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [server ]  player:%d, account:%d,  msg proc:%lld/s,  msg remain:%d,  event wait:%lldms"
			",  CPU usage [T:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]  [Server:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]\n"
			, whileCount
			, server->GetNumPlayer(), server->GetNumAccount(), currMsgHandleCount - prevMsgHandleCount
			, server->GetUnhandeledMsgCount(), (__int64)((double)(currEventWaitTime - prevEventWaitTime) / (double)(liFrequency.QuadPart / 1000))
			, CPUTime.ProcessorTotal(), CPUTime.ProcessorKernel(), CPUTime.ProcessorUser(), CPUTime.ProcessTotal(), CPUTime.ProcessKernel(), CPUTime.ProcessUser());

		// ������� ����
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  sess lim:%lld,  iocp assoc:%lld,  io err (known:%lld, 121:%lld, unknown:%lld)\n"
			, whileCount, netMonitor.GetDisconnBySessionLimit(), netMonitor.GetDisconnByIOCPAssociation()
			, netMonitor.GetDisconnByKnownIoError(), netMonitor.GetDisconnBy121RecvIoError(), netMonitor.GetDisconnByUnknownIoError());

		// ������� ����
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  player lim:%lld, packet (code:%lld,  len:%lld,  decode:%lld,  type:%lld)"
			",  login fail:%lld,  dup:%lld,  account:%lld,  sector:%lld,  timeout (login:%lld,  heart:%lld)\n"
			, whileCount
			, monitor.GetDisconnByPlayerLimit(), netMonitor.GetDisconnByPacketCode(), netMonitor.GetDisconnByPacketLength(), netMonitor.GetDisconnByPacketDecode(), monitor.GetDisconnByInvalidMessageType()
			, monitor.GetDisconnByLoginFail(), monitor.GetDisconnByDupPlayer(), monitor.GetDisconnByInvalidAccountNo(), monitor.GetDisconnByInvalidSector(), monitor.GetDisconnByLoginTimeout(), monitor.GetDisconnByHeartBeatTimeout());

		// �޸�Ǯ. ��Ŷ, �޽���, �÷��̾�
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [pool   ]  packet:%d (alloc:%d, used:%d, free:%d),  message:%d (alloc:%d, used:%d, free:%d),  player:%d (alloc:%d, used:%d, free:%d)\n"
			, whileCount
			, server->GetPacketPoolSize(), server->GetPacketAllocCount(), server->GetPacketActualAllocCount(), server->GetPacketFreeCount()
			, server->GetMsgPoolSize(), server->GetMsgAllocCount(), server->GetMsgActualAllocCount(), server->GetMsgFreeCount()
			, server->GetPlayerPoolSize(), server->GetPlayerAllocCount(), server->GetPlayerActualAllocCount(), server->GetPlayerFreeCount());

		// �ý���
		const PDHCount& pdhCount = pdh.GetPDHCount();
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [system ]  TCP (segment:%lld/s, retrans:%lld/s, recv:%.2fMB/s, send:%.2fMB/s),  memory(MB) [System C:%.2f, P:%.2f, NP:%.2f] [Process C:%.2f, P:%.2f, NP:%.2f]\n"
			, whileCount
			, pdhCount.TCPSegmentsSent, pdhCount.TCPSegmentsRetransmitted, pdhCount.networkRecvBytes / 1048576.0, pdhCount.networkSendBytes / 1048576.0
			, (double)pdhCount.systemComittedBytes / 1048576.0, (double)pdhCount.systemPagedPoolBytes / 1048576.0, (double)pdhCount.systemNonpagedPoolBytes / 1048576.0
			, (double)pdhCount.processPrivateBytes / 1048576.0, (double)pdhCount.processPagedPoolBytes / 1048576.0, (double)pdhCount.processNonpagedPoolBytes / 1048576.0);

		// worker �����庰 GQCS wait �ð�, Ʈ���� ȥ������ Ƚ��
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
		prevEventWaitTime = currEventWaitTime;


		whileCount++;
		}
}