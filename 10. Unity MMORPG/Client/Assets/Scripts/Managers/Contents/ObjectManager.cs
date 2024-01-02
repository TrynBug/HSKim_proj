using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;

// 맵 상의 오브젝트를 관리하는 매니저
public class ObjectManager
{
    public MyPlayerController MyPlayer { get; set; }
    Dictionary<int, GameObject> _objects = new Dictionary<int, GameObject>();

    // id 에서 타입 얻기
    public static GameObjectType GetObjectTypeById(int id)
    {
        int type = (id >> 24) & 0x7F;
        return (GameObjectType)type;
    }


    public void Add(ObjectInfo info, bool myPlayer = false)
    {
        GameObjectType objType = GetObjectTypeById(info.ObjectId);
        if (objType == GameObjectType.Player)
        {
            if (myPlayer)
            {
                if (MyPlayer != null)
                    ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Add. MyPlayer already exists. before id:{MyPlayer.Id}, after id:{info.ObjectId}");

                // 내 플레이어 생성
                GameObject player = Managers.Resource.Instantiate("Creature/MyPlayer");
                player.name = "MyPlayer";
                _objects.Add(info.ObjectId, player);

                MyPlayer = player.GetOrAddComponent<MyPlayerController>();
                MyPlayer.Id = info.ObjectId;
                MyPlayer.PosInfo = info.PosInfo;
                MyPlayer.Stat = info.StatInfo;
            }
            else
            {
                // 다른 플레이어 생성
                GameObject player = Managers.Resource.Instantiate("Creature/Player");
                player.name = info.Name;
                _objects.Add(info.ObjectId, player);

                PlayerController pc = player.GetOrAddComponent<PlayerController>();
                pc.Id = info.ObjectId;
                pc.PosInfo = info.PosInfo;
                pc.Stat = info.StatInfo;
            }
        }
        else if (objType == GameObjectType.Monster)
        {
            // 몬스터 생성
            GameObject monster = Managers.Resource.Instantiate("Creature/Monster");
            monster.name = info.Name;
            _objects.Add(info.ObjectId, monster);

            MonsterController mc = monster.GetOrAddComponent<MonsterController>();
            mc.Id = info.ObjectId;
            mc.PosInfo = info.PosInfo;
            mc.Stat = info.StatInfo;
        }
        else if (objType == GameObjectType.Projectile)
        {
            // 화살 생성
            GameObject go = Managers.Resource.Instantiate("Creature/Arrow");
            go.name = "Arrow";
            _objects.Add(info.ObjectId, go);

            ArrowController ac = go.GetComponent<ArrowController>();
            ac.Id = info.ObjectId;
            ac.PosInfo = info.PosInfo;
            ac.Stat = info.StatInfo;
        }
    }

    public void Add(int id, GameObject go)
    {
        _objects.Add(id, go);
    }

    public void Remove(int id)
    {
        GameObject go;
        if(_objects.Remove(id, out go) == false)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Remove. Can't find object of id. id:{id}");
            return;
        }
        Managers.Resource.Destroy(go);
    }


    // cellPos 위치에 있는 CreatureController 컴포넌트가 있는 오브젝트를 찾는다.
    public GameObject FindCreature(Vector3Int cellPos)
    {
        foreach (GameObject obj in _objects.Values)
        {
            CreatureController cc = obj.GetComponent<CreatureController>();
            if (cc == null)
                continue;

            if (cc.CellPos == cellPos)
                return obj;
        }

        return null;
    }

    // condition 함수의 결과값이 true가 나오는 오브젝트를 찾는다.
    public GameObject Find(Func<GameObject, bool> condition)
    {
        foreach (GameObject obj in _objects.Values)
        {
            if (condition.Invoke(obj))
                return obj;
        }

        return null;
    }

    public GameObject FindById(int id)
    {
        GameObject go = null;
        _objects.TryGetValue(id, out go);
        return go;
    }

    public void Clear()
    {
        foreach (GameObject obj in _objects.Values)
            Managers.Resource.Destroy(obj);
        _objects.Clear();
        MyPlayer = null;
    }
}
