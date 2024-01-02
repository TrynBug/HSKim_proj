using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.EventSystems;
using Google.Protobuf.Protocol;
using static Define;

public class PlayerController : CreatureController
{
    protected Coroutine _coSkill;  // skill 사용 후 후처리(쿨타임 등)를 위한 코루틴
    protected bool _rangeSkill = true;  // 스킬 사용시 근접/원거리 여부



    protected override void Init()
    {
        base.Init();
    }

    protected override void UpdateController()
    {
        base.UpdateController();
    }

    // Idle 상태일때의 업데이트
    protected override void UpdateIdle()
    {
        base.UpdateIdle();
    }

    // 피격됨
    public override void OnDamaged()
    {
        //Debug.Log("Player HIT !");
    }

    // 스킬사용
    public override void UseSkill(int skillId)
    {
        if(skillId == 1)
        {
            _coSkill = StartCoroutine("CoStartPunch");
        }
        else if (skillId == 2)
        {
            _coSkill = StartCoroutine("CoStartShootArrow");
        }
    }


    protected virtual void CheckUpdatedFlag()
    {
    }

    // Punch 사용에 대한 코루틴
    protected IEnumerator CoStartPunch()
    {
        // 쿨타임 후 상태를 Idle로 바꾼다.
        _rangeSkill = false;
        State = CreatureState.Skill;
        yield return new WaitForSeconds(0.5f);
        State = CreatureState.Idle;
        _coSkill = null;
        CheckUpdatedFlag();
    }

    // 화살 사용에 대한 코루틴
    protected IEnumerator CoStartShootArrow()
    {
        // 쿨타임 후 상태를 Idle로 바꾼다.
        _rangeSkill = true;
        State = CreatureState.Skill;
        yield return new WaitForSeconds(0.3f);
        State = CreatureState.Idle;
        _coSkill = null;
        CheckUpdatedFlag();
    }


}
