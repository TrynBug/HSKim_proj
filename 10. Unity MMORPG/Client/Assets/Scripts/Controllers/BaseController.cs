using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;
using Data;
using UnityEditor;
using System;
using ServerCore;

// 모든 컨트롤러의 부모 컨트롤러
// 상태와 이동방향을 정의한다.
public class BaseController : MonoBehaviour
{

    public GameObjectType ObjectType { get; protected set; } = GameObjectType.None;

    protected ObjectInfo _info = new ObjectInfo();
    public virtual ObjectInfo Info {
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
            Cell = Managers.Map.PosToCell(Pos);
            gameObject.transform.position = Managers.Map.ServerPosToClientPos(Pos);

            _bSoftStop = false;
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
    bool _bSoftStop = false;


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

    // 현재 방향에 해당하는 벡터 얻기
    public Vector2 GetDirectionVector(MoveDir dir)
    {
        return Util.GetDirectionVector(dir);
    }




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




    public void Start()
    {
    }

    void Update()
    {
        UpdateController();
    }


    /* update */
    protected virtual void UpdateController()
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

        UpdateSoftStop();
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



    /* stop */
    // 서버와 정지 위치를 동기화시킨다.
    // 만약 서버 cell과 클라이언트 cell이 동일하다면 세부적인 위치는 변경하지 않는다.
    public void SyncStop(Vector2 serverPos)
    {
        Vector2Int serverCell = Managers.Map.PosToCell(serverPos);

        // 내가 멈출 위치에 다른 오브젝트가 있는지 확인한다.
        BaseController objOther = Managers.Map.GetStopObjectAt(serverCell);
        if (objOther != null && objOther != this)
        {
            // 다른 오브젝트가 있다면 나를 해당 위치에 멈추고 다른 오브젝트는 근처에 위치시킨다.
            Managers.Map.TryMoving(objOther, objOther.Pos, checkCollider: false);
            StopAt(serverPos);
            objOther.StopAt(serverPos);
        }
        else
        {
            if(Cell == serverCell)
            {
                // 다른 오브젝트가 없고 내 cell과 서버의 cell이 동일하다면 내 위치에 멈춘다.
                StopAt(Pos);
            }
            else
            {
                // 다른 오브젝트가 없고 내 cell과 서버의 cell이 다르다면 서버 위치에 멈춘다.
                StopAt(serverPos);
            }
        }
    }

    // dest 위치에 멈춤
    // dest 위치에 멈출 수 없다면 주변 위치에 멈춘다.
    public void StopAt(Vector2 dest)
    {
        Vector2 prevPos = Pos;
        Vector2 stopPos;

        // dest 위치에 멈춘다.
        Managers.Map.TryStop(this, dest, out stopPos);
        State = CreatureState.Idle;
        Pos = stopPos;
        Dest = stopPos;

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"BaseController.StopAt. stop:{dest}, {this.ToString(InfoLevel.Position)}");

        _bSoftStop = true;
        _stopStartPos = Managers.Map.ServerPosToClientPos(prevPos);
        _stopEndPos = gameObject.transform.position;
        _totalStopTime = 0.4f;
        _stopTimeAcc = 0;
    }


    Vector2 _stopStartPos;
    Vector2 _stopEndPos;
    float _totalStopTime;
    float _stopTimeAcc;
    protected void UpdateSoftStop()
    {
        if (_bSoftStop == false)
            return;

        // 마우스 클릭에 따른 화면 위치 조정
        // 현재 카메라 위치는 시작 위치에서 도착 위치로 m_fMoveTime 초만에 도착한다.
        // 카메라 위치는 등가속도(a<0)로 이동한다.
        _stopTimeAcc += Time.deltaTime;
        if (_stopTimeAcc > _totalStopTime)
        {
            gameObject.transform.position = new Vector3(_stopEndPos.x, _stopEndPos.y, Config.ObjectDefaultZ); ;
            _bSoftStop = false;
        }
        else
        {
            Vector2 dist = _stopEndPos - _stopStartPos;  // 이동해야할 거리
            Vector2 v0 = dist * 2f / _totalStopTime;          // 시작속도
            Vector2 acc = -1f * v0 / _totalStopTime;          // 가속도
            Vector2 v = acc * _stopTimeAcc + v0;          // 현재 속도
            Vector2 pos = (v0 + v) * _stopTimeAcc / 2f + _stopStartPos;  // 현재 위치
            gameObject.transform.position = new Vector3(pos.x, pos.y, Config.ObjectDefaultZ);
        }
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
