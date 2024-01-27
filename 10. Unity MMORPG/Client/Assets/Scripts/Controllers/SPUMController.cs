using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.Rendering;

public class SPUMController : CreatureController
{
    GameObject _unitRoot;

    public override void Init(ObjectInfo info)
    {
        // 컴포넌트 초기화
        _unitRoot = Util.FindChild(gameObject, "UnitRoot");
        _animator = _unitRoot.GetComponent<Animator>();
        _unitRoot.transform.localPosition = Vector3.zero;
        SortingGroup sort = _unitRoot.GetOrAddComponent<SortingGroup>();
        sort.sortingOrder = 2;

        // init
        base.Init(info);

        // 첫 상태가 Dead 일때 애니메이션 처리
        if(State == CreatureState.Dead)
        {
            _animator.SetTrigger("Die");
            //_animator.Update(1f);
            //var state = _animator.GetCurrentAnimatorStateInfo(0);
            //_animator.speed = 0;
            //_animator.Play(state.shortNameHash, 0, 1f);
            //_animator.Update(0f);

            _animator.speed = 100;
            _animator.Play("3_Death".GetHashCode(), 0, 1f);
            _animator.Update(100f);
        }
    }

    protected override void UpdateAnimation()
    {
        switch (State)
        {
            case CreatureState.Idle:
            case CreatureState.Loading:
                _animator.SetFloat("RunState", 0);
                switch (LookDir)
                {
                    case LookDir.LookLeft:
                        _unitRoot.transform.localScale = new Vector3(1, 1, 1);
                        break;
                    case LookDir.LookRight:
                        _unitRoot.transform.localScale = new Vector3(-1, 1, 1);
                        break;
                }
                break;
            case CreatureState.Moving:
                _animator.SetFloat("RunState", 0.5f);
                switch (LookDir)
                {
                    case LookDir.LookLeft:
                        _unitRoot.transform.localScale = new Vector3(1, 1, 1);
                        break;
                    case LookDir.LookRight:
                        _unitRoot.transform.localScale = new Vector3(-1, 1, 1);
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
