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
    SkillUseInfo _lastUseSkill = new SkillUseInfo() { lastUseTime = 0, skill = Managers.Data.DefaultSkill };  // 마지막으로 사용한 스킬
    SkillUseInfo _usingSkill = null;                  // 현재 사용중인 스킬

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
        Camera.main.transform.position = new Vector3(transform.position.x, transform.position.y, -10);
    }

    // 현재 키보드 입력 얻기
    void GetKeyInput()
    {
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
        if (State == CreatureState.Dead)
            return false;

        if (skillId == SkillId.SkillNone)
            return false;
        
        // 이전에 사용한 스킬의 딜레이가 끝났는지 확인
        int tick = Environment.TickCount;
        if (tick - _lastUseSkill.lastUseTime < _lastUseSkill.skill.skillTime)
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


        // 스킬사용 업데이트
        UpdateSkill();
    }





    protected override void UpdateMoving()
    {
        // 키보드 방향키가 눌려있는 동안은 Dest를 계속해서 이동시킨다.
        Vector2 intersection;
        if (MoveKeyDown == true)
        {
            _speedRateForStop = 1f; // 초기화

            Vector2 dest = Dest + GetDirectionVector(Dir) * Time.deltaTime * Speed;
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
                //Vector2 stopPos;
                //if (Managers.Map.TryStop(this, Dest, out stopPos))
                //{
                //    Pos = stopPos;
                //    Dest = stopPos;
                //}
                //else
                //{
                //    Dest = Pos;
                //}
                //State = CreatureState.Idle;
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


        // 스킬사용 업데이트
        UpdateSkill();


        // debug
        Managers.Map.InspectCell();
    }



    // 스킬 사용에 대한 업데이트
    void UpdateSkill()
    {
        // 스킬 입력 확인
        SkillId skillInput = GetSkillInput();
        if (CanUseSkill(skillInput))
            SendSkillPacket(skillInput);


        // 사용중인 스킬에 대한 스킬딜레이 검사
        if (_usingSkill == null)
            return;

        // 스킬이 아직 시전되지 않았을 경우 시전시간 검사
        int tick = Environment.TickCount;
        if(_usingSkill.casted == false)
        {
            if (tick - _usingSkill.lastUseTime > _usingSkill.skill.castingTime)
            {
                SkillHitCheck(_usingSkill.skill);
                _usingSkill.casted = true;
            }
        }
        // 스킬시전이 끝났을 경우 스킬딜레이 검사
        else
        {
            if (tick - _usingSkill.lastUseTime > _usingSkill.skill.skillTime)
            {
                _usingSkill.casted = false;
                _usingSkill = null;
            }
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




    // 스킬 사용요청 패킷을 보냄
    public void SendSkillPacket(SkillId skillId)
    {
        SkillUseInfo useInfo;
        if (Skillset.TryGetValue(skillId, out useInfo) == false)
            return;

        // 스킬 사용시간 업데이트
        useInfo.lastUseTime = Environment.TickCount;

        // 사용중인 스킬 등록
        _lastUseSkill = useInfo;
        _usingSkill = useInfo;

        // 스킬패킷 전송
        C_Skill skillPacket = new C_Skill();
        skillPacket.SkillId = skillId;
        Managers.Network.Send(skillPacket);

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.OnSkill. {this}, skillId:{skillId}");
    }



    // 스킬 피격판정 검사
    public void SkillHitCheck(SkillData skill)
    {
        // 피격대상 찾기
        List<BaseController> listHitObjects = new List<BaseController>();
        switch (skill.skillType)
        {
            case SkillType.SkillMelee:
                {
                    listHitObjects = Managers.Map.FindObjectsInRect(Pos, new Vector2(skill.rangeX, skill.rangeY), LookDir);
                    break;
                }
            case SkillType.SkillProjectile:
                {
                    BaseController hitObj = Managers.Map.FindObjectInRect(Pos, new Vector2(skill.rangeX, skill.rangeY), LookDir);
                    if (hitObj != null)
                        listHitObjects.Add(hitObj);
                    break;
                }
            case SkillType.SkillInstant:
                {
                    BaseController hitObj = Managers.Map.FindObjectInRect(Pos, new Vector2(skill.rangeX, skill.rangeY), LookDir);
                    if (hitObj != null)
                        listHitObjects.Add(hitObj);
                    break;
                }
        }

        // 피격대상 전송
        C_SkillHit hitPacket = new C_SkillHit();
        hitPacket.SkillId = skill.id;
        foreach (BaseController obj in listHitObjects)
        {
            if (obj.State == CreatureState.Dead)
                continue;
            hitPacket.HitObjectIds.Add(obj.Id);
        }
        Managers.Network.Send(hitPacket);


        // log
        string _log = $"MyPlayerController.SkillHitCheck. {this}, skill:{skill.id}, hit {listHitObjects.Count} objects: ";
        foreach (BaseController obj in listHitObjects)
            _log += $"{obj.Id}, ";
        ServerCore.Logger.WriteLog(LogLevel.Debug, _log);
    }



    // 자동사냥 시작 패킷 전송
    void SendAutoRequest()
    {
        C_SetAuto autoPacket = new C_SetAuto();
        autoPacket.Mode = AutoMode.ModeAuto;
        Managers.Network.Send(autoPacket);

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.SendAutoRequest. mode:{autoPacket.Mode}");
    }
}
