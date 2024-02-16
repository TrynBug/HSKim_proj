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
        // session
        public int SessionId { get; set; }  // 클라이언트 세션ID

        // account
        public int AccountNo { get; set; }
        public string Name { get; set; }
        public string Password { get; set; }

        // login
        public bool LoginRequested { get; set; } = false; // login 요청받음 여부
        public int LastLoginRequest = 0;                 // 마지막으로 login 요청받은 시간
        public bool Login { get; set; } = false;         // login 여부
        

        // player
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
            // login room 입장
            RoomManager.Instance.LoginRoom.EnterRoom(this);
            Logger.WriteLog(LogLevel.Debug, $"ClientSession.OnConnected. sessionId:{SessionId}, endPoint:{endPoint}");
        }

        public override void OnDisconnected(EndPoint endPoint)
        {
            // 플레이어는 반드시 하나의 room에 속한다.
            // 연결끊긴 플레이어는 room에서 감지하고 제거한다.

            Logger.WriteLog(LogLevel.Debug, $"ClientSession.OnDisconnected. sessionId:{SessionId}, playerId:{MyPlayer?.Info.ObjectId}, endPoint:{endPoint}");
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