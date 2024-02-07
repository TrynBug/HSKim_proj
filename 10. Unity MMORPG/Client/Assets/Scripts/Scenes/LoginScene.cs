using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using ServerCore;
using System;
using Data;

public class LoginScene : BaseScene
{
    /* camera */
    public float Speed = 0.05f;
    public int WaitTime = 3000;

    enum CameraState
    {
        RightUp,
        LeftUp,
        RightDown,
        LeftDown,
        Wait
    }
    CameraState _state = CameraState.RightUp;
    CameraState _nextState = CameraState.RightUp;
    GameObject _cameraTarget;
    float _progress = 0;
    int _waitUntil = 0;

    public Vector3 P1 = new Vector3(-1.5f, 1.5f, 0f);
    public Vector3 P2 = new Vector3(-1.5f, 2f, 0f);
    public Vector3 P3 = new Vector3(0.5f, 2f, 0f);
    public Vector3 P4 = new Vector3(0.5f, 1.5f, 0f);

    /* object */
    List<PlayerController> _players = new List<PlayerController>();
    System.Random _rand = new System.Random();

    protected override void Awake()
    {
        base.Awake();

        // 해상도 설정
        //Screen.SetResolution(640, 480, false);  // 해상도 640 x 480 으로 설정하고 fullscreen은 false로 한다.
        Screen.SetResolution(1024, 768, false);

        // 맵 데이터 로드
        Managers.Map.LoadMap(1);
    }

    // BaseScene의 Start에서 호출함
    protected override void Init()
    {
        base.Init();

        ServerCore.Logger.Level = LogLevel.System;

        // UI 생성
        if (Config.DebugUIOn)
            Managers.UI.ShowSceneUI<UI_Debug>("UI_Debug");
        Managers.UI.ShowSceneUI<UI_Login>("UI_Login");

        // packet delay 설정
        if (Config.PacketDelayOn)
        {
            Managers.Network.PacketDelay = Config.PacketDelay;
            Managers.Network.IsDelayedPacket = true;
        }

        // camera
        _cameraTarget = new GameObject();
        _cameraTarget.transform.position = P4;
        Camera.main.transform.position = new Vector3(P4.x, P4.y, -10);

        InitObject();
    }


    void Update()
    {
        if (_cameraTarget == null)
            return;

        // camera position
        _progress += Time.deltaTime * Speed;
        _progress = Mathf.Clamp(_progress, 0f, 1f);
        switch (_state)
        {
            case CameraState.RightUp:
                {
                    _cameraTarget.transform.position = BezierLerp(P4, P3 - (P3 - P4), P1 - (P2 - P1), P1, _progress);

                    if (_progress >= 1f)
                    {
                        _nextState = CameraState.LeftUp;
                        _waitUntil = Environment.TickCount + WaitTime;
                        _state = CameraState.Wait;
                    }
                }
                break;
            case CameraState.LeftUp:
                {
                    _cameraTarget.transform.position = BezierLerp(P1, P1 - (P2 - P1), P3 - (P3 - P4), P4, _progress);

                    if (_progress >= 1f)
                    {
                        _nextState = CameraState.RightDown;
                        _waitUntil = Environment.TickCount + WaitTime;
                        _state = CameraState.Wait;
                    }
                }
                break;
            case CameraState.RightDown:
                {
                    _cameraTarget.transform.position = BezierLerp(P4, P3, P2, P1, _progress);

                    if (_progress >= 1f)
                    {
                        _nextState = CameraState.LeftDown;
                        _waitUntil = Environment.TickCount + WaitTime;
                        _state = CameraState.Wait;
                    }
                }
                break;
            case CameraState.LeftDown:
                {
                    _cameraTarget.transform.position = BezierLerp(P1, P2, P3, P4, _progress);

                    if (_progress >= 1f)
                    {
                        _nextState = CameraState.RightUp;
                        _waitUntil = Environment.TickCount + WaitTime;
                        _state = CameraState.Wait;
                    }
                }
                break;
            case CameraState.Wait:
                {
                    int tick = Environment.TickCount;
                    if (tick > _waitUntil)
                    {
                        _progress = 0f;
                        _state = _nextState;
                    }
                }
                break;
        }


        // attack animation
        {
            int tick = Environment.TickCount;
            foreach (PlayerController player in _players)
            {
                if (tick > player.LastUseSkill.lastUseTime)
                {
                    player.OnSkill(SkillId.SkillAttack);
                    player.LastUseSkill.lastUseTime = tick + _rand.Next(1000, 2000);
                }
                    
            }
            
        }
    }

    public override void Clear()
    {
        Destroy(_cameraTarget);
        _cameraTarget = null;
        foreach (BaseController player in _players)
        {
            Managers.Resource.Destroy(player.gameObject);
        }
        _players.Clear();
    }

    void LateUpdate()
    {
        if (_cameraTarget == null)
            return;
        Vector3 targetPos = new Vector3(_cameraTarget.transform.position.x, _cameraTarget.transform.position.y, -10);
        Camera.main.transform.position = Vector3.Lerp(Camera.main.transform.position, targetPos, Time.deltaTime * 2f);
        //Camera.main.transform.position = targetPos;
    }

    public Vector3 BezierLerp(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, float value)
    {
        Vector3 a = Vector3.Lerp(p1, p2, value);
        Vector3 b = Vector3.Lerp(p2, p3, value);
        Vector3 c = Vector3.Lerp(p3, p4, value);

        Vector3 d = Vector3.Lerp(a, b, value);
        Vector3 e = Vector3.Lerp(b, c, value);

        Vector3 f = Vector3.Lerp(d, e, value);
        return f;
    }

    void InitObject()
    {
        {
            GameObject go = Managers.Resource.Instantiate($"SPUM/SPUM000");
            PlayerController player = go.GetOrAddComponent<PlayerController>();
            ObjectInfo info = new ObjectInfo { PosInfo = new PositionInfo(), StatInfo = new StatInfo(), SPUMId = 0 };
            player.Init(info);
            player.LookDir = LookDir.LookRight;
            go.transform.position = new Vector3(-2.224f, 0.853f, Config.ObjectDefaultZ);
            player.SetHpBarActive(false);
            _players.Add(player);
        }

        {
            GameObject go = Managers.Resource.Instantiate($"SPUM/SPUM001");
            PlayerController player = go.GetOrAddComponent<PlayerController>();
            ObjectInfo info = new ObjectInfo { PosInfo = new PositionInfo(), StatInfo = new StatInfo(), SPUMId = 1 };
            player.Init(info);
            player.LookDir = LookDir.LookLeft;
            go.transform.position = new Vector3(-1.528f, 0.695f, Config.ObjectDefaultZ);
            player.SetHpBarActive(false);
            _players.Add(player);
        }

        {
            GameObject go = Managers.Resource.Instantiate($"SPUM/SPUM020");
            PlayerController player = go.GetOrAddComponent<PlayerController>();
            ObjectInfo info = new ObjectInfo { PosInfo = new PositionInfo(), StatInfo = new StatInfo(), SPUMId = 20 };
            player.Init(info);
            player.LookDir = LookDir.LookLeft;
            go.transform.position = new Vector3(1.846f, 0.228f, Config.ObjectDefaultZ);
            player.SetHpBarActive(false);
            _players.Add(player);
        }

    }
}
