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

        // 맵 데이터 로드
        Managers.Map.LoadMap(1);
    }

    // Start is called before the first frame update
    void Start()
    {
        base.Init();
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public override void Clear()
    {

    }
}
