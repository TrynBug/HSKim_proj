using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using ServerCore;
using DummyClient.Data;
using static DummyClient.Define;
using System.Numerics;

namespace DummyClient.Game
{
    public class CreatureController : BaseController
    {
        /* object */
        public override ObjectInfo Info
        {
            get { return base.Info; }
            set
            {
                base.Info = value;
                Stat = value.StatInfo;
                AutoInfo = value.AutoInfo;
            }
        }

        /* 위치 */
        // 현재상태. 애니메이션 업데이트 추가.
        public override CreatureState State
        {
            get { return base.State; }
            set
            {
                if (base.State == value)
                    return;
                base.State = value;
            }
        }

        // 이동방향. 애니메이션 업데이트 추가.
        public override MoveDir Dir
        {
            get { return base.Dir; }
            set
            {
                if (base.Dir == value)
                    return;
                base.Dir = value;
            }
        }

        /* 스탯 */
        StatInfo _stat = new StatInfo();
        public StatInfo Stat
        {
            get { return _stat; }
            set
            {
                if (_stat == value)
                    return;
                _stat.MergeFrom(value);
                Hp = value.Hp;
            }
        }

        public virtual float Speed
        {
            get { return Stat.Speed; }
            set { Stat.Speed = value; }
        }

        public virtual int Hp
        {
            get { return Stat.Hp; }
            set
            {
                Stat.Hp = value;
            }
        }


        /* util */
        public override bool IsAlive
        {
            get
            {
                if (AutoMode == AutoMode.ModeNone)
                {
                    switch (State)
                    {
                        case CreatureState.Dead:
                        case CreatureState.Loading:
                            return false;
                        default:
                            return true;
                    }
                }
                else
                {
                    if (State == CreatureState.Loading)
                        return false;
                    if (Auto.State == AutoState.AutoDead)
                        return false;
                    return true;
                }
            }
        }
        public override bool IsDead
        {
            get
            {
                if (AutoMode == AutoMode.ModeNone && State == CreatureState.Dead)
                    return true;
                else if (AutoMode == AutoMode.ModeAuto && Auto.State == AutoState.AutoDead)
                    return true;
                return false;
            }
        }




        /* 스킬 */
        // 사용가능한 스킬정보
        Dictionary<SkillId, SkillUseInfo> _skillset = new Dictionary<SkillId, SkillUseInfo>();
        public Dictionary<SkillId, SkillUseInfo> Skillset { get { return _skillset; } }


        /* Auto */
        AutoInfo _autoInfo = new AutoInfo();       // Auto 데이터
        public AutoInfo AutoInfo
        {
            get { return _autoInfo; }
            set
            {
                if (_autoInfo == value)
                    return;
                _autoInfo.MergeFrom(value);
            }
        }
        public AutoMove Auto { get; private set; } = new AutoMove();     // 자동이동 할 때 사용할 데이터


        public override void Init(ObjectInfo info)
        {
            _info.StatInfo = _stat;
            _info.AutoInfo = _autoInfo;

            base.Init(info);

            Auto.Init(this);
        }

        public override void Update()
        {
            if (State == CreatureState.Loading)
            {
                UpdateLoading();
            }
            else if (AutoMode == AutoMode.ModeNone)
            {
                switch (State)
                {
                    case CreatureState.Idle:
                        UpdateIdle();
                        break;
                    case CreatureState.Moving:
                        UpdateMoving();
                        break;
                    case CreatureState.Dead:
                        UpdateDead();
                        break;
                }
            }
            else if (AutoMode == AutoMode.ModeAuto)
            {
                switch (Auto.State)
                {
                    case AutoState.AutoIdle:
                        UpdateAutoIdle();
                        break;
                    case AutoState.AutoChasing:
                        UpdateAutoChasing();
                        break;
                    case AutoState.AutoMoving:
                        UpdateAutoMoving();
                        break;
                    case AutoState.AutoSkill:
                        UpdateAutoSkill();
                        break;
                    case AutoState.AutoDead:
                        UpdateAutoDead();
                        break;
                    case AutoState.AutoWait:
                        UpdateAutoWait();
                        break;
                }
            }
        }


        // Idle 상태 업데이트
        protected override void UpdateIdle()
        {

        }

        // Moving 상태 업데이트
        protected override void UpdateMoving()
        {
        }


        // 서버에서 받은 AutoMove 패킷을 세팅한다.
        public void SetAutoMove(S_AutoMove autoPacket)
        {
            SetAutoMove(autoPacket.AutoInfo, autoPacket.PosInfo);
        }
        public void SetAutoMove(AutoInfo autoInfo, PositionInfo posInfo)
        {
            if (AutoMode != AutoMode.ModeAuto)
                AutoMode = AutoMode.ModeAuto;
        }

        protected virtual void UpdateAutoIdle()
        {
        }

        protected virtual void UpdateAutoChasing()
        {
        }

        protected virtual void UpdateAutoMoving()
        {
        }

        protected virtual void UpdateAutoSkill()
        {
        }

        protected virtual void UpdateAutoDead()
        {
        }


        protected virtual void UpdateAutoWait()
        {
        }

        // 스킬 사용됨
        public virtual void OnSkill(SkillId skillId)
        {
        }

        // 피격됨
        public virtual void OnDamaged(CreatureController attacker, int damage)
        {
        }
        public virtual void OnDamaged(int damage)
        {
        }

        // 사망함
        public virtual void OnDead()
        {
            State = CreatureState.Dead;
            Auto.State = AutoState.AutoDead;
        }
    }
}
