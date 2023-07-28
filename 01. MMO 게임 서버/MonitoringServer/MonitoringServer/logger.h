#pragma once

#include <Windows.h>
#include <vector>

#define LOGGING_LEVEL_DEBUG 10
#define LOGGING_LEVEL_INFO  11
#define LOGGING_LEVEL_WARN  12
#define LOGGING_LEVEL_ERROR 13
#define LOGGING_LEVEL_FATAL 14
#define LOGGING_LEVEL_NONE  15

#define LOGGING_FILE_LENGTH 25
#define LOGGING_NUM_OF_MAX_CHAR 10000

// LOGGING ��ũ��. Log �Լ��� ȣ���ϱ� �� logLevel�� Ȯ���Ͽ� ������ ��µǴ� �α׸� Log �Լ��� ȣ��ǵ��� �Ѵ�.
// ���� ��µ��� �ʴ� �α׿� ���� �Լ� ȣ�� ���, ���� ���� ����� ���� �� �ִ�.
#define LOGGING(logLevel, szFormat, ...) do { \
	if (logLevel >= logger::ex_Logger.GetConsoleLoggingLevel() || logLevel >= logger::ex_Logger.GetFileLoggingLevel()) \
		logger::ex_Logger.Log(logLevel, szFormat, ##__VA_ARGS__); \
} while(0) 

#define LOGGING_VALIST(logLevel, szFormat, vaList) do { \
	if (logLevel >= logger::ex_Logger.GetConsoleLoggingLevel() || logLevel >= logger::ex_Logger.GetFileLoggingLevel()) \
		logger::ex_Logger.Log(logLevel, szFormat, vaList); \
} while(0) 


class CLogger;
namespace logger
{
	extern CLogger ex_Logger;
}



class CLogger
{
	friend class _CLoggerTLS;

public:
	CLogger();
	~CLogger();

private:
	LARGE_INTEGER _filePointer;                   // ���� ���� ������ ��ġ
	wchar_t _szLogFileName[LOGGING_FILE_LENGTH];  // ���ϸ�
	int _consoleLoggingLevel;                     // �ܼ� �α� ����
	int _fileLoggingLevel;                        // ���� �α� ����

	DWORD _tlsIndex;                              // TLS index
	std::vector<_CLoggerTLS*> _vecLoggerTLS;      // ������ ������ �α� ��ü ������ ���� ����
	SRWLOCK _srwlVecLoggerTLS;                    // ���� ���� lock

	// �ܼ� �α� ����
	bool _bUseConsoleLoggingLimit;                // �ܼ� �α� ���� ��뿩��. SetConsoleLoggingLimitLevel �Լ��� ȣ���ϸ� true�� ������.
	long _consoleLoggingLimitCount;               // �ܼ� �α� ���� ī��Ʈ. �ֿܼ� ����� Ƚ���� �����ϰ� �ʹٸ� �� ���� �����Ѵ�. ���Ͽ��� ���������� ��µȴ�. 
												  // �� ���� _limitedConsoleLoggingLevel ���� �Ʒ��� �α׸� �ֿܼ� ����� �� ���� -1 �ȴ�.
												  // �����Ӵ� ��� Ƚ�� ���� �뵵�� ����ϰ� �ʹٸ� �� �����Ӹ��� �� ���� �������־�� �Ѵ�.
	int _consoleLoggingLimitLevel;                // _bUseConsoleLoggingLimit == true && _limitedConsoleLoggingCount == 0 �� �� �� ���� �Ʒ��δ� �ֿܼ� ������� �ʴ´�. (���Ͽ��� ��µ�)
	long _arrUnprintedLogCount[6];                 // �ܼ� �α� �������� ���� ��µ��� ���� �α� Ƚ�� ī��Ʈ

	__int64 _logNum;                              // �α� ��ȣ

public:
	/* getter */
	int GetConsoleLoggingLevel() { return _consoleLoggingLevel; }
	int GetFileLoggingLevel() { return _fileLoggingLevel; }

	/* setter */
	void SetConsoleLoggingLevel(int loggingLevel);
	void SetFileLoggingLevel(int loggingLevel);
	void SetConsoleLoggingLimitLevel(int loggingLevel);
	void SetConsoleLoggingLimitCount(unsigned int count);

public:
	/* logging */
	// Log �Լ��� ���� ������� ���� LOGGING ��ũ�θ� ����ϵ��� ����.
	void Log(int logLevel, const wchar_t* szFormat, ...);
	void Log(int logLevel, const wchar_t* szFormat, va_list vaList);

private:
	/* write head log */
	inline int WriteHeadLog(int logLevel, wchar_t* buffer, int bufLen);

};


class _CLoggerTLS
{
	friend class CLogger;

	HANDLE _hLogfile;    // ���� �ڵ�
	wchar_t* _szLog;     // �α� ���ڿ�
	LARGE_INTEGER _prevFilePointer;     // ���� ���� ������ ��ġ

	CLogger* _pLogger;   // ���� CLogger Ŭ����

	_CLoggerTLS(CLogger* pLogger);
	~_CLoggerTLS();
};
