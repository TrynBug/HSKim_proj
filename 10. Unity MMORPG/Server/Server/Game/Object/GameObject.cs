using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ServerCore;
using System.Numerics;
using Server.Data;

namespace Server.Game
{
    public class GameObject
    {
        public GameObjectType ObjectType { get; protected set; } = GameObjectType.None;
        // bits : [ Unused(1) | Type(7) | Id(24) ]
        public int Id
        {
            get { return Info.ObjectId; }
            set { Info.ObjectId = value; }
        }

        public GameRoom Room { get; set; } = null;
        public ObjectInfo Info { get; private set; } = new ObjectInfo();   // Info.PosInfo 는 PosInfo를 리턴함
        public PositionInfo PosInfo { get; private set; } = new PositionInfo();
        public StatInfo Stat { get; private set; } = new StatInfo();  // Info.StatInfo 는 Stat을 리턴함

        /* 스탯 */
        public float Speed
        {
            get { return Stat.Speed; }
            set { Stat.Speed = value; }
        }

        public int Hp
        {
            get { return Stat.Hp; }
            set { Stat.Hp = Math.Clamp(value, 0, Stat.MaxHp); }
        }

        /* 위치 */
        public Vector2 Pos
        {
            get
            {
                return new Vector2(PosInfo.PosX, PosInfo.PosY);
            }
            set
            {
                PosInfo.PosX = value.x;
                PosInfo.PosY = value.y;
                if(Room != null)
                    Cell = Room.Map.PosToCell(Pos);
            }
        }
        public Vector2 Dest
        {
            get
            {
                return new Vector2(PosInfo.DestX, PosInfo.DestY);
            }
            set
            {
                PosInfo.DestX = value.x;
                PosInfo.DestY = value.y;
            }
        }
        public Vector2Int Cell { get; private set; }

        public CreatureState State
        {
            get { return PosInfo.State; }
            set { PosInfo.State = value; }
        }

        public virtual MoveDir Dir
        {
            get { return PosInfo.MoveDir; }
            set
            {
                if (PosInfo.MoveDir == value)
                    return;
                PosInfo.MoveDir = value;
                switch (PosInfo.MoveDir)
                {
                    case MoveDir.Left:
                    case MoveDir.LeftUp:
                    case MoveDir.LeftDown:
                        LookDir = LookDir.LookLeft;
                        break;
                    case MoveDir.Right:
                    case MoveDir.RightUp:
                    case MoveDir.RightDown:
                        LookDir = LookDir.LookRight;
                        break;
                }
            }
        }

        public virtual LookDir LookDir
        {
            get { return PosInfo.LookDir; }
            set { PosInfo.LookDir = value; }
        }

        public virtual bool MoveKeyDown
        {
            get { return PosInfo.MoveKeyDown; }
            set { PosInfo.MoveKeyDown = value; }
        }



        /* 스킬 */
        // 사용가능한 스킬정보
        Dictionary<int, SkillInfo> _skillset = new Dictionary<int, SkillInfo>();
        public Dictionary<int, SkillInfo> Skillset { get { return _skillset; } }

        // 스킬 키 눌림
        public virtual bool SkillKeyDown { get; set; } = false;




        public GameObject()
        {
            Info.PosInfo = PosInfo;
            Info.StatInfo = Stat;
        }




        // ToString
        public override string? ToString()
        {
            return ToString(InfoLevel.Identity);
        }
        public virtual string? ToString(InfoLevel infoType)
        {
            switch (infoType)
            {
                case InfoLevel.Identity:
                    return $"id:{Id}, type:{ObjectType}, room:{Room?.RoomId}";
                case InfoLevel.Position:
                    return $"id:{Id}, type:{ObjectType}, room:{Room?.RoomId}, pos:{Pos}, dest:{Dest}, cell:{Cell}, state:{State}, dir:{Dir}";
                case InfoLevel.Stat:
                    return $"id:{Id}, type:{ObjectType}, room:{Room?.RoomId}, speed:{Speed}, hp:{Hp}";
                case InfoLevel.All:
                    return $"id:{Id}, type:{ObjectType}, room:{Room?.RoomId}, pos:{Pos}, dest:{Dest}, cell:{Cell}, state:{State}, dir:{Dir}, speed:{Speed}, hp:{Hp}";
                default:
                    return $"id:{Id}, type:{ObjectType}, room:{Room?.RoomId}";
            }
        }




        // update
        public virtual void Update()
        {
            switch (State)
            {
                case CreatureState.Idle:
                    UpdateIdle();
                    break;
                case CreatureState.Moving:
                    UpdateMoving();
                    break;
                case CreatureState.Dead:
                    UpdateDead();
                    break;
            }
        }

        protected virtual void UpdateIdle()
        {
        }

        protected virtual void UpdateMoving()
        {
        }

        protected virtual void UpdateDead()
        {
        }

        protected virtual void UpdateSkill()
        {
        }





        // 현재 방향에 해당하는 벡터 얻기
        public Vector2 GetDirectionVector(MoveDir dir)
        {
            Vector2 direction;
            switch (dir)
            {
                case MoveDir.Up:
                    direction = new Vector2(1, -1).normalized;
                    break;
                case MoveDir.Down:
                    direction = new Vector2(-1, 1).normalized;
                    break;
                case MoveDir.Left:
                    direction = new Vector2(-1, -1).normalized;
                    break;
                case MoveDir.Right:
                    direction = new Vector2(1, 1).normalized;
                    break;
                case MoveDir.LeftUp:
                    direction = new Vector2(0, -1);
                    break;
                case MoveDir.LeftDown:
                    direction = new Vector2(-1, 0);
                    break;
                case MoveDir.RightUp:
                    direction = new Vector2(1, 0);
                    break;
                case MoveDir.RightDown:
                    direction = new Vector2(0, 1);
                    break;
                default:
                    direction = new Vector2(0, 0);
                    break;
            }

            return direction;
        }


        // 스킬 사용이 가능한지 검사함
        public virtual bool CanUseSkill(int skillId, out Skill skill)
        {
            skill = null;
            SkillInfo skillInfo;
            if (Skillset.TryGetValue(skillId, out skillInfo) == false)
                return false;

            int tick = Environment.TickCount;
            if (tick - skillInfo.lastUseTime < skillInfo.skill.cooldown)
                return false;

            Skillset[skillId] = new SkillInfo() { lastUseTime = tick, skill = skillInfo.skill };
            skill = skillInfo.skill;

            return true;
        }


        // 피격됨
        public virtual void OnDamaged(GameObject attacker, int damage)
        {
            if (Room == null)
            {
                Logger.WriteLog(LogLevel.Error, $"GameObject.OnDamaged. Room is null. objectId:{Id}");
                return;
            }

            // HP 감소
            Stat.Hp -= damage;
            if (Stat.Hp <= 0)
                Stat.Hp = 0;

            Logger.WriteLog(LogLevel.Debug, $"GameObject.OnDamaged. damage:{damage}, me:[{this.ToString(InfoLevel.Stat)}], attacker:[{attacker.ToString(InfoLevel.Stat)}]");

            // HP 변경 패킷 전송
            S_ChangeHp changePacket = new S_ChangeHp();
            changePacket.ObjectId = Id;
            changePacket.Hp = Stat.Hp;
            Room._broadcast(changePacket);

            // 사망처리
            if(Stat.Hp <= 0)
            {
                OnDead(attacker);
            }
        }

        // 사망함
        public virtual void OnDead(GameObject attacker)
        {

            // 사망 패킷 전송
            S_Die diePacket = new S_Die();
            diePacket.ObjectId = Id;
            diePacket.AttackerId = attacker.Id;
            Room._broadcast(diePacket);

            Logger.WriteLog(LogLevel.Debug, $"GameObject.OnDead. me:[{this.ToString(InfoLevel.Stat)}], attacker:[{attacker.ToString(InfoLevel.Stat)}]");
        }
    }
}
