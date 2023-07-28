#pragma once

#include "CMemoryPool.h"

//#define QUEUE_ENABLE_LOGGING

namespace netserver
{


using TYPE_ID = unsigned __int64;

#define QUEUE_GET_ADDRESS(Id) (Node*)((Id) & _addressMask)
#define QUEUE_GET_PROC_COUNT(Id) (int)((Id) >> _addressBitCount)
#define QUEUE_MAKE_ID(procCount, address) (((TYPE_ID)(procCount) << _addressBitCount) | (TYPE_ID)(address))

template <typename _Ty>
class alignas(64) CLockfreeQueue
{
public:
	struct Node
	{
		Node* pNext;
		_Ty obj;
	};

private:
	TYPE_ID _headId;  // 64byte aligned
	alignas(64) TYPE_ID _tailId;  // 64byte aligned
	alignas(64) long _size;  // 64byte aligned

	alignas(64) CMemoryPool<Node>* _pPoolNode;
	TYPE_ID _addressMask;   // TYPE_ID ������ �ּҰ��� ������� mask
	__int64 _addressBitCount;   // �������� �ּ��� bit ��. TYPE_ID ������ pop count�� ������� ����

public:
	CLockfreeQueue();
	CLockfreeQueue(int size);
	~CLockfreeQueue();

public:
	int Size() { return _size; }
	int GetPoolSize() { return _pPoolNode->GetPoolSize(); }

	void Enqueue(_Ty& obj);
	bool Dequeue(_Ty& obj);

	// 64byte aligned ��ü ������ ���� new, delete overriding
	void* operator new(size_t size);
	void operator delete(void* p);

};


template <typename _Ty>
CLockfreeQueue<_Ty>::CLockfreeQueue()
	:_size(0), _addressBitCount(0)
{
	// ��� Ǯ ����
	_pPoolNode = new CMemoryPool<Node>(0, true, false);
	
	// �ּ� mask ���
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	_addressMask = (TYPE_ID)sysInfo.lpMaximumApplicationAddress;
	for (int i = 63; i >= 0; i--)
	{
		if (_addressMask & (1i64 << i))
		{
			_addressMask = 0xFFFFFFFF'FFFFFFFF >> (63 - i);  // 0x00007FFF'FFFFFFFF
			_addressBitCount = i + 1;                        // 47
			break;
		}
	}

	// ���̳�� ����
	Node* pDummy = _pPoolNode->Alloc();
	pDummy->pNext = nullptr;
	TYPE_ID dummyId = QUEUE_MAKE_ID(0, pDummy);
	_headId = dummyId;
	_tailId = dummyId;
}


template <typename _Ty>
CLockfreeQueue<_Ty>::CLockfreeQueue(int size)
	:_size(0), _addressBitCount(0)
{
	// ��� Ǯ ����
	_pPoolNode = new CMemoryPool<Node>(size, true, false);

	// �ּ� mask ���
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	_addressMask = (TYPE_ID)sysInfo.lpMaximumApplicationAddress;
	for (int i = 63; i >= 0; i--)
	{
		if (_addressMask & (1i64 << i))
		{
			_addressMask = 0xFFFFFFFF'FFFFFFFF >> (63 - i);  // 0x00007FFF'FFFFFFFF
			_addressBitCount = i + 1;                        // 47
			break;
		}
	}

	// ���̳�� ����
	Node* pDummy = _pPoolNode->Alloc();
	pDummy->pNext = nullptr;
	TYPE_ID dummyId = QUEUE_MAKE_ID(0, pDummy);
	_headId = dummyId;
	_tailId = dummyId;
}


template <typename _Ty>
CLockfreeQueue<_Ty>::~CLockfreeQueue()
{
	// ��� ��带 free �Ѵ�.
	Node* pNode = QUEUE_GET_ADDRESS(_headId);
	Node* pNext;
	while (pNode != nullptr)
	{
		_size--;
		pNext = pNode->pNext;
		_pPoolNode->Free(pNode);
		pNode = pNext;
	}

	// ��� Ǯ ����
	delete _pPoolNode;
}



template <typename _Ty>
void CLockfreeQueue<_Ty>::Enqueue(_Ty& obj)
{
	Node* pNewNode = _pPoolNode->Alloc();
	pNewNode->pNext = nullptr;
	pNewNode->obj = obj;

	TYPE_ID tailId;
	TYPE_ID tailNextId;
	TYPE_ID newTailId;
	Node* pTail;
	Node* pTailNext;
	while (true)
	{
		tailId = _tailId;
		pTail = QUEUE_GET_ADDRESS(tailId);
		pTailNext = pTail->pNext;
		if (pTailNext == nullptr)
		{
			if (InterlockedCompareExchange64((__int64*)&pTail->pNext, (__int64)pNewNode, 0) == 0)
			{
				// tail->next�� new node�� ��ü�ߴٸ� ����.
				break;
			}
			else
			{
				// pTailNext == nullptr �ε� CAS1�� �����ߴٴ� ���� �׻��̿� pTail->pNext �� ��ü�Ǿ��ٴ� �ǹ��̴�.
				// �� ��� pTail->pNext�� ��ü�� �����尡 CAS2�� _tailId�� ��ü�� ���̱� ������ ���� _tailId ��ü�� �õ��� �ʿ�� ����.
				continue;
			}
		}
		else
		{
			// tail->next�� null�� �ƴ϶�� tail�� �ڷ� �б⸦ �õ��Ѵ�.
			tailNextId = QUEUE_MAKE_ID(QUEUE_GET_PROC_COUNT(tailId) + 1, pTailNext);
			if (_tailId != tailId)  // CAS�� �̸� Ȯ��
				continue;
			InterlockedCompareExchange64((__int64*)&_tailId, (__int64)tailNextId, (__int64)tailId);
		}
	}

	// tail->next�� new node�� ��ü�� ��, tail�� �ڷ� �δ�.
	newTailId = QUEUE_MAKE_ID(QUEUE_GET_PROC_COUNT(tailId) + 1, pNewNode);
	InterlockedCompareExchange64((__int64*)&_tailId, (__int64)newTailId, (__int64)tailId);

	InterlockedIncrement(&_size);
}





template <typename _Ty>
bool CLockfreeQueue<_Ty>::Dequeue(_Ty& obj)
{
	TYPE_ID headId;
	TYPE_ID headNextId;
	TYPE_ID tailId;
	TYPE_ID tailNextId;
	Node* pHead;
	Node* pHeadNext;
	Node* pTail;
	Node* pTailNext;

	if (_size <= 0)
		return false;

	long size = InterlockedDecrement(&_size);
	if (size < 0)
	{
		InterlockedIncrement(&_size);
		return false;
	}

	while (true) 
	{
		headId = _headId;
		pHead = QUEUE_GET_ADDRESS(headId);
		pHeadNext = pHead->pNext;
		if (pHeadNext == nullptr)
		{
			// size�� 1 �̻��̾��µ� head->next�� null�� ���� �ٸ������� ������ ��尡 ���� ������� ���� ����. ����ɶ����� ��ٸ�.
			continue;
		}
		
		tailId = _tailId;
		pTail = QUEUE_GET_ADDRESS(tailId);
		pTailNext = pTail->pNext;
		if (pHead == pTail && pTailNext != nullptr)
		{
			// head�� tail�� ������ tail->next�� null�� �ƴϸ� tail�� �ڷ� �и��� ���� ����. tail�� �ڷ� �̴°��� �õ���.
			// ���⼭ CAS�� �����ߴٸ� �ٸ������� tail�� �ڷ� �� ���̱� ������ �׳� �Ѿ.
			tailNextId = QUEUE_MAKE_ID(QUEUE_GET_PROC_COUNT(tailId) + 1, pTailNext);
			InterlockedCompareExchange64((__int64*)&_tailId, (__int64)tailNextId, (__int64)tailId);
		}

		obj = pHeadNext->obj;
		headNextId = QUEUE_MAKE_ID(QUEUE_GET_PROC_COUNT(headId) + 1, pHeadNext);
		if (_headId != headId)  // CAS�� �̸� Ȯ��
			continue;
		if (InterlockedCompareExchange64((__int64*)&_headId, (__int64)headNextId, (__int64)headId) == (__int64)headId)
		{
			// head�� head->next�� ��ü�ϴ°��� �����Ͽ��ٸ� �Ϸ�.
			break;
		}
	} 

	_pPoolNode->Free(pHead);
	return true;
}



// 64byte aligned ��ü ������ ���� new, delete overriding
template <typename _Ty>
void* CLockfreeQueue<_Ty>::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}
template <typename _Ty>
void CLockfreeQueue<_Ty>::operator delete(void* p)
{
	_aligned_free(p);
}




}