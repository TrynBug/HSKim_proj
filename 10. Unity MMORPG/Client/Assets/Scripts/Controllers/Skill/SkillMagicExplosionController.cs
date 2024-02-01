using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


public class SkillMagicExplosionController : SkillController
{
    Vector2 _ownerInitPos;
    Vector2 _targetInitPos;

    protected override void OnPhase(int phase)
    {
        // 초기위치 설정
        if (phase == 1)
        {
            _ownerInitPos = Owner.Pos;
            _targetInitPos = new Vector2(SkillPacket.SkillPosX, SkillPacket.SkillPosY);
        }

        // 위치 조정
        Vector2 dir = (_targetInitPos - _ownerInitPos).normalized;
        Pos = _targetInitPos + dir * SkillData.instant.posInterval * (float)(phase - 1);

        // 이펙트 생성
        if (string.IsNullOrEmpty(SkillData.effect) == false)
            Managers.Object.AddEffect(SkillData.effect, Pos, SkillData.effectOffsetY);
    }
}

