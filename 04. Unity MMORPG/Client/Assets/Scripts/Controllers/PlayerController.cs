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


    // Idle ���� ������Ʈ
    protected override void UpdateIdle()
    {
        // ����Ű ���� �����̰ų�, Dest�� Pos�� ���� �ʴٸ� �̵��Ѵ�.
        if (MoveKeyDown == true || Util.Equals(Pos, Dest) == false)
        {
            State = CreatureState.Moving;
            UpdateMoving();
        }
    }

}
