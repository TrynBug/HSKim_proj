using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEditor;
using UnityEngine;

// Jump Point Search 길찾기 알고리즘
public class JumpPointSearch
{
    enum CellInfo
    {
        FREE,
		OPEN,
		CLOSE,
		WALL,
		DESTINATION,
	};

    enum NodeType
    {
        NONE,
	    OPEN,
	    CLOSE,
	    END
    };

    enum NodeDirection
    {
        NONE,
	    UU,
	    RU,
	    RR,
	    RD,
	    DD,
	    LD,
	    LL,
	    LU,
    };

    class Node : IComparable<Node>
    {
        public int _x;
        public int _y;
        public Node _parent;
        public float _valG;
        public float _valH;
        public float _valF;
        public NodeType _type;
        public NodeDirection _direction;

        public Node(int x, int y, Node parent, float valG, float valH, NodeType type, NodeDirection direction)
        {
            _x = x;
            _y = y;
            _parent = parent;
            _valG = valG;
            _valH = valH;
            _valF = valG + valH;
            _type = type;
            _direction = direction;
        }

        public int CompareTo(Node other)
        {
            if (_valF > other._valF) return -1;
            else if (_valF < other._valF) return 1;
            else return 0;
        }
    }


    /* 사용자 정의 멤버 */
    Cell[,] _cellsOrigin;
    int _maxX;
    int _maxY;
    Vector2Int _start = new Vector2Int(0, 0);
    Vector2Int _end = new Vector2Int(0, 0);

    /* 내부 멤버 */
    CellInfo[,] _cells;
    Node _startNode;
    PriorityQueue<Node> _openList = new PriorityQueue<Node>();
    List<Node> _closeList = new List<Node>();
    List<Vector2Int> _path = new List<Vector2Int>();
    bool _isFoundDest = false;
    BresenhamPath _BPath = new BresenhamPath();


    public void Init(Cell[,] cells, int maxX, int maxY)
    {
        _cellsOrigin = cells;
        _maxX = maxX;
        _maxY = maxY;

        // cell 배열 생성
        _cells = new CellInfo[maxY, maxX];
    }

    void Clear()
    {
        _startNode = null;
        _openList.Clear();
        _closeList.Clear();
        _path.Clear();
        _isFoundDest = false;
    }


    public List<Vector2> SearchPath(Vector2Int start, Vector2Int end)
    {
        Clear();

        _start = new Vector2Int(Math.Clamp(start.x, 0, _maxX - 1), Math.Clamp(start.y, 0, _maxY - 1));
        _end = new Vector2Int(Math.Clamp(end.x, 0, _maxX - 1), Math.Clamp(end.y, 0, _maxY - 1));

        if (_start == _end)
            return new List<Vector2> { Managers.Map.CellToCenterPos(_start), Managers.Map.CellToCenterPos(_end) };

        // _cells 배열 초기화, 도착지점 정보 입력
        for (int y = 0; y < _maxY; y++)
        {
            for (int x = 0; x < _maxX; x++)
            {
                if (_cellsOrigin[y, x].Collider == true)
                    _cells[y, x] = CellInfo.WALL;
                else
                    _cells[y, x] = CellInfo.FREE;
            }
        }
        _cells[_end.y, _end.x] = CellInfo.DESTINATION;

        // 시작노드 생성
        _startNode = new Node(_start.x, _start.y, null, 0f, (float)(Math.Abs(_end.x - _start.x) + Math.Abs(_end.y - _start.y)), NodeType.OPEN, NodeDirection.NONE);
        _cells[_start.y, _start.x] = CellInfo.OPEN;

        // 8개 방향 search
        _searchUpward(_startNode);
        _searchRightUp(_startNode);
        _searchRight(_startNode);
        _searchRightDown(_startNode);
        _searchDown(_startNode);
        _searchLeftDown(_startNode);
        _searchLeft(_startNode);
        _searchLeftUp(_startNode);

        // close list에 시작 노드 등록
        _startNode._type = NodeType.CLOSE;
        _closeList.Add(_startNode);

        // 경로 찾기
        int loopCount = 0;
        while (_searchNextStep() == false)
        {
            if (loopCount++ > 1000)
            {
                Debug.Assert(loopCount < 1000);
                return null;
            }
        }


        // Bresenham 알고리즘으로 보정된 경로 생성
        bool isAccessible = true;
        loopCount = 0;
        List<Vector2> smoothPath = new List<Vector2>();
        smoothPath.Add(Managers.Map.CellToCenterPos(_path[0]));
        for (int from = 0; from < _path.Count - 1;)
        {
            if (loopCount++ > 1000)
            {
                Debug.Assert(loopCount < 1000);
                break;
            }

            for (int to = _path.Count - 1; to > from; to--)
            {
                isAccessible = true;
                _BPath.Init(_path[from], _path[to]);
                while (_BPath.Next())
                {
                    if (CELL(_BPath.x, _BPath.y) == CellInfo.WALL)
                    {
                        isAccessible = false;
                        break;
                    }
                }

                if (isAccessible)
                {
                    smoothPath.Add(Managers.Map.CellToCenterPos(_path[to]));
                    from = to;
                    break;
                }
            }
        }

        // 경로 return
        smoothPath.Reverse();
        return smoothPath;

    }



    // 다음 노드 탐색
    bool _searchNextStep()
    {
        // 남은 open node가 없으면 실패
        if (_openList.Count == 0)
            return true;

        // F value가 가장 작은 open node 획득
        Node parent = _openList.Pop();

        // 현재 노드를 open 에서 close로 변경
        parent._type = NodeType.CLOSE;
        _closeList.Add(parent);

        int x = parent._x;
        int y = parent._y;

        // 현재 노드가 목적지라면 도착
        if (CELL(x, y) == CellInfo.DESTINATION)
        {
            // 목적지에서 시작점까지의 경로를 저장함
            Node nextNode = parent;
            while (nextNode != _startNode)
            {
                _path.Add(new Vector2Int(nextNode._x, nextNode._y));
                nextNode = nextNode._parent;
            }
            _path.Add(new Vector2Int(nextNode._x, nextNode._y));
            return true;
        }

        // 다음 노드 search
        switch (parent._direction)
        {
            case NodeDirection.UU:
                {
                    _searchUpward(parent);

                    _searchLeftUp(parent);
                    _searchRightUp(parent);
                }
                break;

            case NodeDirection.RU:
                {
                    _searchUpward(parent);
                    _searchRight(parent);
                    _searchRightUp(parent);

                    _searchLeftUp(parent);
                    _searchRightDown(parent);
                }
                break;

            case NodeDirection.RR:
                {
                    _searchRight(parent);

                    _searchRightUp(parent);
                    _searchRightDown(parent);
                }
                break;

            case NodeDirection.RD:
                {
                    _searchRight(parent);
                    _searchDown(parent);
                    _searchRightDown(parent);

                    _searchLeftDown(parent);
                    _searchRightUp(parent);
                }
                break;

            case NodeDirection.DD:
                {
                    _searchDown(parent);

                    _searchLeftDown(parent);
                    _searchRightDown(parent);
                }
                break;


            case NodeDirection.LD:
                {
                    _searchLeft(parent);
                    _searchDown(parent);
                    _searchLeftDown(parent);

                    _searchLeftUp(parent);
                    _searchRightDown(parent);
                }
                break;


            case NodeDirection.LL:
                {
                    _searchLeft(parent);

                    _searchLeftUp(parent);
                    _searchLeftDown(parent);
                }
                break;


            case NodeDirection.LU:
                {
                    _searchUpward(parent);
                    _searchLeft(parent);
                    _searchLeftUp(parent);

                    _searchLeftDown(parent);
                    _searchRightUp(parent);
                }
                break;

        }
        return false;
    }



    CellInfo CELL(int x, int y) 
    { 
        return _cells[y, x]; 
    }
    CellInfo UU_CELL(int x, int y) 
    {
        if (y <= 0)
            return CellInfo.WALL;
        else 
            return _cells[y - 1, x]; 
    }
    CellInfo RU_CELL(int x, int y) 
    {
        if (y <= 0 || x >= _maxX - 1)
            return CellInfo.WALL;
        else
            return _cells[y - 1, x + 1]; 
    }
    CellInfo RR_CELL(int x, int y) 
    {
        if (x >= _maxX - 1)
            return CellInfo.WALL;
        else
            return _cells[y, x + 1]; 
    }
    CellInfo RD_CELL(int x, int y) 
    {
        if (y >= _maxY - 1 || x >= _maxX - 1)
            return CellInfo.WALL;
        else
            return _cells[y + 1, x + 1]; 
    }
    CellInfo DD_CELL(int x, int y) 
    {
        if (y >= _maxY - 1)
            return CellInfo.WALL;
        else
            return _cells[y + 1, x]; 
    }
    CellInfo LD_CELL(int x, int y) 
    {
        if (y >= _maxY || x <= 0)
            return CellInfo.WALL;
        else
            return _cells[y + 1, x - 1]; 
    }
    CellInfo LL_CELL(int x, int y) 
    {
        if (x <= 0)
            return CellInfo.WALL;
        else
            return _cells[y, x - 1]; 
    }
    CellInfo LU_CELL(int x, int y) 
    {
        if (y <= 0 || x <= 0)
            return CellInfo.WALL;
        else
            return _cells[y - 1, x - 1]; 
    }

    void _createDestNode(Node parent, float valG, NodeDirection direction)
    {
        if (_isFoundDest == false)
        {
            _isFoundDest = true;
            Node newNode = new Node(_end.x, _end.y, parent , valG, 0f, NodeType.OPEN, direction);
            _openList.Push(newNode);
        }
    }

    void _searchUpward(Node parent)
    {
        int x = parent._x;
        int y = parent._y;
        float valG = parent._valG;
        while (true)
        {
            y -= 1;
            if (y < 0)
                break;
            valG += 1f;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    _createDestNode(parent, valG, NodeDirection.UU);
                }
                break;
            }

            if ((LL_CELL(x, y) == CellInfo.WALL && LU_CELL(x, y) != CellInfo.WALL)
                || (RR_CELL(x, y) == CellInfo.WALL && RU_CELL(x, y) != CellInfo.WALL))
            {
                Node newNode = new Node(x, y, parent
                    , valG, (float)(Math.Abs(_end.x - x) + Math.Abs(_end.y - y))
                    , NodeType.OPEN, NodeDirection.UU);

                _openList.Push(newNode);
                _cells[y, x] = CellInfo.OPEN;
                break;
            }
        }
    }



    void _searchRightUp(Node parent)
    {
        int x = parent._x;
        int y = parent._y;
        float valG = parent._valG;
        while (true)
        {
            x += 1;
            y -= 1;
            if (x >= _maxX || y < 0)
                break;
            valG += 1f;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    _createDestNode(parent, valG, NodeDirection.RU);
                }
                break;
            }

            if ((LL_CELL(x, y) == CellInfo.WALL && LU_CELL(x, y) != CellInfo.WALL)
                || (DD_CELL(x, y) == CellInfo.WALL && RD_CELL(x, y) != CellInfo.WALL)
                || _lookRight(x, y)
                || _lookUpward(x, y))
            {
                Node newNode = new Node(x, y, parent
                    , valG, (float)(Math.Abs(_end.x - x) + Math.Abs(_end.y - y))
                    , NodeType.OPEN, NodeDirection.RU);
                _openList.Push(newNode);
                _cells[y, x] = CellInfo.OPEN;
                break;
            }
        }
    }



    void _searchRight(Node parent)
    {
        int x = parent._x;
        int y = parent._y;
        float valG = parent._valG;
        while (true)
        {
            x += 1;
            if (x >= _maxX)
                break;
            valG += 1f;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    _createDestNode(parent, valG, NodeDirection.RR);
                }
                break;
            }

            if ((UU_CELL(x, y) == CellInfo.WALL && RU_CELL(x, y) != CellInfo.WALL)
                || (DD_CELL(x, y) == CellInfo.WALL && RD_CELL(x, y) != CellInfo.WALL))
            {
                Node newNode = new Node(x, y, parent
                    , valG, (float)(Math.Abs(_end.x - x) + Math.Abs(_end.y - y))
                    , NodeType.OPEN, NodeDirection.RR);
                _openList.Push(newNode);
                _cells[y, x] = CellInfo.OPEN;
                break;
            }
        }
    }



    void _searchRightDown(Node parent)
    {
        int x = parent._x;
        int y = parent._y;
        float valG = parent._valG;
        while (true)
        {
            x += 1;
            y += 1;
            if (x >= _maxX || y >= _maxY)
                break;
            valG += 1f;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    _createDestNode(parent, valG, NodeDirection.RD);
                }
                break;
            }

            if ((LL_CELL(x, y) == CellInfo.WALL && LD_CELL(x, y) != CellInfo.WALL)
                || (UU_CELL(x, y) == CellInfo.WALL && RU_CELL(x, y) != CellInfo.WALL)
                || _lookRight(x, y)
                || _lookDown(x, y))

            {
                Node newNode = new Node(x, y, parent
                    , valG, (float)(Math.Abs(_end.x - x) + Math.Abs(_end.y - y))
                    , NodeType.OPEN, NodeDirection.RD);
                _openList.Push(newNode);
                _cells[y, x] = CellInfo.OPEN;
                break;
            }
        }
    }




    void _searchDown(Node parent)
    {
        int x = parent._x;
        int y = parent._y;
        float valG = parent._valG;
        while (true)
        {
            y += 1;
            if (y >= _maxY)
                break;
            valG += 1f;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    _createDestNode(parent, valG, NodeDirection.DD);
                }
                break;
            }

            if ((LL_CELL(x, y) == CellInfo.WALL && LD_CELL(x, y) != CellInfo.WALL)
                || (RR_CELL(x, y) == CellInfo.WALL && RD_CELL(x, y) != CellInfo.WALL))
            {
                Node newNode = new Node(x, y, parent
                    , valG, (float)(Math.Abs(_end.x - x) + Math.Abs(_end.y - y))
                    , NodeType.OPEN, NodeDirection.DD);
                _openList.Push(newNode);
                _cells[y, x] = CellInfo.OPEN;
                break;
            }
        }
    }





    void _searchLeftDown(Node parent)
    {
        int x = parent._x;
        int y = parent._y;
        float valG = parent._valG;
        while (true)
        {
            x -= 1;
            y += 1;
            if (x < 0 || y >= _maxY)
                break;
            valG += 1f;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    _createDestNode(parent, valG, NodeDirection.LD);
                }
                break;
            }

            if ((RR_CELL(x, y) == CellInfo.WALL && RD_CELL(x, y) != CellInfo.WALL)
                || (UU_CELL(x, y) == CellInfo.WALL && LU_CELL(x, y) != CellInfo.WALL)
                || _lookLeft(x, y)
                || _lookDown(x, y))
            {
                Node newNode = new Node(x, y, parent
                    , valG, (float)(Math.Abs(_end.x - x) + Math.Abs(_end.y - y))
                    , NodeType.OPEN, NodeDirection.LD);
                _openList.Push(newNode);
                _cells[y, x] = CellInfo.OPEN;
                break;
            }
        }
    }






    void _searchLeft(Node parent)
    {
        int x = parent._x;
        int y = parent._y;
        float valG = parent._valG;
        while (true)
        {
            x -= 1;
            if (x < 0)
                break;
            valG += 1f;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    _createDestNode(parent, valG, NodeDirection.LL);
                }
                break;
            }

            if ((UU_CELL(x, y) == CellInfo.WALL && LU_CELL(x, y) != CellInfo.WALL)
                || (DD_CELL(x, y) == CellInfo.WALL && LD_CELL(x, y) != CellInfo.WALL))
            {
                Node newNode = new Node(x, y, parent
                    , valG, (float)(Math.Abs(_end.x - x) + Math.Abs(_end.y - y))
                    , NodeType.OPEN, NodeDirection.LL);
                _openList.Push(newNode);
                _cells[y, x] = CellInfo.OPEN;
                break;
            }
        }
    }



    void _searchLeftUp(Node parent)
    {
        int x = parent._x;
        int y = parent._y;
        float valG = parent._valG;
        while (true)
        {
            x -= 1;
            y -= 1;
            if (x < 0 || y < 0)
                break;
            valG += 1f;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    _createDestNode(parent, valG, NodeDirection.LU);
                }
                break;
            }

            if ((RR_CELL(x, y) == CellInfo.WALL && RU_CELL(x, y) != CellInfo.WALL)
                || (DD_CELL(x, y) == CellInfo.WALL && LD_CELL(x, y) != CellInfo.WALL)
                || _lookLeft(x, y)
                || _lookUpward(x, y))

            {
                Node newNode = new Node(x, y, parent
                    , valG, (float)(Math.Abs(_end.x - x) + Math.Abs(_end.y - y))
                    , NodeType.OPEN, NodeDirection.LU);
                _openList.Push(newNode);
                _cells[y, x] = CellInfo.OPEN;
                break;
            }
        }
    }





    bool _lookRight(int x, int y)
    {
        while (true)
        {
            x += 1;
            if (x >= _maxX)
                return false;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    return true;
                }
                return false;
            }

            if ((UU_CELL(x, y) == CellInfo.WALL && RU_CELL(x, y) != CellInfo.WALL)
                || (DD_CELL(x, y) == CellInfo.WALL && RD_CELL(x, y) != CellInfo.WALL))
            {
                return true;
            }


        }
    }


    bool _lookUpward(int x, int y)
    {
        while (true)
        {
            y -= 1;
            if (y < 0)
                return false;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    return true;
                }
                return false;
            }

            if ((LL_CELL(x, y) == CellInfo.WALL && LU_CELL(x, y) != CellInfo.WALL)
                || (RR_CELL(x, y) == CellInfo.WALL && RU_CELL(x, y) != CellInfo.WALL))
            {
                return true;
            }
        }
    }


    bool _lookDown(int x, int y)
    {
        while (true)
        {
            y += 1;
            if (y >= _maxY)
                return false;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    return true;
                }
                return false;
            }

            if ((LL_CELL(x, y) == CellInfo.WALL && LD_CELL(x, y) != CellInfo.WALL)
                || (RR_CELL(x, y) == CellInfo.WALL && RD_CELL(x, y) != CellInfo.WALL))
            {
                return true;
            }
        }
    }



    bool _lookLeft(int x, int y)
    {
        while (true)
        {
            x -= 1;
            if (x < 0)
                return false;

            if (CELL(x, y) != CellInfo.FREE)
            {
                if (CELL(x, y) == CellInfo.DESTINATION)
                {
                    return true;
                }
                return false;
            }

            if ((UU_CELL(x, y) == CellInfo.WALL && LU_CELL(x, y) != CellInfo.WALL)
                || (DD_CELL(x, y) == CellInfo.WALL && LD_CELL(x, y) != CellInfo.WALL))
            {
                return true;
            }
        }
    }

}

