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
    protected Animator _animator;

    HpBar _hpBar;
    DebugText _debugText;


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
            UpdateHpBar();
        }
    }



    /* 스킬 */
    // 사용가능한 스킬정보
    Dictionary<SkillId, SkillUseInfo> _skillset = new Dictionary<SkillId, SkillUseInfo>();
    public Dictionary<SkillId, SkillUseInfo> Skillset { get { return _skillset; } }


    /* Auto */
    public class AutoInfo
    {
        List<Vector2> _path = new List<Vector2>();
        public List<Vector2> Path
        {
            get { return _path; }
            set 
            { 
                _path = value;
                PathIndex = 0;
            }
        }
        public int PathIndex = 0;
        public BaseController Target = null;
        public float TargetDistance = 0;
        public Vector2Int PrevTargetCell = new Vector2Int(0, 0);

        public S_AutoSkill SkillPacket = null;

        public int WaitUntil = 0;
        public AutoState NextState;
    }
    public AutoInfo Auto { get; private set; } = new AutoInfo();



    public void Awake()
    {
        _animator = GetComponent<Animator>();
    }


    protected override void Init()
    {
        base.Init();

        AddHpBar();
        AddDebugText();
        UpdateAnimation();
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
                Vector2 stopPos;
                if (Managers.Map.TryStop(this, Dest, out stopPos))
                {
                    Pos = stopPos;
                    Dest = stopPos;
                }
                else
                {
                    Dest = Pos;
                }
                State = CreatureState.Idle;
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










    public void SetAutoChase(S_AutoChase chasePacket)
    {
        if (AutoMode != AutoMode.ModeAuto)
            AutoMode = AutoMode.ModeAuto;
        Speed = 7f;

        // 타겟 찾기
        Auto.Target = Managers.Object.FindById(chasePacket.TargetId);

        // 타겟이 지정되면 경로를 찾고 상태를 Chasing으로 변경
        if (Auto.Target != null)
        {
            // 경로 찾기
            Vector2 pos = new Vector2(chasePacket.PosX, chasePacket.PosY);
            Vector2 targetPos = new Vector2(chasePacket.TargetPosX, chasePacket.TargetPosY);
            Auto.PrevTargetCell = Auto.Target.Cell;
            Auto.Path = Managers.Map.SearchPath(pos, targetPos);
            Auto.TargetDistance = chasePacket.Distance;

            // 상태 변경
            AutoState = AutoState.AutoChasing;
            State = CreatureState.Moving;
        }
        // 타겟이 없다면 상태를 Idle로 변경
        else
        {
            AutoState = AutoState.AutoIdle;
            State = CreatureState.Idle;
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"CreatureController.SetAutoChase. me:{Id}, target:[{Auto.Target?.ToString(InfoLevel.Position)}]");
    }

    public void SetAutoWait(S_AutoWait waitPacket)
    {
        Auto.WaitUntil = Environment.TickCount + waitPacket.WaitTime;
        Auto.NextState = waitPacket.NextState;

        AutoState = AutoState.AutoWait;
        State = CreatureState.Idle;
    }

    public void SetAutoSkill(S_AutoSkill skillPacket)
    {
        Auto.SkillPacket = skillPacket;
        AutoState = AutoState.AutoSkill;
        
        OnSkill(skillPacket.SkillId);
    }


    protected override void UpdateAutoIdle()
    {

    }

    protected override void UpdateAutoChasing()
    {
        // 타겟이 없다면 Idle 상태로 돌아감
        if (Auto.Target == null || Auto.Target.State == CreatureState.Dead)
        {
            AutoState = AutoState.AutoIdle;
            State = CreatureState.Idle;
            return;
        }

        // 타겟과의 거리 확인 
        float distance = (Auto.Target.Pos - Pos).magnitude;
        // 추적거리 내라면 움직이지 않음
        if (distance < Auto.TargetDistance)
        {
            State = CreatureState.Idle;

            // 현재위치에 멈춤
            Vector2 stopPos;
            Managers.Map.TryStop(this, Pos, out stopPos);
            Pos = stopPos;
            Dest = stopPos;

            // 방향 수정
            Dir = Util.GetDirectionToDest(Pos, Auto.Target.Pos);

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"CreatureController.UpdateAutoChasing. Stop. {this.ToString(InfoLevel.Position)}");
        }
        // 추적거리 밖이면 움직임
        else
        {
            // 타겟이 cell을 이동했으면 경로 재계산
            if (Auto.PrevTargetCell != Auto.Target.Cell)
            {
                Auto.PrevTargetCell = Auto.Target.Cell;
                Auto.Path = Managers.Map.SearchPath(Pos, Auto.Target.Pos);
            }

            // 경로를 따라 이동함
            State = CreatureState.Moving;
            Auto.PathIndex = AutoMoveToPath(Auto.Path, Auto.PathIndex);
        }
    }


    int AutoMoveToPath(List<Vector2> path, int pathIndex)
    {
        if (pathIndex < 0 || pathIndex >= path.Count)
            return pathIndex;
        State = CreatureState.Moving;

        // 목적지 지정
        Dest = path[pathIndex];

        // 목적지에 도달했다면 현재위치를 목적지로 이동시킴
        Vector2 diff = (Dest - Pos);
        Vector2 dir = diff.normalized;
        float magnitude = diff.magnitude;
        float moveDist = Time.deltaTime * Speed;
        if (magnitude <= moveDist)
        {
            Managers.Map.TryMoving(this, Dest, checkCollider: false);
            Pos = Dest;

            // 만약 다음 경로가 있을 경우 목적지를 재지정함
            pathIndex++;
            if (pathIndex < path.Count)
            {
                Dest = path[pathIndex];

                // 남은 이동거리를 보정함
                {
                    float remainder = moveDist - magnitude;
                    Vector2 pos = Pos + (Dest - Pos).normalized * remainder;
                    Managers.Map.TryMoving(this, pos, checkCollider: false);
                    Pos = pos;
                }
            }
            // 다음 경로가 없을 경우 현재위치에 정지함
            else
            {
                Vector2 stopPos;
                Managers.Map.TryStop(this, Pos, out stopPos);
                Pos = stopPos;
                Dest = stopPos;
                State = CreatureState.Idle;
            }
        }
        // 현재위치 이동
        else
        {
            Vector2 pos = Pos + dir * moveDist;
            Managers.Map.TryMoving(this, pos, checkCollider: false);
            Pos = pos;
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"CreatureController.AutoMoveToPath. {this.ToString(InfoLevel.Position)}");
        return pathIndex;
    }

    protected override void UpdateAutoSkill()
    {

    }

    protected override void UpdateAutoWait()
    {
        int tick = Environment.TickCount;
        if (tick > Auto.WaitUntil)
        {
            AutoState = Auto.NextState;
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

        //// effect 생성
        //GameObject effect = Managers.Resource.Instantiate("Effect/DieEffect");
        //effect.transform.position = transform.position;
        //effect.GetComponent<Animator>().Play("START");
        //GameObject.Destroy(effect, 0.5f);
    }

    // 위치에 멈춤
    public void StopAt(Vector2 pos)
    {
        Vector2 stopPos;
        Managers.Map.TryStop(this, pos, out stopPos);
        Pos = stopPos;
        Dest = stopPos;
        State = CreatureState.Idle;
    }


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
        _hpBar.transform.localScale = gameObject.transform.localScale;
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
        _debugText.transform.localScale = gameObject.transform.localScale;
    }
}
