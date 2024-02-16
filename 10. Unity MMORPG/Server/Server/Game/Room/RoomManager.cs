using System;
using System.Collections.Generic;
using System.Diagnostics.Metrics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
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

        ReaderWriterLockSlim _rwLock = new ReaderWriterLockSlim();
        Dictionary<int, GameRoom> _rooms = new Dictionary<int, GameRoom>();
        Random _rand = new Random();

        public void Init()
        {
            LoginRoom.Init();
        }


        public void RunAllRooms()
        {
            _rwLock.EnterReadLock();
            try
            {
                LoginRoom.Run();
                foreach (GameRoom room in _rooms.Values)
                {
                    room.Run();
                    Logger.WriteLog(LogLevel.Debug, $"RoomManager.RunAllRooms. Start room {room.RoomId}");
                }
                
            }
            finally
            {
                _rwLock.ExitReadLock();
            }
        }

        public GameRoom Add(int mapId)
        {
            GameRoom gameRoom = new GameRoom();
            gameRoom.RoomId = mapId;
            gameRoom.Init(mapId);

            _rwLock.EnterWriteLock();
            try
            {
                _rooms.Add(gameRoom.RoomId, gameRoom);
            }
            finally
            {
                _rwLock.ExitWriteLock();
            }
            return gameRoom;
        }

        public bool Remove(int roomId)
        {
            _rwLock.EnterWriteLock();
            try
            {
                bool deleted = _rooms.Remove(roomId);
                if (deleted == false)
                    Logger.WriteLog(LogLevel.Error, $"RoomManager.Remove. Can't find the room. id:{roomId}");

                return deleted;
            }
            finally
            {
                _rwLock.ExitWriteLock();
            }
        }

        public GameRoom Find(int roomId)
        {
            _rwLock.EnterReadLock();
            try
            {
                GameRoom room = null;
                if (_rooms.TryGetValue(roomId, out room))
                    return room;

                return null;
            }
            finally
            {
                _rwLock.ExitReadLock();
            }
        }

        public GameRoom GetRandomRoom()
        {
            GameRoom room;
            _rwLock.EnterReadLock();
            try
            {
                int roomId = _rand.Next(0, _rooms.Count);
                room = _rooms.GetValueOrDefault(roomId, null);
            }
            finally
            {
                _rwLock.ExitReadLock();
            }
            return room;
        }

        public int GetGamePlayerCount()
        {
            int count = 0;
            _rwLock.EnterReadLock();
            try
            {
                foreach (GameRoom room in _rooms.Values)
                {
                    count += room.PlayerCount;
                }
            }
            finally
            {
                _rwLock.ExitReadLock();
            }
            return count;
        }

        public int GetUpdateRoomCount()
        {
            int count = 0;
            _rwLock.EnterReadLock();
            try
            {
                foreach (GameRoom room in _rooms.Values)
                {
                    if (room.IsRoomUpdate)
                        count++;
                }
            }
            finally
            {
                _rwLock.ExitReadLock();
            }
            return count;
        }

        public RoomState GetRoomState()
        {
            _rwLock.EnterReadLock();
            RoomState state = new RoomState();
            try
            {
                foreach (GameRoom room in _rooms.Values)
                {
                    state.FPS.Update(room.RoomId, room.Time.AvgFPS1s);
                    state.Player.Update(room.RoomId, room.PlayerCount);
                    state.DBJobQueue.Update(room.RoomId, room.DBJobCount);
                    state.DBJob1s.Update(room.RoomId, (int)room.DBExecutedQueryCount1s);
                }
                return state;
            }
            finally
            {
                _rwLock.ExitReadLock();
            }
        }

    }


    public class RoomState
    {
        public StateValueFloat FPS = new StateValueFloat();
        public StateValueInt Player = new StateValueInt();
        public StateValueInt DBJobQueue = new StateValueInt();
        public StateValueInt DBJob1s = new StateValueInt();
    }

    public class StateValueFloat
    {
        public int MinRoom = -1;
        public float Min = float.MaxValue;
        public int MaxRoom = -1;
        public float Max = float.MinValue;
        public float Sum = 0f;
        public int Count = 0;
        public float Avg { get { return Sum / (float)Count; } }

        public void Update(int roomId, float value)
        {
            if (value < Min)
            {
                MinRoom = roomId;
                Min = value;
            }
            if (value > Max)
            {
                MaxRoom = roomId;
                Max = value;
            }
            Sum += value;
            Count++;
        }
    }

    public class StateValueInt
    {
        public int MinRoom = -1;
        public int Min = int.MaxValue;
        public int MaxRoom = -1;
        public int Max = int.MinValue;
        public int Sum = 0;
        public int Count = 0;
        public float Avg { get { return (float)Sum / (float)Count; } }

        public void Update(int roomId, int value)
        {
            if (value < Min)
            {
                MinRoom = roomId;
                Min = value;
            }
            if (value > Max)
            {
                MaxRoom = roomId;
                Max = value;
            }
            Sum += value;
            Count++;
        }
    }

}
