using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;

// 모든 컨트롤러의 부모 컨트롤러
// 상태와 이동방향을 정의하고, 상태와 이동방향에 따라 오브젝트를 이동시키고 애니메이션을 재생한다.
public class BaseController : MonoBehaviour
{
    public int Id { get; set; }

    protected bool _updated = false;   // 변경사항이 있는지 여부

    [SerializeField]
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
    PositionInfo _posInfo = new PositionInfo { State = CreatureState.Idle, MoveDir = MoveDir.Down, PosX = 0, PosY = 0 };
    public PositionInfo PosInfo
    {
        get { return _posInfo; }
        set
        {
            if (_posInfo.Equals(value))
                return;
            CellPos = new Vector3Int(value.PosX, value.PosY, 0);
            State = value.State;
            Dir = value.MoveDir;
        }
    }

    // 이동할 cell 좌표
    public Vector3Int CellPos
    {
        get
        {
            return new Vector3Int(PosInfo.PosX, PosInfo.PosY, 0);
        }
        set
        {
            if (PosInfo.PosX == value.x && PosInfo.PosY == value.y)
                return;

            PosInfo.PosX = value.x;
            PosInfo.PosY = value.y;
            _updated = true;
        }
    }

    // 현재 상태
    public virtual CreatureState State
    {
        get { return PosInfo.State; }
        set
        {
            if (PosInfo.State == value)
                return;
            PosInfo.State = value;
            _updated = true;
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
            _updated = true;
            UpdateAnimation();
        }
    }


    // 바라보는 방향 앞의 Cell 좌표 얻기
    public Vector3Int GetFrontCellPos()
    {
        Vector3Int cellPos = CellPos;

        switch (Dir)
        {
            case MoveDir.Up:
                cellPos += Vector3Int.up;
                break;
            case MoveDir.Down:
                cellPos += Vector3Int.down;
                break;
            case MoveDir.Left:
                cellPos += Vector3Int.left;
                break;
            case MoveDir.Right:
                cellPos += Vector3Int.right;
                break;
        }

        return cellPos;
    }

    // 내 위치를 기준으로 했을 때 dir이 어느 방향에 있는지 알아냄
    public MoveDir GetDirFromVec(Vector3Int dir)
    {
        Vector3Int diff = dir - CellPos;
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
            case CreatureState.Skill:
                UpdateSkill();
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
        if (State != CreatureState.Moving)
            return;

        Vector3 destPos = Managers.Map.CurrentGrid.CellToWorld(CellPos) + new Vector3(0.5f, 0.5f);  // cell 좌표를 world 좌표로 변경
        Vector3 moveDir = destPos - transform.position;     // 도착 좌표와 현재 좌표의 차이 계산

        // 도착 여부를 체크
        float dist = moveDir.magnitude;
        if (dist < Speed * Time.deltaTime)
        {
            // 도착한 경우
            transform.position = destPos;
            MoveToNextPos();
        }
        else
        {
            // 아직 도착하지 않았다면 캐릭터를 이동시킨다.
            transform.position += moveDir.normalized * Speed * Time.deltaTime;
            State = CreatureState.Moving;
        }
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
        if (PosInfo.State == CreatureState.Idle)
        {
            switch (Dir)
            {
                case MoveDir.Up:

                    break;
                case MoveDir.Down:

                    break;
                case MoveDir.Left:

                    break;
                case MoveDir.Right:

                    break;
            }
        }
        else if (PosInfo.State == CreatureState.Moving)
        {
            switch (Dir)
            {
                case MoveDir.Up:

                    break;
                case MoveDir.Down:

                    break;
                case MoveDir.Left:

                    break;
                case MoveDir.Right:

                    break;
            }
        }
        else if (PosInfo.State == CreatureState.Skill)
        {
            switch (Dir)
            {
                case MoveDir.Up:

                    break;
                case MoveDir.Down:

                    break;
                case MoveDir.Left:

                    break;
                case MoveDir.Right:

                    break;
            }
        }
        else if (PosInfo.State == CreatureState.Dead)
        {

        }
    }
}
