/*
����͸� ����
*/

#include "CMonitoringServer.h"
#include "../utils/CCrashDump.h"
#include "../utils/logger.h"

using namespace monitorserver;
void ConsoleOutServerState(std::shared_ptr<CMonitoringServer> server);


int main()
{
	std::shared_ptr<CMonitoringServer> server(new CMonitoringServer());
	bool retStart = server->StartUp();
	ConsoleOutServerState(server);

	return 0;
}



void ConsoleOutServerState(std::shared_ptr<CMonitoringServer> server)
{
	CCrashDump::Init();

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
		}

		// 1�ʸ��� �α� ���
		if (GetTickCount64() - tick < 1000)
			continue;
		tick += 1000;

		const netlib::CNetServer::Monitor& netMonitor = server->GetNetworkMonitor();
		const CMonitoringServer::Monitor& monitor = server->GetMonitor();
		__int64 currAcceptCount = netMonitor.GetAcceptCount();                     // accept Ƚ��
		__int64 currConnectCount = netMonitor.GetConnectCount();                   // connect Ƚ�� (accept �� connect ���ε� Ƚ��)
		__int64 currDisconnectCount = netMonitor.GetDisconnectCount();             // disconnect Ƚ�� (���� release Ƚ��)
		__int64 currRecvCount = netMonitor.GetRecvCount();                         // WSARecv �Լ� ȣ�� Ƚ��      
		__int64 currSendCount = netMonitor.GetSendCount();                         // WSASend �Լ� ȣ�� Ƚ��
		__int64 currRecvCompletionCount = netMonitor.GetRecvCompletionCount();     // recv �Ϸ����� ó��Ƚ��
		__int64 currSendCompletionCount = netMonitor.GetSendCompletionCount();     // send �Ϸ����� ó��Ƚ��
		__int64 currMsgHandleCount = monitor.GetMsgHandleCount();               // ����͸����� ������ �޽��� ó�� Ƚ��
		__int64 currQueryRunCount = server->GetQueryRunCount();                 // ���� ���� Ƚ��

		// ���� ���� ��, WSASend ȣ��Ƚ��, WSARecv ȣ��Ƚ��, recv �Ϸ����� ó�� Ƚ��, send �Ϸ����� ó��Ƚ�� ���
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [network]  session:%d,  accept:%lld (%lld/s),  conn:%lld (%lld/s),  disconn:%lld (%lld/s),  recv:%lld/s,  send:%lld/s,  recvComp:%lld/s,  sendComp:%lld/s\n"
			, whileCount, server->GetNumSession()
			, currAcceptCount, currAcceptCount - prevAcceptCount
			, currConnectCount, currConnectCount - prevConnectCount
			, currDisconnectCount, currDisconnectCount - prevDisconnectCount
			, currRecvCount - prevRecvCount, currSendCount - prevSendCount
			, currRecvCompletionCount - prevRecvCompletionCount, currSendCompletionCount - prevSendCompletionCount);

		// ����͸�����
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [server ]  user:%d, WAN user:%d,  msg proc:%lld/s,  query:%lld/s,  query time:(avg:%.1fms, min:%.1fms, max:%.1fms),  error:%lld\n"
			, whileCount
			, server->GetNumUser(), server->GetNumWANUser(), currMsgHandleCount - prevMsgHandleCount
			, currQueryRunCount - prevQueryRunCount, server->GetAvgQueryRunTime()
			, server->GetMinQueryRunTime() == FLT_MAX ? 0.f : server->GetMinQueryRunTime(), server->GetMaxQueryRunTime()
			, monitor.GetReceivedInvalidDatatype());

		// ������� ����
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [disconn]  sess lim:%lld,  user lim:%lld,  io err (known:%lld, unknown:%lld),  packet:%lld,  login key:%lld,  WAN update:%lld\n"
			, whileCount, netMonitor.GetDisconnBySessionLimit(), monitor.GetDisconnByUserLimit()
			, netMonitor.GetDisconnByKnownIoError(), netMonitor.GetDisconnByUnknownIoError()
			, netMonitor.GetDisconnByPacketCode() + netMonitor.GetDisconnByPacketLength() + netMonitor.GetDisconnByPacketDecode() + monitor.GetDisconnByInvalidMessageType()
			, monitor.GetDisconnByLoginKeyMismatch(), monitor.GetDisconnByWANUserUpdateRequest());

		// �޸�Ǯ. ��Ŷ, �޽���, �÷��̾�
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [pool   ]  packet:%d (alloc:%d, used:%d, free:%d),  user:%d (alloc:%d, used:%d, free:%d)\n"
			, whileCount
			, server->GetPacketPoolSize(), server->GetPacketAllocCount(), server->GetPacketActualAllocCount(), server->GetPacketFreeCount()
			, server->GetUserPoolSize(), server->GetUserAllocCount(), server->GetUserFreeCount());

		// DB
		LOGGING(LOGGING_LEVEL_INFO, L"%lld [DB     ]  query count:%d/s,  query run time (avg:%.1fms , max:%.1fms , min:%.1fms),  error:%lld\n\n"
			, whileCount, currQueryRunCount - prevQueryRunCount
			, server->GetAvgQueryRunTime(), server->GetMaxQueryRunTime()
			, server->GetMinQueryRunTime() == FLT_MAX ? 0.f : server->GetMinQueryRunTime()
			, monitor.GetDBErrorCount());


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
}