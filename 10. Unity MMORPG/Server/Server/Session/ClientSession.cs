using Google.Protobuf;
using Google.Protobuf.Protocol;
using Server.Data;
using Server.Game;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public class ClientSession : PacketSession
    {
        public int SessionId { get; set; }  // 클라이언트 세션ID
        public Player MyPlayer { get; set; }  // 클라이언트와 연결된 플레이어

        // packet을 버퍼에 복사한 뒤 Send
        public void Send(IMessage packet)
        {
            // 패킷의 enum MsgId 값 찾기
            string msgName = packet.Descriptor.Name.Replace("_", string.Empty);  // 패킷의 클래스 이름을 가져온다. _ 문자는 제거한다.
            MsgId msgId = (MsgId)Enum.Parse(typeof(MsgId), msgName);   // enum MsgId 에서 msgName 과 동일한 이름의 값을 찾는다.

            // 버퍼에 패킷데이터 입력
            ushort size = (ushort)packet.CalculateSize();
            byte[] sendBuffer = new byte[size + 4];
            Array.Copy(BitConverter.GetBytes((ushort)(size + 4)), 0, sendBuffer, 0, sizeof(ushort));
            ushort protocolId = (ushort)msgId;
            Array.Copy(BitConverter.GetBytes(protocolId), 0, sendBuffer, 2, sizeof(ushort));
            Array.Copy(packet.ToByteArray(), 0, sendBuffer, 4, size);

            // send
            Send(new ArraySegment<byte>(sendBuffer));
        }

        public override void OnConnected(EndPoint endPoint)
        {
            // 플레이어 생성
            GameRoom room = RoomManager.Instance.Find(1);
            MyPlayer = ObjectManager.Instance.Add<Player>();
            MyPlayer.Init(this, room);
            Logger.WriteLog(LogLevel.Debug, $"ClientSession.OnConnected. sessionId:{SessionId}, playerId:{MyPlayer.Info.ObjectId}, endPoint:{endPoint}");

            // 1번 게임룸에 플레이어 추가
            room.EnterGame(MyPlayer);
        }

        public override void OnDisconnected(EndPoint endPoint)
        {
            Logger.WriteLog(LogLevel.Debug, $"ClientSession.OnDisconnected. sessionId:{SessionId}, playerId:{MyPlayer.Info.ObjectId}, endPoint:{endPoint}");
            GameRoom room = RoomManager.Instance.Find(MyPlayer.Room.RoomId);
            if (room != null)
            {
                room.LeaveGame(MyPlayer.Info.ObjectId);
            }
            ObjectManager.Instance.Remove(MyPlayer.Info.ObjectId);
            SessionManager.Instance.Remove(this);
        }

        public override void OnRecvPacket(ArraySegment<byte> buffer)
        {
            PacketManager.Instance.OnRecvPacket(this, buffer);
        }

        public override void OnSend(int numOfBytes)
        {
            //Logger.WriteLog(LogLevel.Debug, $"ClientSession.OnSend. id:{SessionId}, numByte:{numOfBytes}");
        }
    }
}
