using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Google.Protobuf.Protocol;
using ServerCore;

namespace Server.Game
{
    internal class ObjectManager
    {

        public static ObjectManager Instance { get; } = new ObjectManager();

        object _lock = new object();
        Dictionary<int, Player> _players = new Dictionary<int, Player>();

        // bits : [ Unused(1) | Type(7) | Id(24) ]
        int _counter = 1;

        // id 생성
        int GenerateId(GameObjectType type)
        {
            lock (_lock)
            {
                return ((int)type << 24) | (_counter++);
            }
        }

        // id에 해당하는 오브젝트의 타입 얻기
        public static GameObjectType GetObjectTypeById(int id)
        {
            int type = (id >> 24) & 0x7F;
            return (GameObjectType)type;
        }


        // 오브젝트 추가
        public T Add<T>() where T : GameObject, new()
        {
            T gameObject = new T();

            lock(_lock)
            {
                gameObject.Id = GenerateId(gameObject.ObjectType);

                if(gameObject.ObjectType == GameObjectType.Player)
                {
                    Player player = gameObject as Player;
                    if(player == null)
                    {
                        Logger.WriteLog(LogLevel.Error, $"ObjectManager.Add. object is not a player type. objectId:{gameObject.Id}, type:{gameObject.GetType()}");
                        return null;
                    }
                    _players.Add(gameObject.Id, player);
                }
            }

            return gameObject;
        }

        // 오브젝트 제거
        public bool Remove(int objectId)
        {
            GameObjectType objectType = GetObjectTypeById(objectId);

            lock(_lock)
            {
                if (objectType == GameObjectType.Player)
                {
                    if (_players.Remove(objectId) == false)
                    {
                        Logger.WriteLog(LogLevel.Error, $"PlayerManager.Remove. Can't find the player. objectId:{objectId}");
                        return false;
                    }
                }
            }

            return true;
        }

        // 플레이어 찾기
        public Player Find(int objectId)
        {
            GameObjectType objectType = GetObjectTypeById(objectId);

            lock (_lock)
            {
                if (objectType == GameObjectType.Player)
                {
                    Player player = null;
                    if (_players.TryGetValue(objectId, out player))
                        return player;
                }
            }
            return null;
        }

    }
}
