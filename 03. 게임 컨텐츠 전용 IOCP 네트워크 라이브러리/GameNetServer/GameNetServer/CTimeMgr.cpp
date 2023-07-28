#include <windows.h>
#include <float.h>

#include "CTimeMgr.h"

using namespace game_netserver;

CTimeMgr::CTimeMgr()
	: _timer1s(false), _liDTLastUpdateTime{ 0, }, _liFrequency{ 0, }
	, _frameCount(0), _FPS(0), _DT(0.0), _fDT(0.f), _DTAcc(0.0)
	, _isRunOverOneSec(false), _isDTOverOneSec(false), _isDTOverOneMin(false), _avgDT1s(0.0), _avgFPS1s(0.f), _avgFPS1m(0.f)
	, _minDT{ 0, }, _maxDT{ 0, }, _minFPS{ 0, }, _maxFPS{ 0, }, __extraDT(0)
{
}

CTimeMgr::~CTimeMgr()
{}


/* �ʱ�ȭ */
void CTimeMgr::Init()
{
	_timer1s = false;

	QueryPerformanceCounter(&_liDTLastUpdateTime);
	QueryPerformanceFrequency(&_liFrequency);
	_frameCount = 0;
	_FPS = 0;
	_DT = 0.0;
	_fDT = (float)_DT;
	_DTAcc = 0.0;
	_isRunOverOneSec = false;
	_isDTOverOneSec = false;
	_isDTOverOneMin = false;

	_avgDT1s = 0.0;
	_avgFPS1s = 0.f;
	_avgFPS1m = 0.f;
	_minDT[0] = DBL_MAX;
	_minDT[1] = DBL_MAX;
	_maxDT[0] = DBL_MIN;
	_maxDT[1] = DBL_MIN;
	_minFPS[0] = INT_MAX;
	_minFPS[1] = INT_MAX;
	_maxFPS[0] = INT_MIN;
	_maxFPS[1] = INT_MIN;

	__extraDT = 0.0;
}





/* DT, FPS update */
void CTimeMgr::Update()
{
	LARGE_INTEGER llCurrentTime;
	QueryPerformanceCounter(&llCurrentTime);

	// �ʱ�ȭ���� 1�ʰ� �ȵǾ����� ����
	if (_isRunOverOneSec == false)
	{
		// 1�ʰ� �������� min, max�� �ʱ�ȭ �� ����
		if (llCurrentTime.QuadPart - _liDTLastUpdateTime.QuadPart > _liFrequency.QuadPart)
		{
			_isRunOverOneSec = true;
			_liDTLastUpdateTime = llCurrentTime;
			_liFirstDTTime = _liDTLastUpdateTime;
		}
		return;
	}


	// DT ������Ʈ
	_DT = (double)(llCurrentTime.QuadPart - _liDTLastUpdateTime.QuadPart) / (double)_liFrequency.QuadPart;
//#ifdef _DEBUG
//	// ����� ����� ���� DT�� 1/dfGAME_FPS �ʺ��� ũ�ٸ� 1/dfGAME_FPS�ʷ� ������Ų��.
//	_DT = min(1.0 / (double)(dfGAME_FPS - 1), _DT);
//#endif
	_fDT = (float)_DT;
	_DTAcc += _DT;
	_liDTLastUpdateTime = llCurrentTime;
	
	_qDT1s.push(_DT);
	// ���� 1�� ������ DT ��� ���
	if (_isDTOverOneSec)
	{
		_avgDT1s *= (double)(_qDT1s.size() - 1);
		__extraDT += _DT;
		// queue�� ���� ������ ���� 1���� �۾������� �� ����, �װ͵��� ��հ��� ����Ѵ�.
		while (__extraDT > 0.0 && _qDT1s.size() > 1)
		{
			__extraDT -= _qDT1s.front();
			_avgDT1s -= _qDT1s.front();
			_qDT1s.pop();
		}
		_avgDT1s = (_avgDT1s + _DT) / (double)_qDT1s.size();
	}
	else
	{
		_avgDT1s = _DTAcc / (double)_qDT1s.size();
		if (_DTAcc >= 1.0)
			_isDTOverOneSec = true;
	}

	// min, max ��ü
	if (_DT < _minDT[0])
	{
		_minDT[1] = _minDT[0];
		_minDT[0] = _DT;
	}
	else if (_DT < _minDT[1])
	{
		_minDT[1] = _DT;
	}

	if (_DT > _maxDT[0])
	{
		_maxDT[1] = _maxDT[0];
		_maxDT[0] = _DT;
	}
	else if (_DT > _maxDT[1])
	{
		_maxDT[1] = _DT;
	}

	// 1�� �ֱ� Ÿ�̸� ����
	if (_DTAcc >= 1.0)
		_timer1s = true;
	else
		_timer1s = false;


	// 1�ʸ��� FPS ������Ʈ
	_frameCount++;
	if (_DTAcc >= 1.0)
	{
		while (_DTAcc >= 1.0)
		{
			_FPS = _frameCount;

			_DTAcc -= 1.0;
			_frameCount = 0;

			_qFPS1m.push(_FPS);
			// ���� 1�� ������ FPS ��� ���
			if (_isDTOverOneMin)
			{
				_avgFPS1m = _avgFPS1m - (float)(_qFPS1m.front() - _FPS) / (float)(_qFPS1m.size() - 1);
				// = (_avgFPS1m * (float)(_qFPS1m.size() - 1) - _qFPS1m.front() + _FPS) / (float)(_qFPS1m.size() - 1);
				_qFPS1m.pop();
			}
			else
			{
				_avgFPS1m = (_avgFPS1m * (float)(_qFPS1m.size() - 1) + _FPS) / (float)_qFPS1m.size();
				if (llCurrentTime.QuadPart - _liFirstDTTime.QuadPart > _liFrequency.QuadPart * (LONGLONG)60)
					_isDTOverOneMin = true;
			}


			// min, max ��ü
			if (_FPS < _minFPS[0])
			{
				_minFPS[1] = _minFPS[0];
				_minFPS[0] = _FPS;
			}
			else if (_FPS < _minFPS[1])
			{
				_minFPS[1] = _FPS;
			}

			if (_FPS > _maxFPS[0])
			{
				_maxFPS[1] = _maxFPS[0];
				_maxFPS[0] = _FPS;
			}
			else if (_FPS > _maxFPS[1])
			{
				_maxFPS[1] = _FPS;
			}
		}
	}
}