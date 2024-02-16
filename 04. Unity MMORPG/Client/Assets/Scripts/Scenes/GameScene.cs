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
        //if (Config.DebugOn)
        //{
        //    DrawDebugLine();
        //}
    }

    public override void Clear()
    {

    }

    [SerializeField]
    Color _debugLineColor = Color.blue;
    [SerializeField]
    Vector3 _debugLinePosCorrection = new Vector3(0, 0, 0);
    void DrawDebugLine()
    {
        MapManager map = Managers.Map;
        for (int y = 0; y < map.CellMaxY; y++)
        {
            Vector3 startPos = new Vector3(0, y * map.CellHeight, 0);
            Vector3 endPos = new Vector3(map.CellMaxX * map.CellWidth, y * map.CellHeight, 0);
            UnityEngine.Debug.DrawLine(
                map.ServerPosToClientPos(startPos) + _debugLinePosCorrection,
                map.ServerPosToClientPos(endPos) + _debugLinePosCorrection,
                _debugLineColor);
        }
        for (int x = 0; x < map.CellMaxX; x++)
        {
            Vector3 startPos = new Vector3(x * map.CellWidth, 0, 0);
            Vector3 endPos = new Vector3(x * map.CellWidth, map.CellMaxY * map.CellHeight, 0);
            UnityEngine.Debug.DrawLine(
                map.ServerPosToClientPos(startPos) + _debugLinePosCorrection,
                map.ServerPosToClientPos(endPos) + _debugLinePosCorrection,
                _debugLineColor);
        }
    }

}
