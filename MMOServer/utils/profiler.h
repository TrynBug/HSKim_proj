#pragma once

#include <vector>
#include <unordered_map>

/*
Ŭ���� ����� �������Ϸ�
�ð��� �����Ϸ��� �ϴ� �ڵ� �տ� PROFILE_BEGIN�� ȣ���Ѵ�.
PROFILE_BEGIN(TagName) �� ȣ���ϸ� Profiler ��ü�� �����Ǿ� ��� ���� �� �ڵ����� �Ҹ��ڰ� ȣ��Ǿ� �ð��� �����ȴ�.
���� �����ϰ���� �ڵ尡 ���� ����� ��������� PROFILE_BEGIN(TagName)�� ����ص� �ȴ�.
���� ��� �߰��� ������ ���߰� �ʹٸ� PROFILE_END(TagName) �� �߰��� ����� �� �ִ�.
���� �ϳ��� ��� ������ ���� �κ��� �ڵ带 ���� �����ϰ� �ʹٸ� �� �κ��� ������� ���� �� ��� ���ο��� PROFILE_BEGIN�� ȣ���ؾ� �Ѵ�.

��� ������ ��ũ�η� �ϰ� �ʹٸ� PROFILE_BLOCK_BEGIN, PROFILE_BLOCK_END �� �Բ� ����Ѵ�.
PROFILE_BLOCK_BEGIN �� PROFILE_BEGIN �տ� { �� ���ΰ�, 
PROFILE_BLOCK_END�� } �̴�.
�׷��� �� ���� �׻� �Բ� ���Ǿ�� �Ѵ�.

���� ���ǿ� ���� Tag ���� �ٸ��� �Ͽ� �ð��� �����ϰ� �ʹٸ�,
���� PROFILE_READY_TO_DELAYED_BEGIN �� ȣ���Ͽ� Tag�� �������� ���� Profiler ��ü�� �����Ѵ�.
�׷����� if�� ��� �Ʒ��� ���� PROFILE_DELAYED_BEGIN(TagName) �� ȣ���Ͽ� �ð� ������ �����Ѵ�.
if (val == 0)
	PROFILE_DELAYED_BEGIN("when val is 0")
else
	PROFILE_DELAYED_BEGIN("when val isn't 0")
(���� if�� �Ʒ����� PROFILE_BEGIN(TagName) �� ����ϸ� if���� ����ڸ��� Profiler ��ü�� �Ҹ��ڰ� ȣ��Ǿ���� ���̴�.)


!! ����: ���� �ٸ� ��ҿ��� ���� TagName�� ����Ѵٸ� ������ �߻��� �� ����
�ٵ� ���ڿ� ���ͷ��� �ּҰ� ���α׷��� ����ɶ����� �ȹٲ�°� �³�...?
*/


//#define ENABLE_PROFILER

#ifdef ENABLE_PROFILER
#define PROFILE_BEGIN(TagName)  CProfiler __profiler(L##TagName);
#define PROFILE_END(TagName)    __profiler.End(L##TagName);

#define PROFILE_BLOCK_BEGIN(TagName) { CProfiler __profiler(L##TagName);
#define PROFILE_BLOCK_END            }

#define PROFILE_READY_TO_DELAYED_BEGIN CProfiler __profiler;
#define PROFILE_DELAYED_BEGIN(TagName) __profiler.Begin(L##TagName);


#else
#define PROFILE_BEGIN(TagName)            ;
#define PROFILE_END(TagName)			  ;
#define PROFILE_BLOCK_BEGIN(TagName)	  ;
#define PROFILE_BLOCK_END				  ;
#define PROFILE_READY_TO_DELAYED_BEGIN	  ;
#define PROFILE_DELAYED_BEGIN(TagName)	  ;
#endif




//// ������ Ÿ�ӿ� tag���� ���
//int constexpr _profilerGetTagLength(const wchar_t* tag)
//{
//	return *tag == L'\0' ? 0 : _profilerGetTagLength(tag + 1) + 1;
//}
//
//// �̰� �Ⱦ��� constexpr �Լ� ���ϰ��� �ٸ��Լ� �Ķ���ͷ� ����Ҷ� ������Ÿ�ӿ� ���ȵ�
//template <int N>
//constexpr int _wrapConstexprInt() { return N; }
// 
//#define PROFILE_BEGIN(TagName)  Profiler __profiler(L##TagName, _wrapConstexprInt<_profilerGetTagLength(L##TagName)>());





namespace profiler
{
	void ResetProfilingData();  // �������ϸ� ������ �ʱ�ȭ
	void OutputProfilingData(); // �������ϸ� ������ ���Ϸ� ���
}




// �������ϸ� ��� ���忡 ����� ����ü
struct ProfileSample
{
	long          lFlag;        // option
	const wchar_t* tagName;     // �������� ���� �̸�
	LARGE_INTEGER liStartTime;  // �������ϸ� ���� �ð�
	__int64       iTotalTime;   // ��ü �ҿ�ð� seconds (��½� ȣ��Ƚ���� ������ ����� ����)
	__int64       iMin[2];      // �ּ� �ҿ�ð� seconds ([0] �ּҰ�, [1] �ּҰ� �ٷ���)
	__int64       iMax[2];      // �ִ� �ҿ�ð� seconds ([0] �ִ밪, [1] �ִ밪 �ٷξƷ�)
	__int64       iCall;        // ���� ȣ�� Ƚ��

	ProfileSample(const wchar_t* tagName)
		:tagName(tagName)
		, lFlag(0), liStartTime{ 0, }, iTotalTime(0), iMax{ 0, }, iCall(0)
	{
		iMin[0] = INT64_MAX;
		iMin[1] = INT64_MAX;
	}
};


// �� �������ϸ� ���۰� ���Ḧ �����ϴ� Ŭ����
class CProfiler
{
private:
	const wchar_t*   _tag;
	bool             _isEnd;

public:
	CProfiler();
	CProfiler(const wchar_t* tag);
	~CProfiler();

public:
	void Begin();
	void Begin(const wchar_t* tag);
	void End(const wchar_t* tag);
};


// �������ϸ� ��ü ������ ���� Ŭ����
// �� Ŭ������ �ν��Ͻ��� profiler.cpp ���� ������ �������� 1�� ������
class CThreadProfileManager;
class CProfileManager
{
	friend class CProfiler;

public:
	bool _bProfileStarted;   // �������ϸ��� �ѹ��̶� ��������� ����

public:
	CProfileManager();
	~CProfileManager();

private:
	DWORD _tlsIndex;
	std::vector<CThreadProfileManager*> _vecThreadProfileManager;
	SRWLOCK _srwlThreadProfileManager;

public:
	void ProfileReset();
	void ProfileDataOutText(const wchar_t* strFilePath);
};



// 1�� ������ ���� �������ϸ� ������ ���� Ŭ����
class CThreadProfileManager
{
	friend class CProfiler;
	friend class CProfileManager;

	std::unordered_map<const wchar_t*, ProfileSample*> _mapProfileSample;  // �������� ����
	std::vector<__int64*> _vecProfileHierarchy;   // �������Ϸ� ����. ���� vector�� �� �տ� �ִ� ������ ���� ���� ������ �����̴�.
	__int64 _iTotalProfileCall;   // �������ϸ� ��ü ���� Ƚ��
	__int64 _iTotalProfileTime;   // �������ϸ��� �ϴµ� �ҿ�� ��ü �ð�
	
	bool _bNeedReset;  // reset �ؾ��� ����

public:
	CThreadProfileManager();
	~CThreadProfileManager();

private:
	void Reset();
};