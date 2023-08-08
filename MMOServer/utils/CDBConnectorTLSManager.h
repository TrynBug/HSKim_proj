#pragma once

#include "CDBConnector.h"

class CDBConnectorTLSManager
{
	/* config */
	char _szHost[100];
	char _szUser[100];
	char _szPassword[100];
	char _szDatabase[100];
	unsigned int _port;

	/* DB Connector */
	DWORD _tlsIndexDBConnector;
	std::vector<CDBConnector*> _vecDBConnector;
	SRWLOCK _srwlConnect;   // CDBConnector �� Connect �Լ��� lock
	SRWLOCK _srwlVecDBConnector;

	/* option */
	int _queryRunTimeLimit;  // ms ����. ���� ������ �� �ð� �̻� �ɸ��ٸ� ���� �α׸� �����.

public:
	CDBConnectorTLSManager(const char* host, const char* user, const char* password, const char* database, unsigned int port);
	~CDBConnectorTLSManager();

public:
	/* Get DBConnector*/
	CDBConnector* GetTlsDBConnector();

	/* set */
	void SetQueryRunTimeLimit(int queryRunTimeLimit);

	/* Get */
	int GetNumConnection() { return (int)_vecDBConnector.size(); }
	__int64 GetQueryRunCount();
	float GetMaxQueryRunTime();
	float Get2thMaxQueryRunTime();
	float GetMinQueryRunTime();
	float Get2thMinQueryRunTime();
	float GetAvgQueryRunTime();

};



