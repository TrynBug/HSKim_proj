using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
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


        protected override void UpdateIdle()
        {
        }

        protected override void UpdateMoving()
        {

            Vector2 vecDir = (Dest - Pos).normalized;
            Vector2 pos = Pos + vecDir * Room.Time.DeltaTime * Speed;
            Vector2Int cell = Util.PosToCell(pos);
            if (Room.Map.IsEmptyCell(cell))
            {
                Pos = pos;
            }
            else
            {
                Dest = Pos;
            }






            //Vector2Int destCell = Util.PosToCell(new Vector2(movePosInfo.PosX, movePosInfo.PosY));

            //// cell 이동을 시도한다. (cell 위치가 같아도 성공임)
            //bool isMoved = Map.TryMove(player, destCell);
            //if (isMoved == false)
            //    return;
        }

        protected override void UpdateSkill()
        {
        }

        protected override void UpdateDead()
        {
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
