using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using Google.Protobuf.Protocol;
using static Define;
using ServerCore;

public class PlayerController : SPUMController
{
    public override void Init(ObjectInfo info)
    {
        base.Init(info);

        ObjectType = GameObjectType.Player;
    }


    protected override void UpdateController()
    {
        base.UpdateController();

        UpdateDebugText();
    }


    // Idle 상태 업데이트
    protected override void UpdateIdle()
    {
        // 방향키 눌림 상태이거나, Dest와 Pos가 같지 않다면 이동한다.
        if (MoveKeyDown == true || Util.Equals(Pos, Dest) == false)
        {
            State = CreatureState.Moving;
            UpdateMoving();
        }
    }

}
