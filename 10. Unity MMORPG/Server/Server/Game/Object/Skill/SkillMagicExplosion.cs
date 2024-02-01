using Google.Protobuf.Collections;
using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class SkillMagicExplosion : Skill
    {
        Vector2 _ownerInitPos;
        Vector2 _targetInitPos;

        public override void Init(SkillUseInfo useInfo, GameObject owner, GameObject target, RepeatedField<int> hits)
        {
            base.Init(useInfo, owner, target, hits);

            if (target != null)
                _targetInitPos = target.Pos;
            else
                _targetInitPos = Room.Map.GetValidPos(Owner.Pos + Util.GetDirectionVector(Owner.LookDir) * (SkillData.rangeX / 2));
        }

        protected override void OnPhase(int phase)
        {
            S_SkillHit hitPacket = new S_SkillHit();
            hitPacket.AttackerId = Owner.Id;
            hitPacket.SkillObjectId = Id;
            hitPacket.SkillId = SkillData.id;
            hitPacket.SkillPhase = phase;

            if (phase == 1)
            {
                // 캐스팅이 끝났을 경우
                _ownerInitPos = Owner.Pos;
                if (Owner.Room.IsValidTarget(Target))
                {
                    // 타겟이 있다면 스킬 위치를 타겟위치로 설정
                    _targetInitPos = Target.Pos;
                    Pos = _targetInitPos;
                    hitPacket.TargetId = Target.Id;
                }
                else
                {
                    // 타겟이 없다면 스킬 위치를 적절한 위치로 지정
                    Pos = _targetInitPos;
                    hitPacket.TargetId = -1;
                }
            }
            else
            {
                // 캐스팅 이후에는 phase에 따라 위치를 바꿈
                Vector2 dir = (_targetInitPos - _ownerInitPos).normalized;
                Pos = _targetInitPos + dir * SkillData.instant.posInterval * (float)(phase - 1);
            }

            hitPacket.SkillPosX = Pos.x;
            hitPacket.SkillPosY = Pos.y;


            // 피격체크
            // FindObjectsInRect 함수를 사용하기 위해 스킬 위치를 왼쪽으로 rangeX / 2 만큼 옮긴다음, 현재 위치에서 오른쪽 방향의 사각형 범위를 조사한다.
            Vector2 checkPos = Pos + GetDirectionVector(MoveDir.Left) * SkillData.instant.rangeX / 2f;
            List<GameObject> hits = Room.Map.FindObjectsInRect(checkPos, new Vector2(SkillData.instant.rangeX, SkillData.instant.rangeY), LookDir.LookRight, Owner);
            foreach (GameObject obj in hits)
            {
                int damage = obj.OnDamaged(Owner, Owner.GetSkillDamage(SkillData));
                hitPacket.Hits.Add(new HitInfo { ObjectId = obj.Id, Damage = damage });
            }


            // 스킬 피격패킷 전송
            if (phase == 1)
            {
                Owner.Room._broadcast(hitPacket);
            }
            else
            {
                if(hitPacket.Hits.Count > 0) 
                {
                    Owner.Room._broadcast(hitPacket);
                }
            }

        }
    }
}
