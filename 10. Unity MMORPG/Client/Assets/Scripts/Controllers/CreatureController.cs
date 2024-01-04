using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;

// 움직이고 HP가 있는 오브젝트에 대한 기본 컨트롤러
// HP Bar, 피격 함수, 사망 함수가 있다.
public abstract class CreatureController : BaseController
{
    protected Animator _animator;

    HpBar _hpBar;

    public override int Hp
    {
        get { return base.Hp; }
        set 
        {
            base.Hp = value;
            UpdateHpBar();
        }
    }

    private void Awake()
    {
        _animator = GetComponent<Animator>();
    }

    protected override void Init()
    {
        base.Init();
        AddHpBar();
    }

    // 피격됨
    public virtual void OnDamaged()
    {

    }

    // 사망함
    public virtual void OnDead()
    {
        State = CreatureState.Dead;

        // effect 생성
        GameObject effect = Managers.Resource.Instantiate("Effect/DieEffect");
        effect.transform.position = transform.position;
        effect.GetComponent<Animator>().Play("START");
        GameObject.Destroy(effect, 0.5f);
    }

    public virtual void UseSkill(int skillId)
    {

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
    void UpdateHpBar()
    {
        if (_hpBar == null)
            return;
        float ratio = 0.0f;
        if(Stat.MaxHp > 0)
        {
            ratio = (float)Hp / (float)Stat.MaxHp;
        }
        _hpBar.SetHpBar(ratio);
    }
}
