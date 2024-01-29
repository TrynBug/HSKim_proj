using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;
using Data;
using ServerCore;
using System;


// 움직이고 HP가 있는 오브젝트에 대한 기본 컨트롤러
// HP Bar, 피격 함수, 사망 함수가 있다.
public abstract class CreatureController : BaseController
{
    /* object */
    public override ObjectInfo Info
    {
        get { return base.Info; }
        set
        {
            base.Info = value;
            Stat = value.StatInfo;
            AutoInfo = value.AutoInfo;
        }
    }


    /* 위치 */
    // 현재상태. 애니메이션 업데이트 추가.
    public override CreatureState State
    {
        get { return base.State; }
        set
        {
            if (base.State == value)
                return;
            base.State = value;
            UpdateAnimation();
        }
    }

    // 이동방향. 애니메이션 업데이트 추가.
    public override MoveDir Dir
    {
        get { return base.Dir; }
        set
        {
            if (base.Dir == value)
                return;
            base.Dir = value;
            UpdateAnimation();
        }
    }



    /* 스탯 */
    StatInfo _stat = new StatInfo();
    public StatInfo Stat
    {
        get { return _stat; }
        set
        {
            if (_stat == value)
                return;
            _stat.MergeFrom(value);
            Hp = value.Hp;
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
            UpdateHpBar();
        }
    }


    /* util */
    public override bool IsAlive
    {
        get
        {
            if (AutoMode == AutoMode.ModeNone)
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
            else
            {
                if (State == CreatureState.Loading)
                    return false;
                if (Auto.State == AutoState.AutoDead)
                    return false;
                return true;
            }
        }
    }
    public override bool IsDead { 
        get 
        {
            if (AutoMode == AutoMode.ModeNone && State == CreatureState.Dead)
                return true;
            else if (AutoMode == AutoMode.ModeAuto && Auto.State == AutoState.AutoDead)
                return true;
            return false;
        } 
    }




    /* 스킬 */
    // 사용가능한 스킬정보
    Dictionary<SkillId, SkillUseInfo> _skillset = new Dictionary<SkillId, SkillUseInfo>();
    public Dictionary<SkillId, SkillUseInfo> Skillset { get { return _skillset; } }



    /* Auto */
    AutoInfo _autoInfo = new AutoInfo();       // Auto 데이터
    public AutoInfo AutoInfo
    {
        get { return _autoInfo; }
        set 
        {
            if (_autoInfo == value)
                return;
            _autoInfo.MergeFrom(value);
        }
    }
    public AutoMove Auto { get; private set; } = new AutoMove();     // 자동이동 할 때 사용할 데이터


    /* component */
    protected Animator _animator = null;
    HpBar _hpBar;
    DebugText _debugText;



    public override void Init(ObjectInfo info)
    {
        if(_animator == null)
            _animator = GetComponent<Animator>();
        _info.StatInfo = _stat;
        _info.AutoInfo = _autoInfo;

        base.Init(info);
        
        Auto.Init(this);

        AddHpBar();
        AddDebugText();
        UpdateAnimation();
    }

    protected override void UpdateController()
    {
        if (State == CreatureState.Loading)
        {
            UpdateLoading();
        }
        else if (AutoMode == AutoMode.ModeNone)
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
            switch (Auto.State)
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

        // component
        UpdateDebugText();

        // soft stop
        UpdateSoftStop();
    }






    // Idle 상태 업데이트
    protected override void UpdateIdle()
    {
        // 방향키 눌림 상태이거나, Dest와 Pos가 같지 않다면 이동한다.
        if (MoveKeyDown == true || Util.Equals(Pos, Dest) == false)
        {
            Dir = Util.GetDirectionToDest(Pos, Dest);  // 목적지에 따라 방향 설정
            State = CreatureState.Moving;
            UpdateMoving();
        }
    }

    // Moving 상태 업데이트
    protected override void UpdateMoving()
    {
        // 방향키 눌림 상태인 동안은 Dest를 계속해서 이동시킨다.
        // 키보드 방향키를 누르고있지 않다면 멈출 것이기 때문에 Dest를 이동시키지 않는다.
        Vector2 intersection;
        if (MoveKeyDown == true)
        {
            Vector2 dest = Dest + GetDirectionVector(Dir) * Time.deltaTime * Speed;
            if (Managers.Map.CanGo(Dest, dest, out intersection))
            {
                Dest = dest;
            }
            else
            {
                Dest = intersection;
            }
        }


        // 위치 이동
        Vector2 diff = (Dest - Pos);
        Vector2 dir = diff.normalized;
        Vector2 pos = Pos + dir * Time.deltaTime * Speed;

        // Dest에 도착시 현재위치를 Dest로 변경한다.
        // 만약 이동키가 눌려져있지 않다면 현재 위치에 stop 하고 상태를 idle로 바꾼다.
        if (diff.magnitude <= Time.deltaTime * Speed)
        {
            if (MoveKeyDown)
            {
                if (Managers.Map.TryMoving(this, Dest))
                {
                    Pos = Dest;
                }
                else
                {
                    Dest = Pos;
                }
            }
            else
            {
                StopAt(Dest);
            }
        }
        // 위치 이동
        else if (Managers.Map.CanGo(Pos, pos, out intersection))
        {
            if (Managers.Map.TryMoving(this, pos))
            {
                Pos = pos;
            }
            else
            {
                Dest = Pos;
            }
        }
        // 이동중 부딪혔을 경우 더이상 이동할 수 없기 때문에 Dest를 변경한다.
        else
        {
            if (Managers.Map.TryMoving(this, intersection))
            {
                Pos = intersection;
                Dest = intersection;
            }
            else
            {
                Dest = Pos;
            }
        }


        ServerCore.Logger.WriteLog(LogLevel.Debug, $"CreatureController.UpdateMoving. {this.ToString(InfoLevel.Position)}");
    }










    // 서버에서 받은 AutoMove 패킷을 세팅한다.
    public void SetAutoMove(S_AutoMove autoPacket)
    {
        SetAutoMove(autoPacket.AutoInfo, autoPacket.PosInfo);
    }
    public void SetAutoMove(AutoInfo autoInfo, PositionInfo posInfo)
    { 
        if (AutoMode != AutoMode.ModeAuto)
            AutoMode = AutoMode.ModeAuto;

        // set data
        AutoInfo = autoInfo;
        Dir = posInfo.MoveDir;
        State = posInfo.State;

        // 타겟 찾기
        Auto.Target = Managers.Object.FindById(autoInfo.TargetId) as CreatureController;
        // 스킬 찾기
        Auto.Skill = Managers.Data.SkillDict.GetValueOrDefault(autoInfo.SkillId, null);

        switch (autoInfo.AutoState)
        {
            case AutoState.AutoIdle:
                {
                    Auto.State = AutoState.AutoIdle;
                }
                break;
            case AutoState.AutoChasing:
                {
                    // 타겟이 있다면 경로를 찾고 상태를 Chasing으로 변경
                    if (Auto.Target != null)
                    {
                        // 경로 찾기
                        // 길찾기의 시작좌표는 서버의 현재 Dest, 도착좌표는 서버에서 보내준 타겟 Pos 이다.
                        // 서버와 클라가 항상 동일한 경로를 찾기 위함임
                        Auto.SetPath(new Vector2(posInfo.DestX, posInfo.DestY), new Vector2(autoInfo.TargetPosX, autoInfo.TargetPosY));

                        // chasing 상태로 변경
                        Auto.State = AutoState.AutoChasing;
                    }
                    // 타겟이 없다면 상태를 Idle로 변경
                    else
                    {
                        Auto.State = AutoState.AutoIdle;
                    }
                }
                break;
            case AutoState.AutoMoving:
                {
                    Auto.State = AutoState.AutoMoving;

                    // 경로 지정
                    Auto.SetPath(new Vector2(posInfo.DestX, posInfo.DestY), new Vector2(autoInfo.TargetPosX, autoInfo.TargetPosY));
                }
                break;
            case AutoState.AutoSkill:
                {
                    // 현재위치에 멈춤
                    StopAt(Pos);

                    // 방향 수정
                    Dir = Util.GetDirectionToDest(Pos, Auto.Target.Pos);

                    // 스킬 사용
                    if(Auto.Skill != null)
                        OnSkill(Auto.Skill.id);

                    // skill 상태로 변경
                    Auto.State = AutoState.AutoSkill;
                }
                break;
            case AutoState.AutoWait:
                {
                    Auto.State = AutoState.AutoWait;
                }
                break;
            case AutoState.AutoDead:
                {
                    Auto.State = AutoState.AutoDead;
                }
                break;
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"CreatureController.SetAutoMove. me:{Id}, target:[{Auto.Target?.ToString(InfoLevel.Position)}]");
    }



    protected virtual void UpdateAutoIdle()
    {
    }

    protected virtual void UpdateAutoChasing()
    {
        // 타겟이 없다면 Idle 상태로 돌아감
        if (Auto.Target == null || Auto.Target.IsAlive == false)
        {
            StopAt(Pos);

            Auto.WaitTime = 1000;
            Auto.NextState = AutoState.AutoIdle;
            Auto.State = AutoState.AutoWait;
            Auto.Target = null;

            return;
        }

        // 타겟과의 거리 확인 
        Vector2 dist = Auto.Target.Pos - Pos;
        Vector2 distAbs = new Vector2(Mathf.Abs(dist.x), Mathf.Abs(dist.y));
        LookDir lookToTarget = Util.GetLookToTarget(Pos, Auto.Target.Pos);
        // 추적범위 내에 있고 동시에 스킬범위 내에 있으면 움직이지 않음
        if (distAbs.x < Auto.TargetDistance.x && distAbs.y < Auto.TargetDistance.y
            && Util.IsTargetInRectRange(Pos, lookToTarget, new Vector2(Auto.Skill.rangeX, Auto.Skill.rangeY), Auto.Target.Pos))
        {
            // 현재위치에 멈춤
            StopAt(Pos);

            // 방향 수정
            Dir = Util.GetDirectionToDest(Pos, Auto.Target.Pos);

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"CreatureController.UpdateAutoChasing. Stop. {this.ToString(InfoLevel.Position)}");
        }
        // 추적범위 밖이면 움직임
        else
        {
            // 타겟이 cell을 이동했으면 경로 재계산
            if (Auto.PrevTargetCell != Auto.Target.Cell)
            {
                Auto.SetPath(Pos, Auto.Target.Pos);
            }

            // 경로를 따라 이동함
            State = CreatureState.Moving;
            Auto.MoveThroughPath();
        }
    }

    protected virtual void UpdateAutoMoving()
    {
        Auto.MoveThroughPath();
    }

    protected virtual void UpdateAutoSkill()
    {
        
    }

    protected virtual void UpdateAutoDead()
    {

    }


    protected virtual void UpdateAutoWait()
    {
        long tick = Managers.Time.CurrentTime;
        if (tick > Auto.WaitUntil)
        {
            Auto.State = Auto.NextState;

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"CreatureController.UpdateAutoWait. nextState:{Auto.State}, {this}");
        }
    }



    // 상태에 따라 애니메이션 업데이트
    protected virtual void UpdateAnimation()
    {
    }

    protected virtual void UpdateSkillAnimation()
    {
    }


    // 스킬 사용됨
    public virtual void OnSkill(SkillId skillId)
    {
        UpdateSkillAnimation();
    }



    // 피격됨
    public virtual void OnDamaged(CreatureController attacker, int damage)
    {
        // 대미지 숫자 생성
        Managers.Number.Spawn(NumberType.Damage, transform.position + Vector3.up, damage);

        ServerCore.Logger.WriteLog(ServerCore.LogLevel.Debug, $"CreatureController.OnDamaged. damage:{damage}, me:[{this}], attacker:[{attacker}]");
    }
    public virtual void OnDamaged(int damage)
    {
        OnDamaged(null, damage);
    }


    // 사망함
    public virtual void OnDead()
    {
        State = CreatureState.Dead;
        Auto.State = AutoState.AutoDead;

        //// effect 생성
        //GameObject effect = Managers.Resource.Instantiate("Effect/DieEffect");
        //effect.transform.position = transform.position;
        //effect.GetComponent<Animator>().Play("START");
        //GameObject.Destroy(effect, 0.5f);
    }

    /* compoment */
    // HP Bar 추가
    protected void AddHpBar()
    {
        GameObject go = Managers.Resource.Instantiate("UI/HpBar", transform);
        go.transform.localPosition = new Vector3(0, 1.0f, 0);
        go.name = "HpBar";
        _hpBar = go.GetComponent<HpBar>();
        UpdateHpBar();
    }

    // HP Bar 업데이트
    protected void UpdateHpBar()
    {
        if (_hpBar == null)
            return;
        float ratio = 0.0f;
        if(Stat.MaxHp > 0)
        {
            ratio = (float)Hp / (float)Stat.MaxHp;
        }
        _hpBar.SetHpBar(ratio);
        //_hpBar.transform.localScale = gameObject.transform.localScale;
    }

    // Debug Text 추가
    protected void AddDebugText()
    {
        GameObject go = Managers.Resource.Instantiate("UI/DebugText", transform);
        go.transform.localPosition = new Vector3(0, 1.2f, 0);
        go.name = "DebugText";
        _debugText = go.GetComponent<DebugText>();
        UpdateDebugText();
    }

    // Debug Text 업데이트
    protected void UpdateDebugText()
    {
        if (_debugText == null)
            return;
        _debugText.SetText($"({Pos.x:f1},{Pos.y:f1}) ({Cell.x},{Cell.y})");
        //_debugText.transform.localScale = gameObject.transform.localScale;
    }
}
