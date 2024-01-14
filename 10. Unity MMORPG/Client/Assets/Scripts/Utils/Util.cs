using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using UnityEngine;
using ServerCore;
using Data;
using Google.Protobuf.Protocol;

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


    // float vector 비교
    public static bool Equals(Vector2 a, Vector2 b)
    {
        return (Mathf.Abs(a.x - b.x) + Mathf.Abs(a.y - b.y) < Config.DifferenceTolerance);
    }


    // 반대방향 얻기
    public static MoveDir GetOppositeDirection(MoveDir dir)
    {
        MoveDir opposite;
        switch (dir)
        {
            case MoveDir.Up:
                opposite = MoveDir.Down;
                break;
            case MoveDir.Down:
                opposite = MoveDir.Up;
                break;
            case MoveDir.Left:
                opposite = MoveDir.Right;
                break;
            case MoveDir.Right:
                opposite = MoveDir.Left;
                break;
            case MoveDir.LeftUp:
                opposite = MoveDir.RightDown;
                break;
            case MoveDir.LeftDown:
                opposite = MoveDir.RightUp;
                break;
            case MoveDir.RightUp:
                opposite = MoveDir.LeftDown;
                break;
            case MoveDir.RightDown:
                opposite = MoveDir.LeftUp;
                break;
            default:
                opposite = dir;
                break;
        }

        return opposite;
    }

    // 현재 방향에 해당하는 벡터 얻기
    public static Vector2 GetDirectionVector(MoveDir dir)
    {
        Vector2 direction;
        switch (dir)
        {
            case MoveDir.Up:
                direction = new Vector2(1, -1).normalized;
                break;
            case MoveDir.Down:
                direction = new Vector2(-1, 1).normalized;
                break;
            case MoveDir.Left:
                direction = new Vector2(-1, -1).normalized;
                break;
            case MoveDir.Right:
                direction = new Vector2(1, 1).normalized;
                break;
            case MoveDir.LeftUp:
                direction = new Vector2(0, -1);
                break;
            case MoveDir.LeftDown:
                direction = new Vector2(-1, 0);
                break;
            case MoveDir.RightUp:
                direction = new Vector2(1, 0);
                break;
            case MoveDir.RightDown:
                direction = new Vector2(0, 1);
                break;
            default:
                direction = new Vector2(0, 0);
                break;
        }

        return direction;
    }

}
