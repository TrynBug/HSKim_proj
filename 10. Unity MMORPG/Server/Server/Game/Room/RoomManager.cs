using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ServerCore;

namespace Server.Game
{
    internal class RoomManager
    {
        public static RoomManager Instance { get; } = new RoomManager();

        public int RoomCount { get { return _rooms.Count; } }

        object _lock = new object();
        Dictionary<int, GameRoom> _rooms = new Dictionary<int, GameRoom>();

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


        public void RunAllRooms()
        {
            lock (_lock)
            {
                foreach (GameRoom room in _rooms.Values)
                {
                    room.Run();
                    Logger.WriteLog(LogLevel.Debug, $"RoomManager.RunAllRooms. Start room {room.RoomId}");
                }
            }
        }



    }
}
