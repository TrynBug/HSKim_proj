#pragma once

/* lockfree �޸� Ǯ */

#include <stdlib.h>
#include <Windows.h>

//#define MEMPOOL_ENABLE_LOGGING

using TYPE_ID = unsigned __int64;

#define MEMPOOL_GET_ADDRESS(Id) (BlockNode*)((Id) & _addressMask)
#define MEMPOOL_GET_ALLOC_COUNT(Id) (int)((Id) >> _addressBitCount)
#define MEMPOOL_MAKE_ID(address) (((TYPE_ID)(address->allocCount) << _addressBitCount) | (TYPE_ID)(address))

namespace netserver
{

template <typename _Ty>
class alignas(64) CMemoryPool
{
private:
	struct BlockNode;

	TYPE_ID _topId;  // ���� ��밡���� ����� �ּҿ� alloc Ƚ���� ��ģ �� (64byte aligned)
	alignas(64) long _freeSize;  // pool ���� �̻�� ��ü ���� (64byte aligned)

	alignas(64) long _initPoolSize;     // �ʱ� pool size
	long _poolSize;         // ���� pool size	
	bool _bPlacementNew;    // true �� Alloc, Free �� �� ���� ��ü�� ������, �Ҹ��� ȣ��. 
							// false�� ��ü�� ó�� ������ �� �� ���� ������ ȣ��, �޸�Ǯ�� �Ҹ�� �� ��� ��ü���� �Ҹ��� ȣ��
	bool _isPoolVariable;   // false�� memory pool, true�� free list

	unsigned __int64 _leftVerifyCode;
	unsigned __int64 _rightVerifyCode;
	TYPE_ID _addressMask;   // TYPE_ID ������ �ּҰ��� ������� mask
	__int64 _addressBitCount;    // �������� �ּ��� bit ��. TYPE_ID ������ alloc count�� ������� ����



public:
	CMemoryPool(int initPoolSize, bool bFreeList, bool bPlacementNew);
	~CMemoryPool();

public:
	long GetInitSize() { return _initPoolSize; }
	long GetPoolSize() { return _poolSize; }
	long GetFreeCount() { return _freeSize; }
	long GetUseCount() { return _poolSize - _freeSize; }

	_Ty* Alloc();
	void Free(_Ty* ptr);

	// 64byte aligned ��ü ������ ���� new, delete overriding
	void* operator new(size_t size);
	void operator delete(void* p);

private:
	void Crash();

private:
	//#pragma pack(push, 1)
	struct BlockNode
	{
		BlockNode* pNext;
		unsigned __int64 leftVerifyCode;
		_Ty object;
		int allocCount;
		unsigned __int64 rightVerifyCode;

		BlockNode(unsigned __int64 leftVerifyCode, unsigned __int64 rightVerifyCode)
			: pNext(nullptr), leftVerifyCode(leftVerifyCode), rightVerifyCode(rightVerifyCode), allocCount(0)
		{}
	};
	//#pragma pack(pop)

#ifdef MEMPOOL_ENABLE_LOGGING
public:
	enum class eWorkType
	{
		ALLOC,
		ALLOC_NEW,
		FREE
	};
	struct ThreadLog 
	{
		__int64 index;
		DWORD threadId;
		eWorkType workType;
		TYPE_ID topId;     // ���� top
		TYPE_ID topNextId; // ���� top�� next
		TYPE_ID otherId;   // ALLOC�� ��� ��ȯ�� ���, FREE�� ��� ���� top�� next
	}; // size = 40

	alignas(64) int _sizeLogBuffer;
	ThreadLog* _logBuffer;
	alignas(64) __int64 _logBufferIndex;

	void OutputThreadLog();
#endif
};



template <typename _Ty>
CMemoryPool<_Ty>::CMemoryPool(int initPoolSize, bool bFreeList, bool bPlacementNew)
	:_initPoolSize(initPoolSize), _poolSize(initPoolSize), _freeSize(initPoolSize)
	, _bPlacementNew(bPlacementNew), _isPoolVariable(bFreeList), _addressBitCount(0)
	, _topId(0)
{

	// �����ڵ� ����
	_leftVerifyCode =
		(unsigned __int64)rand() << 60
		| (unsigned __int64)rand() << 45
		| (unsigned __int64)rand() << 30
		| (unsigned __int64)rand() << 15
		| (unsigned __int64)rand();

	_rightVerifyCode =
		(unsigned __int64)rand() << 60
		| (unsigned __int64)rand() << 45
		| (unsigned __int64)rand() << 30
		| (unsigned __int64)rand() << 15
		| (unsigned __int64)rand();

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

	BlockNode* pNewNode;
	BlockNode* pPrevNode;
	// �ʱ� ��� ����
	if (_initPoolSize > 0)
	{
		// ù ��� ���� �� �ʱ�ȭ
		pNewNode = new BlockNode(_leftVerifyCode, _rightVerifyCode);
		if (_bPlacementNew == false)
		{
			// ������ ȣ��
			new (&pNewNode->object) _Ty;
		}
		_topId = (TYPE_ID)pNewNode;
		pPrevNode = pNewNode;

		// ��� ��� ���� �� �ʱ�ȭ
		for (int iCnt = 1; iCnt < _initPoolSize; iCnt++)
		{
			pNewNode = new BlockNode(_leftVerifyCode, _rightVerifyCode);
			if (_bPlacementNew == false)
			{
				// ������ ȣ��
				new (&pNewNode->object) _Ty;
			}

			pPrevNode->pNext = pNewNode;
			pPrevNode = pNewNode;
		}
		pNewNode->pNext = nullptr;
	}


#ifdef MEMPOOL_ENABLE_LOGGING
	_sizeLogBuffer = 0x1ffff;  // 2���� ���� ��� 1�� �̷������ ��
	_logBuffer = (ThreadLog*)malloc(sizeof(ThreadLog) * _sizeLogBuffer);
	memset(_logBuffer, 0, sizeof(ThreadLog) * _sizeLogBuffer);
	_logBufferIndex = -1;
#endif

}





template <typename _Ty>
CMemoryPool<_Ty>::~CMemoryPool()
{
	BlockNode* pNode = MEMPOOL_GET_ADDRESS(_topId);
	BlockNode* pNextNode = pNode;
	// pool ���� ��� ��带 ����
	for (int iCnt = 0; iCnt < _freeSize; iCnt++)
	{
		pNode = pNextNode;
		if (_bPlacementNew == false)
		{
			// �Ҹ��� ȣ��
			(pNode->object).~_Ty();
		}
		pNextNode = pNode->pNext;
		free(pNode);
	}

}






template <typename _Ty>
_Ty* CMemoryPool<_Ty>::Alloc()
{
	TYPE_ID topId;
	TYPE_ID newTopId;
	BlockNode* pTop;
	BlockNode* pNewTop = nullptr;
#ifdef MEMPOOL_ENABLE_LOGGING
	BlockNode* pNewTopNext = nullptr; // debug
#endif
	bool isNewNode = false;
	do {
		topId = _topId;
		pTop = MEMPOOL_GET_ADDRESS(topId);

		// ���� pool ���� ���� ��尡 ���� ���
		if (pTop == nullptr)
		{
			// free list�� ���
			if (_isPoolVariable)
			{
				// ���� ������尡 ���µ� free list�� ��� ��� ����
				pTop = new BlockNode(_leftVerifyCode, _rightVerifyCode);
				isNewNode = true;
				InterlockedIncrement(&_poolSize);
				break; // ���ο� ��带 �����Ͽ��� ��� ������ top�� �������� �ʰ� ���ο� ��常�� �����Ѵ�.
			}
			// memory pool�� ���
			else
			{
				// ���� free node�� ���µ� free list�� �ƴ� ��� crash
				Crash();
				break;
			}
		}
		else
		{
			pNewTop = pTop->pNext;
			if (pNewTop == nullptr)
			{
				newTopId = 0;
#ifdef MEMPOOL_ENABLE_LOGGING
				pNewTopNext = nullptr; // debug
#endif
			}
			else
			{
				newTopId = MEMPOOL_MAKE_ID(pNewTop);
#ifdef MEMPOOL_ENABLE_LOGGING
				pNewTopNext = pNewTop->pNext; // debug
#endif
			}
		}
	} while (InterlockedCompareExchange64((__int64*)&_topId, (__int64)newTopId, (__int64)topId) != (__int64)topId);

#ifdef MEMPOOL_ENABLE_LOGGING
	__int64 logBufferIndex = InterlockedIncrement64(&_logBufferIndex);
	int index = (int)(logBufferIndex & (__int64)_sizeLogBuffer);
	_logBuffer[index].index = logBufferIndex;
	_logBuffer[index].threadId = GetCurrentThreadId();
	_logBuffer[index].workType = isNewNode == false ? eWorkType::ALLOC : eWorkType::ALLOC_NEW;
	_logBuffer[index].topId = isNewNode == false ? newTopId : 0;
	_logBuffer[index].topNextId = pNewTopNext == nullptr ? 0 : MEMPOOL_MAKE_ID(pNewTopNext);
	_logBuffer[index].otherId = MEMPOOL_MAKE_ID(pTop);
#endif

	if (isNewNode == false)
		InterlockedDecrement(&_freeSize);

	// ���� ������ ����̰ų� placementNew �� ��� ������ ȣ��
	if (_bPlacementNew == true
		|| (isNewNode == true && _bPlacementNew == false))
	{
		new (&pTop->object) _Ty;
	}

	// ����� alloc count +1
	pTop->allocCount++;

	return &pTop->object;
}




template <typename _Ty>
void CMemoryPool<_Ty>::Free(_Ty* ptr)
{
	// �ڵ� ����
	BlockNode* pFreeNode = (BlockNode*)((char*)ptr - (unsigned __int64)(&((BlockNode*)nullptr)->object));
	if (pFreeNode->leftVerifyCode != _leftVerifyCode
		|| pFreeNode->rightVerifyCode != _rightVerifyCode)
	{
		Crash();
	}

	if (_bPlacementNew)
	{
		// �Ҹ��� ȣ��
		(pFreeNode->object).~_Ty();
	}

	// _topId�� Free�� ���� ��ü (lockfree)
	TYPE_ID topId;
	TYPE_ID freeNodeId = MEMPOOL_MAKE_ID(pFreeNode);
	BlockNode* pTop;
	do {
		topId = _topId;
		pTop = MEMPOOL_GET_ADDRESS(topId);
		pFreeNode->pNext = pTop;
	} while (InterlockedCompareExchange64((__int64*)&_topId, (__int64)freeNodeId, (__int64)topId) != (__int64)topId);

#ifdef MEMPOOL_ENABLE_LOGGING
	__int64 logBufferIndex = InterlockedIncrement64(&_logBufferIndex);
	int index = (int)(logBufferIndex & (__int64)_sizeLogBuffer);
	_logBuffer[index].index = logBufferIndex;
	_logBuffer[index].threadId = GetCurrentThreadId();
	_logBuffer[index].workType = eWorkType::FREE;
	_logBuffer[index].topId = freeNodeId;
	_logBuffer[index].topNextId = topId;
	_logBuffer[index].otherId = pTop == nullptr ? 0 : (pTop->pNext == nullptr ? 0 : MEMPOOL_MAKE_ID(pTop->pNext));
#endif

	InterlockedIncrement(&_freeSize);
}


// 64byte aligned ��ü ������ ���� new, delete overriding
template <typename _Ty>
void* CMemoryPool<_Ty>::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}
template <typename _Ty>
void CMemoryPool<_Ty>::operator delete(void* p)
{
	_aligned_free(p);
}




template <typename _Ty>
void CMemoryPool<_Ty>::Crash()
{
	int* p = nullptr;
	*p = 0;
}





#ifdef MEMPOOL_ENABLE_LOGGING
template <typename _Ty>
void CMemoryPool<_Ty>::OutputThreadLog()
{
	// ���� ��¥�� �ð��� �˾ƿ´�.
	WCHAR filename[MAX_PATH];
	SYSTEMTIME stNowTime;
	GetLocalTime(&stNowTime);
	wsprintf(filename, L"CMemoryPool_%d%02d%02d_%02d%02d%02d.txt",
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

		BlockNode* topNode = MEMPOOL_GET_ADDRESS(log.topId);
		int topAllocCount = MEMPOOL_GET_ALLOC_COUNT(log.topId);
		BlockNode* topNextNode = MEMPOOL_GET_ADDRESS(log.topNextId);
		int topNextAllocCount = MEMPOOL_GET_ALLOC_COUNT(log.topNextId);
		BlockNode* otherNode = MEMPOOL_GET_ADDRESS(log.otherId);
		int otherAllocCount = MEMPOOL_GET_ALLOC_COUNT(log.otherId);

		if (log.workType == eWorkType::ALLOC)
			lenSzLog = swprintf_s(szLog, 1000, L"#%llu  ALLOC      id: %5u, old top: %p %d (pop) -> top: %p %d, next: %p %d\n"
				, log.index, log.threadId, otherNode, otherAllocCount, topNode, topAllocCount, topNextNode, topNextAllocCount);
		else if (log.workType == eWorkType::ALLOC_NEW)
			lenSzLog = swprintf_s(szLog, 1000, L"#%llu  ALLOC_NEW  id: %5u, old top: %p %d (new) -> top: %p %d, next: %p %d\n"
				, log.index, log.threadId, otherNode, otherAllocCount, topNode, topAllocCount, topNextNode, topNextAllocCount);
		else if (log.workType == eWorkType::FREE)
			lenSzLog = swprintf_s(szLog, 1000, L"#%llu  FREE       id: %5u, old top: %p %d       -> top: %p %d (push)\n"
				, log.index, log.threadId, topNextNode, topNextAllocCount, topNode, topAllocCount);
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



}


/*
�������
@ 20221207
InterlockedCompareExchange�� ���Ǵ� _topId ���, InterlockedIncrement�� ���Ǵ� _freeSize ����� ��ġ�� �� ���� �ű�� �Ʒ��� ���� alignas(64) ��.
_poolSize ����� alignas(64) ���� �ʾҴµ�, ��带 ���� �����Ҷ��� ����ϴ� ���̱� ������ �޸�Ǯ�� ������ �Ŀ��� ������� �ʱ� ������.
	class alignas(64) CMemoryPool
	{
		TYPE_ID _topId;  // ���� ��밡���� ����� �ּҿ� alloc Ƚ���� ��ģ �� (64byte aligned)
		alignas(64) long _freeSize;  // pool ���� �̻�� ��ü ���� (64byte aligned)

�׸��� �޸�Ǯ�� 64byte ��迡 �����Ǵ°��� �����ϱ����� new, delete �Լ��� �������̵���
	void* operator new(size_t size);
	void operator delete(void* p);

�׽�ƮCPU: Intel(R) Core(TM) i5-7500 CPU @ 3.40GHz
�׽�Ʈ����: alloc count ��ŭ alloc, free �ϴ°��� loop count��ŭ �ݺ��Ͽ� �� �ҿ�ð��� ����
num thread      object size     loop count      alloc count     total proc      time(ms)(������)		time(ms)(������)
1				8				10000			10000			100000000		3344  				3328  
1				64				10000			10000			100000000		3687  				3672  
1				1024			10000			10000			100000000		4609  				4609  
1				10240			10000			10000			100000000		6047  				5969  
2				8				10000			10000			200000000		32008 				24179 
2				64				10000			10000			200000000		29101 				26602 
2				1024			10000			10000			200000000		38664 				27718 
2				10240			10000			10000			200000000		36875 				33015 
4				8				10000			10000			400000000		115539				85613 
4				64				10000			10000			400000000		105140				87664 
4				1024			10000			10000			400000000		95148 				94156 
4				10240			10000			10000			400000000		121511				102632
*/
