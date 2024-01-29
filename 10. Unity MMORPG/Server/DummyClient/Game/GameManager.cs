using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using DummyClient.Data;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using ServerCore;


namespace DummyClient.Game
{
    internal class GameManager : JobSerializer
    {
        public static GameManager Instance { get; } = new GameManager();

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

        // control
        public bool StopConnection { get; set; } = false;
        public bool DisconnectAll { get; set; } = false;


        // object
        Dictionary<int, PlayerController> _players = new Dictionary<int, PlayerController>();
        public int PlayerCount { get { return _players.Count; } }

        // Random
        Random _rand = new Random();


        /* push job */
        public void EnterGame(CreatureController gameObject, S_EnterGame packet) { Push(_enterGame, gameObject, packet); }


        // id에 해당하는 오브젝트의 타입 얻기
        public static GameObjectType GetObjectTypeById(int id)
        {
            int type = (id >> 24) & 0x7F;
            return (GameObjectType)type;
        }

        public void Init()
        {
            _timer.Elapsed += ((s, e) => Run());
            _calcFrame.Init();
        }

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
                Logger.WriteLog(LogLevel.Error, $"GameManager Update Error. {ex}");
                throw ex;
            }
            _sleepMilliseconds = _calcSleepTime();
            //Logger.WriteLog(LogLevel.System, $"DT:{Time.DeltaTime}, FPS:{Time.AvgFPS1m}, sleep:{_sleepMilliseconds}, thread:{Thread.CurrentThread.ManagedThreadId}");

            _timer.Interval = Math.Max(1, _sleepMilliseconds);
            _timer.AutoReset = false;  // event가 주기적으로 호출되도록 하는것이 아니라 다음 호출시간을 매번 정해줄 것이기 때문에 AutoReset을 false로 한다.
            _timer.Enabled = true;     // timer를 재활성화 한다.

        }

        // frame update
        List<PlayerController> _disconnectedPlayers = new List<PlayerController>();
        public void _update()
        {
            // job queue 내의 모든 job 실행
            Flush();

            // 플레이어 접속
            if (StopConnection == false)
            {
                int connectionCount = Math.Min(Config.MaxNumberOfClient - SessionManager.Instance.SessionCount, Config.MaxConnectionCountPerFrame);
                if (connectionCount > 0)
                {
                    for (int i = 0; i < connectionCount; i++)
                    {
                        ServerSession session = SessionManager.Instance.Generate();
                        session.Connect();
                    }
                }
            }

            // 플레이어 연결끊기
            foreach (PlayerController player in _players.Values)
            {
                if (DisconnectAll == true)
                {
                    player.Session.Disconnect();
                }
                else
                {
                    float rand = _rand.NextSingle();
                    if (rand < Config.DisconnectProb)
                    {
                        player.Session.Disconnect();
                    }
                }
            }
            DisconnectAll = false;

            // 플레이어 업데이트
            _disconnectedPlayers.Clear();
            foreach (PlayerController player in _players.Values)
            {
                // 연결끊긴 플레이어는 따로처리함
                if (player.Session.IsDisconnected == true)
                {
                    _disconnectedPlayers.Add(player);
                    continue;
                }

                // 업데이트
                player.Update();

            }

            // 연결끊긴 플레이어 삭제
            foreach (PlayerController player in _disconnectedPlayers)
            {
                _players.Remove(player.Id);
                SessionManager.Instance.Remove(player.Session);
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



        public void _enterGame(CreatureController gameObject, S_EnterGame packet)
        {
            if (gameObject == null)
            {
                Logger.WriteLog(LogLevel.Error, $"GameManager.EnterGame. GameObject is null");
                return;
            }

            GameObjectType type = GetObjectTypeById(gameObject.Id);
            if (type == GameObjectType.Player)
            {
                // 플레이어 추가
                PlayerController newPlayer = gameObject as PlayerController;
                if(_players.GetValueOrDefault(newPlayer.Id, null) == null)
                    _players.Add(newPlayer.Id, newPlayer);

                // 로딩완료 패킷 전송
                C_LoadFinished loadPacket = new C_LoadFinished();
                loadPacket.BLoaded = true;
                newPlayer.Session.Send(loadPacket);

                // 오토패킷 전송
                if (newPlayer.AutoMode == AutoMode.ModeNone)
                {
                    newPlayer.AutoMode = AutoMode.ModeAuto;
                    C_SetAuto autoPacket = new C_SetAuto();
                    autoPacket.Mode = AutoMode.ModeAuto;
                    newPlayer.Session.Send(autoPacket);
                }

                Logger.WriteLog(LogLevel.Debug, $"GameManager.EnterGame. {newPlayer}");
            }
        }
    }

}
