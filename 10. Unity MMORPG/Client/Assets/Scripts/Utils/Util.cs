using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using UnityEngine;
using ServerCore;
using Data;

public static class Util
{
    // 게임오브젝트에서 T 컴포넌트를 찾는다. 없다면 Add한다.
    public static T GetOrAddComponent<T>(GameObject go) where T : UnityEngine.Component
    {
        T component = go.GetComponent<T>();
        if (component == null)
            component = go.AddComponent<T>();
        return component;
    }

    // go 오브젝트 아래에서 name 이름의 오브젝트를 찾는다.
    public static GameObject FindChild(GameObject go, string name = null, bool recursive = false)
    {
        Transform transform = FindChild<Transform>(go, name, recursive);
        if (transform == null)
            return null;

        return transform.gameObject;
    }

    // go 오브젝트 아래에서 T 타입의 컴포넌트를 가진 오브젝트를 찾아 해당 컴포넌트를 리턴한다.
    public static T FindChild<T>(GameObject go, string name = null, bool recursive = false) where T : UnityEngine.Object
    {
        if (go == null)
            return null;

        if(recursive == false)
        {
            for(int i=0; i<go.transform.childCount; i++)
            {
                Transform transform = go.transform.GetChild(0);
                if (string.IsNullOrEmpty(name) || transform.name == name)
                {
                    T component = transform.GetComponent<T>();
                    if (component != null)
                        return component;
                }
            }
            
        }
        else
        {
            foreach (T component in go.GetComponentsInChildren<T>())
            {
                if (string.IsNullOrEmpty(name) || component.name == name)
                    return component;
            }
        }

        return null;
    }


    // ServerCore 라이브러리에서 로그 발생시의 동작
    public static void OnNetworkLog(ServerCore.LogLevel level, string log)
    {
        Debug.Log(log);
    }




    public static Vector2Int PosToCell(Vector3 pos)
    {
        return new Vector2Int((int)(pos.x * (float)Config.CellMultiple), (int)(pos.y * (float)Config.CellMultiple));
    }

    public static Vector3 CellToPos(Vector2Int cell, float z = Config.ObjectDefaultZ)
    {
        return new Vector3((float)cell.x / (float)Config.CellMultiple, (float)cell.y / (float)Config.CellMultiple, z);
    }

    public static Vector3 CellToCenterPos(Vector2Int cell, float z = Config.ObjectDefaultZ)
    {
        Vector3 pos = CellToPos(cell, z);
        return new Vector3(pos.x + Config.CellWidth, pos.y + Config.CellHeight, pos.z);
    }

    public static bool Equals(Vector3 a, Vector3 b)
    {
        return (Mathf.Abs(a.x - b.x) + Mathf.Abs(a.y - b.y) < Config.DifferenceTolerance);
    }


}
