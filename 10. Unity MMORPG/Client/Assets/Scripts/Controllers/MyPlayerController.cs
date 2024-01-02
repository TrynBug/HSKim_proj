using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using ServerCore;
using static Define;
using Unity.VisualScripting;
using UnityEngine.Tilemaps;

public class MyPlayerController : PlayerController
{
    public Vector3 PosDest { get; set; }  // 목적지

    bool _isMoveKeyPressed = false;// 키보드 이동키가 눌려짐
    bool _isUpdateCamera = true;

    protected override void Init()
    {
        base.Init();
    }

    protected override void UpdateController()
    {
        switch (State)
        {
            case CreatureState.Idle:
                GetInputDir();
                break;
            case CreatureState.Moving:
                GetInputDir();
                break;
            case CreatureState.Skill:
                break;
            case CreatureState.Dead:
                break;
        }

        base.UpdateController();
    }

    void LateUpdate()
    {
        // 카메라 위치를 플레이어 위치로 업데이트
        if (_isUpdateCamera)
            Camera.main.transform.position = new Vector3(transform.position.x, transform.position.y, -10);
        _isUpdateCamera = true;
    }

    // 현재 키보드로 입력된 방향 얻기
    void GetInputDir()
    {
        _isMoveKeyPressed = true;
        bool keyW = Input.GetKey(KeyCode.W);
        bool keyA = Input.GetKey(KeyCode.A);
        bool keyS = Input.GetKey(KeyCode.S);
        bool keyD = Input.GetKey(KeyCode.D);

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
    }


    private void Update()
    {
        GetInputDir();
        if (_isMoveKeyPressed == true)
        {
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
        }
        else
        {
            _animator.SetFloat("RunState", 0);
        }

        if (Input.GetKeyDown(KeyCode.Z))
        {
            _animator.SetTrigger("Attack");
        }


        if (_isMoveKeyPressed == true)
        {
            Vector3 direction = GetDirectionVector();


            Vector3 pos = transform.position + direction * Time.deltaTime * 5f;
            Vector3Int cell = _ground.LocalToCell(new Vector3(pos.x, pos.y, 0));
            //if (_isMoveKeyPressed == true)
            //    Debug.Log($"pos:{pos}, local:({xLocal.x},{yLocal.y})");
            TileBase tile = _collision.GetTile(cell);
            if (tile == null)
            {
                transform.position = pos;
            }
            else
            {
                // collision에 막힘
                Vector3 posTile = _ground.CellToLocal(cell);
                switch (Dir)
                {
                    case MoveDir.Up:
                        // collision의 왼쪽아래에 있는경우 왼쪽으로 조금 이동함
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftDown))
                            MoveSlightly(MoveDir.Left);
                        // collision의 오른쪽아래에 있는경우 오른쪽으로 조금 이동함
                        else if (IsPlayerAtDirection(posTile, MoveDir.RightDown))
                            MoveSlightly(MoveDir.Right);
                        break;

                    case MoveDir.Down:
                        // collision의 왼쪽위에 있는경우 왼쪽으로 조금 이동함
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftUp))
                            MoveSlightly(MoveDir.Left);
                        // collision의 오른쪽위에 있는 경우 오른쪽으로 조금 이동함
                        else if (IsPlayerAtDirection(posTile, MoveDir.RightUp))
                            MoveSlightly(MoveDir.Right);
                        break;

                    case MoveDir.Left:
                        // collision의 오른쪽위에 있는 경우 위로 조금 이동함
                        if (IsPlayerAtDirection(posTile, MoveDir.RightUp))
                            MoveSlightly(MoveDir.Up);
                        // collision의 오른쪽아래에 있는 경우 아래로 조금 이동함
                        else if (IsPlayerAtDirection(posTile, MoveDir.RightDown))
                            MoveSlightly(MoveDir.Down);
                        break;

                    case MoveDir.Right:
                        // collision의 왼쪽아래에 있는 경우 아래로 조금 이동함
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftDown))
                            MoveSlightly(MoveDir.Down);
                        // collision의 왼쪽위에 있는 경우 위로 조금 이동함
                        else if (IsPlayerAtDirection(posTile, MoveDir.LeftUp))
                            MoveSlightly(MoveDir.Up);
                        break;

                    case MoveDir.LeftUp:
                        // collision의 왼쪽아래에 있는 경우 왼쪽으로 조금 이동함
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftDown))
                            MoveSlightly(MoveDir.Left);
                        break;

                    case MoveDir.LeftDown:
                        // collision의 왼쪽위에 있는경우 왼쪽으로 조금 이동함
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftUp))
                            MoveSlightly(MoveDir.Left);
                        break;

                    case MoveDir.RightUp:
                        // collision의 오른쪽아래에 있는경우 오른쪽으로 조금 이동함
                        if (IsPlayerAtDirection(posTile, MoveDir.RightDown))
                            MoveSlightly(MoveDir.Right);
                        break;
                    case MoveDir.RightDown:
                        // collision의 오른쪽위에 있는경우 오른쪽으로 조금 이동함
                        if (IsPlayerAtDirection(posTile, MoveDir.RightUp))
                            MoveSlightly(MoveDir.Right);
                        break;
                }
            }
        }
    }

    void LateUpdate()
    {
        // 카메라 위치를 플레이어 위치로 업데이트
        if (_isUpdateCamera)
            Camera.main.transform.position = new Vector3(transform.position.x, transform.position.y, -10);
        _isUpdateCamera = true;
    }



    // 현재 방향에 해당하는 벡터 얻기
    Vector3 GetDirectionVector()
    {
        Vector3 direction;
        switch (Dir)
        {
            case MoveDir.Up:
                direction = new Vector3(0, 1, 0);
                break;
            case MoveDir.Down:
                direction = new Vector3(0, -1, 0);
                break;
            case MoveDir.Left:
                direction = new Vector3(-1, 0, 0);
                break;
            case MoveDir.Right:
                direction = new Vector3(1, 0, 0);
                break;
            case MoveDir.LeftUp:
                direction = new Vector3(-1, 1, 0).normalized;
                break;
            case MoveDir.LeftDown:
                direction = new Vector3(-1, -1, 0).normalized;
                break;
            case MoveDir.RightUp:
                direction = new Vector3(1, 1, 0).normalized;
                break;
            case MoveDir.RightDown:
                direction = new Vector3(1, -1, 0).normalized;
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


    // dir 방향으로 약간 움직인다. 대각선으로는 움직이지 않는다.
    // 움직이는것에 성공할 시 _isUpdateCamera = false; 로 설정한다.
    bool MoveSlightly(MoveDir dir)
    {
        Vector3 pos;
        Vector3Int cell;
        switch (dir)
        {
            case MoveDir.Up:
                pos = transform.position + (Vector3.up * Time.deltaTime * 5f / 2f);
                cell = _ground.LocalToCell(new Vector3(transform.position.x, pos.y, 0));
                break;

            case MoveDir.Down:
                pos = transform.position + (Vector3.down * Time.deltaTime * 5f / 2f);
                cell = _ground.LocalToCell(new Vector3(transform.position.x, pos.y, 0));
                break;

            case MoveDir.Left:
                pos = transform.position + (Vector3.left * Time.deltaTime * 5f / 2f);
                cell = _ground.LocalToCell(new Vector3(pos.x, transform.position.y, 0));
                break;

            case MoveDir.Right:
                pos = transform.position + (Vector3.right * Time.deltaTime * 5f / 2f);
                cell = _ground.LocalToCell(new Vector3(pos.x, transform.position.y, 0));
                break;

            default:
                pos = transform.position;
                cell = _ground.LocalToCell(pos);
                break;
        }


        if (_collision.GetTile(cell) == null)
        {
            transform.position = pos;
            _isUpdateCamera = false;
            return true;
        }
        else
        {
            return false;
        }

    }




























    // Idle 상태일때의 업데이트
    protected  void _UpdateIdle()
    {
        // 이동하려 할 경우 Moving 상태로 설정
        if (_isMoveKeyPressed)
        {
            State = CreatureState.Moving;
            return;
        }

        // 공격하려함
        if (Input.GetKey(KeyCode.Space) && _coSkillCooltime == null)
        {
            // 스킬사용 전송
            C_Skill skill = new C_Skill() { Info = new SkillInfo() };
            skill.Info.SkillId = 2;
            Managers.Network.Send(skill);

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.UpdateIdle. Send C_Skill. playerId:{Id}, skillId:{skill.Info.SkillId}");

            // 스킬쿨타임 적용
            _coSkillCooltime = StartCoroutine("CoInputCooltime", 0.2f);
        }
    }

    // Dir에 따라 다음 위치로 이동한다.
    protected  void _MoveToNextPos()
    {
        // 이동키가 안눌렸다면 Idle로 변경
        if (_isMoveKeyPressed == false)
        {
            State = CreatureState.Idle;
            CheckUpdatedFlag();  // 위치에 변경사항이 있다면 서버에 이동패킷 전송
            return;
        }

        // 목적지로 갈 수 있을 경우 CellPos를 목적지로 세팅한다.
        Vector3Int destPos = CellPos;
        switch (Dir)
        {
            case MoveDir.Up:
                destPos += Vector3Int.up;
                break;
            case MoveDir.Down:
                destPos += Vector3Int.down;
                break;
            case MoveDir.Left:
                destPos += Vector3Int.left;
                break;
            case MoveDir.Right:
                destPos += Vector3Int.right;
                break;
        }

        // 이동가능여부 체크
        if (Managers.Map.CanGo(destPos) && Managers.Object.FindCreature(destPos) == null)
        {
            CellPos = destPos;
            State = CreatureState.Moving;
        }

        // 위치에 변경사항이 있다면 서버에 이동패킷 전송
        CheckUpdatedFlag();

    }

    // 위치에 변경사항이 있다면 서버에 이동패킷 전송
    protected override void CheckUpdatedFlag()
    {
        if (_updated)
        {
            C_Move movePacket = new C_Move();
            movePacket.PosInfo = PosInfo;
            Managers.Network.Send(movePacket);
            _updated = false;

            ServerCore.Logger.WriteLog(LogLevel.Debug, $"MyPlayerController.CheckUpdatedFlag. Send C_Move. " +
                $"playerId:{Id}, state:{PosInfo.State}, dir:{PosInfo.MoveDir}, pos:({PosInfo.PosX},{PosInfo.PosY})");
        }
    }

    // 스킬 사용 쿨타임을 적용해주는 코루틴
    Coroutine _coSkillCooltime;
    IEnumerator CoInputCooltime(float time)
    {
        yield return new WaitForSeconds(time);
        _coSkillCooltime = null;
    }
}
