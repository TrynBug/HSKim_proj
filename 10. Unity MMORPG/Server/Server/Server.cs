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
            Logger.WriteLog(LogLevel.Debug, $"Run Server. FPS:{Config.FPS}");

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


            // DNS로 내 IP를 알아낸다.
            string host = Dns.GetHostName();                      // "DESKTOP-FLAO7VA"
            IPHostEntry ipHost = Dns.GetHostEntry(host);          // host에 대한 정보를 알아내어 IPHostEntry 객체로 구성한다.
            IPAddress ipAddr = ipHost.AddressList[0];             // 첫 번째 주소 정보를 가져옴
            IPEndPoint endPoint = new IPEndPoint(ipAddr, 7777);   // port 번호를 7777번으로 설정

            // listen
            Func<Session> fSessionFactory = () => { return SessionManager.Instance.Generate(); };
            _listenSocket.Init(endPoint, fSessionFactory);

            PrintServerStatus();

            while (true)
            {
                //RoomManager.Instance.Find(1).Update();

                Thread.Sleep(10000);
                //Logger.WriteLog(LogLevel.Debug, $"num thread:{ThreadPool.ThreadCount}, delta:{room.Time.DeltaTime}");
                //Logger.WriteLog(LogLevel.System, $"object players:{ObjectManager.Instance.PlayerCount}");
                //for (int i=0; i<RoomManager.Instance.RoomCount; i++)
                //{
                //    GameRoom room = RoomManager.Instance.Find(i);
                //    if (room == null)
                //        continue;
                //    Logger.WriteLog(LogLevel.System, $"{room}, players:{room.PlayerCount}, proj:{room.ProjectileCount}, delta:{room.Time.DeltaTime}");
                //}




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
                $"Memory(MB) [commit:{cm.MemoryCommittedMB:f2}, paged:{cm.MemoryPagedMB:f2}, nonpaged:{cm.MemoryNonpagedMB:f2}]");

            LoginRoom loginRoom = RoomManager.Instance.LoginRoom;
            Logger.WriteLog(LogLevel.System, $"[login  ]  session:{loginRoom.SessionCount}, login:{loginRoom.LoginCount1s}/s, FPS:{loginRoom.Time.AvgFPS1s:f1}, " +
                $"DB [query remain:{loginRoom.DBJobCount}, query:{loginRoom.DBExecutedQueryCount1s}/s]");

            RoomState roomState = RoomManager.Instance.GetRoomState();
            Logger.WriteLog(LogLevel.System, $"[field  ]  rooms:{RoomManager.Instance.RoomCount}(update:{RoomManager.Instance.UpdateRoomCount}), player: {roomState.Player.Sum}, " +
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
            string logThread = $"[thread ]  thread [active:({maxWorkerThreadCount - workerThreadCount},{maxCompletionPortThreadCount - completionPortThreadCount}), " +
                $"min:({minWorkerThreadCount},{minCompletionPortThreadCount})]";
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