using Data;
using Google.Protobuf.Protocol;
using Google.Protobuf.WellKnownTypes;
using ServerCore;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.Rendering;

// 맵 상의 오브젝트를 관리하는 매니저
public class ObjectManager
{
    public MyPlayerController MyPlayer { get; private set; }
    Dictionary<int, BaseController> _players = new Dictionary<int, BaseController>();
    Dictionary<int, BaseController> _projectiles = new Dictionary<int, BaseController>();
    Dictionary<int, BaseController> _effects = new Dictionary<int, BaseController>();
    
    public ICollection<KeyValuePair<int, BaseController>> Players { get { return _players.AsReadOnlyCollection(); } }
    public ICollection<KeyValuePair<int, BaseController>> Projectiles { get { return _projectiles.AsReadOnlyCollection(); } }
    public ICollection<KeyValuePair<int, BaseController>> Effects { get { return _effects.AsReadOnlyCollection(); } }

    public int PlayerCount { get { return _players.Count; } }
    public int ProjectileCount { get { return _projectiles.Count; } }
    public int EffectCount { get { return _effects.Count; } }



    // 내 플레이어 추가
    public MyPlayerController AddMyPlayer(S_EnterGame packet)
    {
        ObjectInfo info = packet.Object;
        GameObjectType objType = Util.GetObjectTypeById(info.ObjectId);
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
        int spumId = info.SPUMId;
        SPUMData spum = Managers.Data.SPUMDict.GetValueOrDefault(spumId, Managers.Data.DefaultSPUM);
        GameObject go = Managers.Resource.Instantiate($"SPUM/{spum.prefabName}");
        go.transform.position = Vector3.zero;
        go.name = "MyPlayer";

        // MyPlayer 등록
        MyPlayerController player = Util.FindChild(go, "UnitRoot").GetOrAddComponent<MyPlayerController>();
        MyPlayer = player;
        SortingGroup sort = player.GetOrAddComponent<SortingGroup>();
        sort.sortingOrder = 2;

        // 객체 초기화
        MyPlayer.Init(packet);

        // object 추가
        _players.Add(info.ObjectId, player);

        // map에 추가
        Managers.Map.Add(player);

        return player;
    }


    // 다른 플레이어 추가
    public PlayerController AddOtherPlayer(ObjectInfo info)
    {
        GameObjectType objType = Util.GetObjectTypeById(info.ObjectId);
        if (objType == GameObjectType.Player)
        {
            // 다른 플레이어 생성
            int spumId = info.SPUMId;
            SPUMData spum = Managers.Data.SPUMDict.GetValueOrDefault(spumId, Managers.Data.DefaultSPUM);
            GameObject go = Managers.Resource.Instantiate($"SPUM/{spum.prefabName}");
            go.transform.position = Vector3.zero;
            go.name = info.Name;

            PlayerController player = Util.FindChild(go, "UnitRoot").GetOrAddComponent<PlayerController>();
            SortingGroup sort = player.GetOrAddComponent<SortingGroup>();
            sort.sortingOrder = 2;
            player.Info = info;
            player.PosInfo = info.PosInfo;
            player.Stat = info.StatInfo;

            // object 추가
            _players.Add(info.ObjectId, player);

            // map에 추가
            Managers.Map.Add(player);

            return player;
            
        }

        return null;
    }

    // 투사체 오브젝트 추가
    public ProjectileController AddProjectile(S_SpawnSkill packet)
    {
        GameObjectType objType = Util.GetObjectTypeById(packet.ObjectId);
        if (objType != GameObjectType.Projectile)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Add. Object type is not Projectile. id:{packet.ObjectId}, type:{objType}");
            return null;
        }

        // 스킬정보 찾기
        SkillData skill;
        if (Managers.Data.SkillDict.TryGetValue(packet.SkillId, out skill) == false)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Add. Can't find skill. id:{packet.ObjectId}, owner:{packet.OwnerId}, skill:{packet.SkillId}");
            return null;
        }


        // 스킬ID에 따른 오브젝트 생성
        switch(skill.id)
        {
            case SkillId.SkillFireball:
                {
                    // fireball 생성
                    GameObject go = Managers.Resource.Instantiate(skill.projectile.prefab);
                    if(go == null)
                    {
                        ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Add. Can't find prefab. id:{packet.ObjectId}, owner:{packet.OwnerId}, skill:{packet.SkillId}, prefab:{skill.projectile.prefab}");
                        return null;
                    }
                    go.name = skill.projectile.name;

                    ProjectileController fireball = go.GetOrAddComponent<ProjectileController>();
                    SpriteRenderer renderer = fireball.GetOrAddComponent<SpriteRenderer>();
                    renderer.sortingOrder = 2;

                    
                    BaseController owner = FindById(packet.OwnerId);
                    BaseController target = FindById(packet.TargetId);
                    fireball.Init(packet.ObjectId, skill, owner, target);

                    // object 추가
                    _projectiles.Add(packet.ObjectId, fireball);

                    return fireball;
                }
        }

        return null;
    }



    // effect 오브젝트 추가
    public EffectController AddEffect(string prefab, Vector2 pos, float offsetY)
    {
        if (string.IsNullOrEmpty(prefab))
            return null;

        // effect 생성
        GameObject go = Managers.Resource.Instantiate(prefab);
        if (go == null)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.AddEffect. Can't find prefab. prefab:{prefab}");
            return null;
        }
        EffectController effect = go.GetOrAddComponent<EffectController>();
        effect.Init(prefab, pos, offsetY);

        // order layer 설정
        SpriteRenderer renderer = effect.GetOrAddComponent<SpriteRenderer>();
        renderer.sortingOrder = 3;

        // object 추가
        _effects.Add(effect.Id, effect);

        return effect;
    }



    // 오브젝트 제거
    public void Remove(int id)
    {
        BaseController obj;
        GameObjectType objType = Util.GetObjectTypeById(id);
        switch (objType)
        {
            case GameObjectType.Player:
                {
                    if (MyPlayer.Id == id)
                        MyPlayer = null;

                    if (_players.Remove(id, out obj) == false)
                        ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Remove. Can't find object of id. id:{id}, type:{objType}");

                    // 맵에서 제거
                    Managers.Map.Remove(obj);

                    // 파괴
                    Managers.Resource.Destroy(obj.gameObject);
                }
                break;

            case GameObjectType.Projectile:
                {
                    if (_projectiles.Remove(id, out obj) == false)
                        ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Remove. Can't find object of id. id:{id}, type:{objType}");

                    Managers.Resource.Destroy(obj.gameObject);
                }
                break;

            case GameObjectType.Effect:
                {
                    if (_effects.Remove(id, out obj) == false)
                        ServerCore.Logger.WriteLog(LogLevel.Error, $"ObjectManager.Remove. Can't find object of id. id:{id}, type:{objType}");

                    Managers.Resource.Destroy(obj.gameObject);
                }
                break;
        }

    }



    public BaseController FindById(int id)
    {
        BaseController obj = null;
        GameObjectType objType = Util.GetObjectTypeById(id);
        switch (objType)
        {
            case GameObjectType.Player:
                _players.TryGetValue(id, out obj);
                break;
            case GameObjectType.Projectile:
                _projectiles.TryGetValue(id, out obj);
                break;
            case GameObjectType.Effect:
                _effects.TryGetValue(id, out obj);
                break;
        }
        
        return obj;
    }

    public T FindById<T>(int id) where T : BaseController
    {
        BaseController obj = FindById(id);
        return obj as T;
    }


    public void Clear()
    {
        MyPlayer = null;
        foreach (BaseController obj in _players.Values)
            Managers.Resource.Destroy(obj.gameObject);
        foreach (BaseController obj in _projectiles.Values)
            Managers.Resource.Destroy(obj.gameObject);
        foreach (BaseController obj in _effects.Values)
            Managers.Resource.Destroy(obj.gameObject);

        _players.Clear();
        _projectiles.Clear();
        _effects.Clear();
        
    }
}
