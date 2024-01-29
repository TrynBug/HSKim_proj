using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;

// 모든 매니저에 쉽게 접근할 수 있게 해주는 매니저 클래스
// 싱글톤으로 구현된다.
public class Managers : MonoBehaviour
{
    static Managers s_instance;
    public static Managers Instance 
    {
        get
        {
            Init();
            return s_instance;
        }
    }

    #region Contents
    MapManager _map = new MapManager();
    ObjectManager _object = new ObjectManager();
    NetworkManager _network = new NetworkManager();
    TimeManager _time = new TimeManager();
    NumberManager _number = new NumberManager();

    public static MapManager Map { get { return Instance._map; } }
    public static ObjectManager Object { get { return Instance._object; } }
    public static NetworkManager Network { get { return Instance._network; } }
    public static TimeManager Time { get { return Instance._time; } }
    public static NumberManager Number { get { return Instance._number; } }
    #endregion


    #region Core
    InputManager _input = new InputManager();
    ResourceManager _resource = new ResourceManager();
    UIManager _ui = new UIManager();
    SceneManagerEx _scene = new SceneManagerEx();
    SoundManager _sound = new SoundManager();
    PoolManager _pool = new PoolManager();
    DataManager _data = new DataManager();

    public static InputManager Input { get { return Instance._input; } }
    public static ResourceManager Resource { get { return Instance._resource; } }
    public static UIManager UI { get { return Instance._ui; } }
    public static SceneManagerEx Scene { get { return Instance._scene; } }
    public static SoundManager Sound { get { return Instance._sound; } }
    public static PoolManager Pool { get { return Instance._pool; } }
    public static DataManager Data { get { return Instance._data; } }
    #endregion

    static void Init()
    {
        // instance가 null일 경우 게임매니저 생성
        if (s_instance == null)
        {
            GameObject go = GameObject.Find("@Managers");    // @Managers 이름의 오브젝트를 찾는다.
            if (go == null)
            {
                go = new GameObject { name = "@Managers" };  // null일 경우 @Managers 이름의 빈 오브젝트를 생성한다.
                go.AddComponent<Managers>();                 // Managers 유니티 스크립트를 부착한다.
            }

            DontDestroyOnLoad(go);  // 씬을 넘어갈 때 이 오브젝트를 파괴하지 않도록 설정.
            s_instance = go.GetComponent<Managers>();      // Instance에 Managers 컴포넌트 할당

            s_instance._data.Init();   // DataManager 초기화
            s_instance._sound.Init();  // SoundManager 초기화
            s_instance._object.Init();
            s_instance._pool.Init();   // PoolManager 초기화
            s_instance._network.Init();
            s_instance._time.Init();
            s_instance._number.Init();
        }
    }

    public static void Clear()
    {
        Input.Clear();
        Sound.Clear();
        UI.Clear();
        Scene.Clear();

        // pool의 clear 순서는 마지막으로 한다.
        Pool.Clear();
    }

    // Start is called before the first frame update
    void Start()
    {
        Init();
    }

    // Update is called once per frame
    void Update()
    {
        _network.OnUpdate();
        _input.OnUpdate();
        _time.OnUpdate();
        _map.OnUpdate();
    }
}
