using System.Net;
using System.Net.Sockets;
using System.Text;
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

        // tick 뒤에 room.Update() 를 실행하는 타이머를 등록한다.
        // AutoReset = true 이기 때문에 tick 마다 계속 실행된다.
        static void TickRoom(GameRoom room, int tick = 100)
        {
            var timer = new System.Timers.Timer();
            timer.Interval = tick;
            timer.Elapsed += ((s, e) => room._update());
            timer.AutoReset = true;
            timer.Enabled = true;     // timer를 실행한다.

            _timers.Add(timer);  // 추후 관리를 위해 timer를 저장해둠
        }

        static void Main(string[] args)
        {
            Logger.Level = LogLevel.Debug;

            // 데이터 파일 로드
            ConfigManager.LoadConfig();
            DataManager.LoadData();


            // 게임룸 생성
            for(int i=1; i<=10; i++)
                RoomManager.Instance.Add(i);
            //TickRoom(room, 50);  // 50 tick 마다 update
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

                Thread.Sleep(1000);
                Logger.WriteLog(LogLevel.Debug, $"num thread:{ThreadPool.ThreadCount}");
            }
        }
    }
}