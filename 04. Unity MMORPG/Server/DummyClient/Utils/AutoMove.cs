using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ServerCore;
using DummyClient.Data;
using static DummyClient.Define;

namespace DummyClient.Game
{
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
                //Info.WaitUntil = Managers.Time.CurrentTime + value;
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
        }


        // 다음 경로로 움직임
        public void MoveThroughPath()
        {
        }
    }
}
