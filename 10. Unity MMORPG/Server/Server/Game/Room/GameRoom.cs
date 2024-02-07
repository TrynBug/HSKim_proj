using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Text;
using Google.Protobuf;
using Google.Protobuf.Protocol;
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
        public int MapId { get; private set; }
        public bool IsRoomUpdate { get; private set; }

        // FPS, Delta Time
        public Time Time { get; private set; } = new Time();
        class CalcFrame           // 프레임 시간 계산을 위한 값들
        {
            public long logicStartTime = 0;       // 로직 시작 tick
            public long logicEndTime = 0;         // 로직 종료 tick
            public long catchUpTime = 0;          // FPS를 유지하기위해 따라잡아야할 tick. 이 값만큼 덜 sleep 해야한다.
            public void Init() { logicStartTime = DateTime.Now.Ticks; }
        };
        CalcFrame _calcFrame = new CalcFrame();
        System.Timers.Timer _timer = new System.Timers.Timer();
        int _sleepMilliseconds = 0;

        // object
        Dictionary<int, Player> _players = new Dictionary<int, Player>();
        Dictionary<int, Skill> _skills = new Dictionary<int, Skill>();
        Dictionary<int, Projectile> _projectiles = new Dictionary<int, Projectile>();

        public int PlayerCount { get { return _players.Count; } }
        public int SkillCount { get { return _skills.Count; } }
        public int ProjectileCount { get { return _projectiles.Count; } }

        // map
        public Map Map { get; private set; } = new Map();

        // pos
        public Vector2 PosCenter { get { return Map.PosCenter; } }
        public Vector2Int CellCenter { get { return Map.CellCenter; } }

        /* push job */
        public void Init(int mapId) { Push(_init, mapId); }
        public void EnterGame(GameObject gameObject) { Push(_enterGame, gameObject); }
        public void EnterGame(RoomTransferInfo transferInfo) { Push(_enterGame, transferInfo); }
        public void LeaveGame(int objectId) { Push(_leaveGame, objectId); }
        public void Broadcast(IMessage packet, Player except = null) { Push(_broadcast, packet, except); }
        public void HandleMove(Player player, C_Move movePacket) { Push(_handleMove, player, movePacket); }
        public void HandleSkill(Player player, C_Skill skillPacket) { Push(_handleSkill, player, skillPacket); }
        public void HandleLoadFinish(Player player, C_LoadFinished loadPacket) { Push(_handleLoadFinish, player, loadPacket); }
        public void HandleRespawn(Player player, C_Respawn rdspawnPacket) { Push(_handleRespawn, player, rdspawnPacket); }
        public void HandleSetAuto(Player player, C_SetAuto autoPacket) { Push(_handleSetAuto, player, autoPacket); }
        public void HandleMapMove(Player player, int mapId) { Push(_handleMapMove, player, mapId); }

        /* util */
        public bool IsValidTarget(GameObject target)
        {
            if (target == null)
                return false;
            if (target.Room.RoomId != RoomId || target.IsAlive == false)
                return false;
            if(target.ObjectType == GameObjectType.Player)
            {
                Player player = target as Player;
                if (player == null)
                    return false;
                if (player.Session.IsDisconnected == true)
                    return false;
            }
            return true;
        }
        public bool IsSameRoom(GameObject target)
        {
            if (target == null)
                return false;
            if (target.Room.RoomId != RoomId)
                return false;
            return true;
        }
        public Player FindPlayer(int playerId)
        {
            return _players.GetValueOrDefault(playerId, null);
        }


        // center를 기준으로 가장 가까운 살아있는 오브젝트를 찾는다.
        // 찾지 못했으면 null을 리턴함
        public GameObject FindObjectNearbyCell(Vector2Int center, GameObject exceptObject = null)
        {
            // 현재 room에 플레이어가 5명 이하이면 전수조사하여 거리가 가장 가까운 플레이어를 리턴함
            float minDistance = float.MaxValue;
            GameObject obj = null;
            if(_players.Count <= 5)
            {
                foreach(Player player in _players.Values)
                {
                    if (player == exceptObject)
                        continue;
                    if (player.IsAlive == false)
                        continue;
                    float dist = (player.Cell - center).sqrtMagnitude;
                    if(dist < minDistance)
                    {
                        minDistance = dist;
                        obj = player;
                    }
                }
                return obj;
            }

            // center를 기준으로 가장 가까운 오브젝트를 찾는다.
            return Map._findObjectNearbyCell(center, exceptObject);
        }




        // FPS에 맞추어 게임룸을 주기적으로 업데이트한다.
        public void Run()
        {
            // 다음 로직의 시작시간은 [현재 로직 종료시간 + sleep한 시간] 이다.
            _calcFrame.logicStartTime = _calcFrame.logicEndTime + (_sleepMilliseconds * (TimeSpan.TicksPerSecond / 1000));
            Time.Update();

            try
            {
                _update();
            }
            catch (Exception ex)
            {
                Logger.WriteLog(LogLevel.Error, $"GameRoom Update Error. room:{this}, error:{ex}");
            }
            _sleepMilliseconds = _calcSleepTime();
            //Logger.WriteLog(LogLevel.Debug, $"room:{RoomId}, DT:{Time.DeltaTime}, FPS:{Time.AvgFPS1m}, sleep:{_sleepMilliseconds}, thread:{Thread.CurrentThread.ManagedThreadId}");

            _timer.Interval = Math.Max(1, _sleepMilliseconds);
            _timer.AutoReset = false;  // event가 주기적으로 호출되도록 하는것이 아니라 다음 호출시간을 매번 정해줄 것이기 때문에 AutoReset을 false로 한다.
            _timer.Enabled = true;     // timer를 재활성화 한다.

        }



        public void _init(int mapId)
        {
            MapId = mapId;
            Map.LoadMap(this);

            // temp
            //Monster monster = ObjectManager.Instance.Add<Monster>();
            //_enterGame(monster);

            _timer.Elapsed += ((s, e) => Run());
            _calcFrame.Init();
        }




        // frame update
        List<Player> _disconnectedPlayers = new List<Player>();
        List<Player> _respawnPlayers = new List<Player>();
        List<RoomTransferInfo> _leavingPlayers = new List<RoomTransferInfo>();
        List<Skill> _deadSkills = new List<Skill>();
        List<Projectile> _deadProjectiles = new List<Projectile>();
        public void _update()
        {
            IsRoomUpdate = true;
            Flush();  // job queue 내의 모든 job 실행

            // 플레이어 업데이트
            _leavingPlayers.Clear();
            _disconnectedPlayers.Clear();
            _respawnPlayers.Clear();
            foreach (Player player in _players.Values)
            {
                // 연결끊긴 플레이어는 따로처리함
                if(player.Session.IsDisconnected == true)
                {
                    _disconnectedPlayers.Add(player);
                    continue;
                }

                // 업데이트
                player.Update();

                // 리스폰 확인
                if(player.Respawn == true)
                {
                    _respawnPlayers.Add(player);
                    continue;
                }    

                // 텔레포트 확인
                TeleportData teleport = Map.GetTeleportUnderObject(player);
                if(teleport != null)
                {
                    int nextRoomId;
                    if(teleport.number == 0)
                        nextRoomId = (RoomId - 1) % DataManager.MapDict.Count;
                    else
                        nextRoomId = (RoomId + 1) % DataManager.MapDict.Count;
                    nextRoomId = nextRoomId < 0 ? DataManager.MapDict.Count - 1 : nextRoomId;
                    _leavingPlayers.Add(new RoomTransferInfo { prevRoomId = RoomId, prevTeleportId = teleport.number, nextRoomId = nextRoomId, player = player });
                    continue;
                }

                // 맵이동 확인
                if(player.MoveRoom == true)
                {
                    _leavingPlayers.Add(new RoomTransferInfo { prevRoomId = RoomId, prevTeleportId = 0, nextRoomId = player.NextRoomId, player = player });
                    player.MoveRoom = false;
                }
            }

            // 스킬 업데이트
            _deadSkills.Clear();
            foreach (Skill skill in _skills.Values)
            {
                skill.Update();

                // 삭제 확인
                if (skill.State == CreatureState.Dead)
                    _deadSkills.Add(skill);
            }




            // 투사체 업데이트
            _deadProjectiles.Clear();
            foreach (Projectile projectile in _projectiles.Values)
            {
                projectile.Update();

                // 삭제 확인
                if (projectile.State == CreatureState.Dead)
                    _deadProjectiles.Add(projectile);
            }


            // 연결끊긴 플레이어 처리
            foreach (Player player in _disconnectedPlayers)
            {
                _leaveGame(player.Id);
                ObjectManager.Instance.Remove(player.Id);
                SessionManager.Instance.Remove(player.Session);
            }

            // 리스폰 플레이어 처리
            foreach (Player player in _respawnPlayers)
            {
                // 현재 room에서 나감
                _leaveGame(player.Id);

                // 스탯 초기화
                player.RecalculateStat();
                player.State = CreatureState.Loading;
                player.Respawn = false;

                // room 변경
                player.Room = RoomManager.Instance.GetRandomRoom();

                // 다음 room에 들어감
                player.Room.EnterGame(player);
            }



            // 이동할 플레이어를 이동시킴
            foreach (RoomTransferInfo transfer in _leavingPlayers)
            {
                GameRoom nextRoom = RoomManager.Instance.Find(transfer.nextRoomId);
                if (nextRoom == null)
                {
                    Logger.WriteLog(LogLevel.Error, $"GameRoom._update. Can't find next room. nextRoom:{transfer.nextRoomId}, {this}, {transfer.player}");
                    continue;
                }

                // 현재 room에서 나감
                _leaveGame(transfer.player.Id);

                // 상태를 Loading으로 변경
                transfer.player.State = CreatureState.Loading;

                // room 변경
                transfer.player.Room = nextRoom;

                // 다음 room에 들어감
                nextRoom.EnterGame(transfer);
            }



            // 삭제될 스킬 삭제
            foreach (Skill skill in _deadSkills)
            {
                _skills.Remove(skill.Id);
            }

            // 삭제될 투사체 삭제
            foreach (Projectile projectile in _deadProjectiles)
            {
                _projectiles.Remove(projectile.Id);
            }

            IsRoomUpdate = false;
        }

        // FPS를 유지하기 위해 sleep 해야하는 시간을 계산한다.
        private int _calcSleepTime()
        {
            long oneFrameTime = TimeSpan.TicksPerSecond / Config.FPS;
            _calcFrame.logicEndTime = DateTime.Now.Ticks;
            long spentTime = Math.Max(0, _calcFrame.logicEndTime - _calcFrame.logicStartTime);
            long sleepTime = Math.Max(0, oneFrameTime - spentTime);
            int sleepMilliseconds = 0;
            // sleep 해야할 시간이 있는 경우
            if (sleepTime > 0)
            {
                // sleep 시간을 ms 단위로 변환하여 sleep 한다.
                sleepMilliseconds = (int)Math.Round((double)(sleepTime * 1000) / (double)TimeSpan.TicksPerSecond);
            }
            return sleepMilliseconds;
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
                // 맵의 빈공간을 찾는다.
                Player newPlayer = gameObject as Player;
                newPlayer.Room = this;
                Vector2Int emptyCell;
                if(Map.FindEmptyCell(newPlayer.Cell, checkObjects:true, out emptyCell) == false)
                {
                    Logger.WriteLog(LogLevel.Error, $"GameRoom.EnterGame. No empty cell in the map. {newPlayer}");
                    return;
                }
                newPlayer.Pos = Map.CellToCenterPos(emptyCell);
                newPlayer.Dest = newPlayer.Pos;
                
                // 맵에 추가
                if (Map.Add(newPlayer) == false)
                    return;

                // room에 추가
                _players.Add(newPlayer.Id, newPlayer);

                // 내 정보 전송
                S_EnterGame enterPacket = new S_EnterGame();
                enterPacket.Object = newPlayer.Info;
                foreach (SkillId skillId in newPlayer.Skillset.Keys)
                    enterPacket.SkillIds.Add(skillId);
                enterPacket.RoomId = RoomId;
                newPlayer.Session.Send(enterPacket);

                // 나에게 다른 플레이어 정보 전송
                {
                    S_Spawn spawnPacket = new S_Spawn();
                    foreach (Player p in _players.Values)
                        if (newPlayer != p)
                            spawnPacket.Objects.Add(p.Info);
                    newPlayer.Session.Send(spawnPacket);
                }


                // 다른 플레이어에게 내 정보 전송
                {
                    S_Spawn spawnPacket = new S_Spawn();
                    spawnPacket.Objects.Add(gameObject.Info);
                    foreach (Player p in _players.Values)
                    {
                        if (p.Id != gameObject.Id)
                            p.Session.Send(spawnPacket);
                    }
                }


                // 서버로딩 완료 세팅
                newPlayer.ServerSideLoading = true;
                if (newPlayer.IsLoadingFinished)
                {
                    S_LoadFinished loadPacket = new S_LoadFinished();
                    loadPacket.ObjectId = newPlayer.Id;
                    _broadcast(loadPacket);

                    newPlayer.State = CreatureState.Idle;
                    newPlayer.Auto.State = AutoState.AutoIdle;
                }
            }
            else if (type == GameObjectType.Projectile)
            {
                Projectile proj = gameObject as Projectile;
                _projectiles.Add(proj.Id, proj);

                // projectile 스폰 정보 전송
                {
                    S_SpawnSkill spawnPacket = new S_SpawnSkill();
                    spawnPacket.ObjectId = proj.Id;
                    spawnPacket.OwnerId = proj.Owner.Id;
                    spawnPacket.TargetId = -1;
                    spawnPacket.SkillId = proj.SkillData.id;
                    spawnPacket.PosInfo = proj.PosInfo;
                    _broadcast(spawnPacket);
                }
            }


            Logger.WriteLog(LogLevel.Debug, $"GameRoom.EnterGame. {gameObject.ToString(InfoLevel.All)}");   
        }

        // room 이동으로 플레이어가 들어왔을때 호출됨
        public void _enterGame(RoomTransferInfo transferInfo)
        {
            // 이동할 적절한 위치를 찾음
            Vector2 posEnterZone = Map.GetPosEnterZone(transferInfo.prevTeleportId == 0 ? 1 : 0);
            Player player = transferInfo.player;
            player.Room = this;
            player.Pos = posEnterZone;
            player.Dest = posEnterZone;
            player.State = CreatureState.Loading;

            // 이동
            _enterGame(player);
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

                // 맵에서 제거
                Map.Remove(player);

                // 플레이어에게 정보 전송
                {
                    S_LeaveGame leavePacket = new S_LeaveGame();
                    player.Session.Send(leavePacket);
                }
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
        public void _broadcast(IMessage packet, Player except = null)
        {
            foreach (Player p in _players.Values)
            {
                if (p == except)
                    continue;
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


            // 클라 pos와 서버 pos가 크게 다를 경우 처리?
            // TBD

            // 현재 이동 가능한 상태인지 확인
            // TBD


            // 목적지에 도착가능한지 확인하고 목적지를 설정한다.
            Vector2 dest = new Vector2(movePosInfo.DestX, movePosInfo.DestY);
            Vector2 intersection;
            if (player.Room.Map.CanGo(player.Pos, dest, out intersection))
            {
                player.Dest = dest; 
            }
            else
            {
                player.Dest = intersection;
            }

            // info update
            player.Dir = movePosInfo.MoveDir;
            player.RemoteState = movePosInfo.State;
            player.RemoteDir = movePosInfo.MoveDir;
            player.MoveKeyDown = movePosInfo.MoveKeyDown;


            // 게임룸 내의 모든 플레이어들에게 브로드캐스팅
            S_Move resMovePacket = new S_Move();
            resMovePacket.ObjectId = info.ObjectId;
            resMovePacket.PosInfo = movePacket.PosInfo.Clone();
            resMovePacket.PosInfo.PosX = player.Pos.x;
            resMovePacket.PosInfo.PosY = player.Pos.y;
            resMovePacket.PosInfo.DestX = player.Dest.x;
            resMovePacket.PosInfo.DestY = player.Dest.y;
            resMovePacket.MoveTime = movePacket.MoveTime;
            _broadcast(resMovePacket);
        }



        // 스킬사용요청 처리
        public void _handleSkill(Player player, C_Skill skillPacket)
        {
            if (player == null)
                return;

            // 스킬 사용이 가능한지 확인
            SkillUseInfo useInfo;
            if (player.CanUseSkill(skillPacket.SkillId, out useInfo) == false)
                return;

            // 타겟 찾기. Melee 스킬 같은 경우에는 타겟이 없을 수 있다.
            GameObject target = _players.GetValueOrDefault(skillPacket.TargetId, null);

            // 타겟이 범위내에 있는지 확인. 범위내에 없다면 타겟을 null로 한다.
            if (target != null)
            {
                if (Util.IsTargetInRectRange(player.Pos, player.LookDir, new Vector2(useInfo.skill.rangeX, useInfo.skill.rangeY), target.Pos) == false)
                {
                    target = null;
                }
            }

            // 스킬 객체 생성
            Skill skill = Skill.Generate(skillPacket.SkillId);
            if (skill == null)
                return;
            skill.Init(useInfo, player, target, skillPacket.Hits);
            _skills.Add(skill.Id, skill);

            // 스킬사용, 스킬사용패킷 broadcast
            player.OnSkill(useInfo);


            ServerCore.Logger.WriteLog(LogLevel.Debug, $"GameRoom._handleSkill. {player}, skill:{skillPacket.SkillId}");
        }



        // 클라이언트의 로딩완료 요청 처리
        public void _handleLoadFinish(Player player, C_LoadFinished loadPacket)
        {
            if (player == null)
                return;

            // 로딩 완료되면 로딩완료 패킷 전송
            player.ClientSideLoading = true;
            if(player.IsLoadingFinished)
            {
                player.State = CreatureState.Idle;
                player.Auto.State = AutoState.AutoIdle;

                S_LoadFinished resPacket = new S_LoadFinished();
                resPacket.ObjectId = player.Id;
                _broadcast(resPacket);
            }

            // 만약 사망한 상태라면 사망패킷 전송
            if (player.Hp <= 0)
                player.OnDead(player);


            ServerCore.Logger.WriteLog(LogLevel.Debug, $"GameRoom._handleLoadFinish. {player}");
        }



        // 클라이언트의 부활요청 처리
        public void _handleRespawn(Player player, C_Respawn respawnPacket)
        {
            _handleRespawn(player);
        }
        public void _handleRespawn(Player player)
        {
            if (player == null)
                return;

            if (player.IsDead == false)
                return;

            // respawn flag 세팅
            // GameRoom의 update 함수에서 나중에 respawn을 처리함
            player.Respawn = true;

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"GameRoom._handleRespawn. {player}");
        }


        // Auto 설정요청 처리
        public void _handleSetAuto(Player player, C_SetAuto autoPacket)
        {
            if (player == null)
                return;

            if (player.SetAutoMode(autoPacket.Mode) == false)
                return;

            // auto 패킷 전송
            S_SetAutoResponse responsePacket = new S_SetAutoResponse();
            responsePacket.ObjectId = player.Id;
            responsePacket.Mode = autoPacket.Mode;
            _broadcast(responsePacket);

            Logger.WriteLog(LogLevel.Debug, $"PacketHandler.C_SetAutoHandler. objectId:{player.Id}, mode:{autoPacket.Mode}");
        }




        // map 이동요청 처리
        public void _handleMapMove(Player player, int mapId)
        {
            if (player == null)
                return;

            MapData map = DataManager.MapDict.GetValueOrDefault(mapId, null);
            if (map == null)
                return;

            player.MoveRoom = true;
            player.NextRoomId = mapId;
        }



        // ToString
        public override string ToString()
        {
            return $"room:{RoomId}";
        }
    }
}
