using Google.Protobuf;
using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using TMPro;
using UnityEngine;
using static PlayerObj;

// 패킷 타입별로 호출할 핸들러 함수를 관리하는 클래스
// 서버에게 받은 패킷을 처리하는 핸들러이기 때문에 S_ 로 시작하는 패킷명에 대한 핸들러 함수만 작성한다.
class PacketHandler
{
    public static void S_EnterGameHandler(PacketSession session, IMessage packet)
    {
        S_EnterGame enterGamePacket = packet as S_EnterGame;
        ServerSession serverSession = session as ServerSession;

        // 맵 초기화
        Managers.Map.LoadMap(enterGamePacket.RoomId);

        // 내 플레이어 생성
        BaseController bc = Managers.Object.AddMyPlayer(enterGamePacket);
        if (bc == null)
            return;



        // 마우스 클릭시 길찾기 테스트
        //Managers.Input.MouseAction += (Define.MouseEvent e) => 
        //{
        //    if(e == Define.MouseEvent.PointerUp)
        //    {
        //        //Debug.Log($"Input.mousePosition : {Input.mousePosition}");
        //        //Debug.Log($"Viewport : {Camera.main.ScreenToViewportPoint(Input.mousePosition)}");
        //        Vector3 mouseWorldPos = Camera.main.ScreenToWorldPoint(new Vector3(Input.mousePosition.x, Input.mousePosition.y, Camera.main.nearClipPlane));
        //        Vector2 serverPos = Managers.Map.ClientPosToServerPos(mouseWorldPos);
        //        Vector2Int serverCell = Managers.Map.PosToCell(serverPos);
        //        Debug.Log($"World Mouse : {mouseWorldPos}, Server pos :{serverPos} cell:{serverCell}");

        //        JumpPointSearch jsp = new JumpPointSearch();
        //        jsp.Init(Managers.Map._cells, Managers.Map.CellMaxX, Managers.Map.CellMaxY);
        //        List<Vector2Int> path = jsp.SearchPath(Managers.Object.MyPlayer.Cell, serverCell);
        //        Debug.Log($"jsp start: {Managers.Object.MyPlayer.Cell}, end: {serverCell}");
        //        foreach (Vector2Int p in path)
        //            Debug.Log($"path: {p}");
        //        GameObject root = GameObject.Find("@Path");
        //        if (root == null)
        //            root = new GameObject { name = "@Path" };
        //        for(int i=0; i< root.transform.childCount; i++)
        //            Managers.Resource.Destroy(root.transform.GetChild(i).gameObject);
        //        for(int i=0; i< path.Count; i++)
        //        {
        //            Vector2Int p = path[i];
        //            GameObject txt = Managers.Resource.Instantiate("DummyText");
        //            txt.transform.SetParent(root.transform);
        //            Vector3 pos = Managers.Map.ServerPosToClientPos(Managers.Map.CellToPos(p));
        //            pos.z = 10;

        //            txt.name = $"#{i} {p}";
        //            txt.transform.position = pos;
        //            TextMeshPro text = txt.GetComponent<TextMeshPro>();
        //            text.text = $"#{i} {p}";
        //        }

        //    }
        //};



        // 로딩완료 패킷 전송 (원래는 모든 첫 spawn 패킷 다받고 보내야함)
        C_LoadFinished loadPacket = new C_LoadFinished();
        loadPacket.BLoaded = true;
        Managers.Network.Send(loadPacket);


        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_EnterGameHandler. room:{enterGamePacket.RoomId} name:{enterGamePacket.Object.Name}, {bc.ToString(Define.InfoLevel.All)}");
    }


    public static void S_LeaveGameHandler(PacketSession session, IMessage packet)
    {
        S_LeaveGame leaveGamePacket = packet as S_LeaveGame;
        ServerSession serverSession = session as ServerSession;

        // 현재 map에서 오브젝트를 제거
        Managers.Map.Clear();

        // 모든 오브젝트를 파괴
        Managers.Object.Clear();
        Managers.Number.Clear();

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_LeaveGameHandler");
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

        // 오브젝트 생성
        foreach(ObjectInfo info in spawnPacket.Objects)
        {
            Managers.Object.AddOtherPlayer(info);
        }

        // 오브젝트 초기 설정
        // 모든 오브젝트가 추가된 다음 세팅해야하는 요소라서 여기서 함
        foreach (ObjectInfo info in spawnPacket.Objects)
        {
            CreatureController creature = Managers.Object.FindById(info.ObjectId) as CreatureController;
            if (creature.AutoMode == AutoMode.ModeAuto)
            {
                creature.SetAutoMove(creature.AutoInfo, creature.PosInfo);
            }
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

    public static void S_SpawnSkillHandler(PacketSession session, IMessage packet)
    {
        S_SpawnSkill spawnPacket = packet as S_SpawnSkill;
        ServerSession serverSession = session as ServerSession;

        // 스킬 오브젝트 생성
        Managers.Object.AddProjectile(spawnPacket);

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_SpawnSkillHandler. id:{spawnPacket.ObjectId}, skill:{spawnPacket.SkillId}");
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
        attacker.OnSkill(skillPacket.SkillId);
    }

    public static void S_SkillHitHandler(PacketSession session, IMessage packet)
    {
        S_SkillHit hitPacket = packet as S_SkillHit;
        ServerSession serverSession = session as ServerSession;

        CreatureController attacker = Managers.Object.FindById(hitPacket.ObjectId) as CreatureController;
        if (attacker == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_SkillHitHandler. Can't find object. objectId:{hitPacket.ObjectId}");
            return;
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_SkillHitHandler. objectId:{hitPacket.ObjectId}, skillId:{hitPacket.SkillId}, hits:{hitPacket.HitObjectIds.Count}");

        // 스킬 찾기
        Data.SkillData skill = Managers.Data.SkillDict.GetValueOrDefault(hitPacket.SkillId, null);

        // 피격된 캐릭터가 없을 경우 이펙트만 생성
        if (hitPacket.HitObjectIds.Count == 0)
        {
            if (string.IsNullOrEmpty(skill.hitEffect) == false)
            {
                Vector2 pos = Managers.Map.GetValidPos(attacker.Pos + (Util.GetDirectionVector(attacker.LookDir) * skill.rangeX));
                Managers.Object.AddEffect(skill.hitEffect, pos, skill.effectOffsetY);
            }
        }
        // 피격된 캐릭터가 있을 경우 해당위치에 이펙트 생성
        else
        {
            foreach (int hitObj in hitPacket.HitObjectIds)
            {
                CreatureController taker = Managers.Object.FindById<CreatureController>(hitObj);
                if (taker != null)
                    Managers.Object.AddEffect(skill.hitEffect, taker.Pos, skill.effectOffsetY);
            }
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_SkillHitHandler. skill:{hitPacket.SkillId}, hits:{hitPacket.HitObjectIds.Count}, {attacker}");
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
        if(changePacket.ChangeType == StatChangeType.ChangeNegative)
            cc.OnDamaged(changePacket.Amount);

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


    public static void S_SetAutoResponseHandler(PacketSession session, IMessage packet)
    {
        S_SetAutoResponse autoPacket = packet as S_SetAutoResponse;
        ServerSession serverSession = session as ServerSession;

        BaseController obj = Managers.Object.FindById(autoPacket.ObjectId);
        if (obj == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_SetAutoResponseHandler. Can't find object. objectId:{autoPacket.ObjectId}");
            return;
        }

        CreatureController cc = obj as CreatureController;
        if (cc == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_SetAutoResponseHandler. Object has no CreatureController component. objectId:{autoPacket.ObjectId}");
            return;
        }

        cc.AutoMode = autoPacket.Mode;
        if (autoPacket.Mode == AutoMode.ModeNone)
        {
            cc.State = CreatureState.Idle;
        }
        else if (autoPacket.Mode == AutoMode.ModeAuto)
        {
            cc.Auto.Init(cc);
        }

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_SetAutoResponseHandler. mode:{autoPacket.Mode}, {cc}");
    }


    public static void S_StopHandler(PacketSession session, IMessage packet)
    {
        S_Stop stopPacket = packet as S_Stop;
        ServerSession serverSession = session as ServerSession;

        BaseController obj = Managers.Object.FindById(stopPacket.ObjectId);
        if (obj == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_StopHandler. Can't find object. objectId:{stopPacket.ObjectId}");
            return;
        }

        CreatureController cc = obj as CreatureController;
        if (cc == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_StopHandler. Object has no CreatureController component. objectId:{stopPacket.ObjectId}");
            return;
        }


        cc.SyncStop(new Vector2(stopPacket.PosX, stopPacket.PosY));

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_StopHandler. id:{stopPacket.ObjectId}, pos:({stopPacket.PosX},{stopPacket.PosY})");
    }


    public static void S_AutoMoveHandler(PacketSession session, IMessage packet)
    {
        S_AutoMove autoPacket = packet as S_AutoMove;
        ServerSession serverSession = session as ServerSession;

        BaseController obj = Managers.Object.FindById(autoPacket.ObjectId);
        if (obj == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_AutoMoveHandler. Can't find object. objectId:{autoPacket.ObjectId}");
            return;
        }

        CreatureController cc = obj as CreatureController;
        if (cc == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_AutoMoveHandler. Object has no CreatureController component. objectId:{autoPacket.ObjectId}");
            return;
        }

        cc.SetAutoMove(autoPacket);

        AutoInfo info = autoPacket.AutoInfo;
        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_AutoMoveHandler. id:{autoPacket.ObjectId}, " +
            $"state:{info.AutoState}, pos:({info.StartPosX},{info.StartPosY}) target:{info.TargetId} tPos:({info.TargetPosX},{info.TargetPosY}), " +
            $"dist:({info.TargetDistX},{info.TargetDistY}), wait:{(float)(info.WaitUntil - Managers.Time.CurrentTime) / (float)TimeSpan.TicksPerSecond}, next:{info.NextState}, skill:{info.SkillId}");
    }


    // 로딩완료 요청 처리
    public static void S_LoadFinishedHandler(PacketSession session, IMessage packet)
    {
        S_LoadFinished loadPacket = packet as S_LoadFinished;
        ServerSession serverSession = session as ServerSession;

        BaseController obj = Managers.Object.FindById(loadPacket.ObjectId);
        if (obj == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"PacketHandler.S_LoadFinishedHandler. Can't find object. objectId:{loadPacket.ObjectId}");
            return;
        }

        // 로딩이 완료되었으므로 상태를 Idle로 변경
        obj.State = CreatureState.Idle;

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"PacketHandler.S_LoadFinishedHandler. objectId:{obj.Id}");
    }

}
