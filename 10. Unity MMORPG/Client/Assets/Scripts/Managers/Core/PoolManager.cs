using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using static UnityEngine.UI.Image;

// object pooling을 관리하기 위한 매니저
public class PoolManager
{
    // 게임 오브젝트 1개 종류에 대한 object pool
    #region
    class Pool
    {
        public GameObject Original { get; private set; }
        public Transform Root { get; set; }

        Stack<Poolable> _poolStack = new Stack<Poolable>();

        // object pool 초기화
        // original: 원본 객체, count: 초기 pool 크기
        public void Init(GameObject original, int count = 5)
        {
            Original = original;
            Root = new GameObject().transform;
            Root.name = $"{original.name}_Root";

            // count 만큼 오브젝트 생성하여 stack에 삽입
            for(int i=0; i<count; i++)
                Push(Create());
        }

        // Poolable 컴포넌트가 부착된 새로운 오브젝트 생성
        Poolable Create()
        {
            GameObject go = Object.Instantiate<GameObject>(Original);
            go.name = Original.name;
            return go.GetOrAddComponent<Poolable>();
        }

        // stack에 객체 삽입. 이 때 객체의 parent를 Root로 설정하고 deactivate 상태로 만든다.
        public void Push(Poolable poolable)
        {
            if (poolable == null)
                return;

            poolable.transform.parent = Root;
            poolable.gameObject.SetActive(false);
            poolable.IsUsing = false;
            _poolStack.Push(poolable);
        }

        // stack에서 객체를 꺼내고 parent를 지정한다. stack에 객체가 없다면 생성한다. 객체를 activate 한다.
        public Poolable Pop(Transform parent)
        {
            Poolable poolable;
            if (_poolStack.Count > 0)
                poolable = _poolStack.Pop();
            else
                poolable = Create();

            poolable.gameObject.SetActive(true);

            // 객체가 DontDestroyOnLoad가 되지 않게 하기위한 작업.
            // 처음에 DontDestroyOnLoad가 아닌 다른 오브젝트(Scene 등)의 하위에 등록해놓으면 DontDestroyOnLoad가 되지 않는다.
            if (parent == null)
                poolable.transform.parent = Managers.Scene.CurrentScene.transform;

            poolable.transform.parent = parent;
            poolable.IsUsing = true;

            return poolable;
        }
    }
    #endregion

    
    Dictionary<string, Pool> _pool = new Dictionary<string, Pool>();   // key가 original의 name이고, value가 object pool
    Transform _root;

    public void Init()
    {
        if(_root == null)
        {
            _root = new GameObject { name = "@Pool_Root" }.transform;
            Object.DontDestroyOnLoad(_root);
        }
    }

    // 전달받은 original에 대한 object pool을 생성하고 _pool에 추가한다.
    public void CreatePool(GameObject original, int count = 5)
    {
        Pool pool = new Pool();
        pool.Init(original, count);
        pool.Root.parent = _root;

        _pool.Add(original.name, pool);
    }
    
    // object를 pool에 반환
    public void Push(Poolable poolable)
    {
        string name = poolable.gameObject.name;

        // 만약 반환하려는 객체의 pool이 없을 경우에는 객체를 파괴함
        if(_pool.ContainsKey(name) == false)
        {
            GameObject.Destroy(poolable.gameObject);
            return;
        }

        // object를 pool에 반환
        _pool[name].Push(poolable);
    }

    // pool에서 object를 가져옴
    public Poolable Pop(GameObject original, Transform parent = null)
    {
        // original에 대한 pool 없을 경우 생성
        if(_pool.ContainsKey(original.name) == false)
            CreatePool(original);

        // pool에서 object를 꺼내 반환
        return _pool[original.name].Pop(parent);
    }

    // name에 해당하는 original 객체를 반환
    public GameObject GetOriginal(string name)
    {
        if (_pool.ContainsKey(name) == false)
            return null;

        return _pool[name].Original;
    }

    // pool 비우기
    public void Clear()
    {
        foreach (Transform child in _root)
            GameObject.Destroy(child.gameObject);

        _pool.Clear();
    }
}
