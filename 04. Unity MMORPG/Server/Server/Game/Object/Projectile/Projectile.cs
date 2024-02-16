using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Server.Data;
using System.Numerics;

namespace Server.Game
{
    public class Projectile : GameObject
    {
        // Projectile 객체 생성함수
        public static Projectile Generate(SkillId skillId)
        {
            switch(skillId)
            {
                case SkillId.SkillFireball:
                    return new Projectile();
                default:
                    return null;
            }
        }


        public SkillData SkillData { get; private set; }
        public GameObject Owner { get; private set; }

        protected Projectile()
        {
            ObjectType = GameObjectType.Projectile;
        }

        public virtual void Init(SkillData skillData, GameObject owner, Vector2 pos, Vector2 dest)
        {
            base.Init();

            SkillData = skillData;
            Owner = owner;

            Room = owner.Room;
            Pos = pos + GetDirectionVector(MoveDir.Up) * SkillData.projectile.offsetY;  // 시작위치를 offsetY 만큼 조정
            Dest = dest + GetDirectionVector(MoveDir.Up) * SkillData.projectile.offsetY;   // 도착위치를 offsetY 만큼 조정
            Dir = Util.GetDirectionToDest(Pos, Dest);
            LookDir = Util.GetLookToTarget(Pos, Dest);
            State = CreatureState.Moving;

            Speed = skillData.projectile.speed;

            // 시작위치를 offsetX 만큼 조정
            // 만약 조정한 위치가 Dest를 넘어갔다면 위치를 Dest로 바꾼다.
            Vector2 posAdj = Pos + Util.GetDirectionVector(LookDir) * SkillData.projectile.offsetX;
            if ((Dest - Pos).squareMagnitude < (posAdj - Pos).squareMagnitude)
                Pos = Dest;
            else
                Pos = posAdj;

        }


        protected override void UpdateMoving()
        {
            Vector2 diff = (Dest - Pos);
            Vector2 dir = diff.normalized;
            Vector2 pos = Pos + dir * Room.Time.DeltaTime * Speed;

            // Dest에 도착시 상태를 Dead로 변경하고 피격처리
            if (diff.magnitude <= Room.Time.DeltaTime * Speed)
            {
                Pos = Dest;
                State = CreatureState.Dead;

                // 피격처리, 피격패킷 전송
                ProcessHit();
            }
            else
            {
                Pos = pos;
            }
        }

        // 피격처리, 피격패킷 전송
        protected void ProcessHit()
        {
            S_SkillHit hitPacket = new S_SkillHit();
            hitPacket.AttackerId = Owner.Id;
            hitPacket.SkillObjectId = Id;
            hitPacket.SkillId = SkillData.id;
            hitPacket.SkillPhase = 1;
            hitPacket.SkillPosX = Pos.x;
            hitPacket.SkillPosY = Pos.y;

            switch (SkillData.projectile.hitType)
            {
                case SkillHitType.SkillHitSingle:
                    break;
                case SkillHitType.SkillHitCircle:
                    {
                        List<GameObject> hits = Room.Map.FindObjectsInCircle(Pos, SkillData.projectile.rangeX, Owner);
                        foreach (GameObject obj in hits)
                        {
                            int damage = obj.OnDamaged(Owner, Owner.GetSkillDamage(SkillData));
                            hitPacket.Hits.Add(new HitInfo { ObjectId = obj.Id, Damage = damage });
                        }
                    }
                    break;
                case SkillHitType.SkillHitRectangle:
                    {
                        Vector2 checkPos = Pos + GetDirectionVector(MoveDir.Left) * SkillData.projectile.rangeX / 2f;
                        List<GameObject> hits = Room.Map.FindObjectsInRect(checkPos, new Vector2(SkillData.projectile.rangeX, SkillData.projectile.rangeY), LookDir.LookRight, Owner);
                        foreach (GameObject obj in hits)
                        {
                            int damage = obj.OnDamaged(Owner, Owner.GetSkillDamage(SkillData));
                            hitPacket.Hits.Add(new HitInfo { ObjectId = obj.Id, Damage = damage });
                        }
                    }
                    break;
            }

            Room._broadcast(hitPacket);
        }
    }
}
