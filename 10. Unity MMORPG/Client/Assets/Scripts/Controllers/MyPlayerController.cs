using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using ServerCore;
using static Define;
using Data;
using System;
using TMPro;
using System.Linq;


public class MyPlayerController : SPUMController
{
    C_Move _lastCMove = null;       // 마지막으로 보낸 C_Move 패킷
    int _lastCMoveSendTime = 0;     // 마지막으로 C_Move 패킷 보낸 시간 (단위:ms)

    float _speedRateForStop = 1f;    // 움직임을 멈출 때 속도를 감소시킬 비율 (목적지와의 거리에 따라 값이 변경됨)

    /* key input */
    public Dictionary<UnityEngine.KeyCode, KeyInput> KeyMap { get; private set; } = new Dictionary<KeyCode, KeyInput>();   // 키보드키와 게임기능키 연결맵
    public Dictionary<KeyInput, SkillId> KeySkillMap { get; private set; } = new Dictionary<KeyInput, SkillId>();          // 게임기능키와 스킬 연결맵
    bool[] _keyInput = new bool[Enum.GetValues(typeof(KeyInput)).Length];



    public void Init(S_EnterGame packet)
    {
        base.Init(packet.Object);

        // 키보드키 매핑 설정
        KeyMap.Add(KeyCode.W, KeyInput.Up);
        KeyMap.Add(KeyCode.A, KeyInput.Left);
        KeyMap.Add(KeyCode.S, KeyInput.Down);
        KeyMap.Add(KeyCode.D, KeyInput.Right);
        KeyMap.Add(KeyCode.Space, KeyInput.Attack);
        KeyMap.Add(KeyCode.Q, KeyInput.SkillA);
        KeyMap.Add(KeyCode.E, KeyInput.SkillB);
        KeyMap.Add(KeyCode.R, KeyInput.SkillC);
        KeyMap.Add(KeyCode.T, KeyInput.SkillD);
        KeyMap.Add(KeyCode.Y, KeyInput.SkillE);
        KeyMap.Add(KeyCode.Alpha1, KeyInput.SkillF);
        KeyMap.Add(KeyCode.Alpha2, KeyInput.SkillG);
        KeyMap.Add(KeyCode.Alpha3, KeyInput.SkillH);
        KeyMap.Add(KeyCode.Alpha4, KeyInput.SkillI);
        KeyMap.Add(KeyCode.Alpha5, KeyInput.SkillJ);
        KeyMap.Add(KeyCode.Alpha6, KeyInput.SkillK);
        KeyMap.Add(KeyCode.P, KeyInput.Auto);
        KeyMap.Add(KeyCode.Return, KeyInput.Enter);

        // 스킬셋 등록
        foreach (SkillId skillId in packet.SkillIds)
        {
            Data.SkillData skill;
            if (Managers.Data.SkillDict.TryGetValue(skillId, out skill))
                Skillset.Add(skillId, new SkillUseInfo() { lastUseTime = 0, skill = skill });
        }

        // 키 스킬 매핑 등록
        for(int i=0; i< packet.SkillIds.Count; i++)
            KeySkillMap.Add(KeyInput.Attack + i, packet.SkillIds[i]);

    }






    GameObject goDebug = null;
    protected override void UpdateController()
    {
        GetKeyInput();
        if (_keyInput[(int)KeyInput.Auto] == true)
        {
            SendAutoRequest();
        }



        base.UpdateController();



        // debug : 충돌지점을 텍스트로 표시
        //if (goDebug == null)
        //{
        //    goDebug = Managers.Resource.Instantiate("DummyText");
        //    goDebug.name = "TextDebug";
        //}
        //TextMeshPro text = goDebug.GetComponent<TextMeshPro>();
        //Vector2 intersection;
        //bool collision = Managers.Map.CollisionDetection(Pos, Dest, out intersection);
        //if(collision)
        //{
        //    goDebug.transform.position = Managers.Map.ServerPosToClientPos(new Vector3(intersection.x, intersection.y, 20));
        //    text.text = $"[{intersection.x:f1},{intersection.y:f1}]";
        //    text.color = Color.black;
        //}

    }

    void LateUpdate()
    {
        // 카메라 위치를 플레이어 위치로 업데이트
        //Camera.main.transform.position = new Vector3(transform.position.x, transform.position.y, -10);
        Vector3 targetPos = new Vector3(transform.position.x, transform.position.y, -10);
        Camera.main.transform.position = Vector3.Lerp(Camera.main.transform.position, targetPos, 0.2f);
    }

    // 현재 키보드 입력 얻기
    void GetKeyInput()
    {
        if (IsLoading)
        {
            Array.Fill(_keyInput, false);
            return;
        }

        // 키보드 입력을 얻음
        foreach (var pair in KeyMap)
        {
            _keyInput[(int)pair.Value] = Input.GetKey(pair.Key);
        }

        bool up = _keyInput[(int)KeyInput.Up];
        bool left = _keyInput[(int)KeyInput.Left];
        bool down = _keyInput[(int)KeyInput.Down];
        bool right = _keyInput[(int)KeyInput.Right];


        MoveKeyDown = true;
        if (up)
        {
            if (left)
                Dir = MoveDir.LeftUp;
            else if (right)
                Dir = MoveDir.RightUp;
            else
                Dir = MoveDir.Up;
        }
        else if (left)
        {
            if (up)
                Dir = MoveDir.LeftUp;
            else if (down)
                Dir = MoveDir.LeftDown;
            else
                Dir = MoveDir.Left;
        }
        else if (down)
        {
            if (left)
                Dir = MoveDir.LeftDown;
            else if (right)
                Dir = MoveDir.RightDown;
            else
                Dir = MoveDir.Down;
        }
        else if (right)
        {
            if (up)
                Dir = MoveDir.RightUp;
            else if (down)
                Dir = MoveDir.RightDown;
            else
                Dir = MoveDir.Right;
        }
        else
        {
            MoveKeyDown = false;
        }
    }

    // 키보드로 입력한 스킬 얻기
    SkillId GetSkillInput()
    {
        foreach (var pair in KeySkillMap)
        {
            if (_keyInput[(int)pair.Key] == true)
                return pair.Value;
        }
        return SkillId.SkillNone;
    }


    // 스킬 사용이 가능한지 검사함
    public bool CanUseSkill(SkillId skillId)
    {
        if (IsAlive == false)
            return false;

        if (skillId == SkillId.SkillNone)
            return false;
        
        // 이전에 사용한 스킬의 딜레이가 끝났는지 확인
        int tick = Environment.TickCount;
        if (tick - LastUseSkill.lastUseTime < LastUseSkill.skill.skillTime)
            return false;
        
        // 사용할 스킬 데이터 얻기
        SkillUseInfo useInfo;
        if (Skillset.TryGetValue(skillId, out useInfo) == false)
            return false;

        // 쿨타임 검사
        if (tick - useInfo.lastUseTime < useInfo.skill.cooldown)
            return false;

        return true;
    }




    protected override void UpdateIdle()
    {
        // 방향키가 눌림
        if (MoveKeyDown == true)
        {
            _speedRateForStop = 1f; // 초기화

            // 목적지 계산
            Vector2 dir = GetDirectionVector(Dir);
            Vector2 dest = Pos + dir * Config.MyPlayerMinimumMove * Speed;   // 처음 움직일 때는 dest를 최소이동거리 만큼 이동시킨다.
            Vector2 intersection;
            if (Managers.Map.CanGo(Pos, dest, out intersection))
            {
                Dest = dest;
            }
            else
            {
                Dest = intersection;
            }

            State = CreatureState.Moving;

            // 서버에 전송
            SendMovePacket();
        }
        // 현재위치와 목적지가 다름
        else if (Util.Equals(Pos, Dest) == false)
        {
            State = CreatureState.Moving;
            SendMovePacket();
        }
    }





    protected override void UpdateMoving()
    {
        // 키보드 방향키가 눌려있는 동안은 Dest를 계속해서 이동시킨다.
        Vector2 intersection;
        if (MoveKeyDown == true)
        {
            _speedRateForStop = 1f; // 초기화

            // dest 이동
            Vector2 dest;
            if (PrevDir == Dir)
            {
                dest = Dest + GetDirectionVector(Dir) * Time.deltaTime * Speed;
            }
            else
            {
                // 만약 방향이 바뀌었다면 dest가 Pos보다 MyPlayerMinimumMove 만큼 멀어져 있도록 한다.
                dest = Pos + GetDirectionVector(Dir) * Config.MyPlayerMinimumMove * Speed;
            }
                
            // dset를 이동 가능한 위치로 지정한다.
            if (Managers.Map.CanGo(Dest, dest, out intersection))
            {
                Dest = dest;
            }
            else
            {
                Dest = intersection;
            }
            
        }
        // 키보드 방향키를 누르고있지 않다면 멈출 것이기 때문에 Dest를 이동시키지 않는다.
        // 그리고 Dest와의 거리에 따라 속도를 감소시킨다.
        else
        {
            // Dest 까지의 거리가 최소이동거리 내라면 Speed를 점차 감소시킨다.
            float distance = (Dest - Pos).magnitude;
            if (distance < Config.MyPlayerMinimumMove * Speed)
            {
                _speedRateForStop = Math.Max(0.1f, distance / (Config.MyPlayerMinimumMove * Speed));
            }
            else
            {
                _speedRateForStop = 1f;
            }
        }


        // 위치 이동
        Vector2 diff = (Dest - Pos);
        Vector2 dir = diff.normalized;
        Vector2 pos = Pos + dir * Time.deltaTime * Speed * _speedRateForStop;

        // Dest에 도착시 현재위치를 Dest로 변경한다.
        // 만약 이동키가 눌려져있지 않다면 현재 위치에 stop 하고 상태를 idle로 바꾼다.
        if (diff.magnitude <= Time.deltaTime * Speed * _speedRateForStop)
        {
            if (MoveKeyDown)
            {
                if (Managers.Map.TryMoving(this, Dest))
                {
                    Pos = Dest;
                }
                else
                {
                    Dest = Pos;
                }
            }
            else
            {
                StopAt(Dest);
            }
        }
        // 위치 이동
        else if (Managers.Map.CanGo(Pos, pos, out intersection))
        {
            if (Managers.Map.TryMoving(this, pos))
            {
                Pos = pos;
            }
            else
            {
                Dest = Pos;
            }
        }
        // 이동중 부딪혔을 경우 더이상 이동할 수 없기 때문에 Dest를 변경한다.
        else
        {
            if (Managers.Map.TryMoving(this, intersection))
            {
                Pos = intersection;
                Dest = intersection;
            }
            else
            {
                Dest = Pos;
            }
        }


        SendMovePacket();
        ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.UpdateMoving. {this.ToString(InfoLevel.Position)}");


        // debug
        Managers.Map.InspectCell();
    }



    // 스킬 사용에 대한 업데이트
    protected override void UpdateSkill()
    {
        base.UpdateSkill();

        if (IsAlive == false)
            return;

        // 스킬 입력 확인
        SkillId skillInput = GetSkillInput();
        if (CanUseSkill(skillInput))
        {
            // 스킬사용이 가능할 경우 타겟을 설정하고 스킬패킷을 보낸다.
            UseSkill(skillInput);
        }
    }


    // 서버에 이동패킷 전송
    // 상태 변화가 있거나, 마지막으로 전송한 시간에서 Config.MovePacketSendInterval 시간 이상 지났을 경우 전송함
    void SendMovePacket(bool forced = false)
    {
        bool bSend = false;
        int tick = Environment.TickCount;
        if (forced)
        {
            bSend = true;
        }
        else if (_lastCMove == null)
        {
            bSend = true;
        }
        else if (PosInfo.MoveDir != _lastCMove.PosInfo.MoveDir
            || PosInfo.State != _lastCMove.PosInfo.State
            || PosInfo.MoveKeyDown != _lastCMove.PosInfo.MoveKeyDown
            || tick - _lastCMoveSendTime > Config.MovePacketSendInterval)
        {
            bSend = true;
        }

        if (!bSend)
            return;

        // 서버에 전송
        C_Move movePacket = new C_Move();
        movePacket.PosInfo = PosInfo.Clone();
        movePacket.MoveTime = Managers.Time.CurrentTime;
        Managers.Network.Send(movePacket);

        _lastCMove = movePacket;
        _lastCMoveSendTime = Environment.TickCount;

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.SendMovePacket. {this.ToString(InfoLevel.Position)}");
    }


    // 내 캐릭터의 스킬 사용
    public void UseSkill(SkillId skillId)
    {
        SkillUseInfo useInfo;
        if (Skillset.TryGetValue(skillId, out useInfo) == false)
            return;

        // 스킬 사용시간 업데이트
        useInfo.lastUseTime = Environment.TickCount;

        // 사용중인 스킬 등록
        LastUseSkill = useInfo;
        UsingSkill = useInfo;

        // 패킷생성
        C_Skill skillPacket = new C_Skill();
        skillPacket.SkillId = skillId;
        skillPacket.TargetId = -1;      // 타겟이 없는 스킬이거나 타겟을 찾지 못했다면 TargetId는 -1


        // 스킬 타입에 따라 피격대상 찾기
        SkillData skill = useInfo.skill;
        List<BaseController> listHitObjects = new List<BaseController>();
        switch (skill.skillType)
        {
            case SkillType.SkillMelee:
                {
                    listHitObjects = Managers.Map.FindObjectsInRect(Pos, new Vector2(skill.rangeX, skill.rangeY), LookDir, this);
                    break;
                }
            case SkillType.SkillProjectile:
                {
                    BaseController hitObj = Managers.Map.FindObjectInRect(Pos, new Vector2(skill.rangeX, skill.rangeY), LookDir, this);
                    if (hitObj != null)
                        listHitObjects.Add(hitObj);
                    break;
                }
            case SkillType.SkillInstant:
                {
                    BaseController hitObj = Managers.Map.FindObjectInRect(Pos, new Vector2(skill.rangeX, skill.rangeY), LookDir, this);
                    if (hitObj != null)
                        listHitObjects.Add(hitObj);
                    break;
                }
        }

        foreach (BaseController obj in listHitObjects)
        {
            if(skillPacket.TargetId == -1)
                skillPacket.TargetId = obj.Id;
            skillPacket.Hits.Add(obj.Id);
        }

        // 스킬사용패킷 전송
        Managers.Network.Send(skillPacket);


        // log
        string _log = $"MyPlayerController.SkillHitCheck. {this}, skill:{skill.id}, hit {listHitObjects.Count} objects: ";
        foreach (BaseController obj in listHitObjects)
            _log += $"{obj.Id}, ";
        ServerCore.Logger.WriteLog(LogLevel.Debug, _log);
    }



    // 자동사냥 시작 패킷 전송
    bool _autoRequestSent = false;
    void SendAutoRequest()
    {
        if (_autoRequestSent == false)
        {
            _autoRequestSent = true;

            C_SetAuto autoPacket = new C_SetAuto();
            if (AutoMode == AutoMode.ModeAuto)
                autoPacket.Mode = AutoMode.ModeNone;
            else
                autoPacket.Mode = AutoMode.ModeAuto;
            Managers.Network.Send(autoPacket);

            StartCoroutine("CoAutoRequestCooltime", 1f);

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.SendAutoRequest. mode:{autoPacket.Mode}");
        }
    }



    // update dead
    bool _respawnSent = false;
    protected override void UpdateDead()
    {
        if (_keyInput[(int)KeyInput.Enter] == true)
        {
            if (_respawnSent == false)
            {
                _respawnSent = true;

                C_Respawn packet = new C_Respawn();
                packet.BRespawn = true;
                Managers.Network.Send(packet);

                StartCoroutine("CoRespawnCooltime", 1f);

                ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.UpdateDead. Send respawn packet.");
            }
        }
    }

    // 부활요청 쿨타임
    IEnumerator CoRespawnCooltime(float time)
    {
        yield return new WaitForSeconds(time);
        _respawnSent = false;
    }
    // 오토요청 쿨타임
    IEnumerator CoAutoRequestCooltime(float time)
    {
        yield return new WaitForSeconds(time);
        _autoRequestSent = false;
    }


    // 사망함
    public override void OnDead()
    {
        base.OnDead();
        Managers.UI.ShowPopupUI<UI_Dead>("UI_Dead");
    }



    /* compoment */
    // HP Bar 추가
    protected override void AddHpBar()
    {
        base.AddHpBar();
        UI_Game gameUI = Managers.UI.SceneUI as UI_Game;
        if(gameUI != null)
        {
            gameUI.SetHealthValue(Stat.MaxHp, Stat.Hp);
        }
    }

    // HP Bar 업데이트
    protected override void UpdateHpBar()
    {
        base.UpdateHpBar();
        UI_Game gameUI = Managers.UI.SceneUI as UI_Game;
        if (gameUI != null)
        {
            gameUI.SetHealth(Hp);
        }
    }
}
