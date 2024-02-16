using Data;
using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

public class SPUMController : CreatureController
{
    // ��ų Ŭ���� ����ð� ������
    static Dictionary<string, float> _clipLengthNormal = null;
    static Dictionary<string, float> _clipLengthHorse = null;
    static object _lockClipLength = new object();

    // spum
    GameObject _unitRoot;
    SPUMData _spum;
    public GameObject UnitRoot { get { return _unitRoot; } }
    public SPUMData SPUM { get { return _spum; } }


    // animation controll
    bool _bPlayingSkill = false;

    public override void Init(ObjectInfo info)
    {
        // spum root ã��, �ִϸ��̼� ��ü
        // ���� Ÿ���������� �׷��� �������� �ִϸ��̼��� �ٸ��� ������ �� ���ο� ���� �ִϸ��̼��� ��ü���ش�.
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

        // ������Ʈ �ʱ�ȭ
        _unitRoot.transform.localPosition = Vector3.zero;
        SortingGroup sort = _unitRoot.GetOrAddComponent<SortingGroup>();
        sort.sortingOrder = 2;

        // init
        base.Init(info);

        // ù ���°� Dead �϶� �ִϸ��̼� ó��
        if(State == CreatureState.Dead)
        {
            //_animator.SetTrigger("Die");

            //_animator.speed = 100;
            //_animator.Play("3_Death".GetHashCode(), 0, 1f);
            //_animator.Update(100f);

            _animator.speed = 100;
            //_animator.Play("4_Death".GetHashCode(), 0, 1f);
            _animator.Play("4_Death", 0, 1f);
            _animator.Update(100f);

            // hp bar
            SetHpBarActive(false);
        }
    }



    protected override void UpdateAnimation()
    {
        if (StopUpdateAnimation)
            return;

        // �ִϸ��̼� ���
        // �� ���ο� ������� �ִϸ��̼� Ŭ������ �̸��� �����ϱ� ������ ���⼭�� �� ���θ� Ȯ������ �ʴ´�.
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
            // ���� ���� ��ų �ִϸ��̼��� ������� ��쿡�� ��� �ִϸ��̼��� ������ �ٸ� �ִϸ��̼��� ������� �ʴ´�.
            if(State == CreatureState.Dead)
            {
                _bPlayingSkill = false;
                _animator.Play("4_Death");
            }
        }

        // debug
        //if(AutoMode == AutoMode.ModeAuto)
        //if (_bPlayingSkill == false)
        //{
        //    switch (State)
        //    {
        //        case CreatureState.Loading:
        //        case CreatureState.Idle:
        //            switch (LookDir)
        //            {
        //                case LookDir.LookLeft:
        //                    ServerCore.Logger.WriteLog(ServerCore.LogLevel.System, $"0_idle, left");
        //                    break;
        //                case LookDir.LookRight:
        //                    ServerCore.Logger.WriteLog(ServerCore.LogLevel.System, $"0_idle, right");
        //                    break;
        //            }
        //            break;
        //        case CreatureState.Moving:
        //            switch (LookDir)
        //            {
        //                case LookDir.LookLeft:
        //                    ServerCore.Logger.WriteLog(ServerCore.LogLevel.System, $"1_Run, left");
        //                    break;
        //                case LookDir.LookRight:
        //                    ServerCore.Logger.WriteLog(ServerCore.LogLevel.System, $"1_Run, right");
        //                    break;
        //            }
        //            break;
        //        case CreatureState.Dead:
        //            ServerCore.Logger.WriteLog(ServerCore.LogLevel.System, $"4_Death");
        //            break;
        //    }
        //}
        //else
        //{
        //    if (State == CreatureState.Dead)
        //    {
        //        ServerCore.Logger.WriteLog(ServerCore.LogLevel.System, $"4_Death");
        //    }
        //}
    }

    protected override void UpdateSkillAnimation(bool heavy)
    {
        if (StopUpdateAnimation)
            return;

        // ������ ���� ��ų �ִϸ��̼��� �����Ѵ�.
        // �� ���ο� ������� �ִϸ��̼� Ŭ������ �̸��� �����ϴ�.
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


        // debug
        //ServerCore.Logger.WriteLog(ServerCore.LogLevel.System, $"2_Skill");
    }


    void UpdateAnimationAfterSkill(string clipName)
    {
        _bPlayingSkill = true;

        if (_spum.hasHorse == true)
        {
            if (_clipLengthHorse == null)
            {
                // _clipLengthHorse �� null�̸� ����
                lock (_lockClipLength)
                {
                    // double check
                    if (_clipLengthHorse == null)
                    {
                        Dictionary<string, float> clipLengthHorse = new Dictionary<string, float>();
                        AnimationClip[] clips = _animator.runtimeAnimatorController.animationClips;
                        foreach (AnimationClip clip in clips)
                            clipLengthHorse.Add(clip.name, clip.length);
                        _clipLengthHorse = clipLengthHorse;  // dictionary ����
                    }
                }
            }

            // clipTime �Ŀ� �ִϸ��̼��� ������Ʈ��
            float clipTime = _clipLengthHorse.GetValueOrDefault(clipName, 0f);
            StartCoroutine("CoUpdateAnimationAfterSkill", clipTime);
        }
        else
        {
            if (_clipLengthNormal == null)
            {
                // _clipLengthNormal �� null�̸� ����
                lock (_lockClipLength)
                {
                    // double check
                    if (_clipLengthNormal == null)
                    {
                        Dictionary<string, float> clipLengthNormal = new Dictionary<string, float>();
                        AnimationClip[] clips = _animator.runtimeAnimatorController.animationClips;
                        foreach (AnimationClip clip in clips)
                            clipLengthNormal.Add(clip.name, clip.length);
                        _clipLengthNormal = clipLengthNormal;  // dictionary ����
                    }
                }
            }

            // clipTime �Ŀ� �ִϸ��̼��� ������Ʈ��
            float clipTime = _clipLengthNormal.GetValueOrDefault(clipName, 0f);
            StartCoroutine("CoUpdateAnimationAfterSkill", clipTime);
        }

    }

    // clipTime �Ŀ� �ִϸ��̼��� ������Ʈ �Ѵ�.
    IEnumerator CoUpdateAnimationAfterSkill(float clipTime)
    {
        yield return new WaitForSeconds(clipTime);
        _bPlayingSkill = false;
        if (State != CreatureState.Dead)
            UpdateAnimation();
    }


    //public void PlaySkillAnimation(bool heavy)
    //{
    //    switch (_spum.spumClass)
    //    {
    //        case SPUMClass.SpumKnight:
    //            if (heavy)
    //                _animator.Play("5_Skill_Normal");
    //            else
    //                _animator.Play("2_Attack_Normal");
    //            break;
    //        case SPUMClass.SpumWizard:
    //            if (heavy)
    //                _animator.Play("5_Skill_Magic");
    //            else
    //                _animator.Play("2_Attack_Magic");
    //            break;
    //        case SPUMClass.SpumArcher:
    //            if (heavy)
    //                _animator.Play("5_Skill_Bow");
    //            else
    //                _animator.Play("2_Attack_Bow");
    //            break;
    //    }
    //}


    /* hp bar */
    protected override void AddHpBar()
    {
        base.AddHpBar();
        if(SPUM.hasHorse == true)
            _healthBar.transform.localPosition = new Vector3(0, 1.25f, 0);
    }


}