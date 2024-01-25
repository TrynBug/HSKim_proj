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
using System.Text;

// json 데이터 클래스가 상속받아야 하는 인터페이스.
// MakeDict 함수는 클래스 내의 데이터를 Dictionary 형태로 변환해준다.
public interface IJsonLoader<Key, Value>
{
    Dictionary<Key, Value> MakeDict();
}

// csv 데이터 클래스가 상속받아야 하는 인터페이스.
// Load 함수는 csv 데이터 1줄을 받아 자신 클래스를 초기화하는 함수이다.
public interface ICSVLoader<Key, DataClass>
{
    Dictionary<Key, DataClass> LoadCSVData(DataSet dataset);
}

// 게임 데이터 파일들을 관리하는 매니저
// 서버와 클라이언트의 DataManager 코드가 다른데, 그 이유는 양쪽에서 공통으로 사용하는 데이터와 그렇지 않은 데이터가 있기 때문이다.
// 예를들면 강화성공확률 같은 경우는 서버쪽에서만 가지고 있어야 할 수가 있다.
public class DataManager
{
    // Stat 데이터 dictionary. key=level, data=StatInfo
    public Dictionary<int, StatInfo> StatDict { get; private set; } = new Dictionary<int, StatInfo>();    
    // Skill 데이터 dictionary. key=skillId, data=Skill
    public Dictionary<SkillId, Data.SkillData> SkillDict { get; private set; } = new Dictionary<SkillId, Data.SkillData>();
    // Item 데이터 dictionary. key=itemId, data=Item
    public Dictionary<int, Data.Item> ItemDict { get; private set; } = new Dictionary<int, Data.Item>();
    public Dictionary<string, Data.Item> SpriteItemDict { get; private set; } = new Dictionary<string, Data.Item>();
    // SPUM 데이터
    public Dictionary<int, Data.SPUMData> SPUMDict { get; private set; } = new Dictionary<int, Data.SPUMData>();
    // Map 데이터
    public Dictionary<int, Data.MapData> MapDict { get; private set; } = new Dictionary<int, Data.MapData>();

    // default data
    public StatInfo DefaultStat { get; private set; }
    public Data.SkillData DefaultSkill { get; private set; }
    public Data.Item DefaultItem { get; private set; }
    public Data.SPUMData DefaultSPUM { get; private set; }

    static bool _isFirstCall = true;
    public void Init()
    {
        // codepace 1252 엑셀 파일을 읽기위해 codepage 등록
        if (_isFirstCall)
        {
            _isFirstCall = false;
            var enc = System.Text.CodePagesEncodingProvider.Instance.GetEncoding(1252);
            Encoding.RegisterProvider(System.Text.CodePagesEncodingProvider.Instance);
        }

        // StatData.json 파일 읽어서 데이터 추출
        StatDict = LoadJson<StatDataLoader, int, StatInfo>("StatData").MakeDict();
        SkillDict = LoadJson<SkillDataLoader, SkillId, SkillData>("SkillData").MakeDict();
        ItemDict = LoadCSV<ItemDataLoader, int, Item>("ItemData");
        SpriteItemDict = ItemDataLoader.MakeSpriteItemDict(ItemDict);
        SPUMDict = LoadJson<SPUMDataLoader, int, SPUMData>("SPUMData").MakeDict();
        MapDict = LoadJson<MapDataLoader, int, MapData>("MapData").MakeDict();

        // default data
        DefaultStat = StatDict.GetValueOrDefault(1, null);
        DefaultSkill = SkillDict.GetValueOrDefault(SkillId.SkillAttack, null);
        DefaultItem = ItemDict.GetValueOrDefault(0, null);
        DefaultSPUM = SPUMDict.GetValueOrDefault(0, null);
    }

    Loader LoadJson<Loader, key, Value>(string path) where Loader : IJsonLoader<key, Value>
    {
        TextAsset textAsset = Managers.Resource.Load<TextAsset>($"Data/{path}");  // json 파일 읽기

        return Newtonsoft.Json.JsonConvert.DeserializeObject<Loader>(textAsset.text);

    }

    Dictionary<Key, DataClass> LoadCSV<Loader, Key, DataClass>(string path) where Loader : ICSVLoader<Key, DataClass>, new()
    {
        TextAsset textAsset = Managers.Resource.Load<TextAsset>($"Data/{path}");  // json 파일 읽기
        using (MemoryStream stream = new MemoryStream(textAsset.bytes))
        {
            using (var reader = ExcelReaderFactory.CreateCsvReader(stream))
            {
                DataSet result = reader.AsDataSet();

                Loader loader = new Loader();
                return loader.LoadCSVData(result);
            }
        }
    }



    // SPUM Editor를 위한 데이터생성 함수
    public Dictionary<string, Data.Item> MakeOnlySpriteItemDict()
    {
        // 아이템 데이터 생성하기
        TextAsset textAsset = Resources.Load<TextAsset>($"Data/ItemData");  // item 데이터파일 열기
        using (MemoryStream stream = new MemoryStream(textAsset.bytes))
        {
            using (var reader = ExcelReaderFactory.CreateCsvReader(stream))
            {
                DataSet result = reader.AsDataSet();

                ItemDataLoader loader = new ItemDataLoader();
                ItemDict = loader.LoadCSVData(result);
            }
        }

        // sprite-item 데이터 생성하기
        SpriteItemDict = ItemDataLoader.MakeSpriteItemDict(ItemDict);
        return SpriteItemDict;
    }
}
