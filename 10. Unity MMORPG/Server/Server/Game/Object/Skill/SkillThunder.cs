using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class SkillThunder : Skill
    {
        protected override void OnPhase(int phase)
        {
            if (phase == 1)
            {
                // 캐스팅이 끝났을 경우
                S_SkillHit hitPacket = new S_SkillHit();
                hitPacket.AttackerId = Owner.Id;
                hitPacket.SkillObjectId = Id;
                hitPacket.SkillId = SkillData.id;
                hitPacket.SkillPhase = phase;

                if (Owner.Room.IsValidTarget(Target))
                {
                    // 타겟이 있다면 스킬 위치를 타겟위치로 설정
                    Pos = Target.Pos;
                    hitPacket.TargetId = Target.Id;
                    hitPacket.SkillPosX = Pos.x;
                    hitPacket.SkillPosY = Pos.y;
                }
                else
                {
                    // 타겟이 없다면 스킬 위치를 적절한 위치로 지정
                    Pos = Room.Map.GetValidPos(Owner.Pos + Util.GetDirectionVector(Owner.LookDir) * (SkillData.rangeX / 2));
                    hitPacket.TargetId = -1;
                    hitPacket.SkillPosX = Pos.x;
                    hitPacket.SkillPosY = Pos.y;
                }

                // 피격체크
                List<GameObject> hits = Room.Map.FindObjectsInCircle(Pos, SkillData.instant.rangeX, Owner);
                foreach (GameObject obj in hits)
                {
                    int damage = obj.OnDamaged(Owner, Owner.GetSkillDamage(SkillData));
                    hitPacket.Hits.Add(new HitInfo { ObjectId = obj.Id, Damage = damage });
                }

                // 스킬 피격패킷 전송
                Owner.Room._broadcast(hitPacket);
            }
            else
            {
                S_SkillHit hitPacket = new S_SkillHit();
                hitPacket.AttackerId = Owner.Id;
                hitPacket.SkillObjectId = Id;
                hitPacket.SkillId = SkillData.id;
                hitPacket.SkillPhase = phase;
                hitPacket.TargetId = -1;
                hitPacket.SkillPosX = Pos.x;
                hitPacket.SkillPosY = Pos.y;

                // 캐스팅 이후의 phase
                // 4개 직선방향에 대한 피격체크를 한다.
                foreach (MoveDir dir in Util.IterateStraightDirection())
                {
                    Vector2 hitPos = Pos + Util.GetDirectionVector(dir) * SkillData.instant.posInterval * (float)(phase - 1);
                    List<GameObject> hits = Room.Map.FindObjectsInCircle(hitPos, SkillData.instant.rangeX, Owner);
                    foreach (GameObject obj in hits)
                    {
                        int damage = obj.OnDamaged(Owner, Owner.GetSkillDamage(SkillData));
                        hitPacket.Hits.Add(new HitInfo { ObjectId = obj.Id, Damage = damage });
                    }
                }

                // 스킬 피격패킷 전송. 첫 phase 이후에는 피격됐을때만 보냄
                if (hitPacket.Hits.Count > 0)
                {
                    Owner.Room._broadcast(hitPacket);
                }
            }
        }
    }
}
