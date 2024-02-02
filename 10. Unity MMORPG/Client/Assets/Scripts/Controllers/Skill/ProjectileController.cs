using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Data;
using UnityEngine;
using System.Collections;


public class ProjectileController : BaseController
{
    public static ProjectileController Generate(SkillId skillId)
    {
        SkillData skill = Managers.Data.SkillDict.GetValueOrDefault(skillId);
        if (skill == null)
            return null;
        GameObject go = Managers.Resource.Instantiate(skill.projectile.prefab);
        SpriteRenderer renderer = go.GetOrAddComponent<SpriteRenderer>();
        renderer.sortingOrder = 2;
        switch (skillId)
        {
            case SkillId.SkillFireball:
                return go.GetOrAddComponent<ProjectileController>();
            default:
                return null;
        }
    }

    public SkillData SkillData { get; private set; }
    public BaseController Owner { get; private set; }

    SpriteRenderer _sprite;

    /* 위치 */
    //public override LookDir LookDir
    //{
    //    get { return base.LookDir; }
    //    set 
    //    { 
    //        base.LookDir = value;
    //        if (_sprite != null)
    //            _sprite.flipX = (value == LookDir.LookLeft);
    //    }
    //}

    /* 스탯 */
    protected float Speed { get { return SkillData.projectile.speed; } }


    public void Init(int objectId, SkillData skillData, BaseController owner, PositionInfo posInfo)
    {
        _sprite = gameObject.GetComponent<SpriteRenderer>();

        base.Init();
        ObjectType = GameObjectType.Projectile;
        

        Info.ObjectId = objectId;
        SkillData = skillData;
        Owner = owner;
        PosInfo = posInfo;

        // 시작위치를 offsetY 만큼 조정
        Pos = owner.Pos + GetDirectionVector(MoveDir.Up) * SkillData.projectile.offsetY;
        // 시작위치를 offsetX 만큼 조정
        // 만약 조정한 위치가 Dest를 넘어갔다면 위치를 Dest로 바꾼다.
        Vector2 posAdj = Pos + Util.GetDirectionVector(LookDir) * SkillData.projectile.offsetX;
        if ((Dest - Pos).sqrMagnitude < (posAdj - Pos).sqrMagnitude)
            Pos = Dest;
        else
            Pos = posAdj;

        // 각도 조정
        Vector3 posClient = Managers.Map.ServerPosToClientPos(Pos);
        Vector3 destClient = Managers.Map.ServerPosToClientPos(Dest);
        Vector3 diff = destClient - posClient;
        float angle = Mathf.Atan2(diff.y, diff.x) * Mathf.Rad2Deg;
        gameObject.transform.rotation = Quaternion.Euler(0f, 0f, angle);
    }



    protected override void UpdateMoving()
    {
        Vector2 diff = (Dest - Pos);
        Vector2 dir = diff.normalized;
        Vector2 pos = Pos + dir * Time.deltaTime * Speed;

        // Dest에 도착시 상태를 Dead로 변경
        // 객체의 삭제는 ObjectManager의 Update에서 수행함
        if (diff.magnitude <= Time.deltaTime * Speed)
        {
            Pos = Dest;
            State = CreatureState.Dead;

        }
        else
        {
            Pos = pos;
        }
    }
}

