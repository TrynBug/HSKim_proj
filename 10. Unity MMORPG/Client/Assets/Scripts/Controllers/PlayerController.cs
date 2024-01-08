using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using Google.Protobuf.Protocol;
using static Define;
using ServerCore;

public class PlayerController : CreatureController
{
    protected Coroutine _coSkill;  // skill 사용 후 후처리(쿨타임 등)를 위한 코루틴

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
    }


    protected override void UpdateIdle()
    {
        if (RemoteState == CreatureState.Moving || Util.Equals(Pos, Dest) == false)
        {
            State = CreatureState.Moving;
            UpdateMoving();
        }
    }

    protected override void UpdateMoving()
    {
        // 서버에게 멈춤 패킷을 받음
        if (MoveKeyDown == false)
        {
            // 현재 Dest와의 거리가 이동거리 내라면 현재위치를 Dest로 세팅
            if ((Dest - Pos).magnitude < Time.deltaTime * Speed)
            {
                Vector2Int destCell = Managers.Map.PosToCell(Dest);
                if (Managers.Map.TryMove(this, destCell))
                {
                    Pos = Dest;
                    State = CreatureState.Idle;
                }
                else
                {
                    Dest = Pos;
                    State = CreatureState.Idle;
                }
            }
            // 이동
            else
            {
                Vector2 dir = (Dest - Pos).normalized;
                Vector2 pos = Pos + dir * Time.deltaTime * Speed;
                Vector2Int cell = Managers.Map.PosToCell(pos);
                if (Managers.Map.TryMove(this, cell))
                {
                    Pos = pos;
                }
                else
                {
                    Dest = Pos;
                    State = CreatureState.Idle;
                }
            }
        }
        // 서버에게 이전에 이동 패킷을 받았음
        else
        {
            // Dest를 이전에 이동했던 방향으로 이동시킨다.
            Vector2 dest = Dest + GetDirectionVector(RemoteDir) * Time.deltaTime * Speed;
            if (Managers.Map.IsMovable(this, dest))
            {
                Dest = dest;
            }

            // 현재 Dest와의 거리가 이동거리 내라면 현재위치를 Dest로 세팅
            if ((Dest - Pos).magnitude < Time.deltaTime * Speed)
            {
                Vector2Int destCell = Managers.Map.PosToCell(Dest);
                if (Managers.Map.TryMove(this, destCell))
                {
                    Pos = Dest;
                }
                else
                {
                    Dest = Pos;
                }
            }
            // 이동
            else
            {
                Vector2 dir = (Dest - Pos).normalized;
                Vector2 pos = Pos + dir * Time.deltaTime * Speed;
                Vector2Int cell = Managers.Map.PosToCell(pos);
                if (Managers.Map.TryMove(this, cell))
                {
                    Pos = pos;
                }
                else
                {
                    Dest = Pos;
                }
            }
        }







        ServerCore.Logger.WriteLog(LogLevel.Debug, $"Player.UpdateMoving. {this.ToString(InfoLevel.Position)}");
    }



    protected override void UpdateAnimation()
    {
        switch (State)
        {
            case CreatureState.Idle:
                _animator.SetFloat("RunState", 0);
                break;
            case CreatureState.Moving:
                _animator.SetFloat("RunState", 0.5f);
                switch (Dir)
                {
                    case MoveDir.Left:
                    case MoveDir.LeftUp:
                    case MoveDir.LeftDown:
                        gameObject.transform.localScale = new Vector3(1, 1, 1);
                        break;
                    case MoveDir.Right:
                    case MoveDir.RightUp:
                    case MoveDir.RightDown:
                        gameObject.transform.localScale = new Vector3(-1, 1, 1);
                        break;
                }
                break;
            case CreatureState.Dead:
                _animator.SetTrigger("Die");
                break;
        }
    }






    // 피격됨
    public override void OnDamaged()
    {
        //Debug.Log("Player HIT !");
    }



    // 스킬사용
    public override void UseSkill(int skillId)
    {
        if(skillId == 1)
        {
            _coSkill = StartCoroutine("CoStartPunch");
        }
        else if (skillId == 2)
        {
            _coSkill = StartCoroutine("CoStartShootArrow");
        }
    }


    protected virtual void CheckUpdatedFlag()
    {
    }

    // Punch 사용에 대한 코루틴
    protected IEnumerator CoStartPunch()
    {
        // 쿨타임 후 상태를 Idle로 바꾼다.
        //_rangeSkill = false;
        //State = CreatureState.Skill;
        yield return new WaitForSeconds(0.5f);
        State = CreatureState.Idle;
        _coSkill = null;
        CheckUpdatedFlag();
    }

    // 화살 사용에 대한 코루틴
    protected IEnumerator CoStartShootArrow()
    {
        // 쿨타임 후 상태를 Idle로 바꾼다.
        //_rangeSkill = true;
        //State = CreatureState.Skill;
        yield return new WaitForSeconds(0.3f);
        State = CreatureState.Idle;
        _coSkill = null;
        CheckUpdatedFlag();
    }


}
