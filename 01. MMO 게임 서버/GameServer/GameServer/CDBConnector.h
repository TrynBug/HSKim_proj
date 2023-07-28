#pragma once

#include "mysql/mysql.h"
#include "mysql/errmsg.h"

#define MAX_QUERY_LENGTH 10000

class CDBConnector
{
private:
	/* config */
	char _szHost[100];
	char _szUser[100];
	char _szPassword[100];
	char _szDatabase[100];
	unsigned int _port;
	bool _bEnableMultiStatements;

	/* MySQL */
	MYSQL  _mysqlConn;
	MYSQL* _pMysqlConn;

	/* Query */
	wchar_t* _query_utf16;
	char* _query_utf8;

	/* error */
	unsigned int _mysqlErrorNo;
	const char* _mysqlError;
	int _queryRunTimeLimit;  // ms ����. ���� ������ �� �ð� �̻� �ɸ��ٸ� ���� �α׸� �����.
	wchar_t* _mysqlErrorBuffer;  // _mysqlError ���ڿ��� utf16���� ��ȯ�Ͽ� ������ ����

	/* monitor */
	__int64 _queryRunCount;      // ���� ���� Ƚ��
	float _maxQueryRunTime[2];   // �ִ� ���� ����ð�. ms ����. [0] ����ū��, [1] �ι�°��ū��
	float _minQueryRunTime[2];   // �ּ� ���� ����ð�. ms ����. [0] ����������, [1] �ι�°��������
	float _avgQueryRunTime;      // ��� ���� ����ð�. ms ����.
	double _accQueryRunTime;     // ���� ���� ����ð�

	LARGE_INTEGER _liCounterFrequency;  


public:
	CDBConnector();
	~CDBConnector();

public:
	/* set */
	void SetQueryRunTimeLimit(int queryRunTimeLimit) { _queryRunTimeLimit = queryRunTimeLimit; }

	/* connect */
	bool Connect(const char* host, const char* user, const char* password, const char* database, unsigned int port);
	bool Connect(const char* host, const char* user, const char* password, const char* database, unsigned int port, bool bEnableMultiStatements);
	void Disconnect();

	/* query */
	int ExecuteQuery(const wchar_t* query, ...);     // ��ȯ���� ���� ������ �����Ѵ�. ���� �� ������ ����� row ���� �����Ѵ�(0~max). ���� �� -1�� �����Ѵ�.
	MYSQL_RES* ExecuteQueryAndGetResult(const wchar_t* query, ...);  // ��ȯ���� �ִ� ������ �����Ѵ�. ���� �� ���� ����� ����ִ� �����͸� �����Ѵ�. ���н� nullptr�� �����Ѵ�.
	
	/* get query result */
	uint64_t GetNumQueryRows(MYSQL_RES* res);
	MYSQL_ROW FetchRow(MYSQL_RES* res);
	void FreeResult(MYSQL_RES* mysqlRes);
	unsigned int GetMysqlErrorNo() { return _mysqlErrorNo; }
	const wchar_t* GetMysqlError() { return _mysqlErrorBuffer; }

	/* get monitor */
	__int64 GetQueryRunCount() { return _queryRunCount; }
	float GetMaxQueryRunTime() { return _maxQueryRunTime[0]; }
	float Get2ndMaxQueryRunTime() { return _maxQueryRunTime[1]; }
	float GetMinQueryRunTime() { return _minQueryRunTime[0]; }
	float Get2ndMinQueryRunTime() { return _minQueryRunTime[1]; }
	float GetAvgQueryRunTime() { return _avgQueryRunTime; }

private:
	/* error */
	void StoreMysqlError();  // _mysqlError�� mysql_error �Լ� ��ȯ���� �����ϰ�, _mysqlErrorBuffer �� utf16���� ��ȯ�� �������ڿ��� �����Ѵ�.

	/* monitor */
	void UpdateMonitor(float queryRunTime);

};

