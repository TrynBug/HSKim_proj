#pragma once

#include <Windows.h>
#include <vector>

namespace game_netserver
{

template <typename _Ty>
class CMemoryPoolTLS
{
private:
	class Chunk
	{
		friend class CMemoryPoolTLS;

		struct BlockNode
		{
			Chunk* _pChunk;
			unsigned __int64 _leftVerifyCode;
			_Ty _object;
			unsigned __int64 _rightVerifyCode;
		};

		Chunk* _pNext;
		BlockNode* _arrNode;
		int _allocIndex;
		long _freeCount;
		bool _isNew;

		Chunk(int chunkSize, unsigned __int64 leftVerifyCode, unsigned __int64 rightVerifyCode)
			: _pNext(nullptr), _allocIndex(0), _freeCount(0), _isNew(true)
		{
			_arrNode = (BlockNode*)malloc(sizeof(BlockNode) * chunkSize);
			for (int i = 0; i < chunkSize; i++)
			{
				_arrNode[i]._pChunk = this;
				_arrNode[i]._leftVerifyCode = leftVerifyCode;
				_arrNode[i]._rightVerifyCode = rightVerifyCode;
			}
		}
		~Chunk()
		{
			free(_arrNode);
		}
	};

	class ThreadChunkManager
	{
		friend class CMemoryPoolTLS;

		Chunk* _pChunk;
		int    _objectAllocCount;

		ThreadChunkManager()
			:_pChunk(nullptr), _objectAllocCount(0)
		{}
	};

private:
	int _initPoolSize;      // �ʱ� pool size
	bool _bPlacementNew;    // true �� Alloc, Free �� �� ���� ��ü�� ������, �Ҹ��� ȣ��. 
							// false�� ��ü�� ó�� ������ �� �� ���� ������ ȣ��, �޸�Ǯ�� �Ҹ�� �� ��� ��ü���� �Ҹ��� ȣ��

	int _chunkSize;          // Chunk �ϳ��� ����ִ� ��ü ��
	long _numFreeChunk;      // pool ���� Chunk �� (�� ����� _srwlPool �������� ����Ǿ����)
	long _numAllocChunk;     // Alloc �� Chunk �� (�� ����� _srwlPool �������� ����Ǿ����)

	SRWLOCK _srwlPool;
	DWORD _tlsIndex;

	std::vector<ThreadChunkManager*> _vecThreadMgr;
	SRWLOCK _srwlVecThreadMgr;

	unsigned __int64 _leftVerifyCode;
	unsigned __int64 _rightVerifyCode;


	Chunk* _pFreeChunk;  // ���� free chunk (�� ����� _srwlPool �������� ����Ǿ����)

public:
	CMemoryPoolTLS(int initPoolSize, bool bPlacementNew, int chunkSize);
	~CMemoryPoolTLS();

public:
	int GetFreeCount() { return _numFreeChunk * _chunkSize; }
	int GetAllocCount() { return _numAllocChunk * _chunkSize; }
	int GetActualAllocCount();
	int GetPoolSize() { return (_numFreeChunk + _numAllocChunk) * _chunkSize; }

	_Ty* Alloc();
	void Free(_Ty* ptr);

private:
	Chunk* AllocChunk();
	void FreeChunk(Chunk* pChunk);

	void Crash();

};



template <typename _Ty>
CMemoryPoolTLS<_Ty>::CMemoryPoolTLS(int initPoolSize, bool bPlacementNew, int chunkSize)
	:_initPoolSize(initPoolSize), _bPlacementNew(bPlacementNew), _chunkSize(chunkSize)
	, _pFreeChunk(nullptr), _numAllocChunk(0)
{
	InitializeSRWLock(&_srwlPool);
	InitializeSRWLock(&_srwlVecThreadMgr);
	_tlsIndex = TlsAlloc();
	if (_tlsIndex == TLS_OUT_OF_INDEXES)
	{
		int* p = 0;
		*p = 0;
	}

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

	// �ʱ� ûũ �� ����
	if (_initPoolSize % _chunkSize == 0)
		_numFreeChunk = _initPoolSize / _chunkSize;
	else
		_numFreeChunk = _initPoolSize / _chunkSize + 1;

	// �ʱ� ûũ ����
	Chunk* pNewChunk;
	Chunk* pPrevChunk;
	if (_numFreeChunk > 0)
	{
		// ù ûũ ���� �� �ʱ�ȭ
		pNewChunk = new Chunk(_chunkSize, _leftVerifyCode, _rightVerifyCode);
		_pFreeChunk = pNewChunk;
		pPrevChunk = pNewChunk;

		// ��� ûũ ���� �� �ʱ�ȭ
		for (int i = 1; i < _numFreeChunk; i++)
		{
			pNewChunk = new Chunk(_chunkSize, _leftVerifyCode, _rightVerifyCode);
			pPrevChunk->_pNext = pNewChunk;
			pPrevChunk = pNewChunk;
		}
		pNewChunk->_pNext = nullptr;
	}
}





template <typename _Ty>
CMemoryPoolTLS<_Ty>::~CMemoryPoolTLS()
{
	AcquireSRWLockExclusive(&_srwlPool);
	Chunk* pChunk = _pFreeChunk;
	Chunk* pNextChunk = _pFreeChunk;
	// pool ���� ��� ��带 ����
	for (int iCnt = 0; iCnt < _numFreeChunk; iCnt++)
	{
		pChunk = pNextChunk;
		if (_bPlacementNew == false)
		{
			// �Ҹ��� ȣ��
			if (pChunk->_isNew == false)
			{
				for (int n = 0; n < _chunkSize; n++)
					(pChunk->_arrNode[n]._object).~_Ty();
			}
			else
			{
				for (int n = 0; n < pChunk->_allocIndex; n++)
					(pChunk->_arrNode[n]._object).~_Ty();
			}
		}
		pNextChunk = pChunk->_pNext;
		delete pChunk;
	}
	ReleaseSRWLockExclusive(&_srwlPool);

	// �����忡 �Ҵ�� ��� �Ŵ����� ��带 ����
	AcquireSRWLockExclusive(&_srwlVecThreadMgr);
	ThreadChunkManager* pMgr;
	for (int iCnt = 0; iCnt < _vecThreadMgr.size(); iCnt++)
	{
		pMgr = _vecThreadMgr[iCnt];
		pChunk = pMgr->_pChunk;
		if (pChunk != nullptr)
		{
			if (_bPlacementNew == false)
			{
				// �Ҹ��� ȣ��
				for (int n = 0; n < pChunk->_allocIndex; n++)
					(pChunk->_arrNode[n]._object).~_Ty();
			}
			delete pChunk;
		}
		delete pMgr;
	}
	_vecThreadMgr.clear();
	ReleaseSRWLockExclusive(&_srwlVecThreadMgr);

	// tls free
	TlsFree(_tlsIndex);
}



template <typename _Ty>
int CMemoryPoolTLS<_Ty>::GetActualAllocCount()
{
	int objectAllocCount = 0;
	AcquireSRWLockShared(&_srwlVecThreadMgr);
	for (int i = 0; i < _vecThreadMgr.size(); i++)
	{
		objectAllocCount += _vecThreadMgr[i]->_objectAllocCount;
	}
	ReleaseSRWLockShared(&_srwlVecThreadMgr);
	return objectAllocCount;
}



template <typename _Ty>
_Ty* CMemoryPoolTLS<_Ty>::Alloc()
{
	// ������ ûũ �Ŵ��� ��������
	ThreadChunkManager* pMgr = (ThreadChunkManager*)TlsGetValue(_tlsIndex);

	// �����忡 �Ŵ����� ���ٸ� ������
	if (pMgr == nullptr)
	{
		pMgr = new ThreadChunkManager;
		AcquireSRWLockExclusive(&_srwlVecThreadMgr);
		_vecThreadMgr.push_back(pMgr);
		ReleaseSRWLockExclusive(&_srwlVecThreadMgr);
		TlsSetValue(_tlsIndex, pMgr);
	}
	// �Ŵ����� ûũ�� ���ٸ� �Ҵ����
	if (pMgr->_pChunk == nullptr)
		pMgr->_pChunk = AllocChunk();
	Chunk* pChunk = pMgr->_pChunk;

	// ������ ��ü ���
	_Ty* pObj = &pChunk->_arrNode[pChunk->_allocIndex]._object;

	// placementNew == true �̰ų�, placementNew == false ������ ���ʷ� ������ ��ü�� ��� ������ ȣ��
	if (_bPlacementNew == true
		|| (pChunk->_isNew == true && _bPlacementNew == false))
	{
		new (pObj) _Ty;
	}

	// ûũ�� allocIndex�� ������Ű�� ������ �������״ٸ� ûũ�� �Ŵ������Լ� ���
	pChunk->_allocIndex++;
	if (pChunk->_allocIndex == _chunkSize)
		pMgr->_pChunk = nullptr;

	// �Ŵ����� object alloc count ����
	pMgr->_objectAllocCount++;
	return pObj;
}


template <typename _Ty>
typename CMemoryPoolTLS<_Ty>::Chunk* CMemoryPoolTLS<_Ty>::AllocChunk()
{
	Chunk* pChunk;
	AcquireSRWLockExclusive(&_srwlPool);
	if (_numFreeChunk > 0)
	{
		pChunk = _pFreeChunk;
		_pFreeChunk = _pFreeChunk->_pNext;
		_numFreeChunk--;
		_numAllocChunk++;
		ReleaseSRWLockExclusive(&_srwlPool);

		pChunk->_pNext = nullptr;
		pChunk->_allocIndex = 0;
		pChunk->_freeCount = 0;
	}
	else
	{
		_numAllocChunk++;
		ReleaseSRWLockExclusive(&_srwlPool);
		pChunk = new Chunk(_chunkSize, _leftVerifyCode, _rightVerifyCode);
	}

	return pChunk;
}




template <typename _Ty>
void CMemoryPoolTLS<_Ty>::Free(_Ty* ptr)
{
	// �ڵ� ����
	typename Chunk::BlockNode* pNode = (typename Chunk::BlockNode*)((char*)ptr - (unsigned __int64)(&((typename Chunk::BlockNode*)nullptr)->_object));
	if (pNode->_leftVerifyCode != _leftVerifyCode
		|| pNode->_rightVerifyCode != _rightVerifyCode)
	{
		Crash();
	}

	// placement new = true �� ��� �Ҹ��� ȣ��
	if (_bPlacementNew)
	{
		(pNode->_object).~_Ty();
	}

	// ��尡 ���Ե� ûũ�� freeCount ����
	long freeCount = InterlockedIncrement(&pNode->_pChunk->_freeCount);

	// ûũ�� freeCount�� ��� �����Ͽ��ٸ� ûũ�� Free��
	if (freeCount == _chunkSize)
		FreeChunk(pNode->_pChunk);


	// ������ ûũ �Ŵ��� ��������
	ThreadChunkManager* pMgr = (ThreadChunkManager*)TlsGetValue(_tlsIndex);

	// �����忡 �Ŵ����� ���ٸ� ������
	if (pMgr == nullptr)
	{
		pMgr = new ThreadChunkManager;
		AcquireSRWLockExclusive(&_srwlVecThreadMgr);
		_vecThreadMgr.push_back(pMgr);
		ReleaseSRWLockExclusive(&_srwlVecThreadMgr);
		TlsSetValue(_tlsIndex, pMgr);
	}

	// �������� object alloc count ����
	pMgr->_objectAllocCount--;

}




template <typename _Ty>
void CMemoryPoolTLS<_Ty>::FreeChunk(Chunk* pChunk)
{
	pChunk->_isNew = false;

	Chunk* pNextChunk;
	AcquireSRWLockExclusive(&_srwlPool);
	pNextChunk = _pFreeChunk;
	_pFreeChunk = pChunk;
	pChunk->_pNext = pNextChunk;

	_numFreeChunk++;
	_numAllocChunk--;
	ReleaseSRWLockExclusive(&_srwlPool);
}




template <typename _Ty>
void CMemoryPoolTLS<_Ty>::Crash()
{
	int* p = nullptr;
	*p = 0;
}





}