using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Data
{
    // 서버 설정값들
    [Serializable]
    public class ServerConfig
    {
        public string dataPath;  // 현재 값은 dataPath="../../../../../Client/Assets/Resources/Data" 이다.
    }

    // 서버 설정파일을 관리하는 매니저
    public class ConfigManager
    {
        public static ServerConfig Config { get; private set; }

        public static void LoadConfig()
        {
            string text = File.ReadAllText("config.json");
            Config = Newtonsoft.Json.JsonConvert.DeserializeObject<ServerConfig>(text);
        }
    }
}
