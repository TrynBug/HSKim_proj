using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


public class SkillThunderController : SkillController
{

    protected override void OnPhase(int phase)
    {
        if (phase == 1)
        {
            // 현재위치에 이펙트 생성
            if(string.IsNullOrEmpty(SkillData.effect) == false)
                Managers.Object.AddEffect(SkillData.effect, Pos, SkillData.effectOffsetY);
        }
        else
        {
            // 4개 직선방향에 이펙트를 생성한다.
            if (string.IsNullOrEmpty(SkillData.effect) == false)
            {
                foreach (MoveDir dir in Util.IterateStraightDirection())
                {
                    Vector2 pos = Pos + Util.GetDirectionVector(dir) * SkillData.instant.posInterval * (float)(phase - 1);
                    Managers.Object.AddEffect(SkillData.effect, pos, SkillData.effectOffsetY);
                }
            }
        }
    }
}

