using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ServerCore;
using static Define;
using UnityEngine;
using Data;


public class AutoMove
{
    public CreatureController Owner { get; private set; }
    public AutoInfo Info { get { return Owner.AutoInfo; } }

    // 상태
    public AutoState State
    {
        get { return Info.AutoState; }
        set
        {
            Info.AutoState = value;
            switch (value)
            {
                case AutoState.AutoIdle:
                    Owner.State = CreatureState.Idle;
                    break;
                case AutoState.AutoChasing:
                    Owner.State = CreatureState.Moving;
                    break;
                case AutoState.AutoMoving:
                    Owner.State = CreatureState.Moving;
                    break;
                case AutoState.AutoSkill:
                    Owner.State = CreatureState.Idle;
                    break;
                case AutoState.AutoWait:
                    Owner.State = CreatureState.Idle;
                    break;
                case AutoState.AutoDead:
                    Owner.State = CreatureState.Dead;
                    break;
            }
        }
    }

    public AutoState NextState
    {
        get { return Info.NextState; }
        set { Info.NextState = value; }
    }

    // Owner
    Vector2 Pos
    {
        get { return Owner.Pos; }
        set { Owner.Pos = value; }
    }

    Vector2 Dest
    {
        get { return Owner.Dest; }
        set { Owner.Dest = value; }
    }



    // 경로
    List<Vector2> _path = new List<Vector2>();
    public List<Vector2> Path
    {
        get { return _path; }
        private set
        {
            _path = value;
            PathIndex = 0;   // 클라이언트는 path의 첫번째 좌표부터 사용함
        }
    }
    public int PathIndex { get; private set; }

    // 타겟
    CreatureController _target;
    public CreatureController Target
    {
        get { return _target; }
        set
        {
            _target = value;
            if (_target == null)
            {
                Info.TargetId = -1;
                Info.TargetPosX = 0;
                Info.TargetPosY = 0;
                PrevTargetCell = new Vector2Int(0, 0);
            }
            else
            {
                Info.TargetId = _target.Id;
                Info.TargetPosX = _target.Pos.x;
                Info.TargetPosY = _target.Pos.y;
                PrevTargetCell = _target.Cell;
            }
        }
    }
    public Vector2Int PrevTargetCell { get; private set; }

    public Vector2 TargetDistance
    {
        get { return new Vector2(Info.TargetDistX, Info.TargetDistY); }
        set
        {
            Info.TargetDistX = value.x;
            Info.TargetDistY = value.y;
        }
    }


    // 스킬
    SkillData _skill;
    public SkillData Skill
    {
        get { return _skill; }
        set
        {
            _skill = value;
            if (_skill != null)
                Info.SkillId = _skill.id;
        }
    }

    // wait
    long _waitTime = 0;   // DateTime.Now.Ticks 기준의 시간
    public long WaitTime
    {
        get { return _waitTime; }
        set
        {
            _waitTime = value;
            Info.WaitUntil = Managers.Time.CurrentTime + value;
        }
    }

    public long WaitUntil { get { return Info.WaitUntil; } }


    public void Init(CreatureController owner)
    {
        Owner = owner;
    }

    // 경로 지정
    public void SetPath(Vector2 startPos, Vector2 endPos)
    {
        //if (Target == null)
        //{
        //    Path.Clear();
        //    Path.Add(startPos);
        //    PathIndex = 0;
        //    PrevTargetCell = Managers.Map.PosToCell(endPos);
        //}
        //else
        //{
        //    Path = Managers.Map.SearchPath(startPos, endPos);
        //    PrevTargetCell = Managers.Map.PosToCell(endPos);
        //}

        Path = Managers.Map.SearchPath(startPos, endPos);
        PrevTargetCell = Managers.Map.PosToCell(endPos);
    }


    // 다음 경로로 움직임
    public void MoveThroughPath()
    {
        if (PathIndex < 0 || PathIndex >= Path.Count)
            return;

        // 목적지 지정
        Owner.Dest = Path[PathIndex];

        // 방향 수정
        Owner.Dir = Util.GetDirectionToDest(Pos, Dest);

        // 목적지에 도달했다면 현재위치를 목적지로 이동시킴
        Vector2 diff = (Dest - Pos);
        Vector2 dir = diff.normalized;
        float magnitude = diff.magnitude;
        float moveDist = Time.deltaTime * Owner.Speed;
        if (magnitude <= moveDist)
        {
            Managers.Map.TryMoving(Owner, Dest, checkCollider: false);
            Pos = Dest;

            // 만약 다음 경로가 있을 경우 목적지를 재지정함
            PathIndex++;
            if (PathIndex < Path.Count)
            {
                Dest = Path[PathIndex];

                // 남은 이동거리를 보정함
                {
                    float remainder = moveDist - magnitude;
                    Vector2 pos = Pos + (Dest - Pos).normalized * remainder;
                    Managers.Map.TryMoving(Owner, pos, checkCollider: false);
                    Pos = pos;
                }
            }
            // 다음 경로가 없을 경우 현재위치에 정지함
            else
            {
                Owner.StopAt(Pos);
            }
        }
        // 현재위치 이동
        else
        {
            Vector2 pos = Pos + dir * moveDist;
            Managers.Map.TryMoving(Owner, pos, checkCollider: false);
            Pos = pos;
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"AutoMove.MoveThroughPath. {Owner.ToString(InfoLevel.Position)}");
    }





    // ToString
    public override string ToString()
    {
        return $"{Owner}, Auto:[state:{State}, pos:({Info.StartPosX},{Info.StartPosY}) target:{Info.TargetId} tPos:({Info.TargetPosX},{Info.TargetPosY}), " +
            $"dist:({Info.TargetDistX},{Info.TargetDistY}), wait:{(float)(Info.WaitUntil - Managers.Time.CurrentTime) / (float)TimeSpan.TicksPerSecond}, next:{Info.NextState}, skill:{Info.SkillId}]";
    }

}



