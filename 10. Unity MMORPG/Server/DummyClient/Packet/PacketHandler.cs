using DummyClient.Game;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace DummyClient
{
    // 패킷 타입별로 호출할 핸들러 함수를 관리하는 클래스
    // 서버에게 받은 패킷을 처리하는 핸들러이기 때문에 S_ 로 시작하는 패킷명에 대한 핸들러 함수만 작성한다.
    public class PacketHandler
    {
        public static void S_EnterGameHandler(PacketSession session, IMessage packet)
        {
            S_EnterGame enterGamePacket = packet as S_EnterGame;
            ServerSession serverSession = session as ServerSession;

            // 플레이어 생성
            PlayerController player = new PlayerController();
            player.Init(serverSession, enterGamePacket);

            // 게임 입장
            GameManager.Instance.EnterGame(player, enterGamePacket);

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_EnterGameHandler. {player}");
        }


        public static void S_LeaveGameHandler(PacketSession session, IMessage packet)
        {
            S_LeaveGame leaveGamePacket = packet as S_LeaveGame;
            ServerSession serverSession = session as ServerSession;
        }


        public static void S_EnterRoomHandler(PacketSession session, IMessage packet)
        {
        }

        public static void S_LeaveRoomHandler(PacketSession session, IMessage packet)
        {
        }


        public static void S_SpawnHandler(PacketSession session, IMessage packet)
        {
            S_Spawn spawnPacket = packet as S_Spawn;
            ServerSession serverSession = session as ServerSession;
        }


        public static void S_DespawnHandler(PacketSession session, IMessage packet)
        {
            S_Despawn despawnPacket = packet as S_Despawn;
            ServerSession serverSession = session as ServerSession;
        }

        public static void S_SpawnSkillHandler(PacketSession session, IMessage packet)
        {
            S_SpawnSkill spawnPacket = packet as S_SpawnSkill;
            ServerSession serverSession = session as ServerSession;
        }


        public static void S_MoveHandler(PacketSession session, IMessage packet)
        {
            S_Move movePacket = packet as S_Move;
            ServerSession serverSession = session as ServerSession;
        }


        public static void S_SkillHandler(PacketSession session, IMessage packet)
        {
            S_Skill skillPacket = packet as S_Skill;
            ServerSession serverSession = session as ServerSession;
        }

        public static void S_SkillHitHandler(PacketSession session, IMessage packet)
        {
            S_SkillHit hitPacket = packet as S_SkillHit;
            ServerSession serverSession = session as ServerSession;
        }


        public static void S_ChangeHpHandler(PacketSession session, IMessage packet)
        {
            S_ChangeHp changePacket = packet as S_ChangeHp;
            ServerSession serverSession = session as ServerSession;
        }

        public static void S_DieHandler(PacketSession session, IMessage packet)
        {
            S_Die diePacket = packet as S_Die;
            ServerSession serverSession = session as ServerSession;

            if (serverSession.MyPlayer != null)
            {
                serverSession.MyPlayer.State = CreatureState.Dead;
                serverSession.MyPlayer.Auto.State = AutoState.AutoDead;
            }
        }


        public static void S_SyncTimeResponseHandler(PacketSession session, IMessage packet)
        {
            S_SyncTimeResponse syncPacket = packet as S_SyncTimeResponse;
            ServerSession serverSession = session as ServerSession;
        }


        public static void S_SetAutoResponseHandler(PacketSession session, IMessage packet)
        {
            S_SetAutoResponse autoPacket = packet as S_SetAutoResponse;
            ServerSession serverSession = session as ServerSession;
        }


        public static void S_StopHandler(PacketSession session, IMessage packet)
        {
            S_Stop stopPacket = packet as S_Stop;
            ServerSession serverSession = session as ServerSession;
        }


        public static void S_AutoMoveHandler(PacketSession session, IMessage packet)
        {
            S_AutoMove autoPacket = packet as S_AutoMove;
            ServerSession serverSession = session as ServerSession;
        }


        // 로딩완료 요청 처리
        public static void S_LoadFinishedHandler(PacketSession session, IMessage packet)
        {
            S_LoadFinished loadPacket = packet as S_LoadFinished;
            ServerSession serverSession = session as ServerSession;

            if (serverSession.MyPlayer != null)
            {
                serverSession.MyPlayer.State = CreatureState.Idle;
            }

        }

        // 로딩완료 요청 처리
        public static void S_LoginResponseHandler(PacketSession session, IMessage packet)
        {
            S_LoginResponse loadPacket = packet as S_LoginResponse;
            ServerSession serverSession = session as ServerSession;
        }

        
    }

}
