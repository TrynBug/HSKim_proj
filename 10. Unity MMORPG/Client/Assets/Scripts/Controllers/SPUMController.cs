using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class SPUMController : CreatureController
{
    protected override void UpdateAnimation()
    {
        switch (State)
        {
            case CreatureState.Idle:
                _animator.SetFloat("RunState", 0);
                break;
            case CreatureState.Moving:
                _animator.SetFloat("RunState", 0.5f);
                switch (LookDir)
                {
                    case LookDir.LookLeft:
                        gameObject.transform.localScale = new Vector3(1, 1, 1);
                        break;
                    case LookDir.LookRight:
                        gameObject.transform.localScale = new Vector3(-1, 1, 1);
                        break;
                }
                break;
            case CreatureState.Dead:
                _animator.SetTrigger("Die");
                break;
        }
    }

    protected override void UpdateSkillAnimation()
    {
        _animator.SetTrigger("Attack");
    }
}
