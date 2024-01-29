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
    public class BaseController
    {
        public GameObjectType ObjectType { get; protected set; } = GameObjectType.None;

        protected ObjectInfo _info = new ObjectInfo();
        public virtual ObjectInfo Info
        {
            get { return _info; }
            set
            {
                _info.ObjectId = value.ObjectId;
                _info.Name = value.Name;
                _info.SPUMId = value.SPUMId;
                _info.AutoMode = value.AutoMode;
                PosInfo = value.PosInfo;
            }
        }

        /* object */
        // bits : [ Unused(1) | Type(7) | Id(24) ]
        public int Id { get { return Info.ObjectId; } }
        public int SPUMId
        {
            get { return Info.SPUMId; }
            set { Info.SPUMId = value; }
        }
        public AutoMode AutoMode
        {
            get { return Info.AutoMode; }
            set { Info.AutoMode = value; }
        }


        /* 위치 */
        PositionInfo _posInfo = new PositionInfo();
        public PositionInfo PosInfo
        {
            get { return _posInfo; }
            set
            {
                Pos = new Vector2(value.PosX, value.PosY);
                Dest = new Vector2(value.DestX, value.DestY);
                State = value.State;
                Dir = value.MoveDir;
                LookDir = value.LookDir;
                MoveKeyDown = value.MoveKeyDown;
            }
        }

        public virtual Vector2 Pos
        {
            get
            {
                return new Vector2(PosInfo.PosX, PosInfo.PosY);
            }
            set
            {
                PosInfo.PosX = value.x;
                PosInfo.PosY = value.y;
                //Cell = Managers.Map.PosToCell(Pos);
            }
        }
        public Vector2 Dest    // 목적지
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
        public Vector2Int Cell { get; private set; }   // 현재 cell
        public int RoomId { get; set; }


        // 현재 상태
        public virtual CreatureState State
        {
            get { return PosInfo.State; }
            set
            {
                if (PosInfo.State == value)
                    return;
                PosInfo.State = value;
            }
        }

        // 이동 방향
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

        // 보는 방향
        public virtual LookDir LookDir
        {
            get { return PosInfo.LookDir; }
            set { PosInfo.LookDir = value; }
        }

        // 이동 키 눌림
        public virtual bool MoveKeyDown
        {
            get { return PosInfo.MoveKeyDown; }
            set { PosInfo.MoveKeyDown = value; }
        }


        /* util */
        public virtual bool IsAlive
        {
            get
            {
                switch (State)
                {
                    case CreatureState.Dead:
                    case CreatureState.Loading:
                        return false;
                    default:
                        return true;
                }
            }
        }
        public virtual bool IsDead { get { return State == CreatureState.Dead; } }
        public bool IsLoading { get { return State == CreatureState.Loading; } }

        /* init */
        // Init 은 객체를 생성한 뒤 1가지 Init 함수만 호출하면 됨
        public virtual void Init()
        {
            _info.PosInfo = _posInfo;
            PosInfo = new PositionInfo { State = CreatureState.Idle, MoveDir = MoveDir.Left, LookDir = LookDir.LookLeft, PosX = 0, PosY = 0, DestX = 0, DestY = 0, MoveKeyDown = false };
        }

        public virtual void Init(ObjectInfo info)
        {
            _info.PosInfo = _posInfo;

            Info = info;
        }



        /* update */
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
                case CreatureState.Loading:
                    UpdateLoading();
                    break;
            }
        }


        // Idle 상태 업데이트
        protected virtual void UpdateIdle()
        {

        }

        // Moving 상태 업데이트
        protected virtual void UpdateMoving()
        {

        }

        // Dead 상태 업데이트
        protected virtual void UpdateDead()
        {

        }

        protected virtual void UpdateLoading()
        {
        }


        /* ToString */
        public override string ToString()
        {
            return ToString(InfoLevel.Identity);
        }
        public virtual string ToString(InfoLevel level)
        {
            switch (level)
            {
                case InfoLevel.Identity:
                    return $"id:{Id}, type:{ObjectType}, room:{RoomId}";
                case InfoLevel.Position:
                    return $"id:{Id}, type:{ObjectType}, room:{RoomId}, pos:{Pos}, dest:{Dest}, state:{State}, dir:{Dir}";
                case InfoLevel.All:
                    return $"id:{Id}, type:{ObjectType}, room:{RoomId}, pos:{Pos}, dest:{Dest}, state:{State}, dir:{Dir}";
                default:
                    return $"id:{Id}, type:{ObjectType}, room:{RoomId}";
            }
        }
    }
}
