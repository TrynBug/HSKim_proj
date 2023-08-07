#pragma once

#include <queue>

namespace netlib_game
{

	/* DT ��� �Ŵ��� */
	class CTimeMgr
	{
	public:
		CTimeMgr();
		~CTimeMgr();

	public:
		bool Is1sTimerSet() { return _timer1s; }

		double GetDT() const { return _DT; }
		float GetfDT() const { return _fDT; }
		double GetAvgDT1s() const { return _avgDT1s; }
		double GetMinDT1st() const { return _minDT[0]; }
		double GetMinDT2nd() const { return _minDT[1]; }
		double GetMaxDT1st() const { return _maxDT[0]; }
		double GetMaxDT2nd() const { return _maxDT[1]; }
		int GetFPS() const { return _FPS; }
		float GetAvgFPS1s() const { return _avgFPS1s; }
		float GetAvgFPS1m() const { return _avgFPS1m; }
		int GetMinFPS1st() const { return _minFPS[0]; }
		int GetMinFPS2nd() const { return _minFPS[1]; }
		int GetMaxFPS1st() const { return _maxFPS[0]; }
		int GetMaxFPS2nd() const { return _maxFPS[1]; }

	public:
		void Init();            // DT �ʱ�ȭ
		void Update();          // DT ������Ʈ

	private:
		/* timer */
		bool _timer1s;                       // 1�� �ֱ� Ÿ�̸�(_DTAcc >= 1.0 �� ������ true�� ��)

		/* DT */
		int _frameCount;                     // FPS ��꿡 ���Ǵ� ��
		LARGE_INTEGER _liFirstDTTime;        // ���� DT ���� �ð�
		LARGE_INTEGER _liDTLastUpdateTime;   // ���������� DT�� ������Ʈ�� �ð�
		LARGE_INTEGER _liFrequency;          // frequency
		double _DT;                          // Delta Time
		float _fDT;                          // Delta Time(float)
		double _DTAcc;                       // ���� DT
		int _FPS;                            // FPS
		bool _isRunOverOneSec;               // �ʱ�ȭ���� 1�ʰ� �������� ����
		bool _isDTOverOneSec;                // ���� DT ���� �� 1�ʰ� �������� ����
		bool _isDTOverOneMin;                // ���� DT ���� �� 1���� �������� ����

		double _minDT[2];                    // �ּ�DT ([0] �ּҰ�, [1] �ּҰ� �ٷ���)
		double _maxDT[2];                    // �ִ�DT ([0] �ִ밪, [1] �ִ밪 �ٷξƷ�)
		double _avgDT1s;                     // ���� 1�� ��� DT
		int _minFPS[2];                      // �ּ�FPS ([0] �ּҰ�, [1] �ּҰ� �ٷ���)
		int _maxFPS[2];                      // �ִ�FPS ([0] �ִ밪, [1] �ִ밪 �ٷξƷ�)
		float _avgFPS1s;                     // ���� 1�� ��� FPS(_avgDT1s ���� FPS�� ȯ��)
		float _avgFPS1m;                     // ���� 1�� ��� FPS

		std::queue<double> _qDT1s;           // 1�� ������ DT ���
		std::queue<int>    _qFPS1m;          // 1�� ������ FPS ���

		double __extraDT;                    // ���������� _avgDT1s ��꿡 ����

	};



}