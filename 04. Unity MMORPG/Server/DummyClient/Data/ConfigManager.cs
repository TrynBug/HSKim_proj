using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DummyClient.Data
{
    [Serializable]
    public class ServerConfig
    {
        /* server */
        public string dataPath;  // 현재 값은 dataPath="../../../../../Client/Assets/Resources/Data" 이다.
        public string serverIP;
        public int serverPort;

        /* room */
        public int FPS;

        /* map */
        public int CellMultiple;

        /* float */
        public float DifferenceTolerance;

        /* dummy client */
        public int MaxNumberOfClient;
        public int MaxConnectionCountPerFrame;
        public float DisconnectProb;
        public float AutoProb;

        /* move */
        public float MyPlayerMinimumMove;
        public int MovePacketSendInterval;
    }

    // 서버 설정파일을 관리하는 매니저
    public class ConfigManager
    {
        public static ServerConfig Config { get; private set; }

        public static void LoadConfig()
        {
            string text = File.ReadAllText("../../../config.json");
            Config = Newtonsoft.Json.JsonConvert.DeserializeObject<ServerConfig>(text);
        }
    }
}
