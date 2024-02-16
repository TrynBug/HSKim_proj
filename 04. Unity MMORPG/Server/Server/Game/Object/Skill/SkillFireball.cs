using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class SkillFireball : Skill
    {
        protected override void OnPhase(int phase)
        {
            if (phase == 1)
            {
                // 캐스팅이 끝남
                Projectile projectile = Projectile.Generate(SkillData.id);
                projectile.Init(SkillData, Owner, OwnerCastedPos, TargetCastedPos);

                Owner.Room.EnterGame(projectile);
            }
        }
    }
}
