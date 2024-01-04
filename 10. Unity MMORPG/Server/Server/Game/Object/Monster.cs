using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Server.Data;
using static System.Net.Mime.MediaTypeNames;

namespace Server.Game
{
    public class Monster : GameObject
    {
        public Monster()
        {
            ObjectType = GameObjectType.Monster;

            // temp
            Stat.Level = 1;
            Stat.Hp = 100;
            Stat.MaxHp = 100;
            Stat.Speed = 5.0f;

            State = CreatureState.Idle;
        }

        // FSM (Finite State Machine)
        public override void Update()
        {
            base.Update();
        }

        Player _target;
        int _searchCellDist = 5;
        int _chaseCellDist = 10;
        long _nextSearchTick = 0;
        protected override void UpdateIdle()
        {
            if (_nextSearchTick > Environment.TickCount64)
                return;
            _nextSearchTick = Environment.TickCount64 + 1000;

            // 타겟 플레이어 찾기
            Player target = Room.FindPlayer(p =>
            {
                Vector2Int dir = p.Cell - Cell;
                return dir.cellDistFromZero <= _searchCellDist;
            });
            if (target == null)
                return;

            _target = target;
            State = CreatureState.Moving;

        }

        long _nextMoveTick = 0;
        int _skillRange = 1;
        protected override void UpdateMoving()
        {
            if (_nextMoveTick > Environment.TickCount64)
                return;
            int moveTick = (int)(1000 / Speed);
            _nextMoveTick = Environment.TickCount64 + moveTick;

            // 타겟이 없거나 다른공간에 있는경우 종료
            if(_target == null || _target.Room != Room || _target.Hp == 0)
            {
                _target = null;
                State = CreatureState.Idle;
                BroadcastMove();
                return;
            }

            // 타겟이 너무 멀리 있을 경우 종료
            Vector2Int dir = _target.Cell - Cell;
            int dist = dir.cellDistFromZero;
            if (dist == 0 || dist > _chaseCellDist)
            {
                _target = null;
                State = CreatureState.Idle;
                BroadcastMove();
                return;
            }

            // 타겟까지의 길 찾기
            List<Vector2Int> path = Room.Map.FindPath(Cell, _target.Cell, checkObjects: false);
            // 길이 없거나(path.Count < 2) 너무 멀리 있을경우 종료
            if (path.Count < 2 || path.Count > _chaseCellDist)
            {
                _target = null;
                State = CreatureState.Idle;
                BroadcastMove();
                return;
            }

            // 사정거리 내라면 스킬 사용
            if(dist <= _skillRange && (dir.x == 0 || dir.y == 0))
            {
                _coolTick = 0;
                //State = CreatureState.Skill;
                return;
            }

            // 이동
            Dir = GetDirFromVec(path[1]);
            Room.Map.TryMove(this, path[1]);
            Logger.WriteLog(LogLevel.Debug, $"Monster.UpdateMoving. Id:{Id}, pos:{Cell}");

            // 이동패킷 전송
            BroadcastMove();
        }

        // 이동패킷 전송
        void BroadcastMove()
        {
            S_Move movePacket = new S_Move();
            movePacket.ObjectId = Id;
            movePacket.PosInfo = PosInfo;
            Room._broadcast(movePacket);
        }

        long _coolTick = 0;
        protected override void UpdateSkill()
        {
            if(_coolTick == 0)
            {
                // 유효한 타겟인지 체크
                if (_target == null || _target.Room != Room || _target.Hp == 0)
                {
                    _target = null;
                    State = CreatureState.Moving;
                    BroadcastMove();
                    return;
                }

                // 스킬 범위 체크
                Vector2Int dir = (_target.Cell - Cell);
                int dist = dir.cellDistFromZero;
                bool canUseSkill = (dist <= _skillRange && (dir.x == 0 || dir.y == 0));
                if(canUseSkill == false)
                {
                    State = CreatureState.Moving;
                    BroadcastMove();
                    return;
                }

                // 타겟 바라보기
                MoveDir lookDir = GetDirFromVec(_target.Cell);
                if(Dir != lookDir)
                {
                    Dir = lookDir;
                    BroadcastMove();
                }

                // 피격 판정
                Skill skillData = null;
                DataManager.SkillDict.TryGetValue(1, out skillData);
                _target.OnDamaged(this, skillData.damage + Stat.Attack);
                Logger.WriteLog(LogLevel.Debug, $"Monster.UpdateSkill. Id:{Id}, pos:{Cell}, targetPos:{_target.Cell}");

                // 스킬 사용 패킷 전송
                S_Skill skill = new S_Skill() { Info = new SkillInfo() };
                skill.ObjectId = Id;
                skill.Info.SkillId = skillData.id;
                Room._broadcast(skill);

                // 스킬 쿨타임 적용
                int coolTick = (int)(1000 * skillData.cooldown);
                _coolTick = Environment.TickCount64 + coolTick;
            }

            if (_coolTick > Environment.TickCount64)
                return;
            _coolTick = 0;
        }

        protected override void UpdateDead()
        {

        }
    }
}
