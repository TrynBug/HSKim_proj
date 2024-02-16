using DamageNumbersPro;
using Google.Protobuf.Protocol;
using ServerCore;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using static Define;

// 숫자 스프라이트를 관리하는 클래스
public class NumberManager
{
    GameObject _root;
    GameObject _rootOriginal;
    GameObject _rootInstance;

    DamageNumber[] _numbers = new DamageNumber[Enum.GetValues(typeof(NumberType)).Length];

    public void Init()
    {
        // root 생성
        _root = new GameObject("@Numbers");
        UnityEngine.Object.DontDestroyOnLoad(_root);
        _rootOriginal = new GameObject("@Original");
        _rootOriginal.transform.SetParent(_root.transform);
        _rootInstance = new GameObject("@Instance");
        _rootInstance.transform.SetParent(_root.transform);


        // 숫자 prefab 생성
        foreach (NumberType type in Enum.GetValues(typeof(NumberType)))
        {
            DamageNumber number = null;
            switch(type)
            {
                case NumberType.Damage:
                    number = Managers.Resource.Instantiate("Number/Damage").GetOrAddComponent<DamageNumber>();
                    break;
                case NumberType.Heal:
                    number = Managers.Resource.Instantiate("Number/Heal").GetOrAddComponent<DamageNumber>();
                    break;
                case NumberType.Exp:
                    number = Managers.Resource.Instantiate("Number/Exp").GetOrAddComponent<DamageNumber>();
                    break;
            }

            if (number == null)
                continue;
            number.gameObject.SetActive(false);
            number.transform.SetParent(_rootOriginal.transform);
            _numbers[(int)type] = number;
        }

    }


    public DamageNumber GetNumberPrefab(NumberType type)
    {
        return _numbers[(int)type];
    }

    public void Spawn(NumberType type, Vector3 pos, int amount)
    {
        DamageNumber number = GetNumberPrefab(type);
        if (number == null)
            return;

        DamageNumber instance = number.Spawn(pos, amount);
        instance.transform.SetParent(_rootInstance.transform);

    }

    public void Clear()
    {
        foreach(Transform child in _rootInstance.transform)
        {
            DamageNumber number = child.GetComponent<DamageNumber>();
            if (number == null)
                continue;
            number.DestroyDNP();
        }
    }
}
