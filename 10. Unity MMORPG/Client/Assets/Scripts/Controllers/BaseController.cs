using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;
using Data;

// 모든 컨트롤러의 부모 컨트롤러
// 상태와 이동방향을 정의한다.
public class BaseController : MonoBehaviour
{
    // bits : [ Unused(1) | Type(7) | Id(24) ]
    public int Id { get; set; }
    public GameObjectType ObjectType { get; protected set; } = GameObjectType.None;

    /* 스탯 */
    StatInfo _stat = new StatInfo();
    public StatInfo Stat
    {
        get { return _stat; }
        set
        {
            if (_stat.Equals(value))
                return;
            Hp = value.Hp;
            _stat.MaxHp = value.MaxHp;
            Speed = value.Speed;
        }
    }

    public virtual float Speed
    {
        get { return Stat.Speed; }
        set { Stat.Speed = value; }
    }

    public virtual int Hp
    {
        get { return Stat.Hp; }
        set
        {
            Stat.Hp = value;
        }
    }


    /* 위치 */
    PositionInfo _posInfo = new PositionInfo { State = CreatureState.Idle, MoveDir = MoveDir.Left, LookDir=LookDir.LookLeft, PosX = 0, PosY = 0, DestX = 0, DestY = 0, MoveKeyDown = false };
    public PositionInfo PosInfo
    {
        get { return _posInfo; }
        set
        {
            if (_posInfo.Equals(value))
                return;
            
            Pos = new Vector2(value.PosX, value.PosY);
            Dest = new Vector2(value.DestX, value.DestY);
            State = value.State;
            Dir = value.MoveDir;
            LookDir = value.LookDir;
        }
    }

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
            Cell = Managers.Map.PosToCell(Pos);
            gameObject.transform.position = Managers.Map.ServerPosToClientPos(Pos);
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
            switch(PosInfo.MoveDir)
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



    /* 스킬 */
    // 사용가능한 스킬정보
    Dictionary<int, SkillInfo> _skillset = new Dictionary<int, SkillInfo>();
    public Dictionary<int, SkillInfo> Skillset { get { return _skillset; } }

    // 스킬 키 눌림
    public virtual bool SkillKeyDown { get; set; } = false;
    


    // 현재 방향에 해당하는 벡터 얻기
    public Vector2 GetDirectionVector(MoveDir dir)
    {
        return Util.GetDirectionVector(dir);
    }






    public void Start()
    {
        Init();
    }

    void Update()
    {
        UpdateController();
    }

    protected virtual void Init()
    {
    }

    protected virtual void UpdateController()
    {
        switch (State)
        {
            case CreatureState.Idle:
                UpdateIdle();
                break;
            case CreatureState.Moving:
                UpdateMoving();     // 이동 목적지까지 캐릭터를 이동시킨다.
                break;
            case CreatureState.Dead:
                UpdateDead();
                break;
        }
    }


    // Idle 상태 업데이트
    protected virtual void UpdateIdle()
    {

    }

    // Moving 상태 업데이트
    // 이동 목적지까지 캐릭터를 이동시킨다.
    protected virtual void UpdateMoving()
    {

    }

    protected virtual void UpdateDead()
    {

    }



    // ToString
    public override string ToString()
    {
        return ToString(InfoLevel.Identity);
    }
    public virtual string ToString(InfoLevel level)
    {
        switch (level)
        {
            case InfoLevel.Identity:
                return $"id:{Id}, type:{ObjectType}, room:{Managers.Map.MapId}";
            case InfoLevel.Position:
                return $"id:{Id}, type:{ObjectType}, room:{Managers.Map.MapId}, pos:{Pos}, dest:{Dest}, cell:{Cell}, state:{State}, dir:{Dir}";
            case InfoLevel.Stat:
                return $"id:{Id}, type:{ObjectType}, room:{Managers.Map.MapId}, speed:{Speed}, hp:{Hp}";
            case InfoLevel.All:
                return $"id:{Id}, type:{ObjectType}, room:{Managers.Map.MapId}, pos:{Pos}, dest:{Dest}, cell:{Cell}, state:{State}, dir:{Dir}, speed:{Speed}, hp:{Hp}";
            default:
                return $"id:{Id}, type:{ObjectType}, room:{Managers.Map.MapId}";
        }
    }
}
