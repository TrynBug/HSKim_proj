using Google.Protobuf.Protocol;
using Google.Protobuf.WellKnownTypes;
using ServerCore;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using Unity.VisualScripting;
using UnityEngine;

// 맵 상의 오브젝트를 관리하는 매니저
public class ObjectManager
{
    public MyPlayerController MyPlayer { get; set; }
    Dictionary<int, BaseController> _objects = new Dictionary<int, BaseController>();

    public int ObjectCount { get { return _objects.Count; } }
    public ICollection<KeyValuePair<int, BaseController>> Objects { get { return _objects.AsReadOnlyCollection(); } }


    // id 에서 타입 얻기
    public static GameObjectType GetObjectTypeById(int id)
    {
        // bits : [ Unused(1) | Type(7) | Id(24) ]
        int type = (id >> 24) & 0x7F;
        return (GameObjectType)type;
    }


    public BaseController Add(S_EnterGame packet)
    {
        ObjectInfo info = packet.Player;
        GameObjectType objType = GetObjectTypeById(info.ObjectId);
        if (objType != GameObjectType.Player)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Add. Object type is not Player. id:{info.ObjectId}, type:{objType}");
            return null;
        }

        if (MyPlayer != null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Add. MyPlayer already exists. before id:{MyPlayer.Id}, new id:{info.ObjectId}");
            return null;
        }

        // 내 플레이어 생성
        string prefab = "Player" + (info.ObjectId & 0xf).ToString("000");   // 00# 형태의 string으로 변경
        GameObject go = Managers.Resource.Instantiate($"Creature/{prefab}");
        go.transform.position = Vector3.zero;
        go.name = "MyPlayer";

        MyPlayerController player = Util.FindChild(go, "UnitRoot").GetOrAddComponent<MyPlayerController>();
        MyPlayer = player;
        MyPlayer.Id = info.ObjectId;
        MyPlayer.PosInfo = info.PosInfo;
        MyPlayer.Stat = info.StatInfo;

        _objects.Add(info.ObjectId, player);

        // 스킬 등록
        foreach (int skillId in packet.SkillIds)
        {
            Data.Skill skill;
            if (Managers.Data.SkillDict.TryGetValue(skillId, out skill))
                MyPlayer.Skillset.Add(skillId, new SkillInfo() { lastUseTime = 0, skill = skill });
        }

        // map에 추가
        Managers.Map.Add(MyPlayer);

        return MyPlayer;
    }


    public BaseController Add(ObjectInfo info)
    {
        GameObjectType objType = GetObjectTypeById(info.ObjectId);
        if (objType == GameObjectType.Player)
        {
            // 다른 플레이어 생성
            string prefab = "Player" + (info.ObjectId & 0xf).ToString("000");   // 00# 형태의 string으로 변경
            GameObject go = Managers.Resource.Instantiate($"Creature/{prefab}");
            go.transform.position = Vector3.zero;
            go.name = info.Name;

            PlayerController player = Util.FindChild(go, "UnitRoot").GetOrAddComponent<PlayerController>();
            player.Id = info.ObjectId;
            player.PosInfo = info.PosInfo;
            player.Stat = info.StatInfo;

            _objects.Add(info.ObjectId, player);

            // map에 추가
            Managers.Map.Add(player);

            return player;
            
        }
        else if (objType == GameObjectType.Monster)
        {
            // 몬스터 생성
            return null;
        }
        else if (objType == GameObjectType.Projectile)
        {
            // 화살 생성
            return null;
        }

        return null;
    }




    public void Add(int id, BaseController obj)
    {
        _objects.Add(id, obj);
    }

    public void Remove(int id)
    {
        BaseController obj;
        if(_objects.Remove(id, out obj) == false)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Remove. Can't find object of id. id:{id}");
            return;
        }
        Managers.Resource.Destroy(obj.gameObject);
    }

    // condition 함수의 결과값이 true가 나오는 오브젝트를 찾는다.
    public BaseController Find(Func<BaseController, bool> condition)
    {
        foreach (BaseController obj in _objects.Values)
        {
            if (condition.Invoke(obj))
                return obj;
        }

        return null;
    }

    public BaseController FindById(int id)
    {
        BaseController obj = null;
        _objects.TryGetValue(id, out obj);
        return obj;
    }

    public T FindById<T>(int id) where T : BaseController
    {
        BaseController obj = null;
        _objects.TryGetValue(id, out obj);
        if (obj == null)
            return null;

        return obj as T;
    }


    public void Clear()
    {
        foreach (BaseController obj in _objects.Values)
            Managers.Resource.Destroy(obj.gameObject);
        _objects.Clear();
        MyPlayer = null;
    }
}
