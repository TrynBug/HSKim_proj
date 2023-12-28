using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ServerCore;

namespace Server.Game
{
    public class GameObject
    {
        public GameObjectType ObjectType { get; protected set; } = GameObjectType.None;
        public int Id
        {
            get { return Info.ObjectId; }
            set { Info.ObjectId = value; }
        }

        public GameRoom Room { get; set; } = null;
        public ObjectInfo Info { get; set; } = new ObjectInfo();   // Info.PosInfo 는 PosInfo를 리턴함
        public PositionInfo PosInfo { get; private set; } = new PositionInfo();
        public StatInfo Stat { get; private set; } = new StatInfo();  // Info.StatInfo 는 Stat을 리턴함

        public Vector2Int CellPos
        {
            get
            {
                return new Vector2Int(PosInfo.PosX, PosInfo.PosY);
            }
            set
            {
                PosInfo.PosX = value.x;
                PosInfo.PosY = value.y;
            }
        }

        public float Speed
        {
            get { return Stat.Speed; }
            set { Stat.Speed = value; }
        }

        public CreatureState State
        {
            get { return PosInfo.State; }
            set { PosInfo.State = value; }
        }

        public MoveDir Dir
        {
            get { return PosInfo.MoveDir; }
            set { PosInfo.MoveDir = value; }
        }

        public GameObject()
        {
            Info.PosInfo = PosInfo;
            Info.StatInfo = Stat;
        }

        public int Hp
        {
            get { return Stat.Hp; }
            set { Stat.Hp = Math.Clamp(value, 0, Stat.MaxHp); }
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
                case CreatureState.Skill:
                    UpdateSkill();
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

        protected virtual void UpdateSkill()
        {
        }

        protected virtual void UpdateDead()
        {
        }


        //  특정 방향 앞의 Cell 좌표 얻기
        public Vector2Int GetFrontCellPos(MoveDir dir)
        {
            Vector2Int cellPos = CellPos;

            switch (dir)
            {
                case MoveDir.Up:
                    cellPos += Vector2Int.up;
                    break;
                case MoveDir.Down:
                    cellPos += Vector2Int.down;
                    break;
                case MoveDir.Left:
                    cellPos += Vector2Int.left;
                    break;
                case MoveDir.Right:
                    cellPos += Vector2Int.right;
                    break;
            }

            return cellPos;
        }

        // 바라보는 방향의 Cell 좌표 얻기
        public Vector2Int GetFrontCellPos()
        {
            return GetFrontCellPos(PosInfo.MoveDir);
        }

        // 내 위치를 기준으로 했을 때 target이 어느 방향에 있는지 알아냄
        public MoveDir GetDirFromVec(Vector2Int target)
        {
            Vector2Int diff = target - CellPos;
            if (diff.x > 0)
                return MoveDir.Right;
            else if (diff.x < 0)
                return MoveDir.Left;
            else if (diff.y > 0)
                return MoveDir.Up;
            else if (diff.y < 0)
                return MoveDir.Down;
            else
                return MoveDir.Down;
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

            Logger.WriteLog(LogLevel.Debug, $"GameObject.OnDamaged. objectId:{Id}, damage:{damage}, HP:{Stat.Hp}");

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
            Logger.WriteLog(LogLevel.Debug, $"GameObject.OnDead. objectId:{Id}, attackerId:{attacker.Id}");

            // 사망 패킷 전송
            S_Die diePacket = new S_Die();
            diePacket.ObjectId = Id;
            diePacket.AttackerId = attacker.Id;
            Room._broadcast(diePacket);

            // 게임룸에서 내보냄
            GameRoom room = Room;
            room._leaveGame(Id);

            // 스탯 초기화후 게임룸에 재입장
            Stat.Hp = Stat.MaxHp;
            PosInfo.State = CreatureState.Idle;
            PosInfo.MoveDir = MoveDir.Down;
            PosInfo.PosX = 0;
            PosInfo.PosY = 0;
            room._enterGame(this);

        }
    }
}
