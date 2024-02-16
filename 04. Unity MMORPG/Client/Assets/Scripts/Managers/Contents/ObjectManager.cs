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

// 맵 상의 오브젝트를 관리하는 매니저
public class ObjectManager
{
    GameObject _root;
    GameObject _rootPlayer;
    GameObject _rootSkill;
    GameObject _rootProjectile;
    GameObject _rootEffect;

    public MyPlayerController MyPlayer { get; private set; }
    Dictionary<int, BaseController> _players = new Dictionary<int, BaseController>();
    Dictionary<int, BaseController> _skills = new Dictionary<int, BaseController>();
    Dictionary<int, BaseController> _projectiles = new Dictionary<int, BaseController>();
    Dictionary<int, BaseController> _effects = new Dictionary<int, BaseController>();
    Dictionary<int, CloneController> _clones = new Dictionary<int, CloneController>();

    public ICollection<KeyValuePair<int, BaseController>> Players { get { return _players.AsReadOnlyCollection(); } }
    public ICollection<KeyValuePair<int, BaseController>> Skills { get { return _skills.AsReadOnlyCollection(); } }
    public ICollection<KeyValuePair<int, BaseController>> Projectiles { get { return _projectiles.AsReadOnlyCollection(); } }
    public ICollection<KeyValuePair<int, BaseController>> Effects { get { return _effects.AsReadOnlyCollection(); } }
    

    public int PlayerCount { get { return _players.Count; } }
    public int SkillCount { get { return _skills.Count; } }
    public int ProjectileCount { get { return _projectiles.Count; } }
    public int EffectCount { get { return _effects.Count; } }

    public void Init()
    {
        _root = new GameObject("@Objects");
        UnityEngine.Object.DontDestroyOnLoad(_root);
        _rootPlayer = new GameObject("@Players");
        _rootPlayer.transform.SetParent(_root.transform);
        _rootSkill = new GameObject("@Skills");
        _rootSkill.transform.SetParent(_root.transform);
        _rootProjectile = new GameObject("@Projectiles");
        _rootProjectile.transform.SetParent(_root.transform);
        _rootEffect = new GameObject("@Effects");
        _rootEffect.transform.SetParent(_root.transform);
    }


    List<BaseController> _endSkills = new List<BaseController>();
    List<BaseController> _deadProjectiles = new List<BaseController>();
    List<BaseController> _deadClones = new List<BaseController>();
    public void OnUpdate()
    {
        // 끝난 스킬 삭제
        _endSkills.Clear();
        foreach (BaseController skill in _skills.Values)
        {
            if(skill.State == CreatureState.Dead)
                _endSkills.Add(skill);
        }
        foreach(BaseController skill in _endSkills)
        {
            Managers.Object.Remove(skill.Id);
        }


        // 사망한 프로젝타일 삭제
        _deadProjectiles.Clear();
        foreach (BaseController projectile in _projectiles.Values)
        {
            if (projectile.State == CreatureState.Dead)
                _deadProjectiles.Add(projectile);
        }
        foreach (BaseController projectile in _deadProjectiles)
        {
            Managers.Object.Remove(projectile.Id);
        }


        
        // debug
        if (Config.DebugOn)
        {
            // 사망한 클론 삭제
            _deadClones.Clear();
            foreach (CloneController clone in _clones.Values)
            {
                if (clone.IsDestroyed)
                    _deadClones.Add(clone);
            }
            foreach (CloneController clone in _deadClones)
            {
                _clones.Remove(clone.Id);
                Managers.Resource.Destroy(clone.gameObject);
            }
        }
    }




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
        go.transform.position = new Vector3(0, 0, Config.ObjectDefaultZ);
        go.name = "MyPlayer";
        go.transform.SetParent(_root.transform);

        // MyPlayer 등록
        MyPlayerController player = go.GetOrAddComponent<MyPlayerController>();
        MyPlayer = player;

        // 객체 초기화
        MyPlayer.Init(packet);

        // object 추가
        _players.Add(info.ObjectId, player);

        // map에 추가
        Managers.Map.Add(player, forced:true);

        // 카메라 위치를 플레이어 위치로 지정
        Camera.main.transform.position = new Vector3(MyPlayer.transform.position.x, MyPlayer.transform.position.y, -10);

        // debug
        if(Config.DebugOn)
        {
            // 클론 생성
            GameObject goClone = Managers.Resource.Instantiate($"SPUM/{spum.prefabName}");
            CloneController clone = goClone.GetOrAddComponent<CloneController>();
            clone.Init(player);
            _clones.Add(clone.Info.ObjectId, clone);
        }

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
            go.transform.position = new Vector3(0, 0, Config.ObjectDefaultZ);
            go.name = info.Name;
            go.transform.SetParent(_rootPlayer.transform);

            PlayerController player = go.GetOrAddComponent<PlayerController>();

            // 초기화
            player.Init(info);

            // object 추가
            _players.Add(info.ObjectId, player);

            // map에 추가
            Managers.Map.Add(player, forced:true);


            // debug
            if (Config.DebugOn)
            {
                // 클론 생성
                GameObject goClone = Managers.Resource.Instantiate($"SPUM/{spum.prefabName}");
                CloneController clone = goClone.GetOrAddComponent<CloneController>();
                clone.Init(player);
                _clones.Add(clone.Info.ObjectId, clone);
            }

            return player;
            
        }

        return null;
    }


    public SkillController AddSkill(S_SkillHit skillPacket)
    {
        // 스킬데이터, 소유자, 타겟 얻기
        BaseController owner = FindById(skillPacket.AttackerId);
        if (owner == null)
            return null;
        BaseController target = FindById(skillPacket.TargetId);
        SkillData skillData = Managers.Data.SkillDict.GetValueOrDefault(skillPacket.SkillId, null);
        if (skillData == null)
            return null;

        // 스킬객체 생성
        SkillController skill = SkillController.Generate(skillPacket.SkillId);
        if (skill == null)
            return null;

        // 초기화
        skill.Init(skillData, owner, target, skillPacket);

        // 등록
        skill.gameObject.transform.SetParent(_rootSkill.transform);
        //skill.gameObject.name = skill.Info.Name;
        _skills.Add(skill.Id, skill);


        return skill;
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

        // owner
        BaseController owner = FindById(packet.OwnerId);
        if (owner == null)
            return null;

        // projectile 생성
        ProjectileController projectile = ProjectileController.Generate(packet.SkillId);
        if (projectile == null)
            return null;
        projectile.transform.SetParent(_rootProjectile.transform);
        projectile.Init(packet.ObjectId, skill, owner, packet.PosInfo);
                    
        _projectiles.Add(packet.ObjectId, projectile);

        return projectile;
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
        go.transform.SetParent(_rootEffect.transform);
        EffectController effect = go.GetOrAddComponent<EffectController>();
        effect.Init(prefab, pos, offsetY);

        // order layer 설정
        SpriteRenderer renderer = effect.GetOrAddComponent<SpriteRenderer>();
        renderer.sortingOrder = 2;

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

            case GameObjectType.Skill:
                {
                    if (_skills.Remove(id, out obj) == false)
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
                obj = _players.GetValueOrDefault(id, null);
                break;
            case GameObjectType.Projectile:
                obj = _projectiles.GetValueOrDefault(id, null);
                break;
            case GameObjectType.Effect:
                obj = _effects.GetValueOrDefault(id, null);
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
        foreach (BaseController obj in _skills.Values)
            GameObject.Destroy(obj.gameObject);
        foreach (BaseController obj in _projectiles.Values)
            Managers.Resource.Destroy(obj.gameObject);
        foreach (BaseController obj in _effects.Values)
            Managers.Resource.Destroy(obj.gameObject);

        _players.Clear();
        _skills.Clear();
        _projectiles.Clear();
        _effects.Clear();


        if(Config.DebugOn)
        {
            foreach (CloneController obj in _clones.Values)
                Managers.Resource.Destroy(obj.gameObject);
            _clones.Clear();
        }
    }
}