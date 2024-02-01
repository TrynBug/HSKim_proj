using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class Projectile : GameObject
    {
        // Projectile 객체 생성함수
        public static Projectile CreateInstance(SkillId skillId)
        {
            switch(skillId)
            {
                case SkillId.SkillFireball:
                    return new Fireball();
                default:
                    return new Projectile();
            }
        }


        public Data.SkillData Skill { get; private set; }
        public GameObject Owner { get; private set; }
        public GameObject Target { get; private set; }



        protected Projectile()
        {
            ObjectType = GameObjectType.Projectile;
        }

        public virtual void Init(Data.SkillData skill, GameObject owner, GameObject target)
        {
            base.Init();

            Skill = skill;
            Owner = owner;
            Target = target;

            Room = owner.Room;
            Pos = owner.Pos;
            Speed = skill.projectile.speed;
            // 목적지 지정 : 타겟이 있다면 타겟의 위치, 타겟이 없다면 최대사거리 위치
            if (target != null)
                Dest = target.Pos;
            else
                Dest = owner.Pos + (Util.GetDirectionVector(owner.LookDir) * skill.rangeX);
            State = CreatureState.Moving;
        }


        protected override void UpdateMoving()
        {
            if (Target == null || Target.Room != Room)
                Target = null;

            if(Target != null)
                Dest = Target.Pos;

            Vector2 diff = (Dest - Pos);
            Vector2 dir = diff.normalized;
            Vector2 pos = Pos + dir * Room.Time.DeltaTime * Speed;

            // Dest에 도착시 상태를 Dead로 변경하고 피격처리
            if (diff.magnitude <= Room.Time.DeltaTime * Speed)
            {
                State = CreatureState.Dead;
                if(Target != null)
                    Target.OnDamaged(this, Owner.Stat.Damage + Skill.damage);
            }
            else
            {
                Pos = pos;
            }
        }
    }
}
