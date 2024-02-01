using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


public class SkillAttackController : SkillController
{

    protected override void OnPhase(int phase)
    {
        if (phase == 1)
        {
            if (string.IsNullOrEmpty(SkillData.effect) == false)
                Managers.Object.AddEffect(SkillData.effect, Pos, SkillData.effectOffsetY);
        }
    }
}

