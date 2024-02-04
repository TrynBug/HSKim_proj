using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using Server.Data;
using Server.Game;
using ServerCore;

namespace Server
{
    internal class Program
    {
        static Listener _listenSocket = new Listener();
        static List<System.Timers.Timer> _timers = new List<System.Timers.Timer>();

        static void Main(string[] args)
        {
            Logger.WriteLog(LogLevel.Debug, $"Run Server. FPS:{Config.FPS}");

            // clock resolution을 1ms로 설정
            WinApi.TimeBeginPeriod(1);

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


            while (true)
            {
                //RoomManager.Instance.Find(1).Update();

                Thread.Sleep(10000);
                //Logger.WriteLog(LogLevel.Debug, $"num thread:{ThreadPool.ThreadCount}, delta:{room.Time.DeltaTime}");
                Logger.WriteLog(LogLevel.System, $"object players:{ObjectManager.Instance.PlayerCount}");
                for (int i=0; i<RoomManager.Instance.RoomCount; i++)
                {
                    GameRoom room = RoomManager.Instance.Find(i);
                    if (room == null)
                        continue;
                    Logger.WriteLog(LogLevel.System, $"{room}, players:{room.PlayerCount}, proj:{room.ProjectileCount}, delta:{room.Time.DeltaTime}");
                }




            }
        }
    }
}