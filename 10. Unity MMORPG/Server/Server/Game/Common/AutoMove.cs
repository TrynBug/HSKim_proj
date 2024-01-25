using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ServerCore;

namespace Server.Game
{
    public class AutoMove
    {
        public GameObject Owner { get; private set; }
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
        GameRoom Room { get { return Owner.Room; } }
        Vector2 Pos { 
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
                PathIndex = 1;
            }
        }
        public int PathIndex { get; private set; }

        // 타겟
        GameObject _target;
        public GameObject Target
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
        SkillUseInfo _skillUse;
        public SkillUseInfo SkillUse
        {
            get { return _skillUse; }
            set
            {
                _skillUse = value;
                if (_skillUse != null)
                    Info.SkillId = _skillUse.skill.id;
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
                Info.WaitUntil = DateTime.Now.Ticks + value;
            }
        }

        public long WaitUntil { get { return Info.WaitUntil; } }


        // init
        public void Init(GameObject owner)
        {
            Owner = owner;

            Path.Clear();
            PathIndex = 1;
            Target = null;
            PrevTargetCell = new Vector2Int(0, 0);
            SkillUse = null;
        }


        // 가장 빨리 사용할 수 있는 스킬을 얻는다.
        public void SetNextSkill()
        {
            SkillUseInfo nextSkill = null;
            int fastestTime = int.MaxValue;
            foreach (SkillUseInfo use in Owner.Skillset.Values)
            {
                if (fastestTime > use.lastUseTime + use.skill.cooldown)
                {
                    fastestTime = use.lastUseTime + use.skill.cooldown;
                    nextSkill = use;
                }
            }

            SkillUse = nextSkill;
            
            // 유지할 거리 재계산
            TargetDistance = (new Vector2(SkillUse.skill.rangeX, SkillUse.skill.rangeY / 2f)) * (3f / 4f);
        }

        // 경로 지정
        public void SetPathToTarget()
        {
            Info.StartPosX = Owner.Pos.x;
            Info.StartPosY = Owner.Pos.y;

            if (Target == null)
            {
                Path.Clear();
                Path.Add(Owner.Pos);
                PathIndex = 1;
            }
            else
            {
                Path = Owner.Room.Map.SearchPath(Owner.Pos, Target.Pos);
                Info.TargetPosX = Target.Pos.x;
                Info.TargetPosY = Target.Pos.y;
                PrevTargetCell = Target.Cell;
            }
        }


        // 패킷 전송
        public void SendAutoPacket()
        {
            S_AutoMove autoPacket = new S_AutoMove();
            autoPacket.ObjectId = Owner.Id;
            autoPacket.PosInfo = Owner.PosInfo;
            autoPacket.AutoInfo = Info;
            Owner.Room._broadcast(autoPacket);


            Logger.WriteLog(LogLevel.Debug, $"AutoMove.SendAutoPacket. {this}");
        }


        // 다음 경로로 움직임
        public void MoveThroughPath()
        {
            if (PathIndex < 0 || PathIndex >= Path.Count)
                return;

            // 목적지 지정
            Owner.Dest = Path[PathIndex];

            // 목적지에 도달했다면 현재위치를 목적지로 이동시킴
            Vector2 diff = (Dest - Pos);
            Vector2 dir = diff.normalized;
            float magnitude = diff.magnitude;
            float moveDist = Room.Time.DeltaTime * Owner.Speed;
            if (magnitude <= moveDist)
            {
                Room.Map.TryMoving(Owner, Dest, checkCollider: false);
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
                        Room.Map.TryMoving(Owner, pos, checkCollider: false);
                        Pos = pos;
                    }
                }
                // 다음 경로가 없을 경우 현재위치에 정지함
                else
                {
                    Vector2 stopPos;
                    Room.Map.TryStop(Owner, Pos, out stopPos);
                    Pos = stopPos;
                    Dest = stopPos;
                }
            }
            // 현재위치 이동
            else
            {
                Vector2 pos = Pos + dir * moveDist;
                Room.Map.TryMoving(Owner, pos, checkCollider: false);
                Pos = pos;
            }

            // 방향 수정
            Owner.Dir = Util.GetDirectionToDest(Pos, Dest);

            Logger.WriteLog(LogLevel.Debug, $"AutoMove.MoveThroughPath. {Owner.ToString(InfoLevel.Position)}");
        }





        // ToString
        public override string ToString()
        {
            return $"{Owner}, Auto:[state:{State}, pos:({Info.StartPosX},{Info.StartPosY}) target:{Info.TargetId} tPos:({Info.TargetPosX},{Info.TargetPosY}), " +
                $"dist:({Info.TargetDistX},{Info.TargetDistY}), wait:{(float)(WaitUntil - DateTime.Now.Ticks) / (float)TimeSpan.TicksPerSecond}, next:{Info.NextState}, skill:{Info.SkillId}]";
        }

    }
}
