using Google.Protobuf.Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Data
{
    // 각각의 json 데이터들에 해당하는 데이터 클래스
    // Json Parser에 의해 json 파일의 데이터들이 이곳의 클래스들로 파싱된다.

    #region Stat
    [Serializable]
    public class StatData : ILoader<int, StatInfo>
    {
        public List<StatInfo> stats = new List<StatInfo>();

        // 읽은 데이터를 Dictionary로 변환하여 리턴하는 함수
        public Dictionary<int, StatInfo> MakeDict()
        {
            Dictionary<int, StatInfo> dict = new Dictionary<int, StatInfo>();

            foreach (StatInfo stat in stats)
                dict.Add(stat.Level, stat);

            return dict;
        }
    }
    #endregion

    #region Skill
    [Serializable]
    public class Skill
    {
        public int id;
        public string name;
        public int cooldown;
        public int damage;
        public SkillType skillType;
        public MeleeInfo melee;
        public ProjectileInfo projectile;
    }

    [Serializable]
    public class ProjectileInfo
    {
        public string name;
        public float speed;
        public int range;
        public string prefab;
    }

    [Serializable]
    public class MeleeInfo
    {
        public float rangeX;
        public float rangeY;
    }

    [Serializable]
    public class SkillData : ILoader<int, Skill>
    {
        public List<Skill> skills = new List<Skill>();

        // 읽은 데이터를 Dictionary로 변환하여 리턴하는 함수
        public Dictionary<int, Skill> MakeDict()
        {
            Dictionary<int, Skill> dict = new Dictionary<int, Skill>();

            foreach (Skill skill in skills)
                dict.Add(skill.id, skill);
            return dict;
        }
    }
    #endregion
}