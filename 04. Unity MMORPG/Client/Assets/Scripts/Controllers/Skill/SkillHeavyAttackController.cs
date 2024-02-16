using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


public class SkillHeavyAttackController : SkillController
{

    protected override void OnPhase(int phase)
    {
        if (phase == 1)
        {
            if (string.IsNullOrEmpty(SkillData.effect) == false)
            {
                foreach (HitInfo hit in SkillPacket.Hits)
                {
                    BaseController target = Managers.Object.FindById(hit.ObjectId);
                    if(target != null)
                        Managers.Object.AddEffect(SkillData.effect, target.Pos, SkillData.effectOffsetY);
                }
            }
        }
    }
}
