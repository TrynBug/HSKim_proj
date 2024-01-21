using Google.Protobuf.Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Data;
using UnityEngine;
using static Define;

namespace Data
{
    // 각각의 json 데이터들에 해당하는 데이터 클래스
    // Json Parser에 의해 json 파일의 데이터들이 이곳의 클래스들로 파싱된다.

    #region Stat
    [Serializable]
    public class StatDataLoader : IJsonLoader<int, StatInfo>
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
    public class SkillData
    {
        public SkillId id;
        public string name;
        public int cooldown;
        public int castingTime;
        public int skillTime;
        public float speedRate;
        public int damage;
        public float rangeX;
        public float rangeY;
        public SkillType skillType;
        public string hitEffect;
        public float effectOffsetY;
        public ProjectileData projectile;
    }

    [Serializable]
    public class ProjectileData
    {
        public string name;
        public float speed;
        public string prefab;
        public float offsetY;
        public string hitEffect;
        public float effectOffsetY;
    }

    [Serializable]
    public class SkillDataLoader : IJsonLoader<SkillId, SkillData>
    {
        public List<SkillData> skills = new List<SkillData>();

        // 읽은 데이터를 Dictionary로 변환하여 리턴하는 함수
        public Dictionary<SkillId, SkillData> MakeDict()
        {
            Dictionary<SkillId, SkillData> dict = new Dictionary<SkillId, SkillData>();

            foreach (SkillData skill in skills)
                dict.Add(skill.id, skill);
            return dict;
        }
    }
    #endregion





    #region Item
    public class ItemDataLoader : ICSVLoader<int, Item>
    {
        // 엑셀 데이터를 전달받아 key=id, value=Item 인 dictionary를 만들어 리턴함
        public Dictionary<int, Item> LoadCSVData(DataSet dataset)
        {
            Dictionary<int, Item> dict = new Dictionary<int, Item>();

            //시트 개수만큼 반복
            for (int i = 0; i < dataset.Tables.Count; i++)
            {
                DataTable table = dataset.Tables[i];

                //해당 시트의 행데이터(한줄씩)로 반복
                for (int j = 1; j < table.Rows.Count; j++)
                {
                    DataRow row = table.Rows[j];

                    // item 생성
                    Item item = new Item();
                    item.id = int.Parse(row[0].ToString());
                    item.type = (EquipmentType)Enum.Parse(typeof(EquipmentType), row[1].ToString());
                    item.subType = (EquipmentSubType)Enum.Parse(typeof(EquipmentSubType), row[2].ToString());
                    item.spriteName = row[3].ToString();
                    item.damage = int.Parse(row[4].ToString());
                    item.rangeX = float.Parse(row[5].ToString());
                    item.rangeY = float.Parse(row[6].ToString());
                    item.attackSpeed = float.Parse(row[7].ToString());
                    item.hp = int.Parse(row[8].ToString());
                    item.defence = int.Parse(row[9].ToString());
                    item.speed = float.Parse(row[10].ToString());

                    // dict에 삽입
                    dict.Add(item.id, item);
                }
            }

            return dict;
        }

        // key가 id인 dictionary를 통해 key가 spriteName인 dictionary를 만들어 리턴함
        public static Dictionary<string, Item> MakeSpriteItemDict(Dictionary<int, Item> itemDict)
        {
            Dictionary<string, Item> dict = new Dictionary<string, Item>();
            foreach (Item item in itemDict.Values)
            {
                dict.Add(item.spriteName, item);
            }
            return dict;
        }
    }

    [Serializable]
    public class Item
    {
        public int id;
        public EquipmentType type;
        public EquipmentSubType subType;
        public string spriteName;
        public int damage;
        public float rangeX;
        public float rangeY;
        public float attackSpeed;
        public int hp;
        public int defence;
        public float speed;
    }


    #endregion



    #region SPUM
    public class SPUMDataLoader : IJsonLoader<int, SPUMData>
    {
        public List<SPUMData> spums = new List<SPUMData>();

        // 읽은 데이터를 Dictionary로 변환하여 리턴하는 함수
        public Dictionary<int, SPUMData> MakeDict()
        {
            Dictionary<int, SPUMData> dict = new Dictionary<int, SPUMData>();

            foreach (SPUMData spum in spums)
            {
                dict.Add(spum.id, spum);
            }

            return dict;
        }
    }

    [Serializable]
    public class SPUMData
    {
        public int id;
        public string prefabName;
        public int back;
        public int cloth;
        public int armor;
        public int helmet;
        public int weaponLeft;
        public int weaponRight;
    }
    #endregion




    #region Map
    [Serializable]
    public class MapData
    {
        public int id;
        public float cellWidth;
        public float cellHeight;
        public int cellBoundMinX;
        public int cellBoundMaxX;
        public int cellBoundMinY;
        public int cellBoundMaxY;
        public List<string> collisions = new List<string>();
        public List<TeleportData> teleports = new List<TeleportData>();
        public List<EnterZoneData> enterZones = new List<EnterZoneData>();
    }

    [Serializable]
    public class TeleportData
    {
        public int number;
        public float posX;
        public float posY;
        public float width;
        public float height;
    }

    [Serializable]
    public class EnterZoneData
    {
        public int number;
        public float posX;
        public float posY;
    }


    [Serializable]
    public class MapDataLoader : IJsonLoader<int, MapData>
    {
        public List<MapData> maps = new List<MapData>();

        // 읽은 데이터를 Dictionary로 변환하여 리턴하는 함수
        public Dictionary<int, MapData> MakeDict()
        {
            Dictionary<int, MapData> dict = new Dictionary<int, MapData>();

            foreach (MapData map in maps)
                dict.Add(map.id, map);
            return dict;
        }
    }
    #endregion
}