using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using Google.Protobuf.Protocol;
using static Define;
using ServerCore;

public class PlayerController : SPUMController
{
    public CreatureState RemoteState { get; set; } = CreatureState.Idle;
    public MoveDir RemoteDir { get; set; } = MoveDir.Down;

    protected override void Init()
    {
        ObjectType = GameObjectType.Player;
        base.Init();
    }


    protected override void UpdateController()
    {
        base.UpdateController();

        UpdateDebugText();
    }


    protected override void UpdateIdle()
    {
        if (MoveKeyDown == true || Util.Equals(Pos, Dest) == false)
        {
            State = CreatureState.Moving;
            UpdateMoving();
        }
    }

    protected override void UpdateMoving()
    {
        // 키보드 방향키가 눌려있는 동안은 Dest를 계속해서 이동시킨다.
        Vector2 intersection;
        if (MoveKeyDown == true)
        {
            Vector2 dest = Dest + GetDirectionVector(RemoteDir) * Time.deltaTime * Speed;
            if (Managers.Map.CanGo(Dest, dest, out intersection))
            {
                Dest = dest;
            }
            else
            {
                Dest = intersection;
            }
        }
        // 키보드 방향키를 누르고있지 않다면 멈출 것이기 때문에 Dest를 이동시키지 않는다.



        // 위치 이동
        Vector2 diff = (Dest - Pos);
        Vector2 dir = diff.normalized;
        Vector2 pos = Pos + dir * Time.deltaTime * Speed;

        // Dest에 도착시 현재위치를 Dest로 변경한다.
        // 만약 이동키가 눌려져있지 않다면 현재 위치에 stop 하고 상태를 idle로 바꾼다.
        if (diff.magnitude <= Time.deltaTime * Speed)
        {
            if (MoveKeyDown)
            {
                if (Managers.Map.TryMoving(this, Dest))
                {
                    Pos = Dest;
                }
                else
                {
                    Dest = Pos;
                }
            }
            else
            {
                Vector2 stopPos;
                if (Managers.Map.TryStop(this, Dest, out stopPos))
                {
                    Pos = stopPos;
                    Dest = stopPos;
                }
                else
                {
                    Dest = Pos;
                }
                State = CreatureState.Idle;
            }
        }
        // 위치 이동
        else if (Managers.Map.CanGo(Pos, pos, out intersection))
        {
            if (Managers.Map.TryMoving(this, pos))
            {
                Pos = pos;
            }
            else
            {
                Dest = Pos;
            }
        }
        // 이동중 부딪혔을 경우 더이상 이동할 수 없기 때문에 Dest를 변경한다.
        else
        {
            if (Managers.Map.TryMoving(this, intersection))
            {
                Pos = intersection;
                Dest = intersection;
            }
            else
            {
                Dest = Pos;
            }
        }


        ServerCore.Logger.WriteLog(LogLevel.Debug, $"Player.UpdateMoving. {this.ToString(InfoLevel.Position)}");
    }

}
