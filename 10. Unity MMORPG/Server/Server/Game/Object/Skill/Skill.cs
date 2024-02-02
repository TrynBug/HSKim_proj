using Google.Protobuf.Collections;
using Google.Protobuf.Protocol;
using Google.Protobuf.WellKnownTypes;
using Server.Data;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Server.Game
{
    public class Skill : GameObject
    {
        public static Skill Generate(SkillId skillId)
        {
            switch(skillId)
            {
                case SkillId.SkillAttack:
                    return new SkillAttack();
                case SkillId.SkillHeavyAttack:
                    return new SkillHeavyAttack();
                case SkillId.SkillThunder:
                    return new SkillThunder();
                case SkillId.SkillMagicExplosion:
                    return new SkillMagicExplosion();
                case SkillId.SkillFireball:
                    return new SkillFireball();
                default:
                    return null;
            }
        }


        public SkillUseInfo UseInfo { get; private set; }
        public SkillData SkillData { get { return UseInfo.skill; } }
        public GameObject Owner { get; private set; }
        public GameObject Target { get; private set; }
        public RepeatedField<int> HitObjects { get; private set; }

        public int CurrentPhase { get; private set; }
        public int NextPhaseTime { get; private set; }

        public Vector2 OwnerInitPos { get; private set; }
        public Vector2 TargetInitPos { get; private set; }
        public Vector2 OwnerCastedPos { get; private set; }
        public Vector2 TargetCastedPos { get; private set; }

        protected Skill()
        {
            ObjectType = GameObjectType.Skill;
        }


        public virtual void Init(SkillUseInfo useInfo, GameObject owner, GameObject target, RepeatedField<int> hits)
        {
            base.Init();

            UseInfo = useInfo;
            Owner = owner;
            Target = target;
            HitObjects = hits;

            CurrentPhase = 0;
            NextPhaseTime = useInfo.lastUseTime + SkillData.castingTime;  // phase 1 변경시간은 캐스팅이 끝나는시간

            // 초기화
            Room = owner.Room;
            Info.Name = $"Skill_{Id}";
            State = CreatureState.Idle;

            OwnerInitPos = Owner.Pos;
            if (Room.IsSameRoom(target))
                TargetInitPos = target.Pos;
            else
                TargetInitPos = Room.Map.GetValidPos(Owner.Pos + Util.GetDirectionVector(Owner.LookDir) * (SkillData.rangeX / 2));
        }


        protected override void UpdateIdle()
        {
            // 스킬 phase가 변경될 때마다 OnPhase 함수를 호출한다.
            // 스킬 phase가 끝나면 상태를 Dead로 변경한다.

            // 캐스팅이 완료되었다면 phase 1
            // 그 이후의 phase는 phaseTimeInterval 시간 간격마다 증가함
            int tick = Environment.TickCount;
            if(tick > NextPhaseTime)
            {
                CurrentPhase++;
                if(CurrentPhase == 1)
                {
                    OwnerCastedPos = Owner.Pos;
                    if (Room.IsSameRoom(Target))
                        TargetCastedPos = Target.Pos;
                    else
                        TargetCastedPos = Room.Map.GetValidPos(Owner.Pos + Util.GetDirectionVector(Owner.LookDir) * (SkillData.rangeX / 2));
                }


                OnPhase(CurrentPhase);

                if(CurrentPhase < SkillData.numPhase)
                {
                    // 다음 phase 시간 계산
                    NextPhaseTime += SkillData.phaseTimeInterval;
                }
                else
                {
                    // 다음 phase가 없다면 상태를 Dead로 변경
                    State = CreatureState.Dead;
                }
            }            
        }

        // phase 변경시 호출됨
        protected virtual void OnPhase(int phase)
        {

        }

    }
}
