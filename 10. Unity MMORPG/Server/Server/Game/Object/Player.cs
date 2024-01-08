using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using static System.Net.Mime.MediaTypeNames;

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


        protected override void UpdateIdle()
        {
            if(RemoteState == CreatureState.Moving || Util.Equals(Pos, Dest) == false)
            {
                State = CreatureState.Moving;
                UpdateMoving();
            }
        }

        protected override void UpdateMoving()
        {
            // 클라이언트에게 멈춤 패킷을 받음
            if(MoveKeyDown == false)
            {
                // 현재 Dest와의 거리가 이동거리 내라면 현재위치를 Dest로 세팅
                if ((Dest - Pos).magnitude < Room.Time.DeltaTime * Speed)
                {
                    Vector2Int destCell = Room.Map.PosToCell(Dest);
                    if (Room.Map.TryMove(this, destCell))
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
                    Vector2 pos = Pos + dir * Room.Time.DeltaTime * Speed;
                    Vector2Int cell = Room.Map.PosToCell(pos);
                    if (Room.Map.TryMove(this, cell))
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
            // 클라이언트에게 이전에 이동 패킷을 받았음
            else
            {
                // Dest를 이동시킨다.
                Vector2 dest = Dest + GetDirectionVector(RemoteDir) * Room.Time.DeltaTime * Speed;
                if (Room.Map.IsMovable(this, dest))
                {
                    Dest = dest;
                }
                
                // 현재 Dest와의 거리가 이동거리 내라면 현재위치를 Dest로 세팅
                if ((Dest - Pos).magnitude < Room.Time.DeltaTime * Speed)
                {
                    Vector2Int destCell = Room.Map.PosToCell(Dest);
                    if (Room.Map.TryMove(this, destCell))
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
                    Vector2 pos = Pos + dir * Room.Time.DeltaTime * Speed;
                    Vector2Int cell = Room.Map.PosToCell(pos);
                    if (Room.Map.TryMove(this, cell))
                    {
                        Pos = pos;
                    }
                    else
                    {
                        Dest = Pos;
                    }
                }
            }



            Logger.WriteLog(LogLevel.Debug, $"Player.UpdateMoving. {this.ToString(InfoLevel.Position)}");
        }

        protected override void UpdateSkill()
        {
        }

        protected override void UpdateDead()
        {
        }


        // 피격됨
        public override void OnDamaged(GameObject attacker, int damage)
        {
            base.OnDamaged(attacker, damage);

            Logger.WriteLog(LogLevel.Debug, $"Player.OnDamaged. myId:{Id}, attackerId:{attacker.Id}, damage:{damage}");
        }

        // 사망함
        public override void OnDead(GameObject attacker)
        {
            base.OnDead(attacker);

            Logger.WriteLog(LogLevel.Debug, $"Player.OnDead. myId:{Id}, attackerId:{attacker.Id}");
        }

    }



}
