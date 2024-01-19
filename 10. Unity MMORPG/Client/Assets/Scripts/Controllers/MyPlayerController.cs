using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using ServerCore;
using static Define;
using Data;
using System;
using TMPro;


public class MyPlayerController : PlayerController
{ 
    C_Move _lastCMove = null;       // 마지막으로 보낸 C_Move 패킷
    int _lastCMoveSendTime = 0;     // 마지막으로 C_Move 패킷 보낸 시간 (단위:ms)

    float _speedRateForStop = 1f;  // 움직임을 멈출 때 속도를 감소시킬 비율

    KeyInput _keyInput = new KeyInput();

    protected override void Init()
    {
        base.Init();
    }


    GameObject goDebug = null;
    protected override void UpdateController()
    {
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
        _keyInput.Up = Input.GetKey(KeyCode.W);
        _keyInput.Left = Input.GetKey(KeyCode.A);
        _keyInput.Down = Input.GetKey(KeyCode.S);
        _keyInput.Right = Input.GetKey(KeyCode.D);
        _keyInput.Attack = Input.GetKeyDown(KeyCode.Space);
        _keyInput.SkillA = Input.GetKeyDown(KeyCode.Q);
        _keyInput.SkillB = Input.GetKeyDown(KeyCode.E);
        _keyInput.SkillC = Input.GetKeyDown(KeyCode.R);

        MoveKeyDown = true;
        if (_keyInput.Up)
        {
            if (_keyInput.Left)
                Dir = MoveDir.LeftUp;
            else if (_keyInput.Right)
                Dir = MoveDir.RightUp;
            else
                Dir = MoveDir.Up;
        }
        else if (_keyInput.Left)
        {
            if (_keyInput.Up)
                Dir = MoveDir.LeftUp;
            else if (_keyInput.Down)
                Dir = MoveDir.LeftDown;
            else
                Dir = MoveDir.Left;
        }
        else if (_keyInput.Down)
        {
            if (_keyInput.Left)
                Dir = MoveDir.LeftDown;
            else if (_keyInput.Right)
                Dir = MoveDir.RightDown;
            else
                Dir = MoveDir.Down;
        }
        else if (_keyInput.Right)
        {
            if (_keyInput.Up)
                Dir = MoveDir.RightUp;
            else if (_keyInput.Down)
                Dir = MoveDir.RightDown;
            else
                Dir = MoveDir.Right;
        }
        else
        {
            MoveKeyDown = false;
        }



        SkillKeyDown = false;
        if (_keyInput.Attack || _keyInput.SkillA || _keyInput.SkillB || _keyInput.SkillC)
        {
            SkillKeyDown = true;
        }
    }


    // 스킬 사용이 가능한지 검사함
    public bool CanUseSkill(SkillId skillId)
    {
        if (State == CreatureState.Dead)
            return false;

        SkillInfo skillInfo;
        if (Skillset.TryGetValue(skillId, out skillInfo) == false)
            return false;

        int tick = Environment.TickCount;
        if (tick - skillInfo.lastUseTime < skillInfo.skill.cooldown)
            return false;

        // 스킬 사용시간 업데이트
        Skillset[skillId] = new SkillInfo() { lastUseTime = tick, skill = skillInfo.skill };

        return true;
    }




    protected override void UpdateIdle()
    {
        GetKeyInput();

        // 방향키 눌림
        if (MoveKeyDown == true)
        {
            _speedRateForStop = 1f; // 초기화

            // 목적지 계산
            Vector2 dir = GetDirectionVector(Dir);
            Vector2 dest = Pos + dir * Config.MyPlayerMinimumMove * Speed;   // 처음 움직일 때는 dest를 최소이동거리 만큼 이동시킨다.
            Vector2 intersection;
            if(Managers.Map.CanGo(Pos, dest, out intersection))
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

        // 스킬키 눌림
        if(SkillKeyDown == true)
        {
            SkillId skillId = SkillId.SkillAttack;
            if (_keyInput.Attack)
                skillId = SkillId.SkillAttack;
            else if (_keyInput.SkillA)
                skillId = SkillId.SkillFireball;
            else if (_keyInput.SkillB)
                skillId = SkillId.SkillLightning;
            else if (_keyInput.SkillC)
                skillId = SkillId.SkillArrow;
            if (CanUseSkill(skillId))
                OnSkill(skillId);
        }
    }

    protected override void UpdateMoving()
    {
        GetKeyInput();

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
                Vector2 stopPos;
                if (Managers.Map.TryStop(this, Dest, out stopPos))
                {
                    Pos = stopPos;
                    Dest = stopPos;
                }
                else
                {
                    Dest = Pos;
                }
                State = CreatureState.Idle;
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


        Managers.Map.InspectCell();



        // 스킬키 눌림
        if (SkillKeyDown == true)
        {
            SkillId skillId = SkillId.SkillAttack;
            if (_keyInput.Attack)
                skillId = SkillId.SkillAttack;
            else if (_keyInput.SkillA)
                skillId = SkillId.SkillFireball;
            else if (_keyInput.SkillB)
                skillId = SkillId.SkillLightning;
            else if (_keyInput.SkillC)
                skillId = SkillId.SkillArrow;
            if (CanUseSkill(skillId))
                OnSkill(skillId);
        }
    }



    void SendMovePacket(bool forced = false)
    {
        bool bSend = false;
        int tick = Environment.TickCount;
        if(forced)
        {
            bSend = true;
        }
        else if (_lastCMove == null)
        {
            bSend = true;
        }
        else if(PosInfo.MoveDir != _lastCMove.PosInfo.MoveDir
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




    // 스킬 사용됨
    public override void OnSkill(SkillId skillId)
    {
        base.OnSkill(skillId);

        SkillInfo skillInfo;
        if (Skillset.TryGetValue(skillId, out skillInfo) == false)
            return;
        
        // 피격대상 찾기
        SkillData skill = skillInfo.skill;
        List<BaseController> hitObjects = new List<BaseController>();
        switch (skill.skillType)
        {
            case SkillType.SkillMelee:
                {
                    hitObjects = Managers.Map.FindObjectsInRect(Pos, new Vector2(skill.melee.rangeX, skill.melee.rangeY), LookDir);
                    break;
                }
            case SkillType.SkillProjectile:
                {
                    BaseController hitObj = Managers.Map.FindObjectInRect(Pos, new Vector2(skill.projectile.rangeX, skill.projectile.rangeY), LookDir);
                    if (hitObj != null)
                        hitObjects.Add(hitObj);
                    break;
                }
            case SkillType.SkillInstant:
                {
                    BaseController hitObj = Managers.Map.FindObjectInRect(Pos, new Vector2(skill.instant.rangeX, skill.instant.rangeY), LookDir);
                    if (hitObj != null)
                        hitObjects.Add(hitObj);
                    break;
                }
        }

        // 스킬패킷 전송
        C_Skill skillPacket = new C_Skill();
        skillPacket.SkillId = skillId;
        foreach (BaseController obj in hitObjects)
        {
            if (obj.State == CreatureState.Dead)
                continue;
            skillPacket.HitObjectIds.Add(obj.Id);
        }
        Managers.Network.Send(skillPacket);


        // log
        string _log = $"MyPlayerController.OnSkill. Id:{Id}, skillId:{skillId}, hit {hitObjects.Count} objects: ";
        foreach (BaseController obj in hitObjects)
            _log += $"{obj.Id}, ";
        ServerCore.Logger.WriteLog(LogLevel.Debug, _log);
    }


}
