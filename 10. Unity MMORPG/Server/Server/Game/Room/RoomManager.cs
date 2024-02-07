using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ServerCore;

namespace Server.Game
{
    internal class RoomManager
    {
        public static RoomManager Instance { get; } = new RoomManager();

        public LoginRoom LoginRoom { get; private set; } = new LoginRoom();
        public int RoomCount { get { return _rooms.Count; } }
        public int UpdateRoomCount { get { return GetUpdateRoomCount(); } }
        public int LoginPlayerCount { get { return LoginRoom.SessionCount; } }
        public int GamePlayerCount { get { return GetGamePlayerCount(); } }

        object _lock = new object();
        Dictionary<int, GameRoom> _rooms = new Dictionary<int, GameRoom>();
        Random _rand = new Random();

        public void Init()
        {
            LoginRoom.Init();
        }


        public void RunAllRooms()
        {
            LoginRoom.Run();
            lock (_lock)
            {
                foreach (GameRoom room in _rooms.Values)
                {
                    room.Run();
                    Logger.WriteLog(LogLevel.Debug, $"RoomManager.RunAllRooms. Start room {room.RoomId}");
                }
            }
        }

        public GameRoom Add(int mapId)
        {
            GameRoom gameRoom = new GameRoom();
            gameRoom.RoomId = mapId;
            gameRoom.Init(mapId);

            lock(_lock)
            {
                _rooms.Add(gameRoom.RoomId, gameRoom);
            }

            return gameRoom;
        }

        public bool Remove(int roomId)
        {
            lock(_lock)
            {
                bool deleted = _rooms.Remove(roomId);
                if(deleted == false)
                    Logger.WriteLog(LogLevel.Error, $"RoomManager.Remove. Can't find the room. id:{roomId}");

                return deleted;
            }
        }

        public GameRoom Find(int roomId)
        {
            lock(_lock)
            {
                GameRoom room = null;
                if (_rooms.TryGetValue(roomId, out room))
                    return room;

                return null;
            }
        }

        public GameRoom GetRandomRoom()
        {
            GameRoom room;
            lock (_lock)
            {
                int roomId = _rand.Next(0, _rooms.Count);
                room = _rooms.GetValueOrDefault(roomId, null);
            }
            return room;
        }

        public int GetGamePlayerCount()
        {
            int count = 0;
            lock (_lock)
            {
                foreach (GameRoom room in _rooms.Values)
                {
                    count += room.PlayerCount;
                }
            }
            return count;
        }

        public int GetUpdateRoomCount()
        {
            int count = 0;
            lock (_lock)
            {
                foreach (GameRoom room in _rooms.Values)
                {
                    if (room.IsRoomUpdate)
                        count++;
                }
            }
            return count;
        }

        public void PrintRoomFrame()
        {
            lock (_lock)
            {
                foreach (GameRoom room in _rooms.Values)
                {
                    Logger.WriteLog(LogLevel.System, $"room:{room.RoomId}, fps:{room.Time.AvgFPS1m}, delta:{room.Time.DeltaTime}");
                }
            }
        }
    }
}
