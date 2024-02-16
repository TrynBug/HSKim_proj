using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Google.Protobuf.Protocol;
using ServerCore;

namespace Server.Game
{
    internal class ObjectManager
    {

        public static ObjectManager Instance { get; } = new ObjectManager();

        public int PlayerCount { get { return _playersObjectId.Count; } }


        ReaderWriterLockSlim _rwLock = new ReaderWriterLockSlim();
        Dictionary<int, Player> _playersObjectId = new Dictionary<int, Player>();
        Dictionary<int, Player> _playersAccountNo = new Dictionary<int, Player>();
        Dictionary<string, Player> _playersName = new Dictionary<string, Player>();



        // id에 해당하는 오브젝트의 타입 얻기
        public static GameObjectType GetObjectTypeById(int id)
        {
            int type = (id >> 24) & 0x7F;
            return (GameObjectType)type;
        }


        // 플레이어 추가
        public void AddPlayer(Player player)
        {
            _rwLock.EnterWriteLock();
            try
            {
                _playersObjectId.Add(player.Id, player);
                _playersAccountNo.Add(player.Session.AccountNo, player);
                _playersName.Add(player.Session.Name, player);
            }
            finally
            {
                _rwLock.ExitWriteLock();
            }
        }

        // 플레이어 제거
        public void RemovePlayer(Player player)
        {
            if (player == null)
                return;

            _rwLock.EnterWriteLock();
            try
            {
                if (_playersObjectId.Remove(player.Id) == false)
                {
                    Logger.WriteLog(LogLevel.Error, $"PlayerManager.Remove. Can't find the player by objectId. {player}");
                }
                if (_playersAccountNo.Remove(player.Session.AccountNo) == false)
                {
                    Logger.WriteLog(LogLevel.Error, $"PlayerManager.Remove. Can't find the player by accountNo. {player}");
                }
                if (_playersName.Remove(player.Session.Name) == false)
                {
                    Logger.WriteLog(LogLevel.Error, $"PlayerManager.Remove. Can't find the player by name. {player}");
                }
            }
            finally
            {
                _rwLock.ExitWriteLock();
            }
        }

        // 플레이어 찾기
        public Player FindByObjectId(int objectId)
        {
            _rwLock.EnterReadLock();
            try
            {
                return _playersObjectId.GetValueOrDefault(objectId, null);
            }
            finally
            {
                _rwLock.ExitReadLock();
            }
        }

        // 플레이어 찾기
        public Player FindByAccountNo(int accountNo)
        {
            _rwLock.EnterReadLock();
            try
            {
                return _playersAccountNo.GetValueOrDefault(accountNo, null);
            }
            finally
            {
                _rwLock.ExitReadLock();
            }
        }

        // 플레이어 찾기
        public Player FindByName(string name)
        {
            _rwLock.EnterReadLock();
            try
            {
                return _playersName.GetValueOrDefault(name, null);
            }
            finally
            {
                _rwLock.ExitReadLock();
            }
        }
    }
}
