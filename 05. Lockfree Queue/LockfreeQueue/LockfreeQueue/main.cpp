/* main �Լ������� Lockfree Queue�� �׽�Ʈ �մϴ�. */

#pragma comment(lib, "Winmm.lib")
#include <iostream>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <sstream>
#include "CCrashDump.h"

//#define MEMPOOL_ENABLE_LOGGING
#include "CLockfreeQueue.h"

struct Object
{
	int _id;
	int _val;

	void SetValue(int id, int val) {
		_id = id;
		_val = val;
	}
	bool IsChanged(int id, int val)
	{
		if (_id == id && _val == val)
			return false;
		else
			return true;
	}
};

void Crash()
{
	int* p = 0;
	*p = 0;
}

std::vector<__int64> g_vecThreadProcCount;      // ������ �۾�Ƚ�� ��Ͽ� ����
CLockfreeQueue<Object*> g_queue;             // ������ ť
//CLockfreeQueueTLS<Object*> g_queue;             // ������ ť
volatile bool g_shutdown = false;            // ������ �����÷���
volatile unsigned __int64 g_deqFailCount = 0;  // debug. dequeue ���� �� ī��Ʈ
HANDLE g_hEvent;                               // �����带 ���ÿ� �����Ű������ �̺�Ʈ


// �۾� ������
unsigned WINAPI ThreadProc(PVOID pParam)
{
	static long sThreadNumber = -1;
	long threadNumber = InterlockedIncrement(&sThreadNumber);

	DWORD threadId = GetCurrentThreadId();
	printf("[Thread] begin %u\n", threadId);

	// object �迭 ���� �� object ����
	__int64 pushCount = (__int64)pParam;
	Object** arrPtrObject = new Object * [pushCount];
	for (int i = 0; i < pushCount; i++)
		arrPtrObject[i] = new Object;

	// �̺�Ʈ�� set�Ǳ⸦ ��ٸ���.
	WaitForSingleObject(g_hEvent, INFINITE);

	// �۾�����
	while (g_shutdown == false)
	{
		for (int i = 0; i < pushCount; i++)
		{
			g_queue.Enqueue(arrPtrObject[i]);
			g_vecThreadProcCount[threadNumber]++;
		}

		for (int i = 0; i < pushCount; i++)
		{
			while (g_queue.Dequeue(arrPtrObject[i]) == false)
				InterlockedIncrement(&g_deqFailCount);
				
			arrPtrObject[i]->SetValue(threadId, i);
			g_vecThreadProcCount[threadNumber]++;
		}

		//if (threadNumber == 1)  // sleep
		//	Sleep(1);
		for (int i = 0; i < pushCount; i++)
		{
			if (arrPtrObject[i]->IsChanged(threadId, i) == true)
				Crash();
		}
	}

	printf("[Thread] end %u\n", threadId);
	return 0;
}



void OutputLog(void* param);

int main()
{
	timeBeginPeriod(1);
	CCrashDump::Init();

	/*
	(��)
	pushCount   �������       alignas         ���proc
		 1000         1             x         29035273
		 1000         2             x          5705456
		 1000         8             x          3622379
		 1000         1          tail         29183792
	     1000         2          tail          5384140
	     1000         8          tail          3865827
		 1000         1    tail, head         28791438
		 1000         2    tail, head          5041984
		 1000         8    tail, head          3565146

	(�п�)
	pushCount   �������       alignas         ���proc
		 1000         1             x          26760800
		 1000         2             x          14114703
		 1000         4             x           9695595
		 1000         1          tail          26292612
		 1000         2          tail          14668106
		 1000         4          tail           9177340
		 1000         1  tail,size,pool        26667958
		 1000         2  tail,size,pool        14669606
		 1000         4  tail,size,pool        10018690
	
	
	*/

	const int numThread = 4;
	__int64 pushCount = 4;

	// �̺�Ʈ ����
	g_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// ������ť���� ����ϴ� �޸�Ǯ ��带 �̸� �����ص�
	Object** arrPtrObject = new Object * [numThread * pushCount];
	for (int i = 0; i < numThread * pushCount; i++)
		arrPtrObject[i] = new Object;
	for (int i = 0; i < numThread * pushCount; i++)
		g_queue.Enqueue(arrPtrObject[i]);
	for (int i = 0; i < numThread * pushCount; i++)
		g_queue.Dequeue(arrPtrObject[i]);
	for (int i = 0; i < numThread * pushCount; i++)
		delete arrPtrObject[i];
	delete[] arrPtrObject;


	// ������ ����
	HANDLE hThreads[numThread];
	unsigned int* threadIds = new unsigned int[numThread];
	for (int i = 0; i < numThread; i++)
	{
		g_vecThreadProcCount.push_back(0);
		hThreads[i] = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, (PVOID)pushCount, 0, &threadIds[i]);
	}

	// CCrashDump�� �α� ��� �۾� ���
	void** paramFinalJob = new void* [3];
	paramFinalJob[0] = (void*)&g_queue;
	paramFinalJob[1] = (void*)threadIds;
	paramFinalJob[2] = (void*)numThread;
	CCrashDump::AddFinalJob(OutputLog, paramFinalJob);


	// 3�� �� �̺�Ʈ set
	Sleep(3000);
	SetEvent(g_hEvent);

	std::wstringstream ss;
	ULONGLONG startTick = GetTickCount64();
	ULONGLONG currentTick;
	long long totalThreadProcCount = 0;
	long long prevTotalThreadProcCount = 0;
	std::vector<__int64> vecThreadProcCount;
	while (true)
	{
		// sleep
		Sleep(1000);
		// 30���� ����������
		//g_shutdown = true;
		currentTick = GetTickCount64();

		// ������ �۾�Ƚ�� ��������
		vecThreadProcCount.clear();
		totalThreadProcCount = 0;
		for (int i = 0; i < numThread; i++)
		{
			vecThreadProcCount.push_back(g_vecThreadProcCount[i]);
			totalThreadProcCount += vecThreadProcCount[i];
		}

		// �α׹��ڿ� ����
		ss.str(L"");
		ss << L"[" << (currentTick - startTick) / 1000 << L" sec] Queue size:" << g_queue.Size() << L",  Node pool size:" << g_queue.GetPoolSize()
			<< L",  Total run count: " << totalThreadProcCount;
		ss << L" (avg:" << (int)((double)totalThreadProcCount / ((double)(currentTick - startTick) / 1000.0)) << "/s),  Per thread run count: (";
		for (int i = 0; i < numThread; i++)
		{
			ss << vecThreadProcCount[i] << L", ";
			totalThreadProcCount += vecThreadProcCount[i];
		}
		ss.seekp(-2, std::ios_base::end); // ������ ", " ���ڿ� ����
		ss << L")\n";

		//ss << L", try try/fail Enq [CAS1:(" << g_queue._failEnqCAS1 + g_queue._succEnqCAS1 << L", " << (double)g_queue._failEnqCAS1 / (double)(g_queue._failEnqCAS1 + g_queue._succEnqCAS1);
		//ss << L"), CAS2:(" << g_queue._failEnqCAS2 + g_queue._succEnqCAS2 << L", " << (double)g_queue._failEnqCAS2 / (double)(g_queue._failEnqCAS2 + g_queue._succEnqCAS2);
		//ss << L"), CAS3:(" << g_queue._failEnqCAS3 + g_queue._succEnqCAS3 << L", " << (double)g_queue._failEnqCAS3 / (double)(g_queue._failEnqCAS3 + g_queue._succEnqCAS3);
		//ss << L")], Deq [wait:" << g_queue._waitDeq;
		//ss << L", CAS1:(" << g_queue._failDeqCAS1 + g_queue._succDeqCAS1 << L", " << (double)g_queue._failDeqCAS1 / (double)(g_queue._failDeqCAS1 + g_queue._succDeqCAS1);
		//ss << L"), CAS2:(" << g_queue._failDeqCAS2 + g_queue._succDeqCAS2 << L", " << (double)g_queue._failDeqCAS2 / (double)(g_queue._failDeqCAS2 + g_queue._succDeqCAS2) << L")]\n";
		prevTotalThreadProcCount = totalThreadProcCount;


		// print
		std::wcout << ss.str();


		// debug
		if (g_queue.GetPoolSize() > numThread * pushCount + 1)
			Crash();
		//printf("pool size:%d\n", g_queue.GetPoolSize());

		// Ű�����Է� ����
		if (GetConsoleWindow() == GetForegroundWindow())
		{
			if (GetAsyncKeyState('Q') || GetAsyncKeyState('q'))
			{
				printf("shutdown\n");
				g_shutdown = true;
				Sleep(1000);
				break;
			}
			if (GetAsyncKeyState('E') || GetAsyncKeyState('e'))
			{
				printf("raise error\n");
				Crash();
				break;
			}
		}

	}

	return 0;
}












void OutputLog(void* param)
{

	// �Ķ���� ���
	void** params = (void**)param;
	CLockfreeQueue<Object*>& queue = *(CLockfreeQueue<Object*>*)params[0];
	DWORD* threadIds = (DWORD*)params[1];
	int numThread = (int)params[2];


	// �� �����尡 �ƴ� �ٸ� �����带 ������Ų��.
	DWORD myThreadId = GetCurrentThreadId();
	HANDLE hThread;
	for (int i = 0; i < numThread; i++)
	{
		if (threadIds[i] != myThreadId)
		{
			hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadIds[i]);
			SuspendThread(hThread);
		}
	}

#ifdef QUEUE_ENABLE_LOGGING
	// stack �α� ���
	queue.OutputThreadLog();
	wprintf(L"finisned output queue log\n");

#ifdef MEMPOOL_ENABLE_LOGGING
	// memory pool �α� ���
	queue.OutputMemoryPoolLog();
	wprintf(L"finisned output memory pool log\n");
#endif
#endif
}