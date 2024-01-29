using Google.Protobuf.Protocol;
using Google.Protobuf.WellKnownTypes;
using DummyClient.Data;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace DummyClient.Game
{
    public class PlayerController : CreatureController
    {
        public ServerSession Session { get; private set; }

        public void Init(ServerSession session, S_EnterGame enterPacket)
        {
            session.MyPlayer = this;
            Session = session;
            RoomId = enterPacket.RoomId;
            base.Init(enterPacket.Object);

            ObjectType = GameObjectType.Player;
        }


        public override void Update()
        {
            base.Update();
        }


        // Idle 상태 업데이트
        protected override void UpdateIdle()
        {
        }

    }
}
