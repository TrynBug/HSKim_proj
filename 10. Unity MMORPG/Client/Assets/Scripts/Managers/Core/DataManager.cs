using Google.Protobuf.Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft;
using ExcelDataReader;
using System.Data;
using System.IO;
using Data;

// json 데이터 클래스가 상속받아야 하는 인터페이스.
// MakeDict 함수는 클래스 내의 데이터를 Dictionary 형태로 변환해준다.
public interface IJsonLoader<Key, Value>
{
    Dictionary<Key, Value> MakeDict();
}

// excel 데이터 클래스가 상속받아야 하는 인터페이스.
// Load 함수는 excel 데이터 1줄을 받아 자신 클래스를 초기화하는 함수이다.
public interface IExcelLoader<Key, DataClass>
{
    Dictionary<Key, DataClass> LoadExcelData(DataSet dataset);
}

// 게임 데이터 파일들을 관리하는 매니저
// 서버와 클라이언트의 DataManager 코드가 다른데, 그 이유는 양쪽에서 공통으로 사용하는 데이터와 그렇지 않은 데이터가 있기 때문이다.
// 예를들면 강화성공확률 같은 경우는 서버쪽에서만 가지고 있어야 할 수가 있다.
public class DataManager
{
    // Stat 데이터 dictionary. key=level, data=StatInfo
    public Dictionary<int, StatInfo> StatDict { get; private set; } = new Dictionary<int, StatInfo>();    
    // Skill 데이터 dictionary. key=skillId, data=Skill
    public Dictionary<int, Data.Skill> SkillDict { get; private set; } = new Dictionary<int, Data.Skill>();
    // Item 데이터 dictionary. key=itemId, data=Item
    public Dictionary<int, Data.Item> ItemDict { get; private set; } = new Dictionary<int, Data.Item>();
    public Dictionary<string, Data.Item> SpriteItemDict { get; private set; } = new Dictionary<string, Data.Item>();
    // SPUM 데이터
    public Dictionary<int, Data.SPUMData> SPUMDict { get; private set; } = new Dictionary<int, Data.SPUMData>();

    // default data
    public Data.Skill DefaultSkill { get; private set; }

    public void Init()
    {
        // StatData.json 파일 읽어서 데이터 추출
        StatDict = LoadJson<Data.StatData, int, StatInfo>("StatData").MakeDict();
        SkillDict = LoadJson<Data.SkillData, int, Data.Skill>("SkillData").MakeDict();
        ItemDict = LoadExcel<ItemDataLoader, int, Item>("ItemData");
        SpriteItemDict = ItemDataLoader.MakeSpriteItemDict(ItemDict);
        SPUMDict = LoadJson<SPUMDataLoader, int, SPUMData>("SPUMData").MakeDict();

        // default data
        Data.Skill defaultSkill;
        SkillDict.TryGetValue(1, out defaultSkill);
        DefaultSkill = defaultSkill;
    }

    Loader LoadJson<Loader, key, Value>(string path) where Loader : IJsonLoader<key, Value>
    {
        TextAsset textAsset = Managers.Resource.Load<TextAsset>($"Data/{path}");  // json 파일 읽기

        return Newtonsoft.Json.JsonConvert.DeserializeObject<Loader>(textAsset.text);

    }

    Dictionary<Key, DataClass> LoadExcel<Loader, Key, DataClass>(string path) where Loader : IExcelLoader<Key, DataClass>, new()
    {
        using (var stream = File.Open($"Assets/Resources/Data/{path}.xlsx", FileMode.Open, FileAccess.Read, FileShare.Read))
        {
            using (var reader = ExcelReaderFactory.CreateReader(stream))
            {
                DataSet result = reader.AsDataSet();

                Loader loader = new Loader();
                return loader.LoadExcelData(result);
            }
        }
    }



    // SPUM Editor를 위한 데이터생성 함수
    public Dictionary<string, Data.Item> MakeOnlySpriteItemDict()
    {
        Dictionary<int, Data.Item> itemDict = LoadExcel<ItemDataLoader, int, Item>("ItemData");
        Dictionary<string, Data.Item> spriteItemDict = ItemDataLoader.MakeSpriteItemDict(itemDict);
        return spriteItemDict;
    }
}
