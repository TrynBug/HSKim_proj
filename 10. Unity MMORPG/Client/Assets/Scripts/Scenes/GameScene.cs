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

        // �ػ� ����
        //Screen.SetResolution(640, 480, false);  // �ػ� 640 x 480 ���� �����ϰ� fullscreen�� false�� �Ѵ�.
        //Screen.SetResolution(1024, 768, false);

        // �� ������ �ε�
        //Managers.Map.LoadMap(1);
    }

    // BaseScene�� Start���� ȣ����
    protected override void Init()
    {
        base.Init();

        // UI ����
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
