using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using ServerCore;
using static Define;
using Unity.VisualScripting;
using UnityEngine.Tilemaps;
using TMPro;
using static UnityEditor.PlayerSettings;
using Data;
using System;


public class MyPlayerController : PlayerController
{ 
    public float RTT { get; set; } = 0.1f;

    bool _isMoveKeyPressed = false;   // 키보드 이동키가 눌려짐
    bool _isSkillKeyPressed = false;  // 스킬 키가 눌려짐

    C_Move _lastCMove;       // 마지막으로 보낸 C_Move 패킷
    int _lastCMoveSendTime;  // 마지막으로 C_Move 패킷 보낸 시간

    protected override void Init()
    {
        base.Init();
        Pos = new Vector3(10, 10);
        Dest = Pos;
        Speed = 5.0f;
    }

    protected override void UpdateController()
    {
        base.UpdateController();
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

        _isMoveKeyPressed = true;
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
            _isMoveKeyPressed = false;
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
        //Vector3 s = Managers.Map.ClientPosToServerPos(transform.position);
        //Vector3 c = Managers.Map.ServerPosToClientPos(s);
        //Debug.Log($"t:{transform.position}, s:{s}, c:{c}");

        GetKeyInput();

        // 목적지 이동
        if (_isMoveKeyPressed == true)
        {
            Vector3 vecDir = GetDirectionVector();
            Vector3 dest = Dest + vecDir * RTT * Speed;
            Dest = dest;
            State = CreatureState.Moving;

            SendMovePacket();
        }

    }

    protected override void UpdateMoving()
    {
        //Debug.Log($"pos:{Pos}, cell:{Cell}");
        GetKeyInput();

        // 목적지 이동
        if (_isMoveKeyPressed == true)
        {
            Vector3 direction = GetDirectionVector();
            Vector3 dest = Dest + direction * Time.deltaTime * Speed;
            Dest = dest;

            SendMovePacket();
        }

        // 이동
        Vector3 vecDir = (Dest - Pos).normalized;
        Vector3 pos = Pos + vecDir * Time.deltaTime * Speed;
        Vector2Int cell = Util.PosToCell(pos);
        if (Managers.Map.IsEmptyCell(cell))
        {
            Pos = pos;
        }
        else
        {
            Dest = Pos;

            SendMovePacket();
        }

        if (_isMoveKeyPressed == false)
        {
            float diff = (Dest - Pos).AbsSumXY();
            if (diff <= Time.deltaTime * Speed)
            {
                Pos = Dest;
                State = CreatureState.Idle;

                SendMovePacket();
                return;
            }
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
        else if(PosInfo.MoveDir != _lastCMove.PosInfo.MoveDir
            || PosInfo.State != _lastCMove.PosInfo.State
            || tick - _lastCMoveSendTime > 200)
        {
            bSend = true;
        }

        if (!bSend)
            return;

        // 서버에 전송
        C_Move movePacket = new C_Move();
        movePacket.PosInfo = PosInfo.Clone();
        Managers.Network.Send(movePacket);

        _lastCMove = movePacket;
        _lastCMoveSendTime = Environment.TickCount;

        ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.UpdateIdle. Send C_Move. {this.ToString(InfoLevel.Position)}");
    }





    // 현재 방향에 해당하는 벡터 얻기
    Vector3 GetDirectionVector()
    {
        Vector3 direction;
        switch (Dir)
        {
            case MoveDir.Up:
                direction = new Vector3(1, -1, 0).normalized;
                break;
            case MoveDir.Down:
                direction = new Vector3(-1, 1, 0).normalized;
                break;
            case MoveDir.Left:
                direction = new Vector3(-1, -1, 0).normalized;
                break;
            case MoveDir.Right:
                direction = new Vector3(1, 1, 0).normalized;
                break;
            case MoveDir.LeftUp:
                direction = new Vector3(0, -1, 0);
                break;
            case MoveDir.LeftDown:
                direction = new Vector3(-1, 0, 0);
                break;
            case MoveDir.RightUp:
                direction = new Vector3(1, 0, 0);
                break;
            case MoveDir.RightDown:
                direction = new Vector3(0, 1, 0);
                break;
            default:
                direction = new Vector3(0, 0, 0);
                break;
        }

        return direction;
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
