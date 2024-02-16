using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Server.Data;
using ServerCore;

namespace Server.Game
{
    public class RoomBase : JobSerializer
    {
        // info
        public int RoomId { get; set; }

        // DB
        protected DBWriter DBWriter { get; private set; } = new DBWriter();
        public int DBJobCount { get { return DBWriter.CurrentJobCount; } }
        public long DBExecutedQueryCount { get { return DBWriter.ExecutedJobCount; } }
        public long DBExecutedQueryCount1s { get; private set; } = 0;

        // FPS, Delta Time
        public Time Time { get; private set; } = new Time();
        class CalcFrame           // 프레임 시간 계산을 위한 값들
        {
            public long logicStartTime = 0;       // 로직 시작 tick
            public long logicEndTime = 0;         // 로직 종료 tick
            public long catchUpTime = 0;          // FPS를 유지하기위해 따라잡아야할 tick. 이 값만큼 덜 sleep 해야한다.
            public void Init() { logicStartTime = DateTime.Now.Ticks; }
        };
        CalcFrame _calcFrame = new CalcFrame();
        System.Timers.Timer _timer = new System.Timers.Timer();
        int _sleepMilliseconds = 0;

        // update
        int _update1sTime = 0;
        long _prevDBExecutedQueryCount = 0;




        // FPS에 맞추어 게임룸을 주기적으로 업데이트한다.
        public void Run()
        {
            // 다음 로직의 시작시간은 [현재 로직 종료시간 + sleep한 시간] 이다.
            _calcFrame.logicStartTime = _calcFrame.logicEndTime + (_sleepMilliseconds * (TimeSpan.TicksPerSecond / 1000));
            Time.Update();

            try
            {
                _update();
            }
            catch (Exception ex)
            {
                Logger.WriteLog(LogLevel.Error, $"GameRoom Update Error. room:{this}, error:{ex}");
            }
            _sleepMilliseconds = _calcSleepTime();
            //Logger.WriteLog(LogLevel.Debug, $"room:{RoomId}, DT:{Time.DeltaTime}, FPS:{Time.AvgFPS1m}, sleep:{_sleepMilliseconds}, thread:{Thread.CurrentThread.ManagedThreadId}");

            _timer.Interval = Math.Max(1, _sleepMilliseconds);
            _timer.AutoReset = false;  // event가 주기적으로 호출되도록 하는것이 아니라 다음 호출시간을 매번 정해줄 것이기 때문에 AutoReset을 false로 한다.
            _timer.Enabled = true;     // timer를 재활성화 한다.

        }

        // init
        public virtual void _init()
        {
            // DB 연결
            DBWriter.Connect(this);

            // 프레임 업데이트 시작
            _timer.Elapsed += ((s, e) => Run());
            _calcFrame.Init();
        }


        // 프레임 업데이트
        public virtual void _update()
        {
            int tick = Environment.TickCount;
            if(tick - _update1sTime >= 1000)
            {
                _update1sTime += 1000;
                _update1s();
            }
        }

        // 1초 간격 업데이트
        public virtual void _update1s()
        {
            long dbExecutedQueryCount = DBExecutedQueryCount;
            DBExecutedQueryCount1s = dbExecutedQueryCount - _prevDBExecutedQueryCount;
            _prevDBExecutedQueryCount = dbExecutedQueryCount;
        }



        // FPS를 유지하기 위해 sleep 해야하는 시간을 계산한다.
        private int _calcSleepTime()
        {
            long oneFrameTime = TimeSpan.TicksPerSecond / Config.FPS;
            _calcFrame.logicEndTime = DateTime.Now.Ticks;
            long spentTime = Math.Max(0, _calcFrame.logicEndTime - _calcFrame.logicStartTime);
            long sleepTime = Math.Max(0, oneFrameTime - spentTime);
            int sleepMilliseconds = 0;
            // sleep 해야할 시간이 있는 경우
            if (sleepTime > 0)
            {
                // sleep 시간을 ms 단위로 변환하여 sleep 한다.
                sleepMilliseconds = (int)Math.Round((double)(sleepTime * 1000) / (double)TimeSpan.TicksPerSecond);
            }
            return sleepMilliseconds;
        }
    }
}
