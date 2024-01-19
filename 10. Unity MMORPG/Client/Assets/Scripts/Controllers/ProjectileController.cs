using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


public class ProjectileController : BaseController
{
    public Data.SkillData Skill { get; private set; }
    public BaseController Owner { get; private set; }
    public BaseController Target { get; private set; }

    SpriteRenderer _sprite;

    public override LookDir LookDir
    {
        get { return base.LookDir; }
        set 
        { 
            base.LookDir = value;
            if (_sprite != null)
                _sprite.flipX = (value == LookDir.LookRight);
        }
    }


    protected override void Init()
    {
        ObjectType = GameObjectType.Projectile;
        if(_sprite == null)
            _sprite = gameObject.GetComponent<SpriteRenderer>();
        base.Init();
    }


    public void Init(int objectId, Data.SkillData skill, BaseController owner, BaseController target)
    {
        if (_sprite == null)
            _sprite = gameObject.GetComponent<SpriteRenderer>();

        Info.ObjectId = objectId;
        Skill = skill;
        Owner = owner;
        Target = target;

        Pos = owner.Pos;
        Speed = skill.projectile.speed;
        if (target != null)
        {
            Dest = target.Pos;
            LookDir = Util.GetLookDirectionToTarget(Owner, Target);
        }
        else
        {
            Dest = owner.Pos + (Util.GetDirectionVector(owner.LookDir) * skill.projectile.rangeX);
            LookDir = Util.GetLookDirectionToTarget(Owner, Dest);

        }
        State = CreatureState.Moving;
    }



    protected override void UpdateMoving()
    {
        if (Target != null)
            Dest = Target.Pos;

        Vector2 diff = (Dest - Pos);
        Vector2 dir = diff.normalized;
        Vector2 pos = Pos + dir * Time.deltaTime * Speed;

        // Dest에 도착시 상태를 Dead로 변경함
        if (diff.magnitude <= Time.deltaTime * Speed)
        {
            State = CreatureState.Dead;
        }
        else
        {
            Pos = pos;
        }
    }



    protected override void UpdateDead()
    {
        // effect 생성 
        Managers.Object.AddEffect(Skill.projectile.effect, Pos);
        // 자신 파괴
        Managers.Object.Remove(Id);
    }
}

