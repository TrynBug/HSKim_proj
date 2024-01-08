using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.Tilemaps;

public class TestScene : BaseScene
{
    protected override void Awake()
    {
        base.Awake();

        // �ػ� ����
        Screen.SetResolution(640, 480, false);  // �ػ� 640 x 480 ���� �����ϰ� fullscreen�� false�� �Ѵ�.

        // �� ������ �ε�
        Managers.Map.LoadMap(1);
    }

    // Start is called before the first frame update
    void Start()
    {
        base.Init();

        // UI ����
        Managers.UI.ShowSceneUI<UI_DebugInfo>("DebugInfo");

        // packet delay ����
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
