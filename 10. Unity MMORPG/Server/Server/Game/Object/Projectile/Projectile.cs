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
        public Data.SkillData Skill { get; private set; }
        public GameObject Owner { get; private set; }
        public GameObject Target { get; private set; }

        public Projectile()
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
            // ������ ���� : Ÿ���� �ִٸ� Ÿ���� ��ġ, Ÿ���� ���ٸ� �ִ��Ÿ� ��ġ
            if (target != null)
                Dest = target.Pos;
            else
                Dest = owner.Pos + (Util.GetDirectionVector(owner.LookDir) * skill.projectile.rangeX);
            State = CreatureState.Moving;
        }


        protected override void UpdateMoving()
        {
            if (Target?.Room != Room)
                Target = null;

            if(Target != null)
                Dest = Target.Pos;

            Vector2 diff = (Dest - Pos);
            Vector2 dir = diff.normalized;
            Vector2 pos = Pos + dir * Room.Time.DeltaTime * Speed;

            // Dest�� ������ ���¸� Dead�� �����ϰ� �ǰ�ó��
            if (diff.magnitude <= Room.Time.DeltaTime * Speed)
            {
                State = CreatureState.Dead;
                Target?.OnDamaged(this, Owner.Stat.Damage + Skill.damage);
            }
            else
            {
                Pos = pos;
            }
        }
    }
}
