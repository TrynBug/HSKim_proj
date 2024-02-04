using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using ServerCore;

public class GameScene : BaseScene
{
    protected override void Awake()
    {
        base.Awake();

        // 해상도 설정
        Screen.SetResolution(640, 480, false);  // 해상도 640 x 480 으로 설정하고 fullscreen은 false로 한다.
        //Screen.SetResolution(1024, 768, false);

        // 맵 데이터 로드
        Managers.Map.LoadMap(1);
    }

    // Start is called before the first frame update
    void Start()
    {
        base.Init();

        ServerCore.Logger.Level = LogLevel.System;

        // UI 생성
        Managers.UI.ShowSceneUI<UI_DebugInfo>("DebugInfo");

        // packet delay 설정
        Managers.Network.PacketDelay = 50;
        Managers.Network.IsDelayedPacket = false;
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public override void Clear()
    {

    }

}
