using ServerCore;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;


public class ServerSession : PacketSession
{
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
		ServerCore.Logger.WriteLog(LogLevel.Debug, $"ServerSession.OnConnected. endPoint:{endPoint}");
	}

	public override void OnDisconnected(EndPoint endPoint)
	{
        ServerCore.Logger.WriteLog(LogLevel.Debug, $"ServerSession.OnDisconnected. endPoint:{endPoint}");
    }

	public override void OnRecvPacket(ArraySegment<byte> buffer)
	{
        //ServerCore.Logger.WriteLog(LogLevel.Debug, $"ServerSession.OnRecvPacket. bytes:{buffer.Count}");

        // 네트워크 코드에서 패킷을 수신했을 경우 PacketQueue에 패킷을 삽입한다.
		// 유니티의 게임오브젝트에 접근하는 코드는 메인 스레드에서만 수행되어야 하는데 이 위치는 네트워크 스레드가 호출하는 코드이기 때문이다.
		// PacketQueue 내의 Update 함수에서 패킷 핸들러 함수를 호출한다.
        PacketManager.Instance.OnRecvPacket(this, buffer, (ushort id, PacketSession s, IMessage p) => { PacketQueue.Instance.Push(id, p); });
	}

	public override void OnSend(int numOfBytes)
	{
        //ServerCore.Logger.WriteLog(LogLevel.Debug, $"ServerSession.OnRecvPacket. bytes:{numOfBytes}");
	}
}