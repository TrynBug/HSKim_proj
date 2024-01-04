using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using ServerCore;

public class GameScene : BaseScene
{
    protected override void Awake()
    {
        base.Awake();

        // 해상도 설정
        Screen.SetResolution(640, 480, false);  // 해상도 640 x 480 으로 설정하고 fullscreen은 false로 한다.

        // 맵 데이터 로드
        Managers.Map.LoadMap(1);
    }


    protected override void Init()
    {
        base.Init();
        SceneType = Define.Scene.Game;

        // 몬스터 생성
        //for (int i = 0; i < 5; i++)
        //{
        //    GameObject monster = Managers.Resource.Instantiate("Creature/Monster");
        //    monster.name = $"Monster_{i + 1}";

        //    Vector3Int pos = new Vector3Int()
        //    {
        //        x = Random.Range(-10, 10),
        //        y = Random.Range(-5, 5)
        //    };
        //    MonsterController mc = monster.GetComponent<MonsterController>();
        //    mc.CellPos = pos;
        //    mc.id = 10000 + i;

        //    Managers.Object.Add(mc.id, monster);
        //}
    }

    public override void Clear()
    {
        
    }
}
