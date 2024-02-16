using Data;
using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using UnityEngine;


public class EffectController : BaseController
{
    // bits : [ Unused(1) | Type(7) | Id(24) ]
    static int _counter = 1;

    // id 생성. Effect는 id를 클라이언트에서 생성한다. 서버에는 effect가 없기 때문
    static int GenerateEffectId()
    {
        return ((int)GameObjectType.Effect << 24) | (Interlocked.Increment(ref _counter));
    }


    public string Prefab { get; protected set; }
    public float OffsetY { get; protected set; }
    Animator _animator;

    public override Vector2 Pos
    {
        get { return base.Pos; }
        set
        {
            base.Pos = value;
            gameObject.transform.position += new Vector3(0, OffsetY, Config.ObjectDefaultZ);
        }
    }





    public void Init(string prefab, Vector2 pos, float offsetY)
    {
        base.Init();
        _animator = gameObject.GetComponent<Animator>();

        ObjectType = GameObjectType.Effect;

        // id 생성. Effect는 id를 클라이언트에서 생성한다. 서버에는 effect가 없기 때문
        Info.ObjectId = GenerateEffectId();

        Prefab = prefab;
        OffsetY = offsetY;

        Pos = pos;  // Pos를 OffsetY 보다 나중에 지정해야함
        State = CreatureState.Idle;
    }


    // 애니메이션의 마지막 key frame 에서 호출될 이벤트
    void EndAnimationEventCallback()
    {
        // 자신 파괴
        Managers.Object.Remove(Id);
    }

}

