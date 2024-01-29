using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using ServerCore;
using System.Net;
using DummyClient.Data;
using DummyClient.Game;

namespace DummyClient
{
    internal class Program
    {
        static void Main(string[] args)
        {
            // 데이터 파일 로드
            ConfigManager.LoadConfig();
            DataManager.LoadData();

            // wait
            //Console.WriteLine($"{Config.ToString()}");
            Logger.WriteLog(LogLevel.System, $"Press any key to start dummy client...");
            Console.ReadKey(true);
            Logger.WriteLog(LogLevel.System, $"Start dummy client");

            // 더미클라이언트 시작
            GameManager.Instance.Init();
            GameManager.Instance.Run();

            // user interface
            UserInterface();


            while (true)
            {
                Thread.Sleep(10000);
                Logger.WriteLog(LogLevel.System, $"session:{SessionManager.Instance.SessionCount} players:{GameManager.Instance.PlayerCount}");
            }
        }


        static System.Timers.Timer _timer = new System.Timers.Timer();
        public static void UserInterface()
        {
            _timer.Elapsed += ((s, e) => _userInterfaceRoutine());
            _timer.Interval = 50;
            _timer.AutoReset = false;  // event가 주기적으로 호출되도록 하는것이 아니라 다음 호출시간을 매번 정해줄 것이기 때문에 AutoReset을 false로 한다.
            _timer.Enabled = true;     // timer를 재활성화 한다.
        }

        static void _userInterfaceRoutine()
        {
            ConsoleKeyInfo result = Console.ReadKey(true);
            if ((result.KeyChar == 'S') || (result.KeyChar == 's'))
            {
                if (GameManager.Instance.StopConnection == false)
                {
                    GameManager.Instance.StopConnection = true;
                    Logger.WriteLog(LogLevel.System, $"Stop Connection");
                }
                else
                {
                    GameManager.Instance.StopConnection = false;
                    Logger.WriteLog(LogLevel.System, $"Resume Connection");
                }
            }
            else if ((result.KeyChar == 'D') || (result.KeyChar == 'd'))
            {
                GameManager.Instance.StopConnection = true;
                GameManager.Instance.DisconnectAll = true;
                Logger.WriteLog(LogLevel.System, $"Disconnect all and stop sonnection");
            }


            _timer.Interval = 50;
            _timer.AutoReset = false;
            _timer.Enabled = true;
        }
    }
}
