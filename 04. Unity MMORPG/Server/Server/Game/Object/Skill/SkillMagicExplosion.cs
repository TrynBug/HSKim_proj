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

        protected override void OnPhase(int phase)
        {
            S_SkillHit hitPacket = new S_SkillHit();
            hitPacket.AttackerId = Owner.Id;
            hitPacket.SkillObjectId = Id;
            hitPacket.SkillId = SkillData.id;
            hitPacket.SkillPhase = phase;

            if (Owner.Room.IsValidTarget(Target))
                hitPacket.TargetId = Target.Id;
            else
                hitPacket.TargetId = -1;
            
            // phase에 따라 위치를 조정함
            Vector2 dir = (TargetCastedPos - OwnerCastedPos).normalized;
            Pos = TargetCastedPos + dir * SkillData.instant.posInterval * (float)(phase - 1);
            hitPacket.SkillPosX = Pos.x;
            hitPacket.SkillPosY = Pos.y;

            // 피격체크
            // FindObjectsInRect 함수를 사용하기 위해 스킬 위치를 왼쪽으로 rangeX / 2 만큼 옮긴다음, 현재 위치에서 오른쪽 방향의 사각형 범위를 조사한다.
            Vector2 checkPos = Pos + GetDirectionVector(MoveDir.Left) * SkillData.instant.rangeX / 2f;
            if (Room.Map.IsInvalidPos(checkPos) == false)
            {
                List<GameObject> hits = Room.Map.FindObjectsInRect(checkPos, new Vector2(SkillData.instant.rangeX, SkillData.instant.rangeY), LookDir.LookRight, Owner);
                foreach (GameObject obj in hits)
                {
                    int damage = obj.OnDamaged(Owner, Owner.GetSkillDamage(SkillData));
                    hitPacket.Hits.Add(new HitInfo { ObjectId = obj.Id, Damage = damage });
                }
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
