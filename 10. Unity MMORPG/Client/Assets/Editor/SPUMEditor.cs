
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO;
using System.Text.RegularExpressions;
using System;
using System.Linq;
using UnityEngine.TextCore.Text;
using Google.Protobuf.Protocol;









#if UNITY_EDITOR
using UnityEditor;
#endif

public class SPUMEditor
{
#if UNITY_EDITOR


    // Prefabs/SPUM 아래의 "SPUM###" 이름의 prefab에서 SPUM 장비 정보를 추출하여 파일로 생성한다.
    // Tools 메뉴 아래에 Generate SPUM Data 메뉴로 만든다.
    // 단축키는 Ctrl + Shift + s 로 한다. attribute에서의 단축키 예약어는 %(Ctrl), #(Shift), &(Alt) 로 지정되어 있다.
    [MenuItem("Tools/Generate SPUM Data %#s")]
    private static void GenerateSPUMData()
    {
        GenerateByPath("Assets/Resources/Data");
        GenerateByPath("../Server/Common/Data");
    }


    private static void GenerateByPath(string path)
    {
        // Prefabs/SPUM 아래의 모든 prefab 조사
        GameObject[] gameObjects = Resources.LoadAll<GameObject>("Prefabs/SPUM");
        if (gameObjects.Length == 0)
            return;

        // 아이템 데이터 로드
        DataManager dataManager = new DataManager();
        Dictionary<string, Data.Item> spriteItemDict = dataManager.MakeOnlySpriteItemDict();

        // SPUMData 데이터 생성
        List<Data.SPUMData> listSpum = new List<Data.SPUMData>();
        Regex regexPrefabNumber = new Regex("Unit([0-9]{1,})$");
        foreach (GameObject go in gameObjects)
        {
            // prefab 이름에서 끝에 숫자를 추출함
            string prefabNumber = regexPrefabNumber.Replace(go.name, "$1");
            int id;
            if (int.TryParse(prefabNumber, out id) == false)
                continue;

            Data.SPUMData spum = new Data.SPUMData();
            spum.id = id;
            spum.prefabName = go.name;

            // 각각의 part에 있는 아이템의 코드를 얻는다.
            spum.back = GetItemCode(go, "P_Back", spriteItemDict);
            spum.cloth = GetItemCode(go, "P_ClothBody", spriteItemDict);
            spum.armor = GetItemCode(go, "P_ArmorBody", spriteItemDict);
            spum.helmet = GetItemCode(go, "P_Helmet", spriteItemDict);
            spum.weaponLeft = GetItemCode(go, "L_Weapon", spriteItemDict);
            if(spum.weaponLeft == 0)
                spum.weaponLeft = GetItemCode(go, "L_Shield", spriteItemDict);
            spum.weaponRight = GetItemCode(go, "R_Weapon", spriteItemDict);
            if (spum.weaponRight == 0)
                spum.weaponRight = GetItemCode(go, "R_Shield", spriteItemDict);
            spum.weaponLeft = GetItemCode(go, "L_Weapon", spriteItemDict);
            spum.horse = GetItemCode(go, "BodyBack", spriteItemDict);

            // horse 여부 확인
            if (spum.horse == 0)
                spum.hasHorse = false;
            else
                spum.hasHorse = true;

            // class 확인
            Data.Item itemLeft = dataManager.ItemDict.GetValueOrDefault(spum.weaponLeft, null);
            Data.Item itemRight = dataManager.ItemDict.GetValueOrDefault(spum.weaponRight, null);
            Define.EquipmentSubType itemLeftType = itemLeft == null ? Define.EquipmentSubType.Empty : itemLeft.subType;
            Define.EquipmentSubType itemRightType = itemRight == null ? Define.EquipmentSubType.Empty : itemRight.subType;
            if (itemLeftType == Define.EquipmentSubType.Weapon_Wand || itemRightType == Define.EquipmentSubType.Weapon_Wand)
                spum.spumClass = SPUMClass.SpumWizard;
            else if (itemLeftType == Define.EquipmentSubType.Weapon_Bow || itemRightType == Define.EquipmentSubType.Weapon_Bow)
                spum.spumClass = SPUMClass.SpumArcher;
            else
                spum.spumClass = SPUMClass.SpumKnight;


            listSpum.Add(spum);
        }


        // json 으로 변환후 write
        string jsonText = Newtonsoft.Json.JsonConvert.SerializeObject(listSpum, Newtonsoft.Json.Formatting.Indented);
        StreamWriter writer = File.CreateText($"{path}/SPUMData.json");
        writer.Write("{\n\"spums\": ");
        writer.Write(jsonText);
        writer.Write("\n}");
        writer.Close();
    }


    // rootObject 아래에서 partName 이름의 오브젝트를 찾은 다음, 해당 오브젝트 아래에서 sprite가 존재하는 SpriteRenderer 컴포넌트를 찾는다.
    // 그런다음 sprite의 이름을 조사하여 아이템코드를 찾아 반환한다.
    static int GetItemCode(GameObject rootObject, string partName, Dictionary<string, Data.Item> spriteItemDict)
    {
        // rootObject 아래에서 partName 이름의 오브젝트를 찾는다.
        GameObject part = Util.FindChild(rootObject, partName, recursive: true);
        if (part == null)
            return 0;

        // part 오브젝트 아래에서 sprite가 존재하는 SpriteRenderer 컴포넌트를 찾는다.
        SpriteRenderer sprite = null;
        foreach (SpriteRenderer sp in part.GetComponentsInChildren<SpriteRenderer>())
        {
            if (sp.sprite != null)
            {
                sprite = sp;
                break;
            }
        }
        if (sprite == null)
            return 0;

        // sprite 이름으로 아이템코드를 찾는다.
        string spriteName = AssetDatabase.GetAssetPath(sprite.sprite);
        spriteName = Path.GetFileNameWithoutExtension(spriteName);
        Data.Item item;
        if (spriteItemDict.TryGetValue(spriteName, out item) == false)
            return 0;

        return item.id;
    }

#endif
}
