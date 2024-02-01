using Data;
using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.Rendering;

public class SPUMController : CreatureController
{
    // 스킬 클립별 재생시간 데이터
    static Dictionary<string, float> _clipLengthNormal = null;
    static Dictionary<string, float> _clipLengthHorse = null;
    static object _lockClipLength = new object();

    // spum
    GameObject _unitRoot;
    SPUMData _spum;
    public SPUMData SPUM { get { return _spum; } }

    // animation controll
    bool _bPlayingSkill = false;

    public override void Init(ObjectInfo info)
    {
        // spum root 찾기, 애니메이션 교체
        // 말을 타고있을때와 그렇지 않을때의 애니메이션이 다르기 때문에 말 여부에 따라 애니메이션을 교체해준다.
        _spum = Managers.Data.SPUMDict.GetValueOrDefault(info.SPUMId, null);
        if (_spum.hasHorse)
        {
            _unitRoot = Util.FindChild(gameObject, "HorseRoot");
            _animator = _unitRoot.GetComponent<Animator>();
            _animator.runtimeAnimatorController = (RuntimeAnimatorController)Resources.Load("Animations/SPUM/HorseNewController");
        }
        else
        {
            _unitRoot = Util.FindChild(gameObject, "UnitRoot");
            _animator = _unitRoot.GetComponent<Animator>();
            _animator.runtimeAnimatorController = (RuntimeAnimatorController)Resources.Load("Animations/SPUM/AnimationNewController");
        }

        // 컴포넌트 초기화
        _unitRoot.transform.localPosition = Vector3.zero;
        SortingGroup sort = _unitRoot.GetOrAddComponent<SortingGroup>();
        sort.sortingOrder = 2;

        // init
        base.Init(info);

        // 첫 상태가 Dead 일때 애니메이션 처리
        if(State == CreatureState.Dead)
        {
            //_animator.SetTrigger("Die");

            //_animator.speed = 100;
            //_animator.Play("3_Death".GetHashCode(), 0, 1f);
            //_animator.Update(100f);

            _animator.speed = 100;
            _animator.Play("4_Death".GetHashCode(), 0, 1f);
            _animator.Update(100f);
        }
    }



    protected override void UpdateAnimation()
    {
        // 애니메이션 재생
        // 말 여부에 상관없이 애니메이션 클립들의 이름은 동일하기 때문에 말 여부를 확인하지 않는다.
        if(_bPlayingSkill == false)
        {
            switch (State)
            {
                case CreatureState.Loading:
                case CreatureState.Idle:
                    _animator.Play("0_idle");
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
                    _animator.Play("1_Run");
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
                    _animator.Play("4_Death");
                    break;
            }
        }
        else
        {
            // 만약 현재 스킬 애니메이션이 재생중일 경우에는 사망 애니메이션을 제외한 다른 애니메이션을 재생하지 않는다.
            if(State == CreatureState.Dead)
            {
                _bPlayingSkill = false;
                _animator.Play("4_Death");
            }
        }
    }

    protected override void UpdateSkillAnimation(bool heavy)
    {
        // 직업에 따라 스킬 애니메이션을 실행한다.
        // 말 여부에 상관없이 애니메이션 클립들의 이름은 동일하다.
        switch (_spum.spumClass)
        {
            case SPUMClass.SpumKnight:
                if (heavy)
                {
                    _animator.Play("5_Skill_Normal");
                    UpdateAnimationAfterSkill("5_Skill_Normal");
                }
                else
                {
                    _animator.Play("2_Attack_Normal");
                    UpdateAnimationAfterSkill("2_Attack_Normal");
                }
                break;
            case SPUMClass.SpumWizard:
                if (heavy)
                {
                    _animator.Play("5_Skill_Magic");
                    UpdateAnimationAfterSkill("5_Skill_Magic");
                }
                else
                {
                    _animator.Play("2_Attack_Magic");
                    UpdateAnimationAfterSkill("2_Attack_Magic");
                }
                break;
            case SPUMClass.SpumArcher:
                if (heavy)
                {
                    _animator.Play("5_Skill_Bow");
                    UpdateAnimationAfterSkill("5_Skill_Bow");
                }
                else
                {
                    _animator.Play("2_Attack_Bow");
                    UpdateAnimationAfterSkill("2_Attack_Bow");
                }
                break;
        }
    }


    void UpdateAnimationAfterSkill(string clipName)
    {
        _bPlayingSkill = true;

        if (_spum.hasHorse == true)
        {
            if (_clipLengthHorse == null)
            {
                // _clipLengthHorse 가 null이면 생성
                lock (_lockClipLength)
                {
                    // double check
                    if (_clipLengthHorse == null)
                    {
                        Dictionary<string, float> clipLengthHorse = new Dictionary<string, float>();
                        AnimationClip[] clips = _animator.runtimeAnimatorController.animationClips;
                        foreach (AnimationClip clip in clips)
                            clipLengthHorse.Add(clip.name, clip.length);
                        _clipLengthHorse = clipLengthHorse;  // dictionary 세팅
                    }
                }
            }

            // clipTime 후에 애니메이션을 업데이트함
            float clipTime = _clipLengthHorse.GetValueOrDefault(clipName, 0f);
            StartCoroutine("CoUpdateAnimationAfterSkill", clipTime);
        }
        else
        {
            if (_clipLengthNormal == null)
            {
                // _clipLengthNormal 가 null이면 생성
                lock (_lockClipLength)
                {
                    // double check
                    if (_clipLengthNormal == null)
                    {
                        Dictionary<string, float> clipLengthNormal = new Dictionary<string, float>();
                        AnimationClip[] clips = _animator.runtimeAnimatorController.animationClips;
                        foreach (AnimationClip clip in clips)
                            clipLengthNormal.Add(clip.name, clip.length);
                        _clipLengthNormal = clipLengthNormal;  // dictionary 세팅
                    }
                }
            }

            // clipTime 후에 애니메이션을 업데이트함
            float clipTime = _clipLengthNormal.GetValueOrDefault(clipName, 0f);
            StartCoroutine("CoUpdateAnimationAfterSkill", clipTime);
        }

    }

    // clipTime 후에 애니메이션을 업데이트 한다.
    IEnumerator CoUpdateAnimationAfterSkill(float clipTime)
    {
        yield return new WaitForSeconds(clipTime);
        _bPlayingSkill = false;
        if (State != CreatureState.Dead)
            UpdateAnimation();
    }


    //protected override void UpdateAnimation()
    //{
    //    switch (State)
    //    {
    //        case CreatureState.Loading:
    //        case CreatureState.Idle:
    //            _animator.SetFloat("RunState", 0);
    //            switch (LookDir)
    //            {
    //                case LookDir.LookLeft:
    //                    _unitRoot.transform.localScale = new Vector3(1, 1, 1);
    //                    break;
    //                case LookDir.LookRight:
    //                    _unitRoot.transform.localScale = new Vector3(-1, 1, 1);
    //                    break;
    //            }
    //            break;
    //        case CreatureState.Moving:
    //            _animator.SetFloat("RunState", 0.5f);
    //            switch (LookDir)
    //            {
    //                case LookDir.LookLeft:
    //                    _unitRoot.transform.localScale = new Vector3(1, 1, 1);
    //                    break;
    //                case LookDir.LookRight:
    //                    _unitRoot.transform.localScale = new Vector3(-1, 1, 1);
    //                    break;
    //            }
    //            break;
    //        case CreatureState.Dead:
    //            _animator.SetTrigger("Die");
    //            break;
    //    }
    //}

    //protected override void UpdateSkillAnimation(string animationName)
    //{
    //    if(string.IsNullOrEmpty(animationName))
    //        _animator.SetTrigger("Attack");

    //}





}
