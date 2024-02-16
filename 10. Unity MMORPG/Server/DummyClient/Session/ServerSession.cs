using ServerCore;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using DummyClient.Game;
using DummyClient.Data;

namespace DummyClient
{
    public class ServerSession : PacketSession
    {
        public int SessionId { get; set; }
        public PlayerController MyPlayer { get; set; } = null;

        public string Name { get; set; }
        public string Password { get; set; }

        public int LastLoginRequestTime { get; set; } = 0;
        public bool Login { get; set; } = false;

        public void Connect()
        {
            IPAddress ipAddr = IPAddress.Parse(ConfigManager.Config.serverIP);
            IPEndPoint endPoint = new IPEndPoint(ipAddr, ConfigManager.Config.serverPort);

            Connector connector = new Connector();
            connector.Connect(endPoint,
                () => { return this; });
        }


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
            GameManager.Instance.OnConnected(this);

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"ServerSession.OnConnected. endPoint:{endPoint}");
        }

        public override void OnDisconnected(EndPoint endPoint)
        {
            ServerCore.Logger.WriteLog(LogLevel.Debug, $"ServerSession.OnDisconnected. endPoint:{endPoint}");
        }

        public override void OnRecvPacket(ArraySegment<byte> buffer)
        {
            PacketManager.Instance.OnRecvPacket(this, buffer);
        }

        public override void OnSend(int numOfBytes)
        {
            //ServerCore.Logger.WriteLog(LogLevel.Debug, $"ServerSession.OnRecvPacket. bytes:{numOfBytes}");
        }
    }
}
