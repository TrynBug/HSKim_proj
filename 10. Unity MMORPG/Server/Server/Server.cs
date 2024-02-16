using System;
using System.Collections.Generic;
using System.Data;
using System.Globalization;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using MySql.Data.MySqlClient;
using Server.Data;
using Server.Game;
using ServerCore;

namespace Server
{
    internal class Program
    {
        static Listener _listenSocket = new Listener();
        static System.Timers.Timer _serverStatusTimer = new System.Timers.Timer();
        static void Main(string[] args)
        {
            // clock resolution을 1ms로 설정
            WinApi.TimeBeginPeriod(1);

            // counter init
            CounterManager.Instance.Init();

            // logger 설정
            Logger.Level = LogLevel.System;
            //Logger.Level = LogLevel.Debug;

            // 데이터 파일 로드
            ConfigManager.LoadConfig();
            DataManager.LoadData();


            // 게임룸 생성
            RoomManager.Instance.Init();
            foreach (MapData map in DataManager.MapDict.Values)
            {
                RoomManager.Instance.Add(map.id);
            }
            RoomManager.Instance.RunAllRooms();

            IPAddress ipAddr = IPAddress.Parse("0.0.0.0");
            IPEndPoint endPoint = new IPEndPoint(ipAddr, ConfigManager.Config.port);

            Logger.WriteLog(LogLevel.System, $"Run Server. IP:{endPoint.Address}, port:{ConfigManager.Config.port}");

            // listen
            Func<Session> fSessionFactory = () => { return SessionManager.Instance.Generate(); };
            _listenSocket.Init(endPoint, fSessionFactory);

            PrintServerStatus();

            while (true)
            {
                Thread.Sleep(Timeout.Infinite);
            }
        }





        static void PrintServerStatus()
        {
            _serverStatusTimer.Elapsed += ((s, e) => _printServerStatus());
            _serverStatusTimer.Interval = 1000;
            _serverStatusTimer.AutoReset = true;
            _serverStatusTimer.Enabled = true;
        }

        static void _printServerStatus()
        {
            CounterManager cm = CounterManager.Instance;
            Logger.WriteLog(LogLevel.System, $"[network]  session:{SessionManager.Instance.SessionCount}, accept:{cm.Accept} ({cm.Accept1s}/s), " +
                $"disconn:{cm.Disconnect} ({cm.Disconnect1s}/s), recv:{cm.Recv1s}/s, send:{cm.Send1s}/s");
            Logger.WriteLog(LogLevel.System, $"[system ]  CPU [total:{cm.CPUTotal:f1}%, user:{cm.CPUUser:f1}%, kernel:{cm.CPUKernel:f1}%], " +
                $"Memory(MB) [System C:{cm.MemoryCommittedMB:f2}, P:{cm.MemoryPagedMB:f2}, NP:{cm.MemoryNonpagedMB:f2}] " +
                $"[Process C:{cm.ProcessMemoryPrivateMB:f2}, P:{cm.ProcessMemoryPagedMB:f2}, NP:{cm.ProcessMemoryNonpagedMB:f2}]");

            LoginRoom loginRoom = RoomManager.Instance.LoginRoom;
            Logger.WriteLog(LogLevel.System, $"[login  ]  session:{loginRoom.SessionCount}, login:{loginRoom.LoginCount1s}/s, FPS:{loginRoom.Time.AvgFPS1s:f1}, " +
                $"DB [query remain:{loginRoom.DBJobCount}, query:{loginRoom.DBExecutedQueryCount1s}/s]");

            RoomState roomState = RoomManager.Instance.GetRoomState();
            Logger.WriteLog(LogLevel.System, $"[field  ]  rooms:{RoomManager.Instance.RoomCount}, " +
                $"player: {roomState.Player.Sum} (max:{roomState.Player.Max}(room:{roomState.Player.MaxRoom})), " +
                $"FPS [avg:{roomState.FPS.Avg:f1}, min:{roomState.FPS.Min:f1}(room {roomState.FPS.MinRoom}), max:{roomState.FPS.Max:f1}(room {roomState.FPS.MaxRoom})], " +
                $"DB [total:{roomState.DBJob1s.Sum}/s, remain max:{roomState.DBJobQueue.Max}(room {roomState.DBJobQueue.MaxRoom}), max:{roomState.DBJob1s.Max}/s(room {roomState.DBJob1s.MaxRoom})]");

            int workerThreadCount;
            int completionPortThreadCount;
            int maxWorkerThreadCount;
            int maxCompletionPortThreadCount;
            int minWorkerThreadCount;
            int minCompletionPortThreadCount;
            ThreadPool.GetAvailableThreads(out workerThreadCount, out completionPortThreadCount);
            ThreadPool.GetMaxThreads(out maxWorkerThreadCount, out maxCompletionPortThreadCount);
            ThreadPool.GetMinThreads(out minWorkerThreadCount, out minCompletionPortThreadCount);
            string logThread = $"[thread ]  active:({maxWorkerThreadCount - workerThreadCount},{maxCompletionPortThreadCount - completionPortThreadCount}), " +
                $"recv:{cm.CurrentRecvThread}(max:{cm.MaxCurrentRecvThread}), send:{cm.CurrentSendThread}(max:{cm.MaxCurrentSendThread}), room:{RoomManager.Instance.UpdateRoomCount}, ";
#if NET6_0
            GCMemoryInfo gcInfo = GC.GetGCMemoryInfo();
            logThread += $", GC({(System.Runtime.GCSettings.IsServerGC ? "server" :"works")}) [percentage:{gcInfo.PauseTimePercentage}, duration:(";
            foreach(TimeSpan time in gcInfo.PauseDurations)
                logThread += $"{time.Ticks / TimeSpan.TicksPerMillisecond}, ";
            logThread += ")";
#endif
            Logger.WriteLog(LogLevel.System, logThread);
            Console.WriteLine("\n");
        }
        
    }
}