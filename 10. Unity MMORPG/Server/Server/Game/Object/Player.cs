using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static System.Net.Mime.MediaTypeNames;

namespace Server.Game
{
    public class Player : GameObject
    {
        public ClientSession Session { get; set; }

        public Player()
        {
            ObjectType = GameObjectType.Player;
        }


        // 피격됨
        public override void OnDamaged(GameObject attacker, int damage)
        {
            base.OnDamaged(attacker, damage);

            Logger.WriteLog(LogLevel.Debug, $"Player.OnDamaged. myId:{Id}, attackerId:{attacker.Id}, damage:{damage}");
        }

        // 사망함
        public override void OnDead(GameObject attacker)
        {
            base.OnDead(attacker);

            Logger.WriteLog(LogLevel.Debug, $"Player.OnDead. myId:{Id}, attackerId:{attacker.Id}");
        }

    }



}
