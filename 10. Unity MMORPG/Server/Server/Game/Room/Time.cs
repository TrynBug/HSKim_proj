using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    // FPS, Delta Time 계산을 위한 클래스
    public class Time
    {
        /* timer */
        public bool Timer1s { get; private set; } = false;  // 1초 주기 타이머(_DTAcc >= 1.0 일 때마다 true가 됨)

        /* DT */
        public float DeltaTime { get; private set; } = 0f;  // Delta Time
        public double AvgDT1s { get; private set; } = 0f;    // 지난 1초 평균 DT
        public double MinDT1st { get; private set; } = double.MaxValue;   // 최소DT (최소값)
        public double MinDT2nd { get; private set; } = double.MaxValue;   // 최소DT (최소값 바로위)
        public double MaxDT1st { get; private set; } = double.MinValue;   // 최대DT (최대값)
        public double MaxDT2nd { get; private set; } = double.MinValue;   // 최대DT (최대값 바로아래)

        /* FPS */
        public int FPS { get; private set; } = 0;
        public float AvgFPS1s { get; private set; } = 0f;   // 지난 1초 평균 FPS(AvgDT1s 값을 FPS로 환산)
        public float AvgFPS1m { get; private set; } = 0f;   // 지난 1분 평균 FPS
        public int MinFPS1st { get; private set; } = int.MaxValue;  // 최소FPS (최소값)
        public int MinFPS2nd { get; private set; } = int.MaxValue;  // 최소FPS (최소값 바로위)
        public int MaxFPS1st { get; private set; } = int.MinValue;  // 최대FPS (최대값)
        public int MaxFPS2nd { get; private set; } = int.MinValue;  // 최대FPS (최대값 바로아래)


        /* private */
        int _frameCount = 0;                     // FPS 계산에 사용되는 값
        long _firstDTTime = 0;                   // 최초 DT 측정 시간
        long _DTLastUpdateTime = DateTime.Now.Ticks;    // 마지막으로 DT를 업데이트한 시간
        double _DT = 0;                          // Delta Time
        double _DTAcc = 0;                       // 누적 DT
        int _FPS = 0;                            // FPS
        bool _isRunOverOneSec = false;           // 초기화된지 1초가 지났는지 여부
        bool _isDTOverOneSec = false;            // 최초 DT 측정 후 1초가 지났는지 여부
        bool _isDTOverOneMin = false;            // 최초 DT 측정 후 1분이 지났는지 여부

        Queue<double> _qDT1s = new Queue<double>();    // 1초 동안의 DT 기록
        Queue<int> _qFPS1m = new Queue<int>();         // 1분 동안의 FPS 기록

        double __extraDT = 0;              // 내부적으로 _avgDT1s 계산에 사용됨


        /* DT, FPS update */
        public void Update()
        {
            long currentTime = DateTime.Now.Ticks;

            // 초기화한지 1초가 안되었으면 종료
            if (_isRunOverOneSec == false)
            {
                // 1초가 지났으면 min, max값 초기화 후 시작
                if (currentTime - _DTLastUpdateTime > TimeSpan.TicksPerSecond)
                {
                    _isRunOverOneSec = true;
                    _DTLastUpdateTime = currentTime;
                    _firstDTTime = _DTLastUpdateTime;
                }
                return;
            }


            // DT 업데이트
            _DT = (double)(currentTime - _DTLastUpdateTime) / (double)TimeSpan.TicksPerSecond;
            //#ifdef _DEBUG
            //	// 디버그 모드일 때는 DT가 1/dfGAME_FPS 초보다 크다면 1/dfGAME_FPS초로 고정시킨다.
            //	_DT = min(1.0 / (double)(dfGAME_FPS - 1), _DT);
            //#endif
            DeltaTime = (float)_DT;
            _DTAcc += _DT;
            _DTLastUpdateTime = currentTime;

            _qDT1s.Enqueue(_DT);
            // 지난 1초 동안의 DT 평균 계산
            if (_isDTOverOneSec)
            {
                AvgDT1s *= (double)(_qDT1s.Count - 1);
                __extraDT += _DT;
                // queue에 남은 값들의 합이 1보다 작아지도록 한 다음, 그것들의 평균값을 계산한다.
                while (__extraDT > 0.0 && _qDT1s.Count() > 1)
                {
                    __extraDT -= _qDT1s.First();
                    AvgDT1s -= _qDT1s.First();
                    _qDT1s.Dequeue();
                }
                AvgDT1s = (AvgDT1s + _DT) / (double)_qDT1s.Count;
            }
            else
            {
                AvgDT1s = _DTAcc / (double)_qDT1s.Count;
                if (_DTAcc >= 1.0)
                    _isDTOverOneSec = true;
            }

            // min, max 교체
            if (_DT < MinDT1st)
            {
                MinDT2nd = MinDT1st;
                MinDT1st = _DT;
            }
            else if (_DT < MinDT2nd)
            {
                MinDT2nd = _DT;
            }

            if (_DT > MaxDT1st)
            {
                MaxDT2nd = MaxDT1st;
                MaxDT1st = _DT;
            }
            else if (_DT > MaxDT2nd)
            {
                MaxDT2nd = _DT;
            }

            // 1초 주기 타이머 설정
            if (_DTAcc >= 1.0)
                Timer1s = true;
            else
                Timer1s = false;


            // 1초마다 FPS 업데이트
            _frameCount++;
            if (_DTAcc >= 1.0)
            {
                while (_DTAcc >= 1.0)
                {
                    _FPS = _frameCount;

                    _DTAcc -= 1.0;
                    _frameCount = 0;

                    _qFPS1m.Enqueue(_FPS);
                    // 지난 1분 동안의 FPS 평균 계산
                    if (_isDTOverOneMin)
                    {
                        AvgFPS1m = AvgFPS1m - (float)(_qFPS1m.First() - _FPS) / (float)(_qFPS1m.Count - 1);
                        // = (AvgFPS1m * (float)(_qFPS1m.Count - 1) - _qFPS1m.First() + _FPS) / (float)(_qFPS1m.Count - 1);
                        _qFPS1m.Dequeue();
                    }
                    else
                    {
                        AvgFPS1m = (AvgFPS1m * (float)(_qFPS1m.Count - 1) + _FPS) / (float)_qFPS1m.Count;
                        if (currentTime - _firstDTTime > TimeSpan.TicksPerMinute)
                            _isDTOverOneMin = true;
                    }


                    // min, max 교체
                    if (_FPS < MinFPS1st)
                    {
                        MinFPS2nd = MinFPS1st;
                        MinFPS1st = _FPS;
                    }
                    else if (_FPS < MinFPS2nd)
                    {
                        MinFPS2nd = _FPS;
                    }

                    if (_FPS > MaxFPS1st)
                    {
                        MaxFPS2nd = MaxFPS1st;
                        MaxFPS1st = _FPS;
                    }
                    else if (_FPS > MaxFPS2nd)
                    {
                        MaxFPS2nd = _FPS;
                    }
                }
            }


        }
    }
}
