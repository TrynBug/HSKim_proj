using Google.Protobuf;
using Google.Protobuf.Protocol;
using Server;
using Server.Game;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;


// 패킷 타입별로 호출할 핸들러 함수를 관리하는 클래스
// 클라이언트에게 받은 패킷을 처리하는 핸들러이기 때문에 C_ 로 시작하는 패킷명에 대한 핸들러 함수만 작성한다.
internal class PacketHandler
{
    // 클라이언트의 이동요청 처리
    public static void C_MoveHandler(PacketSession session, IMessage packet)
    {
        C_Move movePacket = packet as C_Move;
        ClientSession clientSession = session as ClientSession;

        Player player = clientSession.MyPlayer;
        if (player == null)
        {
            Logger.WriteLog(LogLevel.Error, $"PacketHandler.C_MoveHandler. ClientSession has no player. sessionId:{clientSession.SessionId}");
            return;
        }

        GameRoom room = player.Room;
        if(room == null)
        {
            Logger.WriteLog(LogLevel.Error, $"PacketHandler.C_MoveHandler. Player has no room. sessionId:{clientSession.SessionId}, {player}");
            return;
        }

        PositionInfo info = movePacket.PosInfo;
        Logger.WriteLog(LogLevel.Debug, $"PacketHandler.C_MoveHandler. sessionId:{clientSession.SessionId}, " +
            $"packet pos:({info.PosX:f2}, {info.PosY:f2}), dest:({info.DestX:f2},{info.DestY:f2}), state:{info.State}, dir:{info.MoveDir}, move:{info.MoveKeyDown}");

        room.HandleMove(player, movePacket);
    }


    // 클라이언트의 스킬사용 요청 처리
    public static void C_SkillHandler(PacketSession session, IMessage packet)
    {
        C_Skill skillPacket = packet as C_Skill;
        ClientSession clientSession = session as ClientSession;

        Player player = clientSession.MyPlayer;
        if (player == null)
        {
            Logger.WriteLog(LogLevel.Error, $"PacketHandler.C_SkillHandler. ClientSession has no player. sessionId:{clientSession.SessionId}");
            return;
        }

        GameRoom room = player.Room;
        if (room == null)
        {
            Logger.WriteLog(LogLevel.Error, $"PacketHandler.C_SkillHandler. Player has no room. sessionId:{clientSession.SessionId}, {player}");
            return;
        }

        Logger.WriteLog(LogLevel.Debug, $"PacketHandler.C_SkillHandler. sessionId:{clientSession.SessionId}, skillId:{skillPacket.SkillId}, {player}");


        room.HandleSkill(player, skillPacket);
    }



    // 시간동기화 요청 처리
    public static void C_SyncTimeHandler(PacketSession session, IMessage packet)
    {
        C_SyncTime syncTime = packet as C_SyncTime;
        ClientSession clientSession = session as ClientSession;

        S_SyncTimeResponse syncTimeResponse = new S_SyncTimeResponse();
        syncTimeResponse.ClientTime = syncTime.ClientTime;
        syncTimeResponse.ServerTime = TimeManager.ServerTime;
        clientSession.Send(syncTimeResponse);
    }


    // Auto 설정요청 처리
    public static void C_SetAutoHandler(PacketSession session, IMessage packet)
    {
        C_SetAuto autoPacket = packet as C_SetAuto;
        ClientSession clientSession = session as ClientSession;

        clientSession.MyPlayer.Room.HandleSetAuto(clientSession.MyPlayer, autoPacket);
    }


    
    // 로딩완료 요청 처리
    public static void C_LoadFinishedHandler(PacketSession session, IMessage packet)
    {
        C_LoadFinished loadPacket = packet as C_LoadFinished;
        ClientSession clientSession = session as ClientSession;

        Player player = clientSession.MyPlayer;
        if(player.Room == null)
        {
            Logger.WriteLog(LogLevel.Debug, $"PacketHandler.C_LoadFinishedHandler. Player has no room. {player}");
            return;
        }

        clientSession.MyPlayer.Room.HandleLoadFinish(player, loadPacket);

        Logger.WriteLog(LogLevel.Debug, $"PacketHandler.C_LoadFinishedHandler. {player}");
    }


    // 부활 요청 처리
    public static void C_RespawnHandler(PacketSession session, IMessage packet)
    {
        C_Respawn respawnPacket = packet as C_Respawn;
        ClientSession clientSession = session as ClientSession;

        Player player = clientSession.MyPlayer;
        if (player.Room == null)
        {
            Logger.WriteLog(LogLevel.Debug, $"PacketHandler.C_RespawnHandler. Player has no room. {player}");
            return;
        }

        clientSession.MyPlayer.Room.HandleRespawn(player, respawnPacket);

        Logger.WriteLog(LogLevel.Debug, $"PacketHandler.C_RespawnHandler. {player}");
    }


    // debug command
    public static void C_DebugCommandHandler(PacketSession session, IMessage packet)
    {
        C_DebugCommand debugPacket = packet as C_DebugCommand;
        ClientSession clientSession = session as ClientSession;

        Player player = clientSession.MyPlayer;
        if (player.Room == null)
        {
            Logger.WriteLog(LogLevel.Debug, $"PacketHandler.C_RespawnHandler. Player has no room. {player}");
            return;
        }


        string command = debugPacket.Command;
        int mapId;
        if (int.TryParse(command.Substring(1, command.Length - 1), out mapId) == false)
            return;
        clientSession.MyPlayer.Room.HandleMapMove(player, mapId);
    }
}

