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

    Animator _animator;

    public EffectController()
    {
        ObjectType = GameObjectType.Effect;

        // id 생성. Effect는 id를 클라이언트에서 생성한다. 서버에는 effect가 없기 때문
        Info.ObjectId = GenerateEffectId();
    }

    protected override void Init()
    {
        _animator = gameObject.GetComponent<Animator>();
        base.Init();
    }

    public void Init(string prefab, Vector2 pos)
    {
        Prefab = prefab;
        Pos = pos;
        State = CreatureState.Idle;
    }


    protected override void UpdateIdle()
    {
        if(_animator == null)
        {
            State = CreatureState.Dead;
            return;
        }

        // 애니메이션이 끝났다면 상태를 Dead로 변경
        if(_animator.GetCurrentAnimatorStateInfo(0).IsName("End"))
            State = CreatureState.Dead;
    }


    protected override void UpdateDead()
    {
        // 자신 파괴
        Managers.Object.Remove(Id);
    }
}

