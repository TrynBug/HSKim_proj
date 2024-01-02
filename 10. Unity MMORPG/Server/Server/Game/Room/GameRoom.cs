using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using Google.Protobuf.WellKnownTypes;
using Server.Data;
using ServerCore;

namespace Server.Game
{
    // 게임룸
    // JobSerializer를 사용하여 모든 Job을 단일스레드 내에서 처리한다.
    // JobSerializer를 사용하기 때문에 모든 작업은 직접 호출되어서는 안되고 JobSerializer에 삽입되어 처리되어야 한다.
    // 그리고 전송할 패킷을 _pendingList에 넣어두었다 Flush 함수에 의해 한번에 전송하도록 구현되어있다.
    // 때문에 Flush 함수를 누군가 주기적으로 JobSerializer에 넣어주어야 한다.
    public class GameRoom : JobSerializer
    {
        public int RoomId { get; set; }

        // object
        Dictionary<int, Player> _players = new Dictionary<int, Player>();
        Dictionary<int, Monster> _monsters = new Dictionary<int, Monster>();
        Dictionary<int, Projectile> _projectiles = new Dictionary<int, Projectile>();

        // map
        public Map Map { get; private set; } = new Map();

        // pos
        public Vector2 PosCenter { get { return Util.CellToCenterPos(CellCenter); } }
        public Vector2Int CellCenter { get { return new Vector2Int(Map.CellMaxX / 2, Map.CellMaxY / 2); } }


        // push job
        public void Init(int mapId) { Push(_init, mapId); }
        public void EnterGame(GameObject gameObject) { Push(_enterGame, gameObject); }
        public void LeaveGame(int objectId) { Push(_leaveGame, objectId); }
        public void Update() { Push(_update); }
        public void Broadcast(IMessage packet) { Push(_broadcast, packet); }
        public void HandleMove(Player player, C_Move movePacket) { Push(_handleMove, player, movePacket); }
        public void HandleSkill(Player player, C_Skill skillPacket) { Push(_handleSkill, player, skillPacket); }



        // 조건에 맞는 플레이어 찾기
        // TODO. thread unsafe
        public Player FindPlayer(Func<GameObject, bool> condition)
        {
            foreach (Player player in _players.Values)
            {
                if (condition.Invoke(player))
                    return player;
            }
            return null;
        }

        public void _init(int mapId)
        {
            Map.LoadMap(mapId, "../../../../Common/Map");

            // temp
            Monster monster = ObjectManager.Instance.Add<Monster>();
            _enterGame(monster);
        }

        public void _update()
        {
            Flush();  // job queue 내의 모든 job 실행

            foreach (Player player in _players.Values)
            {
                player.Update();
            }
            foreach (Monster monster in _monsters.Values)
            {
                monster.Update();
            }
            foreach (Projectile projectile in _projectiles.Values)
            {
                projectile.Update();
            }

            Flush();  // job queue 내의 모든 job 실행
        }



        // 오브젝트가 게임룸에 들어옴
        public void _enterGame(GameObject gameObject)
        {
            if (gameObject == null)
            {
                Logger.WriteLog(LogLevel.Error, $"GameRoom.EnterGame. GameObject is null");
                return;
            }

            GameObjectType type = ObjectManager.GetObjectTypeById(gameObject.Id);
            if(type == GameObjectType.Player)
            {
                Player newPlayer = gameObject as Player;
                newPlayer.Room = this;
                Vector2Int emptyCell;
                if(Map.GetEmptyCell(newPlayer.Cell, true, out emptyCell) == false)
                {
                    Logger.WriteLog(LogLevel.Error, $"GameRoom.EnterGame. No empty cell in the map. {newPlayer}");
                    return;
                }
                newPlayer.Pos = Util.CellToCenterPos(emptyCell);
                _players.Add(newPlayer.Id, newPlayer);

                // 맵에 추가
                Map.Add(newPlayer);

                // 플레이어에게 정보 전송
                {
                    // 내 정보 전송
                    S_EnterGame enterPacket = new S_EnterGame();
                    enterPacket.Player = newPlayer.Info;
                    newPlayer.Session.Send(enterPacket);

                    // 나에게 다른 플레이어, 몬스터, 화살 정보 전송
                    S_Spawn spawnPacket = new S_Spawn();
                    foreach (Player p in _players.Values)
                        if (newPlayer != p)
                            spawnPacket.Objects.Add(p.Info);

                    foreach(Monster m in _monsters.Values)
                        spawnPacket.Objects.Add(m.Info);

                    foreach (Projectile p in _projectiles.Values)
                        spawnPacket.Objects.Add(p.Info);

                    newPlayer.Session.Send(spawnPacket);
                }
            }
            else if (type == GameObjectType.Monster)
            {
                Monster monster = gameObject as Monster;
                monster.Room = this;
                Vector2Int emptyCell;
                if (Map.GetEmptyCell(monster.Cell, true, out emptyCell) == false)
                {
                    Logger.WriteLog(LogLevel.Error, $"GameRoom.EnterGame. No empty cell in the map. objectId:{monster.Id}");
                    return;
                }
                monster.Pos = Util.CellToCenterPos(emptyCell);
                _monsters.Add(monster.Id, monster);

                // 맵에 추가
                Map.Add(monster);
            }
            else if (type == GameObjectType.Projectile)
            {
                Projectile projectile = gameObject as Projectile;
                projectile.Room = this;
                _projectiles.Add(projectile.Id, projectile);
            }

            // 다른 플레이어에게 정보 전송
            {
                S_Spawn spawnPacket = new S_Spawn();
                spawnPacket.Objects.Add(gameObject.Info);
                foreach (Player p in _players.Values)
                {
                    if (p.Id != gameObject.Id)
                        p.Session.Send(spawnPacket);
                }
            }

            Logger.WriteLog(LogLevel.Debug, $"GameRoom.EnterGame. {gameObject.ToString(InfoLevel.All)}");
            
        }

        // 플레이어가 게임룸에서 나감
        public void _leaveGame(int objectId)
        {
            GameObjectType type = ObjectManager.GetObjectTypeById(objectId);

            if (type == GameObjectType.Player)
            {
                Player player = null;
                if (_players.Remove(objectId, out player) == false)
                {
                    Logger.WriteLog(LogLevel.Error, $"GameRoom.LeaveGame. Can't find player. id:{objectId}");
                    return;
                }
                player.Room = null;

                // 맵에서 제거
                Map.Remove(player);

                // 플레이어에게 정보 전송
                {
                    S_LeaveGame leavePacket = new S_LeaveGame();
                    player.Session.Send(leavePacket);
                }
            }
            else if (type == GameObjectType.Monster)
            {
                Monster monster = null;
                if (_monsters.Remove(objectId, out monster) == false)
                {
                    Logger.WriteLog(LogLevel.Error, $"GameRoom.LeaveGame. Can't find monster. id:{objectId}");
                    return;
                }
                monster.Room = null;

                // 맵에서 제거
                Map.Remove(monster);
            }
            else if (type == GameObjectType.Projectile)
            {
                Projectile projectile = null;
                if (_projectiles.Remove(objectId, out projectile) == false)
                {
                    Logger.WriteLog(LogLevel.Error, $"GameRoom.LeaveGame. Can't find projectile. id:{objectId}");
                    return;
                }
                projectile.Room = null;
            }



            // 다른 플레이어에게 정보 전송
            {
                S_Despawn despawnPacket = new S_Despawn();
                despawnPacket.ObjectIds.Add(objectId);
                foreach (Player p in _players.Values)
                {
                    if (p.Id != objectId)
                        p.Session.Send(despawnPacket);
                }
            }

            Logger.WriteLog(LogLevel.Debug, $"GameRoom.LeaveGame. ObjectId:{objectId}, type:{type}");
        }


        // 패킷 브로드캐스팅
        public void _broadcast(IMessage packet)
        {
            foreach (Player p in _players.Values)
            {
                p.Session.Send(packet);
            }
        }


        // 이동요청 처리
        public void _handleMove(Player player, C_Move movePacket)
        {
            if (player == null)
                return;


            PositionInfo movePosInfo = movePacket.PosInfo;
            ObjectInfo info = player.Info;
            Vector2Int destCell = Util.PosToCell(new Vector2(movePosInfo.PosX, movePosInfo.PosY));

            // cell 이동을 시도한다. (cell 위치가 같아도 성공임)
            bool isMoved = Map.TryMove(player, destCell);
            if (isMoved == false)
                return;


            // cell 이동 성공시 위치이동
            player.Pos = new Vector2(movePosInfo.PosX, movePosInfo.PosY);
            player.Dir = movePosInfo.MoveDir;
            player.State = movePosInfo.State;

            // 게임룸 내의 모든 플레이어들에게 브로드캐스팅
            S_Move resMovePacket = new S_Move();
            resMovePacket.ObjectId = info.ObjectId;
            resMovePacket.PosInfo = info.PosInfo;
            _broadcast(resMovePacket);
        }

        // 스킬사용요청 처리
        public void _handleSkill(Player player, C_Skill skillPacket)
        {
            if (player == null)
                return;

            ObjectInfo info = player.Info;
            if (info.PosInfo.State != CreatureState.Idle)
            {
                Logger.WriteLog(LogLevel.Error, $"GameRoom.HandleSkill. Player state is not in Idle. " +
                    $"objectId:{info.ObjectId}, state:{info.PosInfo.State}, SkillId:{skillPacket.Info.SkillId}");
                return;
            }

            // 플레이어 상태변경
            info.PosInfo.State = CreatureState.Skill;

            // 게임룸 내의 모든 플레이어들에게 브로드캐스팅
            S_Skill resSkillPacket = new S_Skill() { Info = new SkillInfo() };
            resSkillPacket.ObjectId = info.ObjectId;
            resSkillPacket.Info.SkillId = skillPacket.Info.SkillId;
            _broadcast(resSkillPacket);

            // 스킬데이터 가져오기
            Data.Skill skillData = null;
            if(DataManager.SkillDict.TryGetValue(skillPacket.Info.SkillId, out skillData) == false)
            {
                Logger.WriteLog(LogLevel.Error, $"GameRoom.HandleSkill. Can't find skill data. objectId:{info.ObjectId}, skillId:{skillPacket.Info.SkillId}");
                return;
            }


            // 타입별 스킬사용
            switch(skillData.skillType)
            {
                case SkillType.SkillAuto:
                    {
                        // 펀치 스킬
                        Vector2Int skillPos = player.GetFrontCellPos(info.PosInfo.MoveDir);
                        GameObject target = Map.Find(skillPos);
                        if (target != null)
                        {
                            Console.WriteLine("Hit !");
                        }
                    }
                    break;

                case SkillType.SkillProjectile:
                    {
                        // 화살 스킬
                        Arrow arrow = ObjectManager.Instance.Add<Arrow>();
                        if (arrow == null)
                        {
                            Logger.WriteLog(LogLevel.Error, $"GameRoom.HandleSkill. Failed to create arrow. objectId:{info.ObjectId}");
                            return;
                        }

                        arrow.Owner = player;
                        arrow.Data = skillData;
                        arrow.PosInfo.State = CreatureState.Moving;
                        arrow.PosInfo.MoveDir = player.PosInfo.MoveDir;
                        arrow.PosInfo.PosX = player.PosInfo.PosX;
                        arrow.PosInfo.PosY = player.PosInfo.PosY;
                        arrow.Speed = skillData.projectile.speed;
                        _enterGame(arrow);
                    }
                    break;
            }
        }
    }
}
