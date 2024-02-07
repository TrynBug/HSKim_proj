using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using ServerCore;
using System.Diagnostics;
using Data;

public class GameScene : BaseScene
{
    protected override void Awake()
    {
        base.Awake();

        // 해상도 설정
        //Screen.SetResolution(640, 480, false);  // 해상도 640 x 480 으로 설정하고 fullscreen은 false로 한다.
        //Screen.SetResolution(1024, 768, false);

        // 맵 데이터 로드
        //Managers.Map.LoadMap(1);
    }

    // BaseScene의 Start에서 호출함
    protected override void Init()
    {
        base.Init();

        // UI 생성
        if(Config.DebugUIOn)
            Managers.UI.ShowSceneUI<UI_Debug>("UI_Debug");
        Managers.UI.ShowSceneUI<UI_Game>("UI_Game");
    }


    void Update()
    {
        
    }

    public override void Clear()
    {

    }

}
