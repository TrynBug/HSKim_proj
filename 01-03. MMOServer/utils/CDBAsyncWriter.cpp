
#include <Windows.h>
#include <process.h>
#include <strsafe.h>
#include "logger.h"

#include "CDBAsyncWriter.h"





CDBAsyncWriter::CDBAsyncWriter()
	: _pDBConn(nullptr), _hThreadDBWriter(0), _threadDBWriterId(0), _hEventMsg(0), _isClosed(false)
	, _poolQueryRequest(0, true, false)
{

}

CDBAsyncWriter::~CDBAsyncWriter()
{
	if (_isClosed == false)
		Close();
}


/* connect and run thread */
bool CDBAsyncWriter::ConnectAndRunThread(const char* host, const char* user, const char* password, const char* database, unsigned int port)
{
	if (_pDBConn != nullptr)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB Thread] Async DB Writer Thread already has a DB connection\n");
		return false;
	}

	// event ��ü ����
	_hEventMsg = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (_hEventMsg == NULL)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB Thread] Failed to create event object. error:%u\n", GetLastError());
		return false;
	}

	// DB Connector ����
	_pDBConn = new CDBConnector();
	if (_pDBConn->Connect(host, user, password, database, port, true) == false)
	{
		delete _pDBConn;
		_pDBConn = nullptr;
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB Thread] Failed to connect to DB\n");
		return false;
	}

	// ������ ����
	_hThreadDBWriter = (HANDLE)_beginthreadex(NULL, 0, ThreadAsyncDBWriter, (PVOID)this, 0, &_threadDBWriterId);
	if (_hThreadDBWriter == 0)
	{
		delete _pDBConn;
		_pDBConn = nullptr;
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB Thread] Failed to start Async DB Writer Thread. error:%u\n", GetLastError());
		return false;
	}

	return true;
}



/* query */
// ��ȯ���� ���� ������ ��û�Ѵ�. �������� �޽���ť�� ������ �����Ѵ�.
bool CDBAsyncWriter::PostQueryRequest(const wchar_t* query, ...)
{
	_StQueryRequest* pQueryRequest = _poolQueryRequest.Alloc();
	pQueryRequest->msgType = MSG_QUERY_REQUEST;

	va_list vaList;
	va_start(vaList, query);
	// QueryRequest ���ۿ� ���� write
	HRESULT hr = StringCchVPrintf(pQueryRequest->szQuery, MAX_QUERY_LENGTH, query, vaList);
	if (FAILED(hr))
	{
		if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"[DB] query length exceeds buffer. \n\tquery: %s\n", query);
		}
		else
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"[DB] an error occurred within the StringCchVPrinf function. \n\tquery format:%s\n", query);
		}

		_poolQueryRequest.Free(pQueryRequest);
		return false;
	}
	va_end(vaList);

	// �޽���ť�� ���� ����
	_msgQ.Enqueue(pQueryRequest);
	SetEvent(_hEventMsg);
	return true;
}


/* Disconnect */
void CDBAsyncWriter::Close()
{
	if (_isClosed == true)
		return;

	// ������ ���� �޽����� ������.
	_StQueryRequest* pMsg = _poolQueryRequest.Alloc();
	pMsg->msgType = MSG_SHUTDOWN;
	_msgQ.Enqueue(pMsg);
	SetEvent(_hEventMsg);

	// �����尡 ����Ǳ⸦ ��ٸ���.
	BOOL retWait = WaitForSingleObject(_hThreadDBWriter, INFINITE);
	if (retWait != WAIT_OBJECT_0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB] An error occurred when terminate DB Writer Thread. error:%u\n", GetLastError());
	}

	// �޽���ť�� ����.
	_StQueryRequest* pQueryRequest;
	int unprocessedMsgCount = 0;
	while (_msgQ.Dequeue(pQueryRequest))
	{
		_poolQueryRequest.Free(pQueryRequest);
		unprocessedMsgCount++;
	}
	if (unprocessedMsgCount > 0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB] %d query requests were unprocessed\n", unprocessedMsgCount);
	}

	// DB ������ ���´�.
	delete _pDBConn;
	_pDBConn = nullptr;

	_isClosed = true;
	return;
}


/* dynamic alloc */
// 64byte aligned ��ü ������ ���� new, delete overriding
void* CDBAsyncWriter::operator new(size_t size)
{
	return _aligned_malloc(size, 64);
}

void CDBAsyncWriter::operator delete(void* p)
{
	_aligned_free(p);
}

void CDBAsyncWriter::Crash()
{
	int* p = 0;
	*p = 0;
}

/* (static) �񵿱� DB Writer ������ */
unsigned WINAPI CDBAsyncWriter::ThreadAsyncDBWriter(PVOID pParam)
{
	wprintf(L"[DB Thread] begin async DB writer thread\n");
	CDBAsyncWriter& db = *(CDBAsyncWriter*)pParam;

	_StQueryRequest* pQueryRequest;
	while (true)
	{
		// �޽��� ���� �̺�Ʈ�� ��ٸ���.
		DWORD retWait = WaitForSingleObject(db._hEventMsg, INFINITE);
		if (retWait != WAIT_OBJECT_0 && retWait != WAIT_IO_COMPLETION)
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"[DB Thread] Wait for event failed!!, error:%d\n", GetLastError());
			return 0;
		}

		// �޽���ť�� �ִ� ��� ������ ó���Ѵ�.
		while (db._msgQ.Dequeue(pQueryRequest))
		{
			switch (pQueryRequest->msgType)
			{
			case MSG_QUERY_REQUEST:
			{
				if (db._pDBConn->ExecuteQuery(pQueryRequest->szQuery) == -1)
					db.Crash();
				break;
			}
			case MSG_SHUTDOWN:
			{
				db._poolQueryRequest.Free(pQueryRequest);
				return 0;
			}
			default:
			{
				LOGGING(LOGGING_LEVEL_ERROR, L"[DB Thread] Invalid message type. type:%d\n", pQueryRequest->msgType);
				break;
			}
			}
			db._poolQueryRequest.Free(pQueryRequest);
		}
	}
	return 0;

}