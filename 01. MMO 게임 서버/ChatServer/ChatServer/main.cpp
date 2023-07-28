/*
�̱۽����� ä�ü���
*/


#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <list>
#include <unordered_map>
#include <string>
#include <sstream>

#include "CCrashDump.h"
#include "CChatServer.h"

#include "profiler.h"
#include "logger.h"
#include "CCpuUsage.h"
#include "CPDH.h"

void OutputMemoryLog(void* param)
{
	CChatServer* pChatServer = (CChatServer*)param;
#ifdef NET_ENABLE_MEMORY_LOGGING
	pChatServer->OutputMemoryLog();
#endif
}

int main()
{
	CCrashDump::Init();
	timeBeginPeriod(1);
	CCpuUsage CPUTime;

	CPDH pdh;
	pdh.Init();

	CChatServer& chatServer = *new CChatServer();
	bool retStart = chatServer.StartUp();

	// CCrashDump�� �α� ��� �۾� ���
	CCrashDump::AddFinalJob(OutputMemoryLog, &chatServer);

	int numWorkerThread = chatServer.GetNumWorkerThread();
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
	int* arrPrevGQCSWaitTime = new int[numWorkerThread];
	int* arrCurrGQCSWaitTime = new int[numWorkerThread];

	while (true)
	{
		Sleep(50);

		if (GetConsoleWindow() == GetForegroundWindow())
		{
			if (GetAsyncKeyState('E') || GetAsyncKeyState('e'))
			{
				printf("[server] terminate chat server\n");
				chatServer.Shutdown();
				while (chatServer.IsTerminated() == false)
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
			else if (GetAsyncKeyState('O') || GetAsyncKeyState('o'))
			{
				profiler::OutputProfilingData();
				printf("output profiler\n");
			}
#ifdef NET_ENABLE_MEMORY_LOGGING
			else if (GetAsyncKeyState('N') || GetAsyncKeyState('n'))
			{
				printf("output memory log\n");
				OutputMemoryLog(&chatServer);
				
			}
#endif
		}

		// 1�ʸ��� �α� ���
		if (GetTickCount64() - tick < 1000)
			continue;
		tick += 1000;
		CPUTime.UpdateCpuTime();
		pdh.Update();
		

		__int64 currAcceptCount = chatServer.GetAcceptCount();                     // accept Ƚ��
		__int64 currConnectCount = chatServer.GetConnectCount();                   // connect Ƚ�� (accept �� connect ���ε� Ƚ��)
		__int64 currDisconnectCount = chatServer.GetDisconnectCount();             // disconnect Ƚ�� (���� release Ƚ��)
		__int64 currRecvCount = chatServer.GetRecvCount();                         // WSARecv �Լ� ȣ�� Ƚ��      
		__int64 currSendCount = chatServer.GetSendCount();                         // WSASend �Լ� ȣ�� Ƚ��
		__int64 currRecvCompletionCount = chatServer.GetRecvCompletionCount();     // recv �Ϸ����� ó��Ƚ��
		__int64 currSendCompletionCount = chatServer.GetSendCompletionCount();     // send �Ϸ����� ó��Ƚ��
		__int64 currMsgHandleCount = chatServer.GetMsgHandleCount();               // ä�ü��� ������ �޽��� ó�� Ƚ��
		__int64 currEventWaitTime = chatServer.GetEventWaitTime();                 // ä�ü��� WaitForSingleObject �Լ� ȣ��Ƚ��
		chatServer.GetArrGQCSWaitTime(arrCurrGQCSWaitTime);                        // worker������ GQCS wait time 

		// ���� ���� ��, WSASend ȣ��Ƚ��, WSARecv ȣ��Ƚ��, recv �Ϸ����� ó�� Ƚ��, send �Ϸ����� ó��Ƚ�� ���
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [network]  session:%d,  accept:%lld (%lld/s),  conn:%lld (%lld/s),  disconn:%lld (%lld/s),  recv:%lld/s,  send:%lld/s,  recvComp:%lld/s,  sendComp:%lld/s\n"
			, whileCount, chatServer.GetNumSession()
			, currAcceptCount, currAcceptCount - prevAcceptCount
			, currConnectCount, currConnectCount - prevConnectCount
			, currDisconnectCount, currDisconnectCount - prevDisconnectCount
			, currRecvCount - prevRecvCount, currSendCount - prevSendCount
			, currRecvCompletionCount - prevRecvCompletionCount, currSendCompletionCount - prevSendCompletionCount);

		// ä�ü���
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [server ]  player:%d, account:%d,  msg proc:%lld/s,  msg remain:%d,  event wait:%lldms"
			",  CPU usage [T:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]  [Server:%.1f%%%%, U:%.1f%%%%, K:%.1f%%%%]\n"
			, whileCount
			, chatServer.GetNumPlayer(), chatServer.GetNumAccount(), currMsgHandleCount - prevMsgHandleCount
			, chatServer.GetUnhandeledMsgCount(), (__int64)((double)(currEventWaitTime - prevEventWaitTime) / (double)(liFrequency.QuadPart / 1000))
			, CPUTime.ProcessorTotal(), CPUTime.ProcessorKernel(), CPUTime.ProcessorUser(), CPUTime.ProcessTotal(), CPUTime.ProcessKernel(), CPUTime.ProcessUser());

		// ������� ����
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  sess lim:%lld,  iocp assoc:%lld,  io err (known:%lld, 121:%lld, unknown:%lld)\n"
			, whileCount, chatServer.GetDisconnBySessionLimit(), chatServer.GetDisconnByIOCPAssociation()
			, chatServer.GetDisconnByKnownIoError(), chatServer.GetDisconnBy121RecvIoError(), chatServer.GetDisconnByUnknownIoError());

		// ������� ����
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  player lim:%lld, packet (code:%lld,  len:%lld,  decode:%lld,  type:%lld)"
			",  login fail:%lld,  dup:%lld,  account:%lld,  sector:%lld,  timeout (login:%lld,  heart:%lld)\n"
			, whileCount
			, chatServer.GetDisconnByPlayerLimit(), chatServer.GetDisconnByPacketCode(), chatServer.GetDisconnByPacketLength(), chatServer.GetDisconnByPacketDecode(), chatServer.GetDisconnByInvalidMessageType()
			, chatServer.GetDisconnByLoginFail(), chatServer.GetDisconnByDupPlayer(), chatServer.GetDisconnByInvalidAccountNo(), chatServer.GetDisconnByInvalidSector(), chatServer.GetDisconnByLoginTimeout(), chatServer.GetDisconnByHeartBeatTimeout());

		// �޸�Ǯ. ��Ŷ, �޽���, �÷��̾�
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [pool   ]  packet:%d (alloc:%d, used:%d, free:%d),  message:%d (alloc:%d, used:%d, free:%d),  player:%d (alloc:%d, used:%d, free:%d)\n"
			, whileCount
			, chatServer.GetPacketPoolSize(), chatServer.GetPacketAllocCount(), chatServer.GetPacketActualAllocCount(), chatServer.GetPacketFreeCount()
			, chatServer.GetMsgPoolSize(), chatServer.GetMsgAllocCount(),  chatServer.GetMsgActualAllocCount(), chatServer.GetMsgFreeCount()
			, chatServer.GetPlayerPoolSize(), chatServer.GetPlayerAllocCount(), chatServer.GetPlayerActualAllocCount(), chatServer.GetPlayerFreeCount());
		
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
		oss << whileCount << L" [etc    ]  GQCS wait time(ms): (";
		for (int i = 0; i < numWorkerThread; i++)
		{
			if(arrCurrGQCSWaitTime[i] - arrPrevGQCSWaitTime[i] == 0)
				oss << L"?, ";
			else
				oss << arrCurrGQCSWaitTime[i] - arrPrevGQCSWaitTime[i] << L", ";
		}
		oss.seekp(-2, std::ios_base::end); // ������ ", " ���ڿ� ����
		oss << L"),  traffic control:" << chatServer.GetTrafficCongestionControlCount();
		oss << L",  error:" << chatServer.GetOtherErrorCount() << L"\n\n";
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
		memcpy(arrPrevGQCSWaitTime, arrCurrGQCSWaitTime, sizeof(int)* numWorkerThread);


		whileCount++;
	}


	WSACleanup();

	return 0;
}

