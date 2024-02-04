using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using Server.Data;
using ServerCore;

namespace Server.Game
{
    public class LoginRoom : JobSerializer
    {
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

        // object
        Dictionary<int, ClientSession> _sessions = new Dictionary<int, ClientSession>();

        public int SessionCount { get { return _sessions.Count; } }


        /* push job */
        public void Init() { Push(_init); }
        public void EnterRoom(ClientSession session) { Push(_enterRoom, session); }
        public void LeaveRoom(int sessionId) { Push(_leaveRoom, sessionId); }


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



        public void _init()
        {
            _timer.Elapsed += ((s, e) => Run());
            _calcFrame.Init();
        }




        // frame update
        List<ClientSession> _disconnectedSessions = new List<ClientSession>();
        List<ClientSession> _leavingSessions = new List<ClientSession>();
        public void _update()
        {
            Flush();  // job queue 내의 모든 job 실행

            // 플레이어 업데이트
            _leavingSessions.Clear();
            _disconnectedSessions.Clear();
            foreach (ClientSession session in _sessions.Values)
            {
                // 연결끊긴 세션
                if (session.IsDisconnected == true)
                {
                    _disconnectedSessions.Add(session);
                    continue;
                }

                // 로그인 후 룸이동
                if (session.Login == true)
                {
                    _leavingSessions.Add(session);
                }
            }

            // 연결끊긴 세션 처리
            foreach (ClientSession session in _disconnectedSessions)
            {
                _leaveRoom(session.SessionId);
                SessionManager.Instance.Remove(session);
            }

            // 로그인 완료된 세션을 게임룸으로 이동
            foreach (ClientSession session in _leavingSessions)
            {
                // 현재 room에서 나감
                _leaveRoom(session.SessionId);

                // 플레이어 생성
                Player myPlayer = new Player();
                GameRoom room = RoomManager.Instance.GetRandomRoom();
                session.MyPlayer = myPlayer;
                myPlayer.Init(session, room);
                myPlayer.Info.Name = session.Name;

                ObjectManager.Instance.AddPlayer(myPlayer);

                // 로그인 성공 패킷 전송
                S_LoginResponse resPacket = new S_LoginResponse();
                resPacket.Result = 1;
                resPacket.Name = session.Name;
                session.Send(resPacket);

                // 게임룸에 플레이어 추가
                room.EnterGame(myPlayer);
            }
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


        // 세션이 로그인룸에 들어옴
        public void _enterRoom(ClientSession session)
        {
            if (session == null)
            {
                Logger.WriteLog(LogLevel.Error, $"LoginRoom.EnterRoom. session is null");
                return;
            }

            _sessions.Add(session.SessionId, session);

            Logger.WriteLog(LogLevel.Debug, $"LoginRoom.EnterRoom. {session.SessionId}");
        }




        // 세션이 로그인룸에서 나감
        public void _leaveRoom(int sessionId)
        {
            _sessions.Remove(sessionId);

            Logger.WriteLog(LogLevel.Debug, $"LoginRoom.LeaveRoom. session:{sessionId}");
        }

        // ToString
        public override string ToString()
        {
            return $"room:login";
        }
    }
}
