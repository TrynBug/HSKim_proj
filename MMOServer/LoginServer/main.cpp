/*
�α��μ���
redis�� ����Ͽ� ä�ü����� �����Ѵ�.
ä�ü����� LAN���� ��������� �ʴ´�.
*/

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


int main()
{
	CCrashDump::Init();
	timeBeginPeriod(1);
	CCpuUsage CPUTime;

	CPDH pdh;
	pdh.Init();

	CLoginServer& loginServer = *new CLoginServer();
	bool retStart = loginServer.StartUp();


	int numWorkerThread = loginServer.GetNumWorkerThread();
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
				loginServer.Shutdown();
				while (loginServer.IsTerminated() == false)
					continue;
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

		// 1�ʸ��� �α� ���
		if (GetTickCount64() - tick < 1000)
			continue;
		tick += 1000;
		CPUTime.UpdateCpuTime();
		pdh.Update();

		const netlib::CNetServer::Monitor& netMonitor = loginServer.GetNetworkMonitor();
		const CLoginServer::Monitor& monitor = loginServer.GetMonitor();
		__int64 currAcceptCount = netMonitor.GetAcceptCount();                     // accept Ƚ��
		__int64 currConnectCount = netMonitor.GetConnectCount();                   // connect Ƚ�� (accept �� connect ���ε� Ƚ��)
		__int64 currDisconnectCount = netMonitor.GetDisconnectCount();             // disconnect Ƚ�� (���� release Ƚ��)
		__int64 currRecvCount = netMonitor.GetRecvCount();                         // WSARecv �Լ� ȣ�� Ƚ��      
		__int64 currSendCount = netMonitor.GetSendCount();                         // WSASend �Լ� ȣ�� Ƚ��
		__int64 currRecvCompletionCount = netMonitor.GetRecvCompletionCount();     // recv �Ϸ����� ó��Ƚ��
		__int64 currSendCompletionCount = netMonitor.GetSendCompletionCount();     // send �Ϸ����� ó��Ƚ��
		__int64 currLoginCount = monitor.GetLoginCount();                          // �α��� ���� Ƚ��
		__int64 currQueryRunCount = loginServer.GetQueryRunCount();                 // ���� ����Ƚ��


		// ���� ���� ��, WSASend ȣ��Ƚ��, WSARecv ȣ��Ƚ��, recv �Ϸ����� ó�� Ƚ��, send �Ϸ����� ó��Ƚ�� ���
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [network]  session:%d,  accept:%lld (%lld/s),  conn:%lld (%lld/s),  disconn:%lld (%lld/s),  recv:%lld/s,  send:%lld/s,  recvComp:%lld/s,  sendComp:%lld/s\n"
			, whileCount, loginServer.GetNumSession()
			, currAcceptCount, currAcceptCount - prevAcceptCount
			, currConnectCount, currConnectCount - prevConnectCount
			, currDisconnectCount, currDisconnectCount - prevDisconnectCount
			, currRecvCount - prevRecvCount, currSendCount - prevSendCount
			, currRecvCompletionCount - prevRecvCompletionCount, currSendCompletionCount - prevSendCompletionCount);

		// �α��μ���
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [server ]  client:%d,  account:%d,  login:%lld/s"
			",  CPU usage [T:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]  [Server:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]\n"
			, whileCount
			, loginServer.GetNumClient(), loginServer.GetSizeAccountMap(), currLoginCount - prevLoginCount
			, CPUTime.ProcessorTotal(), CPUTime.ProcessorKernel(), CPUTime.ProcessorUser(), CPUTime.ProcessTotal(), CPUTime.ProcessKernel(), CPUTime.ProcessUser());

		// ������� ����
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

		// �޸�Ǯ. ��Ŷ, �޽���, �÷��̾�
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [pool   ]  packet:%d (alloc:%d, used:%d, free:%d),  client:%d (alloc:%d, used:%d, free:%d)\n"
			, whileCount
			, loginServer.GetPacketPoolSize(), loginServer.GetPacketAllocCount(), loginServer.GetPacketActualAllocCount(), loginServer.GetPacketFreeCount()
			, loginServer.GetClientPoolSize(), loginServer.GetClientAllocCount(), loginServer.GetClientActualAllocCount(), loginServer.GetClientFreeCount());

		// DB
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [DB     ]  connection:%d,  query count:%d/s,  query run time (avg:%.1fms , max:%.1fms , min:%.1fms)\n"
			, whileCount, loginServer.GetNumDBConnection(), currQueryRunCount - prevQueryRunCount
			, loginServer.GetAvgQueryRunTime(), loginServer.GetMaxQueryRunTime()
			, loginServer.GetMinQueryRunTime() == FLT_MAX ? 0.f : loginServer.GetMinQueryRunTime());

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
		oss << whileCount << L" [etc    ]  traffic control:" << netMonitor.GetTrafficCongestionControlCount();
		oss << L",  deferred closesocket:" << netMonitor.GetDeferredDisconnectCount() << L" (msgQ:" << loginServer.GetSizeMsgQDeferredCloseSocket();
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


	WSACleanup();

	return 0;
}

