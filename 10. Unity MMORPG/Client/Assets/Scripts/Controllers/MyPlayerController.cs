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
    bool _isSkillKeyPressed = false;  // 스킬 키가 눌려짐

    C_Move _lastCMove = null;       // 마지막으로 보낸 C_Move 패킷
    int _lastCMoveSendTime = 0;  // 마지막으로 C_Move 패킷 보낸 시간

    float _speedRateForStop = 1f;  // 움직임을 멈출 때 속도를 감소시킬 비율

    protected override void Init()
    {
        base.Init();
        //Pos = new Vector2(10, 10);
        //Dest = Pos;
        //Speed = 5.0f;
    }


    GameObject goDebug = null;
    protected override void UpdateController()
    {
        base.UpdateController();



        // debug
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
        bool keyW = Input.GetKey(KeyCode.W);
        bool keyA = Input.GetKey(KeyCode.A);
        bool keyS = Input.GetKey(KeyCode.S);
        bool keyD = Input.GetKey(KeyCode.D);
        bool KeyZ = Input.GetKeyDown(KeyCode.Z);

        MoveKeyDown = true;
        if (keyW)
        {
            if (keyA)
                Dir = MoveDir.LeftUp;
            else if (keyD)
                Dir = MoveDir.RightUp;
            else
                Dir = MoveDir.Up;
        }
        else if (keyA)
        {
            if (keyW)
                Dir = MoveDir.LeftUp;
            else if (keyS)
                Dir = MoveDir.LeftDown;
            else
                Dir = MoveDir.Left;
        }
        else if (keyS)
        {
            if (keyA)
                Dir = MoveDir.LeftDown;
            else if (keyD)
                Dir = MoveDir.RightDown;
            else
                Dir = MoveDir.Down;
        }
        else if (keyD)
        {
            if (keyW)
                Dir = MoveDir.RightUp;
            else if (keyS)
                Dir = MoveDir.RightDown;
            else
                Dir = MoveDir.Right;
        }
        else
        {
            MoveKeyDown = false;
        }



        _isSkillKeyPressed = false;
        if (KeyZ)
        {
            _isSkillKeyPressed = true;
        }
    }

    protected override void UpdateAnimation()
    {
        switch(State)
        {
            case CreatureState.Idle:
                _animator.SetFloat("RunState", 0);
                break;
            case CreatureState.Moving:
                _animator.SetFloat("RunState", 0.5f);
                switch (Dir)
                {
                    case MoveDir.Left:
                    case MoveDir.LeftUp:
                    case MoveDir.LeftDown:
                        gameObject.transform.localScale = new Vector3(1, 1, 1);
                        break;
                    case MoveDir.Right:
                    case MoveDir.RightUp:
                    case MoveDir.RightDown:
                        gameObject.transform.localScale = new Vector3(-1, 1, 1);
                        break;
                }
                break;
            case CreatureState.Dead:
                _animator.SetTrigger("Die");
                break;
        }

        if(_isSkillKeyPressed)
        {
            _animator.SetTrigger("Attack");
        }
    }



    protected override void UpdateIdle()
    {
        GetKeyInput();

        // 방향키 눌림
        if (MoveKeyDown == true)
        {
            _speedRateForStop = 1f; // 초기화

            // 목적지 계산
            Vector2 pos = Pos;
            Vector2 dir = GetDirectionVector(Dir);
            Vector2 dest = Pos + dir * Config.MyPlayerMinimumMove * Speed;   // 처음 움직일 때는 dest를 MyPlayerMinimumMove 만큼 이동시킨다.
            Vector2 finalDest;    // 최종 목적지
            int loopCount = 0;
            while (true)
            {
                loopCount++;
                Debug.Assert(loopCount < 1000, $"MyPlayerController.UpdateIdle loopCount:{loopCount}");

                // 목적지에 도착했으면 break
                if ((dest - pos).magnitude <= Time.deltaTime * Speed)
                {
                    if (Managers.Map.IsMovable(this, dest) == false)
                        finalDest = pos;
                    else
                        finalDest = dest;
                    break;
                }

                // 1 frame 이동을 시도한다.
                pos += dir * Time.deltaTime * Speed;
                if (Managers.Map.IsMovable(this, pos))  // TBD: 이동할 때 object 충돌을 고려할지 생각해봐야함
                {
                    continue;
                }
                else
                {
                    finalDest = pos - dir * Time.deltaTime * Speed;
                    break;
                }
            }


            // 목적지 설정
            Dest = finalDest;
            State = CreatureState.Moving;

            // 서버에 전송
            SendMovePacket();
        }
    }

    protected override void UpdateMoving()
    {
        GetKeyInput();

        // 키보드 방향키가 눌려있는 동안은 Dest를 계속해서 이동시킨다.
        if (MoveKeyDown == true)
        {
            _speedRateForStop = 1f; // 초기화

            Vector2 direction = GetDirectionVector(Dir);
            Vector2 dest = Dest + direction * Time.deltaTime * Speed;

            if (Managers.Map.IsMovable(this, dest))
            {
                Dest = dest;
            }
        }
        // 키보드 방향키를 누르고있지 않다면 Dest를 이동시키지 않는다.
        else
        {
            // Dest 까지의 거리가 MyPlayerMinimumMove 내라면 Speed를 점차 감소시킨다.
            float diff = (Dest - Pos).magnitude;
            if(diff < Config.MyPlayerMinimumMove * Speed)
            {
                _speedRateForStop = diff / (Config.MyPlayerMinimumMove * Speed);
            }
            else
            {
                _speedRateForStop = 1f;
            }

            // Dest에 도착시 상태를 Idle로 바꾼다.
            if (diff <= Time.deltaTime * Speed)
            {
                if (Managers.Map.TryMove(this, Dest))
                {
                    Pos = Dest;
                }

                State = CreatureState.Idle;
                SendMovePacket();
                return;
            }
        }


        // 실제 위치 이동
        Vector2 vecDir = (Dest - Pos).normalized;
        Vector2 pos = Pos + vecDir * Time.deltaTime * Speed * _speedRateForStop;
        if (Managers.Map.TryMove(this, pos))
        {
            Pos = pos;
        }
        else
        {
            Dest = Pos;
        }

        SendMovePacket();
        ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.UpdateMoving. {this.ToString(InfoLevel.Position)}");
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

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.UpdateIdle. Send C_Move. {this.ToString(InfoLevel.Position)}");
    }




    // tile의 position(positionOfTile)을 기준으로 플레이어가 dirDesired 방향에 위치하는지를 확인한다.
    bool IsPlayerAtDirection(Vector3 positionOfTile, MoveDir expectedDirection)
    {
        Vector3 diff = transform.position - positionOfTile;
        switch (expectedDirection)
        {
            case MoveDir.LeftUp:
                if (diff.x < 0.03f && diff.y > 0.24f)
                    return true;
                break;
            case MoveDir.LeftDown:
                if (diff.x < 0.03f && diff.y < 0.26f)
                    return true;
                break;
            case MoveDir.RightUp:
                if (diff.x > -0.03f && diff.y > 0.24f)
                    return true;
                break;
            case MoveDir.RightDown:
                if (diff.x > -0.03f && diff.y < 0.26f)
                    return true;
                break;
        }

        return false;
    }


    //// dir 방향으로 약간 움직인다. 대각선으로는 움직이지 않는다.
    //// 움직이는것에 성공할 시 _isUpdateCamera = false; 로 설정한다.
    //bool MoveSlightly(MoveDir dir)
    //{
    //    Vector3 pos;
    //    Vector3Int cell;
    //    switch (dir)
    //    {
    //        case MoveDir.Up:
    //            pos = transform.position + (Vector3.up * Time.deltaTime * 5f / 2f);
    //            cell = _ground.LocalToCell(new Vector3(transform.position.x, pos.y, 0));
    //            break;

    //        case MoveDir.Down:
    //            pos = transform.position + (Vector3.down * Time.deltaTime * 5f / 2f);
    //            cell = _ground.LocalToCell(new Vector3(transform.position.x, pos.y, 0));
    //            break;

    //        case MoveDir.Left:
    //            pos = transform.position + (Vector3.left * Time.deltaTime * 5f / 2f);
    //            cell = _ground.LocalToCell(new Vector3(pos.x, transform.position.y, 0));
    //            break;

    //        case MoveDir.Right:
    //            pos = transform.position + (Vector3.right * Time.deltaTime * 5f / 2f);
    //            cell = _ground.LocalToCell(new Vector3(pos.x, transform.position.y, 0));
    //            break;

    //        default:
    //            pos = transform.position;
    //            cell = _ground.LocalToCell(pos);
    //            break;
    //    }


    //    if (_collision.GetTile(cell) == null)
    //    {
    //        transform.position = pos;
    //        _isUpdateCamera = false;
    //        return true;
    //    }
    //    else
    //    {
    //        return false;
    //    }

    //}




























    //// Idle 상태일때의 업데이트
    //protected  void _UpdateIdle()
    //{
    //    // 이동하려 할 경우 Moving 상태로 설정
    //    if (_isMoveKeyPressed)
    //    {
    //        State = CreatureState.Moving;
    //        return;
    //    }

    //    // 공격하려함
    //    if (Input.GetKey(KeyCode.Space) && _coSkillCooltime == null)
    //    {
    //        // 스킬사용 전송
    //        C_Skill skill = new C_Skill() { Info = new SkillInfo() };
    //        skill.Info.SkillId = 2;
    //        Managers.Network.Send(skill);

    //        ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.UpdateIdle. Send C_Skill. playerId:{Id}, skillId:{skill.Info.SkillId}");

    //        // 스킬쿨타임 적용
    //        _coSkillCooltime = StartCoroutine("CoInputCooltime", 0.2f);
    //    }
    //}

    //// Dir에 따라 다음 위치로 이동한다.
    //protected  void _MoveToNextPos()
    //{
    //    // 이동키가 안눌렸다면 Idle로 변경
    //    if (_isMoveKeyPressed == false)
    //    {
    //        State = CreatureState.Idle;
    //        CheckUpdatedFlag();  // 위치에 변경사항이 있다면 서버에 이동패킷 전송
    //        return;
    //    }

    //    // 목적지로 갈 수 있을 경우 CellPos를 목적지로 세팅한다.
    //    Vector3Int destPos = CellPos;
    //    switch (Dir)
    //    {
    //        case MoveDir.Up:
    //            destPos += Vector3Int.up;
    //            break;
    //        case MoveDir.Down:
    //            destPos += Vector3Int.down;
    //            break;
    //        case MoveDir.Left:
    //            destPos += Vector3Int.left;
    //            break;
    //        case MoveDir.Right:
    //            destPos += Vector3Int.right;
    //            break;
    //    }

    //    // 이동가능여부 체크
    //    if (Managers.Map.CanGo(destPos) && Managers.Object.FindCreature(destPos) == null)
    //    {
    //        CellPos = destPos;
    //        State = CreatureState.Moving;
    //    }

    //    // 위치에 변경사항이 있다면 서버에 이동패킷 전송
    //    CheckUpdatedFlag();

    //}

    //// 위치에 변경사항이 있다면 서버에 이동패킷 전송
    //protected override void CheckUpdatedFlag()
    //{
    //    if (_updated)
    //    {
    //        C_Move movePacket = new C_Move();
    //        movePacket.PosInfo = PosInfo;
    //        Managers.Network.Send(movePacket);
    //        _updated = false;

    //        ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.CheckUpdatedFlag. Send C_Move. " +
    //            $"playerId:{Id}, state:{PosInfo.State}, dir:{PosInfo.MoveDir}, pos:({PosInfo.PosX},{PosInfo.PosY})");
    //    }
    //}

    //// 스킬 사용 쿨타임을 적용해주는 코루틴
    //Coroutine _coSkillCooltime;
    //IEnumerator CoInputCooltime(float time)
    //{
    //    yield return new WaitForSeconds(time);
    //    _coSkillCooltime = null;
    //}
}
