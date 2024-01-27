using Google.Protobuf.Protocol;
using Google.Protobuf.WellKnownTypes;
using Server.Data;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class Player : GameObject
    {
        public ClientSession Session { get; private set; }

        public CreatureState RemoteState { get; set; } = CreatureState.Idle;
        public MoveDir RemoteDir { get; set; } = MoveDir.Down;

        SPUMData _SPUM = null;
        public SPUMData SPUM {
            get { return _SPUM; }
            set { 
                _SPUM = value;
                SPUMId = _SPUM.id;
            }
        }

        public Equipment Equip { get; set; } = new Equipment();



        public Player()
        {
            ObjectType = GameObjectType.Player;
        }


        public void Init(ClientSession session, GameRoom room)
        {
            base.Init();

            Session = session;

            // 위치 초기화
            Room = room;
            Info.Name = $"Player_{Id}";
            Dir = MoveDir.Left;
            Pos = room.Map.GetRandomEmptyPos();
            Dest = Pos;

            // 상태 초기화
            State = CreatureState.Loading;   // 초기상태는 Loading

            // 초기스탯 가져오기
            StatInfo stat = DataManager.StatDict.GetValueOrDefault(1, DataManager.DefaultStat);
            StatOrigin.MergeFrom(stat);

            // SPUM 데이터 가져오기
            Random rand = new Random();
            SPUMId = rand.Next(0, DataManager.SPUMDict.Count);
            SPUM = DataManager.SPUMDict.GetValueOrDefault(SPUMId, DataManager.DefaultSPUM);

            // 장비아이템 초기화
            Equip.SetEquipment(SPUM);

            // 스탯 계산
            RecalculateStat();

            // 스킬 초기화
            Item itemWeapon = Equip.WeaponLeft;
            if (itemWeapon.subType == EquipmentSubType.Empty)
                itemWeapon = Equip.WeaponRight;
            if (itemWeapon != null)
            {
                switch (itemWeapon.subType)
                {
                    case EquipmentSubType.Weapon_Wand:
                        {
                            foreach (SkillData skill in DataManager.SkillDict.Values)
                            {
                                if(skill.skillType == SkillType.SkillProjectile || skill.skillType == SkillType.SkillInstant)
                                    Skillset.Add(skill.id, new SkillUseInfo() { skill = skill });
                            }
                        }
                        break;
                    default:
                        {
                            foreach (SkillData skill in DataManager.SkillDict.Values)
                            {
                                if (skill.skillType == SkillType.SkillMelee)
                                    Skillset.Add(skill.id, new SkillUseInfo() { skill = skill });
                            }
                        }
                        break;
                }
            }

            //foreach(SkillData skill in DataManager.SkillDict.Values)
            //    Skillset.Add(skill.id, new SkillUseInfo() { skill = skill });
        }
    


        protected override void UpdateIdle()
        {
            if(MoveKeyDown == true || Util.Equals(Pos, Dest) == false)
            {
                State = CreatureState.Moving;
                UpdateMoving();
            }
        }


        protected override void UpdateMoving()
        {
            // 키보드 방향키가 눌려있는 동안은 Dest를 계속해서 이동시킨다.
            Vector2 intersection;
            if (MoveKeyDown == true)
            {
                Vector2 dest = Dest + GetDirectionVector(RemoteDir) * Room.Time.DeltaTime * Speed;
                if (Room.Map.CanGo(Dest, dest, out intersection))
                {
                    Dest = dest;
                }
                else
                {
                    Dest = intersection;
                }
            }
            // 키보드 방향키를 누르고있지 않다면 멈출 것이기 때문에 Dest를 이동시키지 않는다.



            // 위치 이동
            Vector2 diff = (Dest - Pos);
            Vector2 dir = diff.normalized;
            Vector2 pos = Pos + dir * Room.Time.DeltaTime * Speed;

            // Dest에 도착시 현재위치를 Dest로 변경한다.
            // 만약 이동키가 눌려져있지 않다면 현재 위치에 stop 하고 상태를 idle로 바꾼다.
            if (diff.magnitude <= Room.Time.DeltaTime * Speed)
            {
                if (MoveKeyDown)
                {
                    if (Room.Map.TryMoving(this, Dest))
                    {
                        Pos = Dest;
                    }
                    else
                    {
                        Dest = Pos;
                    }
                }
                else
                {
                    StopAt(Dest);
                }
            }
            // 위치 이동
            else if (Room.Map.CanGo(Pos, pos, out intersection))
            {
                if (Room.Map.TryMoving(this, pos))
                {
                    Pos = pos;
                }
                else
                {
                    Dest = Pos;
                }
            }
            // 이동중 부딪혔을 경우 더이상 이동할 수 없기 때문에 Dest를 변경한다.
            else
            {
                if (Room.Map.TryMoving(this, intersection))
                {
                    Pos = intersection;
                    Dest = intersection;
                }
                else
                {
                    Dest = Pos;
                }
            }



            Logger.WriteLog(LogLevel.Debug, $"Player.UpdateMoving. {this.ToString(InfoLevel.Position)}");
        }


        protected override void UpdateDead()
        {
        }

        // 사용중인 스킬에 대한 업데이트
        protected override void UpdateSkill()
        {
            // 사용중인 스킬에 대한 스킬딜레이 검사
            if (_usingSkill == null)
                return;

            // 사용중인 스킬의 스킬시전이 끝났을 경우 스킬딜레이 검사
            if (_usingSkill.casted == true)
            {
                int tick = Environment.TickCount;
                if (tick - _usingSkill.lastUseTime > _usingSkill.skill.skillTime)
                {
                    _usingSkill.casted = false;
                    _usingSkill = null;
                }
            }
        }







        protected override void UpdateAutoIdle()
        {
            State = CreatureState.Idle;

            // 타겟 찾기
            Auto.Target = Room.Map.FindAliveObjectNearbyCell(Cell, exceptObject: this);

            // 타겟이 지정됨
            if (Auto.Target != null)
            {
                // 다음 스킬 지정
                Auto.SetNextSkill();

                // 상태 변경
                Auto.State = AutoState.AutoChasing;

                // 경로 찾기
                Auto.SetPathToTarget();

                // 타겟 지정 패킷을 보냄
                Auto.SendAutoPacket();

                Logger.WriteLog(LogLevel.Debug, $"Player.UpdateAutoIdle. me:[{this.ToString(InfoLevel.Position)}], target:[{Auto.Target?.ToString(InfoLevel.Position)}]");
            }
        }

        protected override void UpdateAutoChasing()
        {
            // 타겟이 없다면 Idle 상태로 돌아감
            if(Room.IsValidTarget(Auto.Target) == false)
            {
                StopAt(Pos);

                Auto.WaitTime = TimeSpan.TicksPerSecond;
                Auto.NextState = AutoState.AutoIdle;
                Auto.State = AutoState.AutoWait;
                Auto.Target = null;

                // wait 패킷 전송
                Auto.SendAutoPacket();
                return;
            }

            // 타겟과의 거리 확인 
            Vector2 dist = Auto.Target.Pos - Pos;
            Vector2 distAbs = dist.Abs;
            LookDir lookToTarget = Util.GetLookToTarget(Pos, Auto.Target.Pos);
            // 타겟이 추적범위 내에 있고 동시에 스킬범위 내에 있으면 움직이지 않음
            if (distAbs.x < Auto.TargetDistance.x && distAbs.y < Auto.TargetDistance.y
                && Util.IsTargetInRectRange(Pos, lookToTarget, new Vector2(Auto.SkillUse.skill.rangeX, Auto.SkillUse.skill.rangeY), Auto.Target.Pos))
            {
                // 현재위치에 멈춤
                StopAt(Pos);

                // 방향 수정
                Dir = Util.GetDirectionToDest(Pos, Auto.Target.Pos);

                Logger.WriteLog(LogLevel.Debug, $"Player.UpdateAutoChasing. Stop. {this.ToString(InfoLevel.Position)}");
            }
            // 타겟이 추적범위 밖이면 움직임
            else
            {
                // 타겟이 cell을 이동했으면 경로 재계산
                if (Auto.PrevTargetCell != Auto.Target.Cell)
                {
                    Auto.SetPathToTarget();
                }

                // 경로를 따라 이동함
                State = CreatureState.Moving;
                Auto.MoveThroughPath();
            }


            // 스킬을 사용할 수 있고 타겟이 스킬범위내에 있을 경우
            if (CanUseSkill(Auto.SkillUse))
            {
                if (Util.IsTargetInRectRange(Pos, lookToTarget, new Vector2(Auto.SkillUse.skill.rangeX, Auto.SkillUse.skill.rangeY), Auto.Target.Pos))
                {
                    // 현재위치에 멈춤
                    StopAt(Pos);

                    // 방향 수정
                    Dir = Util.GetDirectionToDest(Pos, Auto.Target.Pos);

                    // 스킬 사용
                    AutoSkillUse(Auto.SkillUse, Auto.Target);

                    // Skill 상태로 변경
                    Auto.State = AutoState.AutoSkill;

                    // Skill 사용패킷 전송
                    Auto.SendAutoPacket();

                    return;
                }
            }
        }


        protected override void UpdateAutoMoving()
        {

            Logger.WriteLog(LogLevel.Debug, $"Player.UpdateAutoMoving. {this.ToString(InfoLevel.Position)}");
        }

        protected override void UpdateAutoSkill()
        {
            // 스킬 피격처리가 가능할때까지 기다림
            if (CanSkillHit(Auto.SkillUse.skill.id, out _) == false)
                return;

            // 스킬 피격처리
            AutoSkillHit(Auto.SkillUse.skill, Auto.Target);

            // 스킬 재선정
            Auto.SetNextSkill();

            // wait 후 Chasing 상태로 변경
            Auto.WaitTime = TimeSpan.TicksPerSecond;
            Auto.NextState = AutoState.AutoChasing;
            Auto.State = AutoState.AutoWait;

            // wait 패킷 전송
            Auto.SendAutoPacket();

            Logger.WriteLog(LogLevel.Debug, $"Player.UpdateAutoSkill. {this}, skill:{Auto.SkillUse.skill.id}");
        }

        protected override void UpdateAutoWait()
        {
            long tick = DateTime.Now.Ticks;
            if (tick > Auto.WaitUntil)
            {
                Auto.State = Auto.NextState;

                Logger.WriteLog(LogLevel.Debug, $"GameObject.UpdateAutoWait. nextState:{Auto.State}, {this}");
                return;
            }
        }

        protected override void UpdateAutoDead()
        {
            int tick = Environment.TickCount;
            if(tick - DeadTime > 60000)
            {
                Room._handleRespawn(this);
                DeadTime = int.MaxValue;

                Logger.WriteLog(LogLevel.Debug, $"GameObject.UpdateAutoDead. Revive.");
            }
        }




        void AutoSkillUse(SkillUseInfo useInfo, GameObject target)
        {
            // 스킬 사용시간 업데이트
            useInfo.lastUseTime = Environment.TickCount;

            // 사용중인 스킬 등록
            _lastUseSkill = useInfo;
            _usingSkill = useInfo;
        }

        void AutoSkillHit(SkillData skill, GameObject target)
        {
            if (skill == null)
                return;

            // 스킬 피격확인
            S_SkillHit resHitPacket = new S_SkillHit();
            resHitPacket.ObjectId = Id;
            resHitPacket.SkillId = skill.id;
            switch (skill.skillType)
            {
                case SkillType.SkillMelee:
                    {
                        // 스킬범위내의 오브젝트를 찾는다.
                        List<GameObject> objects = Room.Map.FindObjectsInRect(Pos, new Vector2(skill.rangeX, skill.rangeY), LookDir, this);
                        // 피격됨
                        foreach (GameObject obj in objects)
                        {
                            if (obj.IsAlive == false)
                                continue;
                            obj.OnDamaged(this, Stat.Damage + skill.damage);
                            resHitPacket.HitObjectIds.Add(obj.Id);
                        }
                    }
                    break;

                case SkillType.SkillProjectile:
                    {
                        if (target == null || target.IsAlive == false)
                            break;

                        // target이 스킬범위내에 존재하는지 확인
                        if (Util.IsTargetInRectRange(Pos, LookDir, new Vector2(skill.rangeX, skill.rangeY), target.Pos) == true)
                        {
                            // projectile의 경우에는 여기에서 피격대상을 전송하지 않고 투사체만 생성한다.
                            Projectile projectile = Projectile.CreateInstance(skill.id);
                            projectile.Init(skill, this, target);
                            Room._enterGame(projectile);
                        }
                    }
                    break;

                case SkillType.SkillInstant:
                    {
                        if (target == null || target.IsAlive == false)
                            break;

                        // target이 스킬범위내에 존재하면 피격판정
                        if (Util.IsTargetInRectRange(Pos, LookDir, new Vector2(skill.rangeX, skill.rangeY), target.Pos) == true)
                        {
                            // 피격됨
                            int damage = target.OnDamaged(this, Stat.Damage + skill.damage);
                            resHitPacket.HitObjectIds.Add(target.Id);
                        }
                    }
                    break;
            }

            // 게임룸 내의 모든 플레이어들에게 브로드캐스팅
            Room._broadcast(resHitPacket);


            ServerCore.Logger.WriteLog(LogLevel.Debug, $"Player.AutoSkillHit. {this}, skill:{resHitPacket.SkillId}, hits:{resHitPacket.HitObjectIds.Count}");
        }














        // 스킬 사용함
        public virtual void OnSkill(SkillUseInfo useInfo)
        {
            // 스킬 사용시간 업데이트
            useInfo.lastUseTime = Environment.TickCount;

            // 사용중인 스킬 등록
            _lastUseSkill = useInfo;
            _usingSkill = useInfo;

            // 스킬패킷 전송
            S_Skill skillPacket = new S_Skill();
            skillPacket.SkillId = useInfo.skill.id;
            skillPacket.ObjectId = Id;
            Room._broadcast(skillPacket);

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"Player.OnSkill. {this}, skill:{useInfo.skill.id}");
        }
        public virtual void OnSkill(SkillId skillId)
        {
            SkillUseInfo useInfo;
            if (Skillset.TryGetValue(skillId, out useInfo) == false)
                return;

            OnSkill(useInfo);
        }






        // 스킬 사용이 가능한지 검사함
        public virtual bool CanUseSkill(SkillId skillId, out SkillUseInfo skillUseInfo)
        {
            skillUseInfo = null;
            if (IsAlive == false)
                return false;
            if (skillId == SkillId.SkillNone)
                return false;

            // 이전에 사용한 스킬의 딜레이가 끝났는지 확인
            int tick = Environment.TickCount;
            if (tick - _lastUseSkill.lastUseTime < (_lastUseSkill.skill.skillTime - 50))
                return false;

            // 사용할 스킬 데이터 얻기
            SkillUseInfo useInfo;
            if (Skillset.TryGetValue(skillId, out useInfo) == false)
                return false;

            // 쿨타임 검사
            if (tick - useInfo.lastUseTime < useInfo.skill.cooldown)
                return false;

            skillUseInfo = useInfo;
            return true;
        }

        public virtual bool CanUseSkill(SkillUseInfo skillUseInfo)
        {
            if (skillUseInfo == null)
                return false;
            if (IsAlive == false)
                return false;

            // 이전에 사용한 스킬의 딜레이가 끝났는지 확인
            int tick = Environment.TickCount;
            if (tick - _lastUseSkill.lastUseTime < (_lastUseSkill.skill.skillTime - 50))
                return false;

            // 쿨타임 검사
            if (tick - skillUseInfo.lastUseTime < skillUseInfo.skill.cooldown)
                return false;

            return true;
        }


        // 스킬 피격처리가 가능한지 검사함
        public virtual bool CanSkillHit(SkillId skillId, out SkillData skill)
        {
            skill = null;

            if (IsAlive == false)
                return false;
            if (skillId == SkillId.SkillNone)
                return false;
            if (_usingSkill == null)
                return false;
            if (skillId != _usingSkill.skill.id)
                return false;

            // 스킬 시전이 끝났는지 검사함
            int tick = Environment.TickCount;
            if (tick - _usingSkill.lastUseTime > (_usingSkill.skill.castingTime - 50))
            {
                _usingSkill.casted = true;
            }

            // 스킬시전이 끝났다면 피격처리가 가능함
            skill = _usingSkill.skill;
            return (_usingSkill.casted == true);
        }



        // 장비 데이터를 참고하여 Stat을 재계산한다.
        public void RecalculateStat()
        {
            if (StatOrigin == null || SPUM == null || Stat == null)
                return;

            Stat.MergeFrom(StatOrigin);
            Stat.MaxHp += Equip.Hp;
            Stat.Hp = Stat.MaxHp;
            Stat.Damage += Equip.Damage;
            Stat.Defence += Equip.Defence;
            Stat.RangeX += Equip.RangeX;
            Stat.RangeY += Equip.RangeY;
            Stat.AttackSpeed += Equip.AttackSpeed;
            Stat.Speed += Equip.Speed;
        }

    }



}
