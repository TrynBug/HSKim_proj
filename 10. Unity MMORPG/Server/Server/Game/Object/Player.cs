using Google.Protobuf.Protocol;
using Google.Protobuf.WellKnownTypes;
using Server.Data;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class Player : GameObject
    {
        public ClientSession Session { get; set; }

        public CreatureState RemoteState { get; set; } = CreatureState.Idle;
        public MoveDir RemoteDir { get; set; } = MoveDir.Down;


        public Player()
        {
            ObjectType = GameObjectType.Player;
        }


        public void Init(ClientSession session, GameRoom room)
        {
            Session = session;

            Room = room;
            Info.Name = $"Player_{Id}";
            State = CreatureState.Idle;
            Dir = MoveDir.Left;
            Pos = room.PosCenter;
            Dest = Pos;

            StatInfo stat = null;
            DataManager.StatDict.TryGetValue(1, out stat);
            Stat.MergeFrom(stat);

            Skillset.Add(DataManager.DefaultSkill.id, new SkillInfo() { lastUseTime = 0, skill = DataManager.DefaultSkill });
            Skill skill;
            if (DataManager.SkillDict.TryGetValue(2, out skill))
                Skillset.Add(skill.id, new SkillInfo() { lastUseTime = 0, skill = skill });
        }
    


        protected override void UpdateIdle()
        {
            if(MoveKeyDown == true || Util.Equals(Pos, Dest) == false)
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
                Vector2 dest = Dest + GetDirectionVector(RemoteDir) * Room.Time.DeltaTime * Speed;
                if (Room.Map.CanGo(Dest, dest, out intersection))
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
            Vector2 pos = Pos + dir * Room.Time.DeltaTime * Speed;

            // Dest에 도착시 현재위치를 Dest로 변경한다.
            // 만약 이동키가 눌려져있지 않다면 현재 위치에 stop 하고 상태를 idle로 바꾼다.
            if (diff.magnitude <= Room.Time.DeltaTime * Speed)
            {
                if (MoveKeyDown)
                {
                    if (Room.Map.TryMoving(this, Dest))
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
                    if (Room.Map.TryStop(this, Dest, out stopPos))
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
            else if (Room.Map.CanGo(Pos, pos, out intersection))
            {
                if (Room.Map.TryMoving(this, pos))
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
                if (Room.Map.TryMoving(this, intersection))
                {
                    Pos = intersection;
                    Dest = intersection;
                }
                else
                {
                    Dest = Pos;
                }
            }



            Logger.WriteLog(LogLevel.Debug, $"Player.UpdateMoving. {this.ToString(InfoLevel.Position)}");
        }


        protected override void UpdateDead()
        {
        }


        // 피격됨
        public override void OnDamaged(GameObject attacker, int damage)
        {
            base.OnDamaged(attacker, damage);
        }

        // 사망함
        public override void OnDead(GameObject attacker)
        {
            base.OnDead(attacker);
        }

    }



}
