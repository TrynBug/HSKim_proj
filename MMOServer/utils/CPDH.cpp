#pragma comment(lib, "pdh.lib")

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <strsafe.h>

#include "CPDH.h"




CPDH::CPDH()
    : _dwFlagsQueryEntry(0xFFFFFFFF)
    , _szProcessName { 0, }, _pdh_Status(0), _pdh_Query(0)
    , _pdhQueryResult{ 0, }, _arrPdhNetworkQueryResult(nullptr), _numNetworkInterface(0), _pdhCount{ 0, }
{
}

CPDH::CPDH(DWORD dwFlagsQueryEntry)
    : _dwFlagsQueryEntry(dwFlagsQueryEntry)
    , _szProcessName{ 0, }, _pdh_Status(0), _pdh_Query(0)
    , _pdhQueryResult{ 0, }, _arrPdhNetworkQueryResult(nullptr), _numNetworkInterface(0), _pdhCount{ 0, }
{
}

CPDH::~CPDH()
{

}



bool CPDH::Init()
{
    // Create a query.
    _pdh_Status = PdhOpenQuery(NULL, NULL, &_pdh_Query);
    if (_pdh_Status != ERROR_SUCCESS)
    {
        wprintf(L"[PDH] PdhOpenQuery failed with status 0x%x.\n", _pdh_Status);
        if (_pdh_Query)
        {
            PdhCloseQuery(_pdh_Query);
        }
        return false;
    }


    /* ��Ʈ��ũ ī��Ʈ�� ��� ���� ����ϱ� */
    WCHAR* szCounters = NULL;
    WCHAR* szInterfaces = NULL;
    DWORD dwCounterSize = 0;
    DWORD dwInterfaceSize = 0;

    // PdhEnumObjectItems �� ���ؼ� "Network Interface" �׸񿡼� ���� �� �ִ� �����׸�(Counters), �������̽� �׸�(Interfaces)�� ����.
    // �׷��� ������ ���̸� �𸣱� ������ ���� ������ ���̸� �˱� ���ؼ� Out Buffer ���ڵ��� NULL �����ͷ� �־ ����� Ȯ����.
    // �׸��� ��� �̴����� �̸��� �������� ���� ������� �̴������� �����̴������� ����� �����Ҽ�����.
    PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);

    // ������ �����Ҵ� �� �ٽ� ȣ��!
    // szCounters �� szInterfaces ���ۿ��� �������� ���ڿ��� ������ ���´�.
    // 2���� �迭�� �ƴϰ�, �׳� NULL �����ͷ� ������ ���ڿ����� dwCounterSize, dwInterfaceSize ���̸�ŭ ������ �������.
    // �̸� ���ڿ� ������ ��� ������ Ȯ���ؾ���. aaa\0bbb\0ccc\0ddd �̵� ��
    szCounters = new WCHAR[dwCounterSize];      // 512
    szInterfaces = new WCHAR[dwInterfaceSize];  // 36
    if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
    {
        PdhCloseQuery(_pdh_Query);
        delete[] szCounters;
        delete[] szInterfaces;
        szInterfaces = nullptr;
        szCounters = nullptr;
        return false;
    }


    // ��Ʈ��ũ �������̽� ���� ã��, ����ŭ ��Ʈ��ũ count ����ü�� �����.
    _numNetworkInterface = 0;
    for (unsigned int i = 0; i < dwInterfaceSize - 1; i++)
    {
        if (*(szInterfaces + i) == L'\0')
            _numNetworkInterface++;
    }
    _arrPdhNetworkQueryResult = new _PDHNetworkQueryResult[_numNetworkInterface];

    // ��Ʈ��ũ ī��Ʈ�� ��� ���� ���
    // Bytes Received/sec�� �����̹� ���ڸ� �����Ͽ� �� ��Ʈ��ũ ����Ϳ��� �޴� ����Ʈ�� �����Դϴ�. Network Interface\\Bytes Received/sec�� Network Interface\\Bytes Total/sec�� ���� �����Դϴ�.
    // Bytes Sent/sec �� �����̹� ���ڸ� �����Ͽ� ��Ʈ��ũ ����Ϳ��� ���� ����Ʈ�� �����Դϴ�. Network Interface\\Bytes Sent/sec�� Network Interface\\Bytes Total/sec�� ���� �����Դϴ�.
    WCHAR* szCur = szInterfaces;
    WCHAR szQuery[1024] = { 0, };
    int idx = 0;
    if (_dwFlagsQueryEntry & PDH_FLAG_QUERY_ENTRY_NETWORK)
    {
        for (unsigned int i = 0; i < dwInterfaceSize - 1; i++)
        {
            if (*(szInterfaces + i) == L'\0')
            {
                // szInterfaces �� ���ڿ� ������ ��� ��Ʈ��ũ count ����ü�� �����Ѵ�.
                _arrPdhNetworkQueryResult[idx].szName[0] = L'\0';
                wcscpy_s(_arrPdhNetworkQueryResult[idx].szName, szCur);

                // ���� ���
                szQuery[0] = L'\0';
                StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);  // "\Network Interface(Realtek PCIe GbE Family Controller)\Bytes Received/sec"
                PdhAddCounter(_pdh_Query, szQuery, NULL, &_arrPdhNetworkQueryResult[idx].networkRecvBytes);  // �޴� ������ ����Ʈ�� ���� ���. ������ ���ڴ� Handle to the counter that was added to the query �̴�.

                szQuery[0] = L'\0';
                StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);  // "\Network Interface(Realtek PCIe GbE Family Controller)\Bytes Sent/sec"
                PdhAddCounter(_pdh_Query, szQuery, NULL, &_arrPdhNetworkQueryResult[idx].networkSendBytes);  // ������ ������ ����Ʈ�� ���� ���

                szCur = szInterfaces + i + 1;
                idx++;
            }
        }
    }


    /* TCP segment, ������ ���� ����ϱ� */
    // Segments Sent/sec�� �ٽ� ���۵Ǵ� ����Ʈ�� ����ִ� ���׸�Ʈ�� �����ϰ�, ���� ���ῡ �ִ� ���׸�Ʈ�� �����Ͽ� ���׸�Ʈ�� ������ �����Դϴ�.
    // Segments Retransmitted/sec�� ���׸�Ʈ�� �ٽ� �����ϴ� �����Դϴ�. �� ������ ���۵� ����Ʈ�� �ϳ� �̻� ���Ե� ���׸�Ʈ�� �����ϴ� ���Դϴ�.
    if (_dwFlagsQueryEntry & PDH_FLAG_QUERY_ENTRY_NETWORK)
    {
        PdhAddCounter(_pdh_Query, L"\\TCPv4\\Segments Sent/sec", NULL, &_pdhQueryResult.TCPSegmentsSent);
        PdhAddCounter(_pdh_Query, L"\\TCPv4\\Segments Retransmitted/sec", NULL, &_pdhQueryResult.TCPSegmentsRetransmitted);
    }

    /* �ý��� �޸� ī��Ʈ�� ��� ���� ����ϱ� */
    // Available Bytes�� ��ǻ�Ϳ��� ����Ǵ� ���μ����� �Ҵ��ϰų� �ý��ۿ��� ����� �� �ִ� ���� �޸��� ��(����Ʈ)�Դϴ�. �̰��� ��� ���̰ų� ��� �ְų� 0���� ä���� ������ ��Ͽ� �Ҵ�� �޸��� �Ѱ��Դϴ�.
    // Committed Bytes�� Ŀ�Ե� ���� �޸��� ��(����Ʈ)�Դϴ�. Ŀ�Ե� �޸𸮴� ��ũ ����¡ ������ ��ũ�� �ٽ� ��� �� �ʿ䰡 ���� ��츦 ���� ����� ���� �޸��Դϴ�. �� ���� ����̺꿡 �ϳ� �̻��� ����¡ ������ ���� �� �ֽ��ϴ�. �� ī���ʹ� �ֱٿ� ������ ���� ǥ���ϸ� ��հ��� �ƴմϴ�.
    // Pool Nonpaged Bytes�� ������¡ Ǯ�� ũ��(����Ʈ)�̰�, ������¡ Ǯ�� ��ũ�� �� �� ������ �Ҵ�Ǿ� �ִ� ���� ���� �޸𸮿� ���� �־�� �ϴ� ��ü�� ���Ǵ� �ý��� ���� �޸� �����Դϴ�. Memory\\Pool Nonpaged Bytes�� Process\\Pool Nonpaged Bytes�� �ٸ��� ���ǹǷ� Process(_Total)\\Pool Nonpaged Bytes�� ���� ���� �� �ֽ��ϴ�. �� ī���ʹ� �ֱٿ� ������ ���� ǥ���ϸ� ��հ��� �ƴմϴ�.
    // Pool Paged Bytes�� ����¡ Ǯ�� ũ��(����Ʈ)�̰�, ����¡ Ǯ�� ������ �ʰ� ���� �� ��ũ�� �� �� �ִ� ��ü�� ���Ǵ� �ý��� ���� �޸� �����Դϴ�. Memory\\Pool Paged Bytes�� Process\\Pool Paged Bytes�� �ٸ��� ���ǹǷ� Process(_Total)\\Pool Paged Bytes�� ���� ���� �� �ֽ��ϴ�. �� ī���ʹ� �ֱٿ� ������ ���� ǥ���ϸ� ��հ��� �ƴմϴ�.
    if (_dwFlagsQueryEntry & PDH_FLAG_QUERY_ENTRY_SYSTEM_MEMORY)
    {
        PdhAddCounter(_pdh_Query, L"\\Memory\\Available Bytes", NULL, &_pdhQueryResult.systemAvailableBytes);
        PdhAddCounter(_pdh_Query, L"\\Memory\\Committed Bytes", NULL, &_pdhQueryResult.systemComittedBytes);
        PdhAddCounter(_pdh_Query, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_pdhQueryResult.systemNonpagedPoolBytes);
        PdhAddCounter(_pdh_Query, L"\\Memory\\Pool Paged Bytes", NULL, &_pdhQueryResult.systemPagedPoolBytes);
    }

    /* ���μ��� �޸� ī��Ʈ�� ��� ���� ����ϱ� */
    // ���μ��� �̸� ���(.exe����)
    WCHAR szExePath[MAX_PATH];
    int lenPath = GetModuleFileNameW(nullptr, szExePath, MAX_PATH);  // ���μ��� ��ü ��� ���(lenPath�� null ������ ���ڿ�����)
    int locDot = lenPath;  // ������ dot ��ġ
    for (int i = lenPath; i >= 0; i--)
    {
        if (szExePath[i] == L'.' && locDot == lenPath)  // ������ dot ��ġ ���
            locDot = i;
        if (szExePath[i] == L'\\')  // ������ \ ��ġ�� ���������� \��ġ+1 ���� ������dot��ġ���� memcpy �Ѵ�.
        {
            memcpy(_szProcessName, szExePath + i + 1, (locDot - i - 1) * 2);
            szExePath[locDot - i - 1] = L'\0';
            break;
        }
    }

    // ���μ��� �޸� ī��Ʈ�� ��� ���� ����ϱ�
    // Page Faults/sec�� �� ���μ������� ���� ���� �����忡�� �߻��ϴ� ������ ��Ʈ ���� �����Դϴ�. ������ ��Ʈ�� ���μ����� �� �޸��� �۾� ���տ� ���� ���� �޸� �������� ������ �� �߻��մϴ�. �̰��� �������� ���� ��� ��Ͽ� ���� �ʰ� �̹� �� �޸𸮿� �ְų� �������� �����ϴ� �ٸ� ���μ������� ���ǰ� ������ ��ũ���� �������� �������� �ʽ��ϴ�.
    // Pool Nonpaged Bytes�� ������¡ Ǯ�� ũ��(����Ʈ)�̰�, ������¡ Ǯ�� ��ũ�� �� �� ������ �Ҵ�Ǿ� �ִ� ���� ���� �޸𸮿� ���� �־�� �ϴ� ��ü�� ���Ǵ� �ý��� ���� �޸� �����Դϴ�. Memory\\Pool Nonpaged Bytes�� Process\\Pool Nonpaged Bytes�� �ٸ��� ���ǹǷ� Process(_Total)\\Pool Nonpaged Bytes�� ���� ���� �� �ֽ��ϴ�. �� ī���ʹ� �ֱٿ� ������ ���� ǥ���ϸ� ��հ��� �ƴմϴ�.
    // Pool Paged Bytes�� ����¡ Ǯ�� ũ��(����Ʈ)�̰�, ����¡ Ǯ�� ������ �ʰ� ���� �� ��ũ�� �� �� �ִ� ��ü�� ���Ǵ� �ý��� ���� �޸� �����Դϴ�. Memory\\Pool Paged Bytes�� Process\\Pool Paged Bytes�� �ٸ��� ���ǹǷ� Process(_Total)\\Pool Paged Bytes�� ���� ���� �� �ֽ��ϴ�. �� ī���ʹ� �ֱٿ� ������ ���� ǥ���ϸ� ��հ��� �ƴմϴ�.
    // Private Bytes�� �� ���μ����� �Ҵ��Ͽ� �ٸ� ���μ����ʹ� ������ �� ���� �޸��� ���� ũ��(����Ʈ)�Դϴ�.
    // Working Set�� �� ���μ����� ���� �۾� ������ ���� ũ��(����Ʈ)�Դϴ�. �۾� ������ ���μ����� �����尡 �ֱٿ� �۾��� �޸� �������� �����Դϴ�. ��ǻ�Ϳ� �ִ� �� �޸𸮰� �Ѱ踦 �ʰ��ϸ� �������� ��� ���� �ƴ϶� ���μ����� �۾� ���տ� ���� �ֽ��ϴ�. �� �޸𸮰� �Ѱ� �̸��̸� �������� �۾� ���տ��� �������ϴ�. �� �������� �ʿ��ϸ� �� �޸𸮿��� �������� ���� ����Ʈ ���� ó���Ǿ� �ٽ� �۾� ���տ� �ְ� �˴ϴ�.
    // Working Set - Private�� �ٸ� ���μ����� �����ϰų� ������ �� �ִ� �۾� ������ �ƴ϶� �� ���μ����� ����ϰ� �ִ� �۾� ������ ũ��(����Ʈ)�Դϴ�.
    if (_dwFlagsQueryEntry & PDH_FLAG_QUERY_ENTRY_PROCESS_MEMORY)
    {
        StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Page Faults/sec", _szProcessName);
        PdhAddCounter(_pdh_Query, szQuery, NULL, &_pdhQueryResult.processPageFaultRate);
        StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Pool Nonpaged Bytes", _szProcessName);
        PdhAddCounter(_pdh_Query, szQuery, NULL, &_pdhQueryResult.processNonpagedPoolBytes);
        StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Pool Paged Bytes", _szProcessName);
        PdhAddCounter(_pdh_Query, szQuery, NULL, &_pdhQueryResult.processPagedPoolBytes);
        StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Private Bytes", _szProcessName);
        PdhAddCounter(_pdh_Query, szQuery, NULL, &_pdhQueryResult.processPrivateBytes);
        StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Working Set", _szProcessName);
        PdhAddCounter(_pdh_Query, szQuery, NULL, &_pdhQueryResult.processWorkingSet);
        StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Process(%s)\\Working Set - Private", _szProcessName);
        PdhAddCounter(_pdh_Query, szQuery, NULL, &_pdhQueryResult.processPrivateWorkingSet);
    }


    /* ���� ���� ���� */
    // Most counters require two sample values to display a formatted value. PDH stores the current sample value and the previously collected sample value. 
    // This call retrieves the first value that will be used by PdhGetFormattedCounterValue in the first iteration of the loop.
    // Note that this value is lost if the counter does not require two values to compute a displayable value.
    _pdh_Status = PdhCollectQueryData(_pdh_Query);
    if (_pdh_Status != ERROR_SUCCESS)
    {
        wprintf(L"[PDH] PdhCollectQueryData failed with 0x%x.\n", _pdh_Status);
        PdhCloseQuery(_pdh_Query);
        delete[] szCounters;
        delete[] szInterfaces;
        delete[] _arrPdhNetworkQueryResult;
        szInterfaces = nullptr;
        szCounters = nullptr;
        _arrPdhNetworkQueryResult = nullptr;
        return false;
    }

    return true;
}


bool CPDH::Update()
{
    _pdh_Status = PdhCollectQueryData(_pdh_Query);
    if (_pdh_Status != ERROR_SUCCESS)
    {
        wprintf(L"[PDH] PdhCollectQueryData failed with status 0x%x.\n", _pdh_Status);
        return false;
    }

    /* ��Ʈ��ũ ī��Ʈ �հ踦 ����� ���� ����*/
    PDH_FMT_COUNTERVALUE counterValue;
    _pdhCount.networkRecvBytes = 0.0;
    _pdhCount.networkSendBytes = 0.0;
    if (_dwFlagsQueryEntry & PDH_FLAG_QUERY_ENTRY_NETWORK)
    {
        for (int i = 0; i < _numNetworkInterface; i++)
        {
            _pdh_Status = PdhGetFormattedCounterValue(_arrPdhNetworkQueryResult[i].networkRecvBytes, PDH_FMT_DOUBLE, NULL, &counterValue);
            if (_pdh_Status == 0)
                _pdhCount.networkRecvBytes += counterValue.doubleValue;
            else
                wprintf(L"[PDH] PdhGetFormattedCounterValue 1 failed with status 0x%x.\n", _pdh_Status);

            _pdh_Status = PdhGetFormattedCounterValue(_arrPdhNetworkQueryResult[i].networkSendBytes, PDH_FMT_DOUBLE, NULL, &counterValue);
            if (_pdh_Status == 0)
                _pdhCount.networkSendBytes += counterValue.doubleValue;
            else
                wprintf(L"[PDH] PdhGetFormattedCounterValue 2 failed with status 0x%x.\n", _pdh_Status);
        }
    }

    /* �ý���, ���μ��� ī��Ʈ ���� */
#define _PDH_STORE_QUERY_RESULT_DOUBLE(counter) \
    _pdh_Status = PdhGetFormattedCounterValue(_pdhQueryResult.counter, PDH_FMT_DOUBLE, NULL, &counterValue); \
    if (_pdh_Status == 0) \
        _pdhCount.counter = counterValue.doubleValue; \
    else \
        wprintf(L"[PDH] PdhGetFormattedCounterValue %s failed with status 0x%x.\n", L#counter, _pdh_Status);

#define _PDH_STORE_QUERY_RESULT_INT64(counter) \
    _pdh_Status = PdhGetFormattedCounterValue(_pdhQueryResult.counter, PDH_FMT_LARGE, NULL, &counterValue); \
    if (_pdh_Status == 0) \
        _pdhCount.counter = counterValue.largeValue; \
    else \
        wprintf(L"[PDH] PdhGetFormattedCounterValue %s failed with status 0x%x.\n", L#counter, _pdh_Status);

    if (_dwFlagsQueryEntry & PDH_FLAG_QUERY_ENTRY_NETWORK) 
    {
        _PDH_STORE_QUERY_RESULT_INT64(TCPSegmentsSent);
        _PDH_STORE_QUERY_RESULT_INT64(TCPSegmentsRetransmitted);
    }
    
    if (_dwFlagsQueryEntry & PDH_FLAG_QUERY_ENTRY_SYSTEM_MEMORY)
    {
        _PDH_STORE_QUERY_RESULT_INT64(systemAvailableBytes);
        _PDH_STORE_QUERY_RESULT_INT64(systemComittedBytes);
        _PDH_STORE_QUERY_RESULT_INT64(systemNonpagedPoolBytes);
        _PDH_STORE_QUERY_RESULT_INT64(systemPagedPoolBytes);
    }

    if (_dwFlagsQueryEntry & PDH_FLAG_QUERY_ENTRY_PROCESS_MEMORY)
    {
        _PDH_STORE_QUERY_RESULT_DOUBLE(processPageFaultRate);
        _PDH_STORE_QUERY_RESULT_INT64(processNonpagedPoolBytes);
        _PDH_STORE_QUERY_RESULT_INT64(processPagedPoolBytes);
        _PDH_STORE_QUERY_RESULT_INT64(processPrivateBytes);
        _PDH_STORE_QUERY_RESULT_INT64(processWorkingSet);
        _PDH_STORE_QUERY_RESULT_INT64(processPrivateWorkingSet);
    }


    return true;
}


