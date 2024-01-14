using Google.Protobuf.Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft;

// json 데이터 클래스가 상속받아야 하는 인터페이스.
// MakeDict 함수는 클래스 내의 데이터를 Dictionary 형태로 변환해준다.
public interface ILoader<Key, Value>
{
    Dictionary<Key, Value> MakeDict();
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
    public Data.Skill DefaultSkill { get; private set; }


    public void Init()
    {
        // StatData.json 파일 읽어서 데이터 추출
        StatDict = LoadJson<Data.StatData, int, StatInfo>("StatData").MakeDict();
        SkillDict = LoadJson<Data.SkillData, int, Data.Skill>("SkillData").MakeDict();

        // default data
        Data.Skill defaultSkill;
        SkillDict.TryGetValue(1, out defaultSkill);
        DefaultSkill = defaultSkill;
    }

    Loader LoadJson<Loader, key, Value>(string path) where Loader : ILoader<key, Value>
    {
        TextAsset textAsset = Managers.Resource.Load<TextAsset>($"Data/{path}");  // json 파일 읽기

        return Newtonsoft.Json.JsonConvert.DeserializeObject<Loader>(textAsset.text);

        //return JsonUtility.FromJson<Loader>(textAsset.text);  // json 파일에서 Value 형식의 데이터를 읽어 Loader에 입력
    }

}
