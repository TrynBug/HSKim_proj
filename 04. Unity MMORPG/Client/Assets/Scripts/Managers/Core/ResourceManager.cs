using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ResourceManager
{
    public T Load<T>(string path) where T : Object
    {
        // 로드하려는 오브젝트가 GameObject라면 pool에 있는지 찾아본다.
        if(typeof(T) == typeof(GameObject))
        {
            string name = path;
            int index = name.LastIndexOf('/');
            if (index >= 0)
                name = name.Substring(index + 1);

            // pool에서 찾았다면 리턴
            GameObject go = Managers.Pool.GetOriginal(name);
            if (go != null)
                return go as T;
        }

        // pool에서 못찾았다면 로드하여 리턴
        return Resources.Load<T>(path);
    }

    public GameObject Instantiate(string path, Transform parent = null)
    {
        GameObject original = Load<GameObject>($"Prefabs/{path}");
        if(original == null)
        {
            Debug.Log($"Failed to load prefab : {path}");
            return null;
        }

        // 생성하려는 객체가 Poolable 컴포넌트를 가지고 있다면 pool에서 가져옴
        if (original.GetComponent<Poolable>() != null)
            return Managers.Pool.Pop(original, parent).gameObject;

        // 생성하려는 객체가 Poolable 컴포넌트를 가지고있지 않다면 Instantiate
        GameObject go = Object.Instantiate(original, parent);
        go.name = original.name;   // prefab을 Instantiate 하면 이름에 "(Clone)"이 포함되는데 이것을 제거하기위해 원본이름을 덮어씀
        return go;
    }

    public void Destroy(GameObject go)
    {
        if (go == null)
            return;

        // 파괴하려는 객체가 Poolable 컴포넌트를 가지고 있다면 pool에 반환함
        Poolable poolable = go.GetComponent<Poolable>();
        if(poolable != null)
        {
            Managers.Pool.Push(poolable);
            return;
        }

        // 생성하려는 객체가 Poolable 컴포넌트를 가지고있지 않다면 파괴함
        Object.Destroy(go);
    }

    public void Destroy(GameObject go, float time)
    {
        if (go == null)
            return;
        Object.Destroy(go, time);
    }

}
