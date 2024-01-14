using Google.Protobuf;
using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// 패킷 타입별로 호출할 핸들러 함수를 관리하는 클래스
// 서버에게 받은 패킷을 처리하는 핸들러이기 때문에 S_ 로 시작하는 패킷명에 대한 핸들러 함수만 작성한다.
class PacketHandler
{
    public static void S_EnterGameHandler(PacketSession session, IMessage packet)
    {
        S_EnterGame enterGamePacket = packet as S_EnterGame;
        ServerSession serverSession = session as ServerSession;

        // 내 플레이어 생성
        BaseController bc = Managers.Object.Add(enterGamePacket);
        if (bc == null)
            return;

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_EnterGameHandler. name:{enterGamePacket.Player.Name}, {bc.ToString(Define.InfoLevel.All)}");
    }


    public static void S_LeaveGameHandler(PacketSession session, IMessage packet)
    {
        S_LeaveGame leaveGamePacket = packet as S_LeaveGame;
        ServerSession serverSession = session as ServerSession;

        // 내 플레이어 제거
        Managers.Object.Clear();

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_LeaveGameHandler");
    }


    public static void S_SpawnHandler(PacketSession session, IMessage packet)
    {
        S_Spawn spawnPacket = packet as S_Spawn;
        ServerSession serverSession = session as ServerSession;

        // 오브젝트 생성
        foreach(ObjectInfo info in spawnPacket.Objects)
        {
            Managers.Object.Add(info);
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_SpawnHandler. objects:{spawnPacket.Objects}");
    }


    public static void S_DespawnHandler(PacketSession session, IMessage packet)
    {
        S_Despawn despawnPacket = packet as S_Despawn;
        ServerSession serverSession = session as ServerSession;

        // 오브젝트 제거
        foreach (int id in despawnPacket.ObjectIds)
        {
            Managers.Object.Remove(id);
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_DespawnHandler. objectIds:{despawnPacket.ObjectIds}");
    }


    public static void S_MoveHandler(PacketSession session, IMessage packet)
    {
        S_Move movePacket = packet as S_Move;
        ServerSession serverSession = session as ServerSession;

        // 내 캐릭터에 대한 이동패킷은 무시한다. 클라에서 서버보다 먼저 캐릭터를 이동시키기 때문이다.
        if (Managers.Object.MyPlayer.Id == movePacket.ObjectId)
            return;

        BaseController obj = Managers.Object.FindById(movePacket.ObjectId);
        if (obj == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_MoveHandler. Can't find object. objectId:{movePacket.ObjectId}");
            return;
        }
        
        PlayerController pc = obj as PlayerController;
        if (pc == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_MoveHandler. Object has no PlayerController component. objectId:{movePacket.ObjectId}");
            return;
        }

        // 목적지만 교체해준다.
        PositionInfo info = movePacket.PosInfo;
        pc.Dest = new Vector2(info.DestX, info.DestY);
        pc.Dir = info.MoveDir;
        pc.RemoteState = info.State;
        pc.RemoteDir = info.MoveDir;
        pc.MoveKeyDown = info.MoveKeyDown;

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_MoveHandler. " +
            $"objectId:{movePacket.ObjectId}, state:{info.State}, dir:{info.MoveDir}" +
            $", pos:({info.PosX:f2},{info.PosY:f2}), dest:({info.DestX:f2},{info.DestY:f2}), move:{info.MoveKeyDown}");
    }


    public static void S_SkillHandler(PacketSession session, IMessage packet)
    {
        S_Skill skillPacket = packet as S_Skill;
        ServerSession serverSession = session as ServerSession;

        CreatureController attacker = Managers.Object.FindById(skillPacket.ObjectId) as CreatureController;
        if (attacker == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_SkillHandler. Can't find object. objectId:{skillPacket.ObjectId}");
            return;
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_SkillHandler. objectId:{skillPacket.ObjectId}, skillId:{skillPacket.SkillId}");

        // 스킬 사용
        if(attacker != Managers.Object.MyPlayer)
            attacker.OnSkill(skillPacket.SkillId);

        // 피격됨
        foreach(int objectId in skillPacket.HitObjectIds)
        {
            CreatureController taker = Managers.Object.FindById<CreatureController>(objectId);
            if(taker != null)
                taker.OnDamaged(attacker, attacker.Stat.Attack);
        }
    }


    public static void S_ChangeHpHandler(PacketSession session, IMessage packet)
    {
        S_ChangeHp changePacket = packet as S_ChangeHp;
        ServerSession serverSession = session as ServerSession;

        BaseController obj = Managers.Object.FindById(changePacket.ObjectId);
        if (obj == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_ChangeHpHandler. Can't find object. objectId:{changePacket.ObjectId}");
            return;
        }

        CreatureController cc = obj as CreatureController;
        if (cc == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_ChangeHpHandler. Object has no CreatureController component. objectId:{changePacket.ObjectId}");
            return;
        }

        cc.Hp = changePacket.Hp;

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_ChangeHpHandler. objectId:{changePacket.ObjectId}, Hp:{changePacket.Hp}");
    }

    public static void S_DieHandler(PacketSession session, IMessage packet)
    {
        S_Die diePacket = packet as S_Die;
        ServerSession serverSession = session as ServerSession;

        BaseController obj = Managers.Object.FindById(diePacket.ObjectId);
        if (obj == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_DieHandler. Can't find object. objectId:{diePacket.ObjectId}");
            return;
        }

        CreatureController cc = obj as CreatureController;
        if (cc == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_DieHandler. Object has no CreatureController component. objectId:{diePacket.ObjectId}");
            return;
        }

        cc.Hp = 0;
        cc.OnDead();

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_DieHandler. objectId:{diePacket.ObjectId}, attackerId:{diePacket.AttackerId}");
    }


    public static void S_SyncTimeResponseHandler(PacketSession session, IMessage packet)
    {
        S_SyncTimeResponse syncPacket = packet as S_SyncTimeResponse;
        ServerSession serverSession = session as ServerSession;

        Managers.Time.SyncTime(syncPacket);
    }

}
