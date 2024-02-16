using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class SkillAttack : Skill
    {
        protected override void OnPhase(int phase)
        {
            if (phase == 1)
            {
                // 캐스팅이 끝남
                S_SkillHit hitPacket = new S_SkillHit();
                hitPacket.AttackerId = Owner.Id;
                hitPacket.SkillObjectId = Id;
                hitPacket.SkillId = SkillData.id;
                hitPacket.SkillPhase = phase;

                // 피격체크
                foreach(int targetId in HitObjects)
                {
                    Player target = Owner.Room.FindPlayer(targetId);
                    if (target == null)
                        continue;
                    if (target.IsAlive == false)
                        continue;
                    if (Util.IsTargetInRectRange(Owner.Pos, Owner.LookDir, new Vector2(SkillData.rangeX, SkillData.rangeY), target.Pos))
                    {
                        int damage = target.OnDamaged(Owner, Owner.GetSkillDamage(SkillData));
                        hitPacket.Hits.Add(new HitInfo { ObjectId = target.Id, Damage = damage });
                    }
                }

                // 스킬 피격패킷 전송
                Owner.Room._broadcast(hitPacket);
            }
        }
    }
}
