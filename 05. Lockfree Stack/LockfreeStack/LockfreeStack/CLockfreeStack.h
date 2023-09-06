#pragma once
#include <Windows.h>
#include "CMemoryPool.h"

//#define STACK_ENABLE_LOGGING


using TYPE_ID = unsigned __int64;

#define STACK_GET_ADDRESS(Id) (Node*)((Id) & _addressMask)
#define STACK_GET_PROC_COUNT(Id) (int)((Id) >> _addressBitCount)
#define STACK_MAKE_ID(procCount, address) (((TYPE_ID)(procCount) << _addressBitCount) | (TYPE_ID)(address))


template <typename _Ty>
class alignas(64) CLockfreeStack
{
private:
	struct Node
	{
		Node* pNext;
		_Ty obj;
	};

	TYPE_ID _topId;             // 64byte aligned
	alignas(64) long _size;     // 64byte aligned

	alignas(64) CMemoryPool<Node> _poolNode;
	TYPE_ID _addressMask;   // TYPE_ID ������ �ּҰ��� ������� mask
	int _addressBitCount;   // �������� �ּ��� bit ��. TYPE_ID ������ pop count�� ������� ����

public:
	CLockfreeStack();
	~CLockfreeStack();

public:
	int Size() { return _size; }
	int GetPoolSize() { return _poolNode.GetPoolSize(); }

	void Push(_Ty obj);
	bool Pop(_Ty& obj);

	// 64byte aligned ��ü ������ ���� new, delete overriding
	void* operator new(size_t size);
	void operator delete(void* p);

#ifdef STACK_ENABLE_LOGGING
public:
	enum class eWorkType
	{
		PUSH,
		POP
	};
	struct ThreadLog
	{
		__int64 index;
		DWORD threadId;
		eWorkType workType;
		Node* topNode;      // ���� top
		Node* topNextNode;  // ���� top�� next
		Node* otherNode;    // PUSH�� ��� ���� top�� next, POP�� ��� pop�� ���
	};

	alignas(64) int _sizeLogBuffer;
	ThreadLog* _logBuffer;
	alignas(64) __int64 _logBufferIndex;

	void Crash()
	{
		int* p = 0;
		*p = 0;
	}

	void OutputThreadLog();
	void OutputMemoryPoolLog() { _poolNode.OutputThreadLog(); }
#endif
};



template <typename _Ty>
CLockfreeStack<_Ty>::CLockfreeStack()
	:_topId(0), _size(0), _poolNode(0, true, false), _addressBitCount(0)
{
	// �ּ� mask ���
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	_addressMask = (TYPE_ID)sysInfo.lpMaximumApplicationAddress;
	for (int i = 63; i >= 0; i--)
	{
		if (_addressMask & (1i64 << i))
		{
			_addressMask = 0xFFFFFFFF'FFFFFFFF >> (63 - i);  // 0x00007fff'fffeffff
			_addressBitCount = i + 1;                        // 47
			break;
		}
	}

#ifdef STACK_ENABLE_LOGGING
	_sizeLogBuffer = 0x1ffff;  // 2���� ���� ��� 1�� �̷������ ��
	_logBuffer = (ThreadLog*)malloc(sizeof(ThreadLog) * _sizeLogBuffer);
	memset(_logBuffer, 0, sizeof(ThreadLog) * _sizeLogBuffer);
	_logBufferIndex = -1;
#endif
}


template <typename _Ty>
CLockfreeStack<_Ty>::~CLockfreeStack()
{
	// ��� ��带 free �Ѵ�.
	Node* pNode = STACK_GET_ADDRESS(_topId);
	Node* pNext;
	while (pNode != nullptr)
	{
		_size--;
		pNext = pNode->pNext;
		_poolNode.Free(pNode);
		pNode = pNext;
	}

	// �޸�Ǯ�� �Ҹ��ڿ��� ��� ��带 delete �Ѵ�.
}



template <typename _Ty>
void CLockfreeStack<_Ty>::Push(_Ty obj)
{
	Node* pNewNode = _poolNode.Alloc();
	pNewNode->obj = obj;

	TYPE_ID topId;
	TYPE_ID newNodeId;
	Node* pTop;
	do {
		topId = _topId;
		pTop = STACK_GET_ADDRESS(topId);
		pNewNode->pNext = pTop;
		newNodeId = STACK_MAKE_ID(STACK_GET_PROC_COUNT(topId) + 1, pNewNode);
	} while (InterlockedCompareExchange64((__int64*)&_topId, (__int64)newNodeId, (__int64)topId) != (__int64)topId);  // Push

#ifdef STACK_ENABLE_LOGGING
	// Interlock �Լ��� Push�� ������ ���� �޸𸮿� �α׸� ����Ѵ�.
	__int64 logBufferIndex = InterlockedIncrement64(&_logBufferIndex);     // �α� ��ȣ ���
	int index = (int)(logBufferIndex & (__int64)_sizeLogBuffer);           // �α׸� ����� �迭 �ε��� ���
	_logBuffer[index].index = logBufferIndex;                              // �α� �迭�� �α� ��ȣ ���
	_logBuffer[index].threadId = GetCurrentThreadId();                     //            ������ID ���
	_logBuffer[index].workType = eWorkType::PUSH;                          //            �۾� ����(Push) ���
	_logBuffer[index].topNode = pNewNode;                                  //            ���� top ��� �ּ� ���
	_logBuffer[index].topNextNode = pNewNode->pNext;                       //            ���� top->next ��� �ּ� ���
	_logBuffer[index].otherNode = pTop == nullptr ? nullptr : pTop->pNext; //            ���� top ��� �ּ� ���
#endif

	InterlockedIncrement(&_size);
}


template <typename _Ty>
bool CLockfreeStack<_Ty>::Pop(_Ty& obj)
{
	TYPE_ID topId;
	TYPE_ID newTopId;
	Node* pTop;
	Node* pNewTop;

	if (_size <= 0)
		return false;
	long size = InterlockedDecrement(&_size);
	if (size < 0)
	{
		InterlockedIncrement(&_size);
		return false;
	}

	do {
		topId = _topId;
		pTop = STACK_GET_ADDRESS(topId);
		pNewTop = pTop->pNext;
		newTopId = STACK_MAKE_ID(STACK_GET_PROC_COUNT(topId) + 1, pNewTop);
	} while (InterlockedCompareExchange64((__int64*)&_topId, (__int64)newTopId, (__int64)topId) != (__int64)topId);

#ifdef STACK_ENABLE_LOGGING
	__int64 logBufferIndex = InterlockedIncrement64(&_logBufferIndex);
	int index = (int)(logBufferIndex & (__int64)_sizeLogBuffer);
	_logBuffer[index].index = logBufferIndex;
	_logBuffer[index].threadId = GetCurrentThreadId();
	_logBuffer[index].workType = eWorkType::POP;
	_logBuffer[index].topNode = pNewTop;
	_logBuffer[index].topNextNode = pNewTop == nullptr ? nullptr : pNewTop->pNext;
	_logBuffer[index].otherNode = pTop;
#endif

	obj = pTop->obj;
	_poolNode.Free(pTop);
	return true;
}



// 64byte aligned ��ü ������ ���� new, delete overriding
template <typename _Ty>
void* CLockfreeStack<_Ty>::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}
template <typename _Ty>
void CLockfreeStack<_Ty>::operator delete(void* p)
{
	_aligned_free(p);
}





/*
�������
@ 20221208
Interlock�� ���Ǵ� _topId, _size ����� ��ġ�� �� ���� �ű�� alignas(64) ��.

�׸��� �޸�Ǯ�� 64byte ��迡 �����Ǵ°��� �����ϱ����� new, delete �Լ��� �������̵���
	void* operator new(size_t size);
	void operator delete(void* p);

�׽�ƮCPU: Intel(R) Core(TM) i5-7500 CPU @ 3.40GHz
�׽�Ʈ����: 5�� push, pop �ϴ°��� 30�ʵ��� �ݺ��Ͽ� �� ���� Ƚ���� ����
# alignas ������, 4������
[30 sec] stack size:13, thread proc count: (151540, 74652890, 77443153, 75436301, )
[30 sec] stack size:9, thread proc count: (151070, 75477021, 75276392, 77536356, )
[30 sec] stack size:10, thread proc count: (151660, 74034551, 75360611, 76260951, )
[30 sec] stack size:9, thread proc count: (151240, 76835561, 75547718, 75325760, )
# alignas ����, 4������
[30 sec] stack size:9, thread proc count: (151800, 81863963, 82414467, 82377860, )
[30 sec] stack size:5, thread proc count: (151030, 82027166, 82167928, 82490583, )
[30 sec] stack size:11, thread proc count: (151710, 82719828, 81855998, 83242741, )
[30 sec] stack size:9, thread proc count: (151090, 83410171, 83857833, 82899067, )
*/




#ifdef STACK_ENABLE_LOGGING
template <typename _Ty>
void CLockfreeStack<_Ty>::OutputThreadLog()
{
	// ���� ��¥�� �ð��� �˾ƿ´�.
	WCHAR filename[MAX_PATH];
	SYSTEMTIME stNowTime;
	GetLocalTime(&stNowTime);
	wsprintf(filename, L"CLockfreeStack_%d%02d%02d_%02d%02d%02d.txt",
		stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);

	// ���� �ε��� ���
	int sizeLogBuffer = _sizeLogBuffer;
	__int64 logBufferIndex = _logBufferIndex;
	int startIndex;
	int endIndex = (int)(logBufferIndex + 1 & (__int64)sizeLogBuffer);
	if (logBufferIndex < sizeLogBuffer)
		startIndex = 0;
	else
		startIndex = (int)((logBufferIndex + 1) & (__int64)sizeLogBuffer);

	// ���� ����
	HANDLE hLogFile = ::CreateFile(filename,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	// �α� ���
	int index = startIndex;
	wchar_t szLog[1000];
	int lenSzLog;
	bool isFirst = true;
	while (isFirst == true || index != endIndex)
	{
		isFirst = false;

		ThreadLog& log = _logBuffer[index];

		if (log.workType == eWorkType::PUSH)
			lenSzLog = swprintf_s(szLog, 1000, L"#%llu  PUSH  id: %5u, old top: %p        -> top: %p (alloc)\n"
				, log.index, log.threadId, log.topNextNode, log.topNode);
		else if (log.workType == eWorkType::POP)
			lenSzLog = swprintf_s(szLog, 1000, L"#%llu  POP   id: %5u, old top: %p (free) -> top: %p, next: %p\n"
				, log.index, log.threadId, log.otherNode, log.topNode, log.topNextNode);
		else
			lenSzLog = swprintf_s(szLog, 1000, L"error, type:%d\n", (int)log.workType);

		WriteFile(hLogFile, szLog, lenSzLog * sizeof(wchar_t), NULL, NULL);

		index++;
		if (index >= sizeLogBuffer)
			index = 0;
	}

	CloseHandle(hLogFile);
}
#endif