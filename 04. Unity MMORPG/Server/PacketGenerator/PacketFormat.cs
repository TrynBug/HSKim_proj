using System;
using System.Collections.Generic;
using System.Text;

namespace PacketGenerator
{
    class PacketFormat
    {
        // {0} 패킷 등록
        public static string managerFormat =
@"using Google.Protobuf;
using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections.Generic;

/*
이 파일은 PacketGenerator에 의해 자동으로 생성되었습니다.
*/

// 수신한 데이터를 패킷으로 변환하고, 패킷ID에 따른 핸들러 함수 호출을 관리하는 클래스
class PacketManager
{{
	#region Singleton
	static PacketManager _instance = new PacketManager();
	public static PacketManager Instance {{ get {{ return _instance; }} }}
	#endregion

	PacketManager()
	{{
		Register();
	}}

	// key:패킷ID, value:buffer를 패킷으로 변환하는 함수
	Dictionary<ushort, Func<PacketSession, ArraySegment<byte>, IMessage>> _makeFunc = new Dictionary<ushort, Func<PacketSession, ArraySegment<byte>, IMessage>>();
	// key:패킷ID, value:패킷 수신시 호출할 함수
	Dictionary<ushort, Action<PacketSession, IMessage>> _handler = new Dictionary<ushort, Action<PacketSession, IMessage>>();
		
    // 초기화
    public void Init()
    {{
        Register();
    }}

	// 패킷ID별 handler 함수를 미리 등록한다. 이 함수는 프로그램이 시작될 때 미리 호출해야 한다.
	public void Register()
	{{{0}
	}}

    // buffer에서 패킷ID를 추출하고, 패킷ID에 해당하는 패킷을 생성한다(_makeFunc에서 찾은 MakePacket<T> 함수를 호출하여).
    // 그런다음 실제 패킷을 처리하는 핸들러 함수를 호출한다(_handler에서 찾은 핸들러함수를 호출하여).
    // 만약 onRecvCallback 인자가 null이 아니라면 핸들러 함수 대신 onRecvCallback을 호출한다.
    public void OnRecvPacket(PacketSession session, ArraySegment<byte> buffer, Action<ushort, PacketSession, IMessage> onRecvCallback = null)
    {{
        ushort count = 0;
        ushort size = BitConverter.ToUInt16(buffer.Array, buffer.Offset);
        count += 2;
        ushort id = BitConverter.ToUInt16(buffer.Array, buffer.Offset + count);
        count += 2;

        Func<PacketSession, ArraySegment<byte>, IMessage> func;
        if (_makeFunc.TryGetValue(id, out func))
        {{
            IMessage packet = func.Invoke(session, buffer);
            if (onRecvCallback != null)
                onRecvCallback.Invoke(id, session, packet);
            else
                GetPacketHandler(id)?.Invoke(session, packet);
        }}
    }}

    // buffer를 T 타입 패킷으로 변환한다.
    T MakePacket<T>(PacketSession session, ArraySegment<byte> buffer) where T : IMessage, new()
    {{
        T packet = new T();
        packet.MergeFrom(buffer.Array, buffer.Offset + 4, buffer.Count - 4);
        return packet;
    }}

    // 패킷ID에 해당하는 핸들러함수를 얻는다.
    public Action<PacketSession, IMessage> GetPacketHandler(ushort msgId)
    {{
        Action<PacketSession, IMessage> action;
        if (_handler.TryGetValue(msgId, out action))
            return action;
        return null;
    }}
}}";

        // {0} MsgId
        // {1} 패킷 이름
        public static string managerRegisterFormat =
@"		
		_makeFunc.Add((ushort)MsgId.{0}, MakePacket<{1}>);
		_handler.Add((ushort)MsgId.{0}, PacketHandler.{1}Handler);";

    }
}