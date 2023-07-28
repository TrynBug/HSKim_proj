#include <iostream>
#include <string>
#include <sstream>
#include <windows.h>
#include <unordered_map>
#include <list>


using std::make_pair;
using std::wostringstream;
using std::wstring;


#include "profiler.h"


// ���� �������� �Ŵ��� ��ü ����
CProfileManager g_profileManager;


/* �ܼ� ���ø����̼��� ��쿡�� console control handler �� ����Ͽ�
����ڰ� �ܼ�â ���� �� �������ϸ� ����� ���Ϸ� ��µ� �� �ֵ��� �ؾ��� */
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		g_profileManager.ProfileDataOutText(L"");
		break;
	}
	return true;
}


// �������ϸ� ������ �ʱ�ȭ
void profiler::ResetProfilingData()
{
	g_profileManager.ProfileReset();
}

// �������ϸ� ������ ���Ϸ� ���
void profiler::OutputProfilingData()
{
	g_profileManager.ProfileDataOutText(L"");
}



CProfiler::CProfiler()
	: _tag(nullptr), _isEnd(false)
{
}

CProfiler::CProfiler(const wchar_t* tag)
	: _tag(tag), _isEnd(false)
{
	Begin();
}


CProfiler::~CProfiler()
{
	if (_isEnd == false)  // PROFILE_END �� ȣ���Ѵ��� �Ҹ��ڰ� �ѹ��� ȣ��Ǵ°��� ������
		End(_tag);
}

void CProfiler::Begin(const wchar_t* tag)
{
	_tag = tag;
	Begin();
}

// profiling ����
void CProfiler::Begin()
{
	LARGE_INTEGER liProfileTime;
	QueryPerformanceCounter(&liProfileTime);

	// TLS���� �� �������� �������� �Ŵ����� �����´�.
	PVOID pTlsValue = TlsGetValue(g_profileManager._tlsIndex);
	
	if (pTlsValue == nullptr)
	{
		// ������ �������� �Ŵ����� ���ٸ� �����Ѵ�.
		pTlsValue = new CThreadProfileManager;
		TlsSetValue(g_profileManager._tlsIndex, pTlsValue);

		// �������� �Ŵ����� ������ �������� �Ŵ����� ����Ѵ�.
		AcquireSRWLockExclusive(&g_profileManager._srwlThreadProfileManager);
		g_profileManager._vecThreadProfileManager.push_back((CThreadProfileManager*)pTlsValue);
		ReleaseSRWLockExclusive(&g_profileManager._srwlThreadProfileManager);
	}
	CThreadProfileManager& mgr = *(CThreadProfileManager*)pTlsValue;

	
	// reset �÷��װ� ���õǾ��� ��� reset�Ѵ�.
	if (mgr._bNeedReset == true)
		mgr.Reset();


	// tag�� mapProfileSample�� �ִ��� �˻��Ѵ�.
	auto iter = mgr._mapProfileSample.find(_tag);

	// ���� tag�� ���ٸ� �������� �����͸� �����Ѵ�.
	if (iter == mgr._mapProfileSample.end())
	{
		ProfileSample* pProfileSample = new ProfileSample(_tag);
		mgr._mapProfileSample.insert(make_pair(_tag, pProfileSample));
		iter = mgr._mapProfileSample.find(_tag);
	}
	ProfileSample* pSample = iter->second;


	// ���� �������� ������ �������� ���� ���Ϳ� �ִ´�. 
	mgr._vecProfileHierarchy.push_back(&(iter->second->iTotalTime));


	// �������ϸ� ���� �ð� ���
	QueryPerformanceCounter(&(pSample->liStartTime));
	// �������ϸ� ��ü�� �ҿ�� �ð� ���
	mgr._iTotalProfileCall++;
	__int64 profileTime = pSample->liStartTime.QuadPart - liProfileTime.QuadPart;
	mgr._iTotalProfileTime += profileTime;
	// �������� ������ �ڽ��� ������ ��� ������ �ҿ�ð����� �������ϸ� ��ü �ҿ�ð��� ����.
	for (int i = 0; i < (int)mgr._vecProfileHierarchy.size() - 1; i++)
		*(mgr._vecProfileHierarchy[i]) -= profileTime;

}


// profiling ����
void CProfiler::End(const wchar_t* tag)
{
	// ���� �ð��� ���Ѵ�.
	LARGE_INTEGER liEndTime;
	QueryPerformanceCounter(&liEndTime);

	// ���� tag != _tag �� ���, ���ڿ��� ���غ��� ���ٸ� ok, �ٸ��ٸ� error.
	if (tag != _tag)
	{
		if (wcscmp(tag, _tag) != 0)
		{
			int* p = nullptr;
			*p = 0;
		}
	}

	// ���� �������Ϸ����� End�� 2�� ���� ȣ��� ��� error
	if (_isEnd == true)
	{
		int* p = nullptr;
		*p = 0;
	}


	// TLS���� ������ �������� �Ŵ����� �����´�.
	CThreadProfileManager& mgr = *(CThreadProfileManager*)TlsGetValue(g_profileManager._tlsIndex);

	// �ҿ�ð��� ����Ѵ�.
	auto iter = mgr._mapProfileSample.find(_tag);

	ProfileSample* pSample = iter->second;
	__int64 timeDelta = liEndTime.QuadPart - pSample->liStartTime.QuadPart;
	pSample->iTotalTime += timeDelta;
	pSample->iCall++;


	// min, max ��ü
	if (timeDelta < pSample->iMin[0])
	{
		pSample->iMin[1] = pSample->iMin[0];
		pSample->iMin[0] = timeDelta;
	}
	else if (timeDelta < pSample->iMin[1])
	{
		pSample->iMin[1] = timeDelta;
	}

	if (timeDelta > pSample->iMax[0])
	{
		pSample->iMax[1] = pSample->iMax[0];
		pSample->iMax[0] = timeDelta;
	}
	else if (timeDelta > pSample->iMax[1])
	{
		pSample->iMax[1] = timeDelta;
	}

	// ���� �÷��� ����
	_isEnd = true;

	// ���� �������� ������ �������� ���� ���Ϳ��� �����Ѵ�.
	mgr._vecProfileHierarchy.pop_back();

	// �������ϸ� ��ü�� �ҿ�� �ð� ���
	LARGE_INTEGER liProfileTime;
	QueryPerformanceCounter(&liProfileTime);
	__int64 profileTime = liProfileTime.QuadPart - liEndTime.QuadPart;
	mgr._iTotalProfileTime += profileTime;

	// �������� ������ �ڽ��� ������ ��� ������ �ҿ�ð����� �������ϸ� ��ü �ҿ�ð��� ����.
	for (int i = 0; i < (int)mgr._vecProfileHierarchy.size(); i++)
		*(mgr._vecProfileHierarchy[i]) -= profileTime;

}








CProfileManager::CProfileManager()
{
	_tlsIndex = TlsAlloc();
	if (_tlsIndex == TLS_OUT_OF_INDEXES)
	{
		printf("TlsAlloc failed. return value:%u, error:%u\n", _tlsIndex, GetLastError());
	}

	InitializeSRWLock(&_srwlThreadProfileManager);
}


CProfileManager::~CProfileManager()
{
	if(g_profileManager._bProfileStarted)
		ProfileDataOutText(L"");
}


// profiling �����͸� Text ���Ϸ� ����Ѵ�.
// Parameters: (const wchar_t*) ��µ� ���� �̸�
void CProfileManager::ProfileDataOutText(const wchar_t* strFilePath)
{
	// ��ü �������ϸ� �����͸� ����Ʈ�� ����
	// �����ϴ� �߰��� ProfileSample ����ü ���� ����� �� ������, ������ ������ �����ϱ�� ��.
	// �׸��� �����ϴ� �߰��� CThreadProfileManager �� map�� �߰��� insert�� �ɼ� �ִµ� �װŶ����� ���������� ������
	std::list<ProfileSample> listProfileSample;
	__int64 totalProfileCall = 0;
	__int64 totalProfileTime = 0;

	CThreadProfileManager* pThreadMgr;
	AcquireSRWLockShared(&_srwlThreadProfileManager);
	for (size_t i = 0; i < _vecThreadProfileManager.size(); i++)
	{
		pThreadMgr = _vecThreadProfileManager[i];
		if (pThreadMgr->_bNeedReset == true) // �����ؾ��ϴµ� ���� �������� �Ѿ
			continue;

		totalProfileCall += pThreadMgr->_iTotalProfileCall;
		totalProfileTime += pThreadMgr->_iTotalProfileTime;
		for (auto iter = pThreadMgr->_mapProfileSample.begin(); iter != pThreadMgr->_mapProfileSample.end(); ++iter)
		{
			listProfileSample.push_back(ProfileSample(*(iter->second)));
		}
	}
	ReleaseSRWLockShared(&_srwlThreadProfileManager);

	// tag�� ���� �������ϸ� �����͸� ��ħ
	for (auto iter = listProfileSample.begin(); iter != listProfileSample.end(); ++iter)
	{
		ProfileSample& sample = *iter;

		auto iterInner = iter;
		++iterInner;
		for (; iterInner != listProfileSample.end(); )
		{
			ProfileSample& sampleInner = *iterInner;
			if (sample.tagName == sampleInner.tagName)
			{
				// tag�� ���� 2�� �������ϸ� �����͸� ��ģ��.
				sample.iTotalTime += sampleInner.iTotalTime;
				sample.iCall += sampleInner.iCall;

				if (sample.iMax[0] > sampleInner.iMax[0])
				{
					sample.iMax[1] = max(sample.iMax[1], sampleInner.iMax[0]);
				}
				else
				{
					sample.iMax[0] = sampleInner.iMax[0];
					sample.iMax[1] = max(sample.iMax[0], sampleInner.iMax[1]);
				}
				
				if (sample.iMin[0] < sampleInner.iMin[0])
				{
					sample.iMin[1] = min(sample.iMin[1], sampleInner.iMin[0]);
				}
				else
				{
					sample.iMin[0] = sampleInner.iMin[0];
					sample.iMin[1] = min(sample.iMin[0], sampleInner.iMin[1]);
				}
				

				// ������ �����ʹ� ����
				iterInner = listProfileSample.erase(iterInner);
			}
			else
			{
				++iterInner;
			}
		}
	}


	// ���ϸ� ����
	struct tm curTime;
	time_t longTime;
	time(&longTime);
	localtime_s(&curTime, &longTime);
	WCHAR szProfileFileName[31];
	swprintf_s(szProfileFileName, 31, L"Profiling_%04d%02d%02d_%02d%02d%02d.txt"
		, curTime.tm_year, curTime.tm_mon + 1, curTime.tm_mday, curTime.tm_hour, curTime.tm_min, curTime.tm_sec);

	wstring strProfileFileName = strFilePath;
	strProfileFileName += szProfileFileName;


	// ���� ����
	FILE* fProfile;
	_wfopen_s(&fProfile, strProfileFileName.c_str(), L"w, ccs=UTF-16LE");
	wstring strHeader = L"Name\tTotal(��)\tAverage(��)\tMin(��)\tMax(��)\tCall(num)\n";
	fwrite(strHeader.c_str(), 2, strHeader.size(), fProfile);

	// �������ϸ� ������ ��� �غ�
	LARGE_INTEGER liFreq;
	QueryPerformanceFrequency(&liFreq);
	double dFreq = (double)liFreq.QuadPart;
	wostringstream oss;
	wstring strOutput;
	oss.flags(oss.fixed);
	oss.precision(4);


	// �������ϸ� ������ ���
	for (auto iter = listProfileSample.begin(); iter != listProfileSample.end(); ++iter)
	{
		ProfileSample& pSample = *iter;
		__int64 iTotalTime = pSample.iTotalTime;
		__int64 iCall = pSample.iCall;

		if (iCall > 2)
		{
			iTotalTime = iTotalTime - pSample.iMin[0] - pSample.iMax[0];
			iCall -= 2;
		}

		double dTotal = (double)iTotalTime / dFreq * 1000000.0;
		double dAverage = (double)iTotalTime / (double)iCall / dFreq * 1000000.0;
		double dMin = (double)pSample.iMin[1] / dFreq * 1000000.0;
		double dMax = (double)pSample.iMax[1] / dFreq * 1000000.0;

		oss.str(L"");
		oss << pSample.tagName << L"\t" << dTotal << L"\t" << dAverage << L"\t" <<
			dMin << L"\t" << dMax << L"\t" << iCall << L"\n";
		strOutput = oss.str();
		fwrite(strOutput.c_str(), 2, strOutput.size(), fProfile);
	}


	// �������ϸ� ��ü �ҿ�ð� ���
	oss.str(L"");
	oss << L"Total profiling time\t" << totalProfileTime << L"\t"
		<< (double)totalProfileTime / (double)totalProfileCall / dFreq * 1000000.0
		<< L"\t\t\t" << totalProfileCall << L"\n";
	strOutput = oss.str();
	fwrite(strOutput.c_str(), 2, strOutput.size(), fProfile);



	fclose(fProfile);
}



// ��ü �������ϸ� �����͸� �ʱ�ȭ�Ѵ�.
// �ʱ�ȭ �÷��׸� �����ϰ� ���� �ʱ�ȭ�� �����庰�� ���� �����Ѵ�. (�������� ���ÿ� lock�� �����ʱ� ����)
void CProfileManager::ProfileReset(void)
{
	AcquireSRWLockShared(&_srwlThreadProfileManager);
	for (size_t i = 0; i < _vecThreadProfileManager.size(); i++)
	{
		_vecThreadProfileManager[i]->_bNeedReset = true;
	}
	ReleaseSRWLockShared(&_srwlThreadProfileManager);
}




CThreadProfileManager::CThreadProfileManager()
	:_iTotalProfileCall(0), _iTotalProfileTime(0), _bNeedReset(false)
{
	// �������ϸ� ����� �÷��� on
	if (InterlockedExchange8((char*)&g_profileManager._bProfileStarted, true) == false)
	{
		// �÷��׸� ó�� on�� ��, �ܼ� ���ø����̼��� ��쿡�� console control handler �� ����Ͽ� ����ڰ� �ܼ�â ���� �� �������ϸ� ����� ���Ϸ� ��µ� �� �ֵ��� ��
	#ifdef _CONSOLE
		BOOL fSuccess = SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
	#endif
	}
}

CThreadProfileManager::~CThreadProfileManager()
{
}

// ������ ���� �������ϸ� �����͸� �ʱ�ȭ�Ѵ�.
void CThreadProfileManager::Reset()
{
	ProfileSample* pSample;
	for (auto iter = _mapProfileSample.begin(); iter != _mapProfileSample.end(); ++iter)
	{
		pSample = iter->second;

		// pSample->liStartTime.QuadPart = 0; // liStartTime�� �ʱ�ȭ���� ����
		pSample->iTotalTime = 0;
		pSample->iMin[0] = INT64_MAX;
		pSample->iMin[1] = INT64_MAX;
		pSample->iMax[0] = 0;
		pSample->iMax[1] = 0;
		pSample->iCall = 0;
	}

	_iTotalProfileCall = 0;
	_iTotalProfileTime = 0;
	_bNeedReset = false;
}



