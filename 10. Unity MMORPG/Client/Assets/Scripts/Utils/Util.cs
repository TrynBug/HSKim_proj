using System.Collections;
using System.Collections.Generic;
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


    // 현재 방향에 해당하는 벡터 얻기
    public static Vector2 GetDirectionVector(LookDir dir)
    {
        Vector2 direction;
        switch (dir)
        {
            case LookDir.LookLeft:
                direction = new Vector2(-1, -1).normalized;
                break;
            case LookDir.LookRight:
                direction = new Vector2(1, 1).normalized;
                break;
            default:
                direction = new Vector2(0, 0);
                break;
        }

        return direction;
    }


    // origin을 기준으로 dir 방향의 cell을 찾는다.
    public static Vector2Int FindCellOfDirection(MoveDir dir, Vector2Int origin)
    {
        Vector2Int cell;
        switch (dir)
        {
            case MoveDir.Up:
                cell = new Vector2Int(origin.x + 1, origin.y - 1);
                break;
            case MoveDir.Down:
                cell = new Vector2Int(origin.x - 1, origin.y + 1);
                break;
            case MoveDir.Left:
                cell = new Vector2Int(origin.x - 1, origin.y - 1);
                break;
            case MoveDir.Right:
                cell = new Vector2Int(origin.x + 1, origin.y + 1);
                break;
            case MoveDir.LeftUp:
                cell = new Vector2Int(origin.x, origin.y - 1);
                break;
            case MoveDir.LeftDown:
                cell = new Vector2Int(origin.x - 1, origin.y);
                break;
            case MoveDir.RightUp:
                cell = new Vector2Int(origin.x + 1, origin.y);
                break;
            case MoveDir.RightDown:
                cell = new Vector2Int(origin.x, origin.y + 1);
                break;
            default:
                cell = origin;
                break;
        }
        return cell;
    }


    // pos에서 dest로의 방향 얻기
    public static MoveDir GetDirectionToDest(Vector2 pos, Vector2 dest)
    {
        const float r1 = Mathf.PI / 180f * (22.5f + 45f * 0);
        const float r2 = Mathf.PI / 180f * (22.5f + 45f * 1);
        const float r3 = Mathf.PI / 180f * (22.5f + 45f * 2);
        const float r4 = Mathf.PI / 180f * (22.5f + 45f * 3);
        const float r5 = Mathf.PI / 180f * -(22.5f + 45f * 3);
        const float r6 = Mathf.PI / 180f * -(22.5f + 45f * 2);
        const float r7 = Mathf.PI / 180f * -(22.5f + 45f * 1);
        const float r8 = Mathf.PI / 180f * -(22.5f + 45f * 0);

        Vector2 diff = (dest - pos).normalized;
        float radian = Mathf.Atan2(-diff.y, diff.x);

        if(radian > 0)
        {
            if (radian < r1)
                return MoveDir.RightUp;
            else if (radian < r2)
                return MoveDir.Up;
            else if (radian < r3)
                return MoveDir.LeftUp;
            else if (radian < r4)
                return MoveDir.Left;
            else
                return MoveDir.LeftDown;
        }
        else
        {
            if (radian < r5)
                return MoveDir.LeftDown;
            else if (radian < r6)
                return MoveDir.Down;
            else if (radian < r7)
                return MoveDir.RightDown;
            else if (radian < r8)
                return MoveDir.Right;
            else
                return MoveDir.RightUp;
        }
    }




    // transform을 기준으로 origin 에서 target를 바라볼 때의 바라보는 방향 얻기
    public static LookDir GetLookDirectionToTarget(BaseController origin, BaseController target)
    {
        if (origin.transform.position.x > target.transform.position.x)
            return LookDir.LookLeft;
        else
            return LookDir.LookRight;
    }

    public static LookDir GetLookDirectionToTarget(BaseController origin, Vector2 target)
    {
        if (origin.transform.position.x > Managers.Map.ServerPosToClientPos(target).x)
            return LookDir.LookLeft;
        else
            return LookDir.LookRight;
    }



    // id 에서 타입 얻기
    public static GameObjectType GetObjectTypeById(int id)
    {
        // bits : [ Unused(1) | Type(7) | Id(24) ]
        int type = (id >> 24) & 0x7F;
        return (GameObjectType)type;
    }


    // posOrigin을 기준으로, look 방향으로 사각형의 range 범위 내에 target이 위치하는지를 확인함
    public static bool IsTargetInRectRange(Vector2 posOrigin, LookDir look, Vector2 range, Vector2 posTarget)
    {
        // 점을 0도 방향으로 회전시키기 위한 cos, sin 값
        Vector2 R;
        if (look == LookDir.LookLeft)
            R = new Vector2(Mathf.Cos(Mathf.PI / 180f * 135f), Mathf.Sin(Mathf.PI / 180f * 135f));
        else
            R = new Vector2(Mathf.Cos(Mathf.PI / 180f * 315f), Mathf.Sin(Mathf.PI / 180f * 315f));

        // target이 range 범위의 사각형 안에 존재하는지 확인
        Vector2 objectPos;
        objectPos.x = (posTarget.x - posOrigin.x) * R.x - (posTarget.y - posOrigin.y) * R.y;
        objectPos.y = (posTarget.x - posOrigin.x) * R.y + (posTarget.y - posOrigin.y) * R.x;
        if (objectPos.x > 0 && objectPos.x < range.x && objectPos.y > -range.y / 2 && objectPos.y < range.y / 2)
        {
            return true;
        }

        return false;

    }

}
