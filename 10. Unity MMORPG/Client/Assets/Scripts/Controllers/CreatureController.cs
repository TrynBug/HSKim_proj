using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;
using Data;
using static UnityEngine.GraphicsBuffer;

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
