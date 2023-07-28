/*
����͸� ����
*/

#include "CMonitoringServer.h"
#include "CCrashDump.h"
#include "logger.h"


int main()
{
	CCrashDump::Init();
	timeBeginPeriod(1);

	CMonitoringServer& server = *new CMonitoringServer();
	bool retStart = server.StartUp();
	if (retStart == false)
		return 0;

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

	while (true)
	{
		Sleep(50);

		if (GetConsoleWindow() == GetForegroundWindow())
		{
			if (GetAsyncKeyState('E') || GetAsyncKeyState('e'))
			{
				printf("[server] terminate chat server\n");
				server.Shutdown();
				while (server.IsTerminated() == false)
				{
					Sleep(100);
				}
				break;
			}
			else if (GetAsyncKeyState('C') || GetAsyncKeyState('c'))
			{
				int* p = 0;
				*p = 0;
				break;
			}
		}

		// 1�ʸ��� �α� ���
		if (GetTickCount64() - tick < 1000)
			continue;
		tick += 1000;

		__int64 currAcceptCount = server.GetAcceptCount();                     // accept Ƚ��
		__int64 currConnectCount = server.GetConnectCount();                   // connect Ƚ�� (accept �� connect ���ε� Ƚ��)
		__int64 currDisconnectCount = server.GetDisconnectCount();             // disconnect Ƚ�� (���� release Ƚ��)
		__int64 currRecvCount = server.GetRecvCount();                         // WSARecv �Լ� ȣ�� Ƚ��      
		__int64 currSendCount = server.GetSendCount();                         // WSASend �Լ� ȣ�� Ƚ��
		__int64 currRecvCompletionCount = server.GetRecvCompletionCount();     // recv �Ϸ����� ó��Ƚ��
		__int64 currSendCompletionCount = server.GetSendCompletionCount();     // send �Ϸ����� ó��Ƚ��
		__int64 currMsgHandleCount = server.GetMsgHandleCount();               // ����͸����� ������ �޽��� ó�� Ƚ��
		__int64 currQueryRunCount = server.GetQueryRunCount();                 // ���� ���� Ƚ��

		// ���� ���� ��, WSASend ȣ��Ƚ��, WSARecv ȣ��Ƚ��, recv �Ϸ����� ó�� Ƚ��, send �Ϸ����� ó��Ƚ�� ���
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [network]  session:%d,  accept:%lld (%lld/s),  conn:%lld (%lld/s),  disconn:%lld (%lld/s),  recv:%lld/s,  send:%lld/s,  recvComp:%lld/s,  sendComp:%lld/s\n"
			, whileCount, server.GetNumSession()
			, currAcceptCount, currAcceptCount - prevAcceptCount
			, currConnectCount, currConnectCount - prevConnectCount
			, currDisconnectCount, currDisconnectCount - prevDisconnectCount
			, currRecvCount - prevRecvCount, currSendCount - prevSendCount
			, currRecvCompletionCount - prevRecvCompletionCount, currSendCompletionCount - prevSendCompletionCount);

		// ����͸�����
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [server ]  user:%d, WAN user:%d,  msg proc:%lld/s,  query:%lld/s,  query time:(avg:%.1fms, min:%.1fms, max:%.1fms),  error:%lld\n"
			, whileCount
			, server.GetNumUser(), server.GetNumWANUser(), currMsgHandleCount - prevMsgHandleCount
			, currQueryRunCount - prevQueryRunCount, server.GetAvgQueryRunTime()
			, server.GetMinQueryRunTime() == FLT_MAX ? 0.f : server.GetMinQueryRunTime(), server.GetMaxQueryRunTime()
			, server.GetReceivedInvalidDatatype());

		// ������� ����
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  sess lim:%lld,  user lim:%lld,  io err (known:%lld, unknown:%lld),  packet:%lld,  login key:%lld,  WAN update:%lld\n"
			, whileCount, server.GetDisconnBySessionLimit(), server.GetDisconnByUserLimit()
			, server.GetDisconnByKnownIoError(), server.GetDisconnByUnknownIoError()
			, server.GetDisconnByPacketCode() + server.GetDisconnByPacketLength() + server.GetDisconnByPacketDecode() + server.GetDisconnByInvalidMessageType()
			, server.GetDisconnByLoginKeyMismatch(), server.GetDisconnByWANUserUpdateRequest());

		// �޸�Ǯ. ��Ŷ, �޽���, �÷��̾�
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [pool   ]  packet:%d (alloc:%d, used:%d, free:%d),  user:%d (alloc:%d, used:%d, free:%d)\n"
			, whileCount
			, server.GetPacketPoolSize(), server.GetPacketAllocCount(), server.GetPacketActualAllocCount(), server.GetPacketFreeCount()
			, server.GetUserPoolSize(), server.GetUserAllocCount(), server.GetUserFreeCount());

		// DB
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [DB     ]  query count:%d/s,  query run time (avg:%.1fms , max:%.1fms , min:%.1fms),  error:%lld\n\n"
			, whileCount, currQueryRunCount - prevQueryRunCount
			, server.GetAvgQueryRunTime(), server.GetMaxQueryRunTime()
			, server.GetMinQueryRunTime() == FLT_MAX ? 0.f : server.GetMinQueryRunTime()
			, server.GetDBErrorCount());


		prevAcceptCount = currAcceptCount;
		prevConnectCount = currConnectCount;
		prevDisconnectCount = currDisconnectCount;
		prevRecvCount = currRecvCount;
		prevSendCount = currSendCount;
		prevRecvCompletionCount = currRecvCompletionCount;
		prevSendCompletionCount = currSendCompletionCount;
		prevMsgHandleCount = currMsgHandleCount;
		prevQueryRunCount = currQueryRunCount;


		whileCount++;
	}


	WSACleanup();

	return 0;
}

