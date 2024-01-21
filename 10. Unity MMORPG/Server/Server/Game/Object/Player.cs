using Google.Protobuf.Protocol;
using Google.Protobuf.WellKnownTypes;
using Server.Data;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class Player : GameObject
    {
        public ClientSession Session { get; set; }

        public CreatureState RemoteState { get; set; } = CreatureState.Idle;
        public MoveDir RemoteDir { get; set; } = MoveDir.Down;

        SPUMData _SPUM = null;
        public SPUMData SPUM {
            get { return _SPUM; }
            set { 
                _SPUM = value;
                Info.SPUMId = _SPUM.id;
            }
        }

        public Equipment Equip { get; set; } = new Equipment();

        /* 스킬 */
        // 사용가능한 스킬정보
        Dictionary<SkillId, SkillUseInfo> _skillset = new Dictionary<SkillId, SkillUseInfo>();
        public Dictionary<SkillId, SkillUseInfo> Skillset { get { return _skillset; } }

        SkillUseInfo _lastUseSkill = new SkillUseInfo() { lastUseTime = 0, skill = DataManager.DefaultSkill };  // 마지막으로 사용한 스킬
        SkillUseInfo _usingSkill = null;                  // 현재 사용중인 스킬


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
            State = CreatureState.Idle;
            Dir = MoveDir.Left;
            Pos = room.PosCenter;
            Dest = Pos;

            // 초기스탯 가져오기
            StatInfo stat = DataManager.StatDict.GetValueOrDefault(1, DataManager.DefaultStat);
            StatOrigin.MergeFrom(stat);

            // SPUM 데이터 가져오기
            int spumId = (Id & 0xf) % DataManager.SPUMDict.Count;
            SPUM = DataManager.SPUMDict.GetValueOrDefault(spumId, DataManager.DefaultSPUM);

            // 장비아이템 초기화
            Equip.SetEquipment(SPUM);

            // 스탯 계산
            RecalculateStat();


            // 스킬 초기화
            foreach(SkillData skill in DataManager.SkillDict.Values)
                Skillset.Add(skill.id, new SkillUseInfo() { skill = skill });
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
                    Vector2 stopPos;
                    if (Room.Map.TryStop(this, Dest, out stopPos))
                    {
                        Pos = stopPos;
                        Dest = stopPos;
                    }
                    else
                    {
                        Dest = Pos;
                    }
                    State = CreatureState.Idle;
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

        // 피격됨
        public override int OnDamaged(GameObject attacker, int damage)
        {
            return base.OnDamaged(attacker, damage);
        }

        // 사망함
        public override void OnDead(GameObject attacker)
        {
            base.OnDead(attacker);
        }







        // 스킬 사용이 가능한지 검사함
        public virtual bool CanUseSkill(SkillId skillId, out SkillUseInfo skillUseInfo)
        {
            skillUseInfo = null;

            if (State == CreatureState.Dead)
                return false;

            if (skillId == SkillId.SkillNone)
                return false;

            // 이전에 사용한 스킬의 딜레이가 끝났는지 확인
            int tick = Environment.TickCount;
            if (tick - _lastUseSkill.lastUseTime < _lastUseSkill.skill.skillTime)
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


        // 스킬 피격처리가 가능한지 검사함
        public virtual bool CanSkillHit(SkillId skillId, out SkillData skill)
        {
            skill = null;

            if (State == CreatureState.Dead)
                return false;
            if (skillId == SkillId.SkillNone)
                return false;
            if (_usingSkill == null)
                return false;
            if (skillId != _usingSkill.skill.id)
                return false;

            // 스킬 시전이 끝났는지 검사함
            int tick = Environment.TickCount;
            if (tick - _usingSkill.lastUseTime > _usingSkill.skill.castingTime)
            {
                _usingSkill.casted = true;
            }

            // 스킬시전이 끝났다면 피격처리가 가능함
            skill = _usingSkill.skill;
            return (_usingSkill.casted == true);
        }



        // 장비 데이터를 참고하여 Stat을 재계산한다.
        private void RecalculateStat()
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
