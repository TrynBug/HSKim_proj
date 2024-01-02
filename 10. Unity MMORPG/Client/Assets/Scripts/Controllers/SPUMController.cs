using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.SocialPlatforms;
using UnityEngine.Tilemaps;

public class SPUMController : MonoBehaviour
{

    private Animator animator;


    GameObject _goMap;
    Tilemap _ground;
    Tilemap _wall;
    Tilemap _collision;

    MoveDir Dir { get; set; } = MoveDir.Down;
    bool _isMoveKeyPressed = false;
    bool _isUpdateCamera = true;

    private void Awake()
    {
        animator = GetComponent<Animator>();
    }

    private void Start()
    {
        _goMap = GameObject.Find("Map_001");
        _ground = Util.FindChild<Tilemap>(_goMap, "Ground", true);
        _wall = Util.FindChild<Tilemap>(_goMap, "Walls", true);
        _collision = Util.FindChild<Tilemap>(_goMap, "Collision", true);

        int xMax = _ground.cellBounds.xMax;
        int xMin = _ground.cellBounds.xMin;
        int yMax = _ground.cellBounds.yMax;
        int yMin = _ground.cellBounds.yMin;

        for (int y = yMin; y < yMax; y++)
        {
            for (int x = xMin; x < xMax; x++)
            {
                GameObject go = Managers.Resource.Instantiate("DummyText");
                Vector3 local = _ground.CellToLocal(new Vector3Int(x, y, 0));

                go.name = $"{go.name}_({x},{y})";
                go.transform.position = new Vector3(local.x, local.y, go.transform.position.z);
                TextMeshPro text = go.GetComponent<TextMeshPro>();
                text.text = $"({x},{y})";
            }
        }
    }


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
        if(_isMoveKeyPressed == true)
        {
            animator.SetFloat("RunState", 0.5f);
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
            animator.SetFloat("RunState", 0);
        }

        if (Input.GetKeyDown(KeyCode.Z))
        {
            animator.SetTrigger("Attack");
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
                // collision�� ����
                Vector3 posTile = _ground.CellToLocal(cell);
                switch(Dir)
                {
                    case MoveDir.Up:
                        // collision�� ���ʾƷ��� �ִ°�� �������� ���� �̵���
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftDown))
                            MoveSlightly(MoveDir.Left);
                        // collision�� �����ʾƷ��� �ִ°�� ���������� ���� �̵���
                        else if (IsPlayerAtDirection(posTile, MoveDir.RightDown))
                            MoveSlightly(MoveDir.Right);
                        break;

                    case MoveDir.Down:
                        // collision�� �������� �ִ°�� �������� ���� �̵���
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftUp))
                            MoveSlightly(MoveDir.Left);
                        // collision�� ���������� �ִ� ��� ���������� ���� �̵���
                        else if (IsPlayerAtDirection(posTile, MoveDir.RightUp))
                            MoveSlightly(MoveDir.Right);
                        break;

                    case MoveDir.Left:
                        // collision�� ���������� �ִ� ��� ���� ���� �̵���
                        if (IsPlayerAtDirection(posTile, MoveDir.RightUp))
                            MoveSlightly(MoveDir.Up);
                        // collision�� �����ʾƷ��� �ִ� ��� �Ʒ��� ���� �̵���
                        else if (IsPlayerAtDirection(posTile, MoveDir.RightDown))
                            MoveSlightly(MoveDir.Down);
                        break;

                    case MoveDir.Right:
                        // collision�� ���ʾƷ��� �ִ� ��� �Ʒ��� ���� �̵���
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftDown))
                            MoveSlightly(MoveDir.Down);
                        // collision�� �������� �ִ� ��� ���� ���� �̵���
                        else if (IsPlayerAtDirection(posTile, MoveDir.LeftUp))
                            MoveSlightly(MoveDir.Up);
                        break;

                    case MoveDir.LeftUp:
                        // collision�� ���ʾƷ��� �ִ� ��� �������� ���� �̵���
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftDown))
                            MoveSlightly(MoveDir.Left);
                        break;

                    case MoveDir.LeftDown:
                        // collision�� �������� �ִ°�� �������� ���� �̵���
                        if (IsPlayerAtDirection(posTile, MoveDir.LeftUp))
                            MoveSlightly(MoveDir.Left);
                        break;

                    case MoveDir.RightUp:
                        // collision�� �����ʾƷ��� �ִ°�� ���������� ���� �̵���
                        if (IsPlayerAtDirection(posTile, MoveDir.RightDown))
                            MoveSlightly(MoveDir.Right);
                        break;
                    case MoveDir.RightDown:
                        // collision�� ���������� �ִ°�� ���������� ���� �̵���
                        if (IsPlayerAtDirection(posTile, MoveDir.RightUp))
                            MoveSlightly(MoveDir.Right);
                        break;
                }
            }
        }

        //float moveX = Input.GetAxis("Horizontal");
        //float moveY = Input.GetAxis("Vertical");
        //Vector3 moveVector = new Vector3(moveX, moveY, 0f);
        //Vector3 pos = transform.position + moveVector.normalized * Time.deltaTime * 5f;
        //Vector3Int xLocal = _ground.LocalToCell(new Vector3(pos.x, transform.position.y, 0));
        //Vector3Int yLocal = _ground.LocalToCell(new Vector3(transform.position.x, pos.y, 0));
        //if(moveX != 0 || moveY != 0)
        //    Debug.Log($"move:{moveVector}, pos:{pos}, local:({xLocal.x},{yLocal.y})");
        //TileBase xTile = _collision.GetTile(xLocal);
        //TileBase yTile = _collision.GetTile(yLocal);
        //if (xTile != null && yTile != null)
        //{
        //    return;
        //}
        //else if (xTile != null)
        //{
        //    transform.position = new Vector3(transform.position.x, pos.y, transform.position.z);
        //}
        //else if (yTile != null)
        //{
        //    transform.position = new Vector3(pos.x, transform.position.y, transform.position.z);
        //}
        //else
        //{
        //    transform.position = pos;
        //}
    }

    void LateUpdate()
    {
        // ī�޶� ��ġ�� �÷��̾� ��ġ�� ������Ʈ
        if(_isUpdateCamera)
            Camera.main.transform.position = new Vector3(transform.position.x, transform.position.y, -10);
        _isUpdateCamera = true;
    }



    // ���� ���⿡ �ش��ϴ� ���� ���
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

    // tile�� position(positionOfTile)�� �������� �÷��̾ dirDesired ���⿡ ��ġ�ϴ����� Ȯ���Ѵ�.
    bool IsPlayerAtDirection(Vector3 positionOfTile, MoveDir expectedDirection)
    {
        Vector3 diff = transform.position - positionOfTile;
        switch(expectedDirection)
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


    // dir �������� �ణ �����δ�. �밢�����δ� �������� �ʴ´�.
    // �����̴°Ϳ� ������ �� _isUpdateCamera = false; �� �����Ѵ�.
    bool MoveSlightly(MoveDir dir)
    {
        Vector3 pos;
        Vector3Int cell;
        switch(dir)
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
}
