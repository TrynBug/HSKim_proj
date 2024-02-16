using ExcelDataReader;
using ServerCore;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.IO;
using UnityEngine;

[Serializable]
public class ConfigManager
{
    /* server */
    public string ServerIP;
    public int ServerPort;


    public void Init()
    {
        TextAsset textAsset = Managers.Resource.Load<TextAsset>($"Data/Config");  // config.json 파일 읽기

        ConfigManager config = Newtonsoft.Json.JsonConvert.DeserializeObject<ConfigManager>(textAsset.text);

        ServerIP = config.ServerIP;
        ServerPort = config.ServerPort;
    }
}
