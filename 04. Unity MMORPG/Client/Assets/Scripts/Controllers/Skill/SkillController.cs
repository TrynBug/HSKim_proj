using Data;
using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


public class SkillController : BaseController
{
    public static SkillController Generate(SkillId skillId)
    {
        GameObject go = Managers.Resource.Instantiate("Skill/SkillBase");
        switch (skillId)
        {
            case SkillId.SkillAttack:
                return go.GetOrAddComponent<SkillAttackController>();
            case SkillId.SkillHeavyAttack:
                return go.GetOrAddComponent<SkillHeavyAttackController>();
            case SkillId.SkillThunder:
                return go.GetOrAddComponent<SkillThunderController>();
            case SkillId.SkillMagicExplosion:
                return go.GetOrAddComponent<SkillMagicExplosionController>();
            default:
                return null;
        }
    }


    public SkillData SkillData { get; private set; }
    public BaseController Owner { get; private set; }
    public BaseController Target { get; private set; }
    public S_SkillHit SkillPacket { get; private set; }

    public int CurrentPhase { get; private set; }
    public int NextPhaseTime { get; private set; }


    protected SkillController()
    {
        ObjectType = GameObjectType.Skill;
    }


    public void Init(SkillData skillData, BaseController owner, BaseController target, S_SkillHit skillPacket)
    {
        base.Init();

        SkillData = skillData;
        Owner = owner;
        Target = target;
        SkillPacket = skillPacket;

        CurrentPhase = skillPacket.SkillPhase - 1;
        NextPhaseTime = Environment.TickCount;  // 서버에서 phase를 받은 프레임에 바로 해당 phase를 실행하도록 시간을 설정함

        // 초기화
        Info.ObjectId = skillPacket.SkillObjectId;
        Info.Name = $"skill_{Id}";

        if (target != null)
            Pos = target.Pos;
        else
            Pos = new Vector2(skillPacket.SkillPosX, skillPacket.SkillPosY);
        State = CreatureState.Idle;
    }


    protected override void UpdateIdle()
    {
        // 스킬 phase가 변경될 때마다 OnPhase 함수를 호출한다.
        // 스킬 phase가 끝나면 상태를 Dead로 변경한다.

        // 캐스팅이 완료되었다면 phase 1
        // 그 이후의 phase는 phaseTimeInterval 시간 간격마다 증가함
        int tick = Environment.TickCount;
        if (tick >= NextPhaseTime)
        {
            CurrentPhase++;
            OnPhase(CurrentPhase);

            if (CurrentPhase < SkillData.numPhase)
            {
                // 다음 phase 시간 계산
                NextPhaseTime += SkillData.phaseTimeInterval;
            }
            else
            {
                // 다음 phase가 없다면 상태를 Dead로 변경. 객체의 삭제는 ObjectManager의 update에서 수행함
                OnEnd();
                State = CreatureState.Dead;
            }
        }
    }

    // phase 변경시 호출됨
    protected virtual void OnPhase(int phase)
    {

    }

    // phae가 끝났을때 호출됨
    protected virtual void OnEnd()
    {

    }

}

