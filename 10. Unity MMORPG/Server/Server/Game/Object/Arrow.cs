using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Google.Protobuf.Protocol;
using ServerCore;

namespace Server.Game
{
    public class Arrow : Projectile
    {
        public GameObject Owner { get; set; }   // 화살을 발사한 오브젝트

        long _nextMoveTick = 0;

        public override void Update()
        {
            //if (Data == null || Data.projectile == null || Owner == null || Room == null)
            //{
            //    Logger.WriteLog(LogLevel.Error, $"Arrow.Update. Required data is missing. " +
            //        $"Data:{Data != null}, Data.Proj:{Data.projectile != null}, Owner:{Owner != null}, Room:{Room != null}");
            //    return;
            //}

            //// 1000/speed ms 마다 업데이트
            //// speed는 1초에 움직이는 거리임. 그래서 1000/speed ms 마다 위치를 1씩 증가시켜줌
            //long tick = (long)(1000 / Speed);  
            //if (_nextMoveTick >= Environment.TickCount64)
            //    return;
            //_nextMoveTick = Environment.TickCount64 + tick;

            //Vector2Int destPos = GetFrontCellPos();
            //if(Room.Map.CanGo(destPos))
            //{
            //    // 이동 처리
            //    CellPos = destPos;
            //    S_Move movePacket = new S_Move();
            //    movePacket.ObjectId = Id;
            //    movePacket.PosInfo = PosInfo;
            //    Room._broadcast(movePacket);
            //}
            //else
            //{
            //    // 충돌 처리
            //    GameObject target = Room.Map.Find(destPos);
            //    if(target != null)
            //    {
            //        // 피격 판정
            //        target.OnDamaged(this, Data.damage + Owner.Stat.Attack);

            //    }

            //    // 소멸
            //    Room._leaveGame(Id);
            //}
        }
    }
}
