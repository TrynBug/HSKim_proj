using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;
using Data;
using UnityEditor;
using System;

// 모든 컨트롤러의 부모 컨트롤러
// 상태와 이동방향을 정의한다.
public class BaseController : MonoBehaviour
{
    // bits : [ Unused(1) | Type(7) | Id(24) ]
    public int Id { get { return Info.ObjectId; } }
    public GameObjectType ObjectType { get; protected set; } = GameObjectType.None;

    public ObjectInfo Info { get; set; } = new ObjectInfo();


    /* 위치 */
    PositionInfo _posInfo = new PositionInfo { State = CreatureState.Idle, MoveDir = MoveDir.Left, LookDir=LookDir.LookLeft, PosX = 0, PosY = 0, DestX = 0, DestY = 0, MoveKeyDown = false };
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



    /* Auto */
    AutoMode _autoMode = AutoMode.ModeNone;
    public AutoMode AutoMode 
    {
        get { return _autoMode; }
        set
        {
            _autoMode = value;
            if (_autoMode == AutoMode.ModeNone)
                State = CreatureState.Idle;
            else if (_autoMode == AutoMode.ModeAuto)
                AutoState = AutoState.AutoIdle;
        }
    }
    public AutoState AutoState { get; protected set; } = AutoState.AutoIdle;



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

    // update
    protected virtual void UpdateController()
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
        else if (AutoMode == AutoMode.ModeAuto)
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
            case InfoLevel.All:
                return $"id:{Id}, type:{ObjectType}, room:{Managers.Map.MapId}, pos:{Pos}, dest:{Dest}, cell:{Cell}, state:{State}, dir:{Dir}";
            default:
                return $"id:{Id}, type:{ObjectType}, room:{Managers.Map.MapId}";
        }
    }
}
