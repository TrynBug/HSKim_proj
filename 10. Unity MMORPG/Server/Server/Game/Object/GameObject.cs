using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ServerCore;
using System.Numerics;
using Server.Data;
using System.Threading;

namespace Server.Game
{
    public class GameObject
    {
        // bits : [ Unused(1) | Type(7) | Id(24) ]
        static int _counter = 1;

        // id 생성
        static int GenerateId(GameObjectType type)
        {
            return ((int)type << 24) | (Interlocked.Increment(ref _counter));
        }



        public GameObjectType ObjectType { get; protected set; } = GameObjectType.None;
        // bits : [ Unused(1) | Type(7) | Id(24) ]
        public int Id { get { return Info.ObjectId; } }

        public GameRoom Room { get; set; } = null;
        public ObjectInfo Info { get; private set; } = new ObjectInfo();   
        public PositionInfo PosInfo { get; private set; } = new PositionInfo();  // Info.PosInfo 는 PosInfo를 리턴함
        public StatInfo StatOrigin { get; private set; } = new StatInfo();   // 초기 스탯 데이터
        public StatInfo Stat { get; private set; } = new StatInfo();       // Info.StatInfo 는 Stat을 리턴함

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

        public Vector2 Range
        {
            get { return new Vector2(Stat.RangeX, Stat.RangeY); }
            set 
            { 
                Stat.RangeX = value.x;
                Stat.RangeY = value.y;
            }
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



        /* Auto */
        public AutoMode AutoMode { get; protected set; } = AutoMode.ModeNone;
        public AutoState AutoState { get; protected set; } = AutoState.AutoIdle;
        public class AutoInfo
        {
            List<Vector2> _path = new List<Vector2>();
            public List<Vector2> Path
            {
                get { return _path; }
                set
                {
                    _path = value;
                    PathIndex = 1;
                }
            }
            public int PathIndex = 1;
            public GameObject Target = null;
            public Vector2Int PrevTargetCell = new Vector2Int(0, 0);
            public float TargetDistance = 0;

            public SkillUseInfo SkillUse = null;

            public int WaitUntil = 0;
            public AutoState NextState;
        }
        public AutoInfo Auto { get; set; } = new AutoInfo();




        // 생성자
        public GameObject()
        {
            Info.PosInfo = PosInfo;
            Info.StatInfo = Stat;
        }

        // init
        public virtual void Init()
        {
            Info.ObjectId = GenerateId(ObjectType);
        }






        // ToString
        public override string ToString()
        {
            return ToString(InfoLevel.Identity);
        }
        public virtual string ToString(InfoLevel infoType)
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
            if (AutoMode == AutoMode.ModeNone)
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
            else if(AutoMode == AutoMode.ModeAuto)
            {
                switch (AutoState)
                {
                    case AutoState.AutoIdle:
                        UpdateAutoIdle();
                        break;
                    case AutoState.AutoChasing:
                        UpdateAutoChasing();
                        break;
                    case AutoState.AutoMoving:
                        UpdateAutoMoving();
                        break;
                    case AutoState.AutoSkill:
                        UpdateAutoSkill();
                        break;
                    case AutoState.AutoDead:
                        UpdateAutoDead();
                        break;
                    case AutoState.AutoWait:
                        UpdateAutoWait();
                        break;
                }
            }

            UpdateSkill();
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



        protected virtual void UpdateAutoIdle()
        {
        }

        protected virtual void UpdateAutoChasing()
        {
        }

        protected virtual void UpdateAutoMoving()
        {
        }

        protected virtual void UpdateAutoSkill()
        {
        }

        protected virtual void UpdateAutoDead()
        {
        }

        protected virtual void UpdateAutoWait()
        {
            int tick = Environment.TickCount;
            if(tick > Auto.WaitUntil)
            {
                AutoState = Auto.NextState;

                ServerCore.Logger.WriteLog(LogLevel.Debug, $"GameObject.UpdateAutoWait. nextState:{AutoState}, {this}");

                return;
            }

            
        }

        // 현재 방향에 해당하는 벡터 얻기
        public Vector2 GetDirectionVector(MoveDir dir)
        {
            return Util.GetDirectionVector(dir);
        }


        // 피격됨
        // 리턴값 : 실제로 받은 데미지
        public virtual int OnDamaged(GameObject attacker, int damage)
        {
            if (Room == null)
            {
                Logger.WriteLog(LogLevel.Error, $"GameObject.OnDamaged. Room is null. objectId:{Id}");
                return 0;
            }

            // HP 감소
            int finalDamage = Math.Max(damage - Stat.Defence, 1);
            Stat.Hp = Math.Max(Stat.Hp - finalDamage, 0);

            Logger.WriteLog(LogLevel.Debug, $"GameObject.OnDamaged. damage:{finalDamage}, me:[{this.ToString(InfoLevel.Stat)}], attacker:[{attacker.ToString(InfoLevel.Stat)}]");

            // HP 변경 패킷 전송
            S_ChangeHp changePacket = new S_ChangeHp();
            changePacket.ObjectId = Id;
            changePacket.Hp = Stat.Hp;
            changePacket.Amount = finalDamage;
            changePacket.ChangeType = StatChangeType.ChangeNegative;
            Room._broadcast(changePacket);

            // 사망처리
            if(Stat.Hp <= 0 && State != CreatureState.Dead)
            {
                OnDead(attacker);
            }

            return finalDamage;
        }

        // 사망함
        public virtual void OnDead(GameObject attacker)
        {
            // 상태 변경
            State = CreatureState.Dead;


            // 사망 패킷 전송
            S_Die diePacket = new S_Die();
            diePacket.ObjectId = Id;
            diePacket.AttackerId = attacker.Id;
            Room._broadcast(diePacket);

            Logger.WriteLog(LogLevel.Debug, $"GameObject.OnDead. me:[{this.ToString(InfoLevel.Stat)}], attacker:[{attacker.ToString(InfoLevel.Stat)}]");
        }


        // Set Auto
        public bool SetAutoMode(AutoMode autoMode)
        {
            if (AutoMode == autoMode)
                return false;

            AutoMode = autoMode;
            if (autoMode == AutoMode.ModeNone)
            {
                State = CreatureState.Idle;
            }
            else if (autoMode == AutoMode.ModeAuto)
            {
                Speed = 7f;
                AutoState = AutoState.AutoIdle;
            }

            return true;
        }
    }
}
