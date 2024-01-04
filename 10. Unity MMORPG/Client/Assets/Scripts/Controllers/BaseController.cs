using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;
using Data;

// 모든 컨트롤러의 부모 컨트롤러
// 상태와 이동방향을 정의하고, 상태와 이동방향에 따라 오브젝트를 이동시키고 애니메이션을 재생한다.
public class BaseController : MonoBehaviour
{
    public int Id { get; set; }
    public GameObjectType ObjectType { get; protected set; } = GameObjectType.None;

    // 스탯 정보
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


    // 위치 정보
    PositionInfo _posInfo = new PositionInfo { State = CreatureState.Idle, MoveDir = MoveDir.Down, PosX = 0, PosY = 0, DestX = 0, DestY = 0 };
    public PositionInfo PosInfo
    {
        get { return _posInfo; }
        set
        {
            if (_posInfo.Equals(value))
                return;
            
            Pos = new Vector3(value.PosX, value.PosY, Config.ObjectDefaultZ);
            Dest = new Vector3(value.DestX, value.DestY, Config.ObjectDefaultZ);
            State = value.State;
            Dir = value.MoveDir;
        }
    }

    public Vector3 Pos
    {
        get
        {
            return new Vector3(PosInfo.PosX, PosInfo.PosY, Config.ObjectDefaultZ);
        }
        set
        {
            PosInfo.PosX = value.x;
            PosInfo.PosY = value.y;
            Cell = Util.PosToCell(Pos);
            gameObject.transform.position = Managers.Map.ServerPosToClientPos(Pos);
        }
    }
    public Vector3 Dest { get; protected set; }      // 목적지
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
            UpdateAnimation();
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
            UpdateAnimation();
        }
    }


    // 바라보는 방향 앞의 Cell 좌표 얻기
    public Vector2Int GetFrontCell()
    {
        Vector2Int cell = Cell;

        switch (Dir)
        {
            case MoveDir.Up:
                cell += Vector2Int.up;
                break;
            case MoveDir.Down:
                cell += Vector2Int.down;
                break;
            case MoveDir.Left:
                cell += Vector2Int.left;
                break;
            case MoveDir.Right:
                cell += Vector2Int.right;
                break;
        }

        return cell;
    }

    // 내 위치를 기준으로 했을 때 dir이 어느 방향에 있는지 알아냄
    public MoveDir GetDirFromVec(Vector2Int dir)
    {
        Vector2Int diff = dir - Cell;
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
        UpdateAnimation();
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

    protected virtual void MoveToNextPos()
    {
    }


    protected virtual void UpdateSkill()
    {

    }

    protected virtual void UpdateDead()
    {

    }



    // 상태에 따라 애니메이션 업데이트
    protected virtual void UpdateAnimation()
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
