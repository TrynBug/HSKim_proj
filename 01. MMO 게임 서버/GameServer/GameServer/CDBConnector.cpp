#pragma comment(lib, "libmysql.lib")
#include <iostream>
#include <Windows.h>
#include <strsafe.h>

#include "CDBConnector.h"
#include "logger.h"


CDBConnector::CDBConnector()
	: _szHost{ 0, }, _szUser{ 0, }, _szPassword{ 0, }, _szDatabase{ 0, }, _port(0), _bEnableMultiStatements(false)
	, _mysqlConn(), _pMysqlConn(nullptr), _mysqlErrorNo(0), _mysqlError(nullptr)
	, _maxQueryRunTime{ 0, }, _minQueryRunTime{ 0, }, _avgQueryRunTime(0.f), _accQueryRunTime(0.0)
{
	_query_utf16 = new wchar_t[MAX_QUERY_LENGTH];
	_query_utf8 = new char[MAX_QUERY_LENGTH];
	_mysqlErrorBuffer = new wchar_t[MAX_QUERY_LENGTH];

	_queryRunTimeLimit = 20000;

	_minQueryRunTime[0] = FLT_MAX;
	_minQueryRunTime[1] = FLT_MAX;
	QueryPerformanceFrequency(&_liCounterFrequency);
}


CDBConnector::~CDBConnector()
{
	if(_pMysqlConn != nullptr)
		mysql_close(_pMysqlConn);
	_pMysqlConn = nullptr;
	delete[] _query_utf16;
	delete[] _query_utf8;
	delete[] _mysqlErrorBuffer;
}


bool CDBConnector::Connect(const char* host, const char* user, const char* password, const char* database, unsigned int port)
{
	return Connect(host, user, password, database, port, false);
}


bool CDBConnector::Connect(const char* host, const char* user, const char* password, const char* database, unsigned int port, bool bEnableMultiStatements)
{
	strcpy_s(_szHost, 100, host);
	strcpy_s(_szUser, 100, user);
	strcpy_s(_szPassword, 100, password);
	strcpy_s(_szDatabase, 100, database);
	_port = port;
	_bEnableMultiStatements = bEnableMultiStatements;

	// DB �ʱ�ȭ
	mysql_init(&_mysqlConn);

	// DB ����
	if (bEnableMultiStatements)
	{
		_pMysqlConn = mysql_real_connect(&_mysqlConn, host, user, password, database, port, (char*)NULL, CLIENT_MULTI_STATEMENTS);
	}
	else
	{
		_pMysqlConn = mysql_real_connect(&_mysqlConn, host, user, password, database, port, (char*)NULL, 0);
	}

	if (_pMysqlConn == NULL)
	{
		StoreMysqlError();
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB] Mysql connection error:%s\n", _mysqlErrorBuffer);
		return false;
	}

	// auto reconnect �ɼ� enable
	bool optReconnect = 0;
	mysql_options(_pMysqlConn, MYSQL_OPT_RECONNECT, &optReconnect);

	return true;
}


void CDBConnector::Disconnect()
{
	if (_pMysqlConn != nullptr)
		mysql_close(_pMysqlConn);
	_pMysqlConn = nullptr;
}


/* query */
// ��ȯ���� ���� ������ �����Ѵ�. ���� �� ������ ����� row ���� �����Ѵ�(0~max). ���� �� -1�� �����Ѵ�.
int CDBConnector::ExecuteQuery(const wchar_t* query, ...)
{
	va_list vaList;
	va_start(vaList, query);

	// ���ۿ� ���� write
	HRESULT hr = StringCchVPrintf(_query_utf16, MAX_QUERY_LENGTH, query, vaList);
	if (FAILED(hr))
	{
		if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"[DB] query length exceeds buffer. \n\tquery: %s\n", _query_utf16);
		}
		else
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"[DB] an error occurred within the StringCchVPrinf function. \n\tquery format:%s\n", query);
		}

		return -1;
	}
	va_end(vaList);

	// utf16 ������ utf8�� ��ȯ
	int retConversion = WideCharToMultiByte(CP_UTF8, 0, _query_utf16, -1, _query_utf8, MAX_QUERY_LENGTH, NULL, NULL);
	if (retConversion == 0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB] failed to convert query utf-16 to utf-8. error:%u \n\tquery format:%s \n\tquery utf16:%s\n"
			, GetLastError(), query, _query_utf16);
		return -1;
	}

	// ���� ����
	LARGE_INTEGER liStart;
	LARGE_INTEGER liEnd;
	QueryPerformanceCounter(&liStart);
	int queryState = mysql_query(_pMysqlConn, _query_utf8);
	QueryPerformanceCounter(&liEnd);
	if (queryState != 0)
	{
		// ���� error
		StoreMysqlError();
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB] an error occurred while executing the query. error:%s \n\tquery format:%s \n\tquery utf16:%s\n"
			, _mysqlErrorBuffer, query, _query_utf16);

		return -1;
	}

	// ���� ����ð� ��� �� ����͸� ������Ʈ
	float queryRunTime = (float)((liEnd.QuadPart - liStart.QuadPart) * 1000) / (float)_liCounterFrequency.QuadPart;
	if ((int)queryRunTime > _queryRunTimeLimit)
	{
		// ���� ������ �ʹ� ���� �ɷ���. �����α� ���
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB] query execution took too long time. time:%f \n\tquery utf16:%s\n"
			, queryRunTime, _query_utf16);
	}
	UpdateMonitor(queryRunTime);


	// ������ ����� row ���� ��´�.
	int numAffectedRows = 0;
	if (_bEnableMultiStatements)
	{
		// ������ 1�� �̻��� ���
		numAffectedRows = (int)mysql_affected_rows(_pMysqlConn);  // ù mysql_query ȣ�⿡ ���� ��� ���
		while (true)
		{
			queryState = mysql_next_result(_pMysqlConn);
			if (queryState == 0)
			{
				numAffectedRows += (int)mysql_affected_rows(_pMysqlConn);  // ù��° ������ ������ ���� ��� ���
			}
			else if (queryState == -1)   // mysql_next_result ��ȯ���� -1 �̸� ���̻� ����� ���°�
			{
				break;
			}
			else
			{
				StoreMysqlError();
				LOGGING(LOGGING_LEVEL_ERROR, L"[DB] an error occurred while executing the query. error:%s \n\tquery format:%s \n\tquery utf16:%s\n"
					, _mysqlErrorBuffer, query, _query_utf16);
				return -1;
			}
		}
	}
	else
	{
		numAffectedRows = (int)mysql_affected_rows(_pMysqlConn);
	}

	return numAffectedRows;
}

// ��ȯ���� �ִ� ������ �����Ѵ�. ���� �� ���� ����� ����ִ� �����͸� �����Ѵ�. ���н� nullptr�� �����Ѵ�.
MYSQL_RES* CDBConnector::ExecuteQueryAndGetResult(const wchar_t* query, ...)
{
	va_list vaList;
	va_start(vaList, query);

	// ���ۿ� ���� write
	HRESULT hr = StringCchVPrintf(_query_utf16, MAX_QUERY_LENGTH, query, vaList);
	if (FAILED(hr))
	{
		if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"[DB] query length exceeds buffer. \n\tquery:%s\n", _query_utf16);
		}
		else
		{
			LOGGING(LOGGING_LEVEL_ERROR, L"[DB] an error occurred within the StringCchVPrinf function. \n\tquery format:%s\n", query);
		}

		return nullptr;
	}
	va_end(vaList);

	// utf16 ������ utf8�� ��ȯ
	int retConversion = WideCharToMultiByte(CP_UTF8, 0, _query_utf16, -1, _query_utf8, MAX_QUERY_LENGTH, NULL, NULL);
	if (retConversion == 0)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB] failed to convert query utf-16 to utf-8. error:%u \n\tquery format:%s \n\tquery utf16:%s\n"
			, GetLastError(), query, _query_utf16);
		return nullptr;
	}

	// ���� ����
	LARGE_INTEGER liStart;
	LARGE_INTEGER liEnd;
	QueryPerformanceCounter(&liStart);
	int queryState = mysql_query(_pMysqlConn, _query_utf8);
	QueryPerformanceCounter(&liEnd);
	if (queryState != 0)
	{
		// ���� error
		StoreMysqlError();
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB] an error occurred while executing the query. error:%s \n\tquery format:%s \n\tquery utf16:%s\n"
			, _mysqlErrorBuffer, query, _query_utf16);
		return nullptr;
	}

	// ���� ����ð� ��� �� ����͸� ������Ʈ
	float queryRunTime = (float)((liEnd.QuadPart - liStart.QuadPart) * 1000) / (float)_liCounterFrequency.QuadPart;
	if ((int)queryRunTime > _queryRunTimeLimit)
	{
		// ���� ������ �ʹ� ���� �ɷ���. �����α� ���
		LOGGING(LOGGING_LEVEL_ERROR, L"[DB] query execution took too long time. time:%.2fms \n\tquery utf16:%s \n\tquery utf8:%s\n"
			, queryRunTime, _query_utf16, _query_utf8);
	}
	UpdateMonitor(queryRunTime);

	// ���� ����� ��´�.
	MYSQL_RES* myRes = mysql_store_result(_pMysqlConn);
	if (myRes == nullptr)
	{
		// ���� ����� ���� ������. ���� ����� ���� �������� �ƴϸ� ����� ���� ���Ѱ����� Ȯ����.
		if (mysql_field_count(_pMysqlConn) == 0)
		{
			// ���� ����� ���� ������.
			LOGGING(LOGGING_LEVEL_ERROR, L"[DB] ExecuteQueryAndGetResult function expected to get a result, but the query wasn't with a result. \n\tquery:%s\n"
				, _query_utf16);
		}
		else
		{
			// mysql_store_result �Լ��� �����͸� �����ؾ� ������ �׷��� ������. error
			StoreMysqlError();
			LOGGING(LOGGING_LEVEL_ERROR, L"[DB] Mysql should have returned a result, but it didn't. error:%s \n\tquery format:%s \n\tquery utf16:%s\n"
				, _mysqlErrorBuffer, query, _query_utf16);

		}
		return nullptr;
	}

	

	return myRes;
}


void CDBConnector::FreeResult(MYSQL_RES* mysqlRes)
{
	if (mysqlRes != nullptr)
		mysql_free_result(mysqlRes);

}










/* get query result */
uint64_t CDBConnector::GetNumQueryRows(MYSQL_RES* res)
{
	if (res == nullptr)
		return 0;

	return mysql_num_rows(res);
}

MYSQL_ROW CDBConnector::FetchRow(MYSQL_RES* res)
{
	return mysql_fetch_row(res);
}





/* error */
// _mysqlError�� mysql_error �Լ� ��ȯ���� �����ϰ�, _mysqlErrorBuffer �� utf16���� ��ȯ�� �������ڿ��� �����Ѵ�.
void CDBConnector::StoreMysqlError()
{
	_mysqlErrorNo = mysql_errno(_pMysqlConn);
	_mysqlError = mysql_error(_pMysqlConn);
	MultiByteToWideChar(CP_UTF8, 0, _mysqlError, -1, _mysqlErrorBuffer, MAX_QUERY_LENGTH);
}


/* monitor */
void CDBConnector::UpdateMonitor(float queryRunTime)
{
	if (queryRunTime > _maxQueryRunTime[0])
	{
		_maxQueryRunTime[1] = _maxQueryRunTime[0];
		_maxQueryRunTime[0] = queryRunTime;
	}
	else if (queryRunTime > _maxQueryRunTime[1])
	{
		_maxQueryRunTime[1] = queryRunTime;
	}

	if (queryRunTime < _minQueryRunTime[0])
	{
		_minQueryRunTime[1] = _minQueryRunTime[0];
		_minQueryRunTime[0] = queryRunTime;
	}
	else if (queryRunTime < _minQueryRunTime[1])
	{
		_minQueryRunTime[1] = queryRunTime;
	}

	_queryRunCount++;
	_accQueryRunTime += (double)queryRunTime;
	_avgQueryRunTime = (float)(_accQueryRunTime / (double)_queryRunCount);
}






