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
    Dictionary<int, GameObject> _objects = new Dictionary<int, GameObject>();

    // id 에서 타입 얻기
    public static GameObjectType GetObjectTypeById(int id)
    {
        // bits : [ Unused(1) | Type(7) | Id(24) ]
        int type = (id >> 24) & 0x7F;
        return (GameObjectType)type;
    }


    public BaseController Add(ObjectInfo info, bool myPlayer = false)
    {
        GameObjectType objType = GetObjectTypeById(info.ObjectId);
        if (objType == GameObjectType.Player)
        {
            if (myPlayer)
            {
                if (MyPlayer != null)
                    ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Add. MyPlayer already exists. before id:{MyPlayer.Id}, new id:{info.ObjectId}");

                // 내 플레이어 생성
                string prefab = "Player" + (info.ObjectId & 0xf).ToString("000");   // 00# 형태의 string으로 변경
                GameObject player = Managers.Resource.Instantiate($"Creature/{prefab}");
                player.transform.position = Vector3.zero;
                player.name = "MyPlayer";
                _objects.Add(info.ObjectId, player);

                GameObject unitRoot = Util.FindChild(player, "UnitRoot");
                MyPlayer = unitRoot.GetOrAddComponent<MyPlayerController>();
                MyPlayer.Id = info.ObjectId;
                MyPlayer.PosInfo = info.PosInfo;
                MyPlayer.Stat = info.StatInfo;

                // map에 추가
                Managers.Map.Add(MyPlayer);

                return MyPlayer;
            }
            else
            {
                // 다른 플레이어 생성
                string prefab = "Player" + (info.ObjectId & 0xf).ToString("000");   // 00# 형태의 string으로 변경
                GameObject player = Managers.Resource.Instantiate($"Creature/{prefab}");
                player.transform.position = Vector3.zero;
                player.name = info.Name;
                _objects.Add(info.ObjectId, player);

                GameObject unitRoot = Util.FindChild(player, "UnitRoot");
                PlayerController pc = unitRoot.GetOrAddComponent<PlayerController>();
                pc.Id = info.ObjectId;
                pc.PosInfo = info.PosInfo;
                pc.Stat = info.StatInfo;

                // map에 추가
                Managers.Map.Add(pc);

                return pc;
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

            return mc;
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

            return ac;
        }

        return null;
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


    // cell 위치에 있는 CreatureController 컴포넌트가 있는 오브젝트를 찾는다.
    public GameObject FindCreature(Vector2Int cell)
    {
        foreach (GameObject obj in _objects.Values)
        {
            CreatureController cc = obj.GetComponent<CreatureController>();
            if (cc == null)
                continue;

            if (cc.Cell == cell)
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
