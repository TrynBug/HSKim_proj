using Google.Protobuf.Protocol;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using static Define;
using ServerCore;
using UnityEngine.Diagnostics;
using UnityEngine.Tilemaps;
using TMPro;
using Data;

public struct Pos
{
    public Pos(int y, int x) { Y = y; X = x; }
    public int Y;
    public int X;
}

public struct PQNode : IComparable<PQNode>
{
    public int F;
    public int G;
    public int Y;
    public int X;

    public int CompareTo(PQNode other)
    {
        if (F == other.F)
            return 0;
        return F < other.F ? 1 : -1;
    }
}

// 현재 맵의 grid 데이터를 관리하는 클래스
// 유니티의 Tools -> Generate Map 메뉴에 의해 생성된 map collision 파일 데이터를 가지고 있는다.
// map collision 데이터를 참조하여 grid 상에서 특정 좌표가 이동 가능한 좌표인지를 판단해준다.
public class MapManager
{
    // 맵 ID
    public int MapId {get; private set; }

    // 현재 맵의 grid
    public Grid CurrentGrid { get; private set; }
    public Tilemap TileGround { get; private set; }

    int _gridMaxX;
    int _gridMinX;
    int _gridMaxY;
    int _gridMinY;
    float _gridCellWidth;
    float _gridCellHeight;

    // cell 개수를 몇배수로 할지 결정함
    public int CellMultiple { get { return Data.Config.CellMultiple; } }

    // 1개 cell 크기
    public float CellWidth { get; private set; }    // cell 1개 너비
    public float CellHeight { get; private set; }   // cell 1개 높이

    // min,max 좌표
    public float PosMaxX { get; private set; }    // x 좌표 최대값
    public float PosMaxY { get; private set; }    // y 좌표 최대값
    public int CellMaxX { get; private set; }  // cell index x 최대값
    public int CellMaxY { get; private set; }  // cell index y 최대값
    public int TotalCellCount { get { return CellMaxX * CellMaxY; } }  // cell 개수

    // grid
    bool[,] _collision;
    BaseController[,] _objects;


    // utils
    public Vector2Int PosToCell(Vector2 pos)
    {
        return new Vector2Int((int)(pos.x / CellWidth), (int)(pos.y / CellHeight));
    }

    public Vector2 CellToPos(Vector2Int cell)
    {
        return new Vector2((float)cell.x * CellWidth, (float)cell.y * CellHeight);
    }

    public Vector2 CellToCenterPos(Vector2Int cell)
    {
        Vector2 pos = CellToPos(cell);
        return new Vector2(pos.x + CellWidth / 2f, pos.y + CellHeight / 2f);
    }

    public Vector3 ServerPosToClientPos(Vector2 serverPos)
    {
        Vector2 clientPos = new Vector2(serverPos.x + _gridMinX, _gridMaxY - serverPos.y);
        clientPos = TileGround.CellToLocalInterpolated(clientPos);
        clientPos += new Vector2(-_gridCellWidth / 2f, _gridCellHeight / 2f);
        return new Vector3(clientPos.x, clientPos.y, Config.ObjectDefaultZ);
    }

    public Vector2 ClientPosToServerPos(Vector2 clientPos)
    {
        Vector2 serverPos = clientPos += new Vector2(_gridCellWidth / 2f, -_gridCellHeight / 2f);
        serverPos = TileGround.LocalToCellInterpolated(new Vector2(serverPos.x, serverPos.y));
        serverPos = new Vector2(serverPos.x - _gridMinX, _gridMaxY - serverPos.y);
        return serverPos;
    }



    // check
    public bool IsInvalidPos(Vector2 pos)
    {
        return (pos.x < 0 || pos.x >= PosMaxX || pos.y < 0 || pos.y >= PosMaxY);
    }
    public bool IsInvalidCell(Vector2Int cell)
    {
        return (cell.x < 0 || cell.x >= CellMaxX || cell.y < 0 || cell.y >= CellMaxY);
    }
    // 비어있는 cell인지 확인
    public bool IsEmptyCell(Vector2Int cell, bool checkObjects = true)
    {
        if (IsInvalidCell(cell))
            return false;

        if (checkObjects)
            return (_collision[cell.y, cell.x] == false) && (_objects[cell.y, cell.x] == null);
        else
            return _collision[cell.y, cell.x] == false;
    }
    public bool IsEmptyCell(Vector2 pos, bool checkObjects = true)
    {
        return IsEmptyCell(PosToCell(pos), checkObjects);
    }
    // bc가 dest로 이동 가능한지 확인.
    // bc.Cell 이 dest와 같다면 같은cell에 있기 때문에 리턴값은 true이다.
    public bool IsMovable(BaseController bc, Vector2Int dest, bool checkObjects = true)
    {
        if (bc.Cell == dest)
            return true;
        return IsEmptyCell(dest, checkObjects);
    }
    public bool IsMovable(BaseController bc, Vector2 dest, bool checkObjects = true)
    {
        Vector2Int cell = PosToCell(dest);
        if (bc.Cell == cell)
            return true;
        return IsEmptyCell(cell, checkObjects);
    }


    // 맵 데이터 로드
    public void LoadMap(int mapId)
    {
        DestroyMap();
        MapId = mapId;

        string mapName = "Map_" + mapId.ToString("000");   // mapId를 00# 형태의 string으로 변경
        GameObject go = Managers.Resource.Instantiate($"Map/{mapName}");  // mapId에 해당하는 map 생성
        go.name = "Map";

        // Collision 타일맵을 찾은 다음, Active를 false로 하여 화면상에서는 보이지 않도록 만든다.
        GameObject collision = Util.FindChild(go, "Collision", true);
        if (collision != null)
            collision.SetActive(false);

        // grid 얻기
        CurrentGrid = go.GetComponent<Grid>();
        TileGround = Util.FindChild<Tilemap>(go, "Ground", true);

        // map collision 파일 로드
        TextAsset txt = Managers.Resource.Load<TextAsset>($"Map/{mapName}");
        StringReader reader = new StringReader(txt.text);

        // cell 크기 얻기
        _gridCellWidth = float.Parse(reader.ReadLine());
        _gridCellHeight = float.Parse(reader.ReadLine());
        CellWidth = 1f / (float)CellMultiple;     // Cell 크기는 1/CellMultiple 로 고정
        CellHeight = 1f / (float)CellMultiple;

        // grid의 min, max 위치 얻기
        _gridMinX = int.Parse(reader.ReadLine());
        _gridMaxX = int.Parse(reader.ReadLine());
        _gridMinY = int.Parse(reader.ReadLine());
        _gridMaxY = int.Parse(reader.ReadLine());
        int minX = _gridMinX;
        int maxX = _gridMaxX + 1;
        int minY = _gridMinY;
        int maxY = _gridMaxY + 1;
        CellMaxX = (maxX - minX) * CellMultiple;
        CellMaxY = (maxY - minY) * CellMultiple;
        PosMaxX = CellMaxX * CellWidth;
        PosMaxY = CellMaxY * CellHeight;

        // grid 생성
        _collision = new bool[CellMaxY, CellMaxX];
        _objects = new BaseController[CellMaxY, CellMaxX];

        // grid 상에서 갈 수 있는 위치 얻기
        int xCount = maxX - minX;
        int yCount = maxY - minY;
        for (int y = 0; y < yCount; y++)
        {
            string line = reader.ReadLine();
            for (int x = 0; x < xCount; x++)
            {
                if (line[x] == '1')
                {
                    _collision[y * CellMultiple, x * CellMultiple] = true;
                    _collision[y * CellMultiple, x * CellMultiple + 1] = true;
                    _collision[y * CellMultiple + 1, x * CellMultiple] = true;
                    _collision[y * CellMultiple + 1, x * CellMultiple + 1] = true;
                }
                else
                {
                    _collision[y * CellMultiple, x * CellMultiple] = false;
                    _collision[y * CellMultiple, x * CellMultiple + 1] = false;
                    _collision[y * CellMultiple + 1, x * CellMultiple] = false;
                    _collision[y * CellMultiple + 1, x * CellMultiple + 1] = false;
                }

            }
        }






        //DrawDebug();
    }

    void DrawDebug()
    {
        GameObject _goMap;
        Tilemap _ground;
        Tilemap _wall;
        Tilemap _collision;
        _goMap = GameObject.Find("Map_001");
        _ground = Util.FindChild<Tilemap>(_goMap, "Ground", true);
        _wall = Util.FindChild<Tilemap>(_goMap, "Walls", true);
        _collision = Util.FindChild<Tilemap>(_goMap, "Collision", true);

        int xMax = _ground.cellBounds.xMax;
        int xMin = _ground.cellBounds.xMin;
        int yMax = _ground.cellBounds.yMax;
        int yMin = _ground.cellBounds.yMin;
        int xBound = xMax - xMin;
        int yBound = yMax - yMin;

        for (int y = 0; y < yBound; y++)
        {
            for (int x = 0; x < xBound; x++)
            {
                GameObject go = Managers.Resource.Instantiate("DummyText");
                Vector3 pos = Managers.Map.ServerPosToClientPos(new Vector2(x, y));
                pos.z = 10;

                go.name = $"{go.name}_({x},{y})";
                go.transform.position = pos;
                TextMeshPro text = go.GetComponent<TextMeshPro>();
                text.text = $"({x},{y})";
            }
        }
    }


    public void DestroyMap()
    {
        GameObject map = GameObject.Find("Map");
        if(map != null)
        {
            GameObject.Destroy(map);
            CurrentGrid = null;
        }
    }

    // 특정 위치의 오브젝트 얻기
    public BaseController Find(Vector2Int cell)
    {
        if (IsInvalidCell(cell))
            return null;

        return _objects[cell.y, cell.x];
    }

    // 맵에 오브젝트 추가
    public bool Add(BaseController obj)
    {
        if (IsInvalidCell(obj.Cell))
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"Map.Add. Invalid Cell. {obj.ToString(InfoLevel.Position)}");
            return false;
        }

        int x = obj.Cell.x;
        int y = obj.Cell.y;
        if (IsEmptyCell(obj.Cell) == false)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"Map.Add. Cell already occupied " +
                $"cell:{obj.Cell}, collider:{_collision[y, x]}, occupied:[{_objects[y, x]}], me:[{obj}]");
            return false;
        }

        _objects[y, x] = obj;
        ServerCore.Logger.WriteLog(LogLevel.Debug, $"Map.Add. {obj.ToString(InfoLevel.Position)}");
        return true;
    }

    // 맵에서 오브젝트 제거
    public bool Remove(BaseController obj)
    {
        if (IsInvalidCell(obj.Cell))
            return false;

        int x = obj.Cell.x;
        int y = obj.Cell.y;
        if (_objects[y, x] != obj)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"Map.Remove. Object mismatch. cell:{obj.Cell}, resident:{_objects[y, x]} me:{obj}");
            return false;
        }

        _objects[y, x] = null;
        ServerCore.Logger.WriteLog(LogLevel.Debug, $"Map.Remove. {obj.ToString(InfoLevel.Position)}");
        return true;
    }


    // object를 위치로 이동
    // cell 이동에 오류가 없으면 true를 리턴한다.
    // 현재 cell과 목적지 cell의 위치가 같아도 오류가 없으면 true를 리턴한다.
    public bool TryMove(BaseController obj, Vector2Int dest)
    {
        int x = obj.Cell.x;
        int y = obj.Cell.y;
        if (x == dest.x && y == dest.y)
            return true;

        if (IsInvalidCell(obj.Cell) || IsInvalidCell(dest))
            return false;

        if (IsEmptyCell(dest) == false)
        {
            if (_objects[dest.y, dest.x] != null)
                ServerCore.Logger.WriteLog(LogLevel.Error, $"Map.Move. Cell already occupied. " +
                    $"cell:{obj.Cell}, collider:{_collision[dest.y, dest.x]}, occupied:[{_objects[dest.y, dest.x]}], me:[{obj}]");
            return false;
        }

        if (_objects[y, x] != obj)
        {
            ServerCore.Logger.WriteLog(LogLevel.Error, $"Map.Move. Object mismatch. " +
                $"cell:{obj.Cell}, resident:[{_objects[y, x]}], me:[{obj}]");
            return false;
        }

        // grid 이동
        _objects[y, x] = null;
        _objects[dest.y, dest.x] = obj;
        return true;
    }
    public bool TryMove(BaseController obj, Vector2 dest)
    {
        return TryMove(obj, PosToCell(dest));
    }

    // center를 기준으로 가장 가까운 빈 공간을 찾아준다.
    // 찾았으면 true, 찾지 못했으면 false를 리턴함
    public bool FindEmptyCell(Vector2Int center, bool checkObjects, out Vector2Int emptyCell)
    {
        if (IsInvalidCell(center))
        {
            emptyCell = Vector2Int.zero;
            return false;
        }

        int x = center.x;
        int y = center.y;
        int searchCount = 0;
        MoveDir dir = MoveDir.Down;

        int mustMoveCount = 0;  // 현재 방향으로 반드시 이동해야 하는 횟수
        int currMoveCount = 0;  // 현재 방향으로 지금까지 이동한 횟수
        while (true)
        {
            // 찾아본 cell의 수가 전체 cell의 수보다 크면 실패
            if (searchCount >= TotalCellCount)
            {
                emptyCell = Vector2Int.zero;
                return false;
            }

            // 좌표가 grid 내일 경우 오브젝트를 검색한다.
            if (!(x < 0 || x >= CellMaxX || y < 0 || y >= CellMaxY))
            {
                if (checkObjects)
                {
                    if (!_collision[y, x] && _objects[y, x] == null)
                    {
                        emptyCell = new Vector2Int(x, y);
                        return true;
                    }
                }
                else
                {
                    if (!_collision[y, x])
                    {
                        emptyCell = new Vector2Int(x, y);
                        return true;
                    }
                }
                searchCount++;
            }

            // 다음 방향으로 이동한다.
            // Down 방향으로 이동중일 때 mustMoveCount 만큼 이동했다면 Left로 방향을 바꾼다. 그런다음 mustMoveCount를 1 증가시킨다.
            // Left 방향으로 이동중일 때 mustMoveCount 만큼 이동했다면 Up으로 방향을 바꾼다.
            // Up 방향으로 이동중일 때 mustMoveCount 만큼 이동했다면 right로 방향을 바꾼다. 그런다음 mustMoveCount를 1 증가시킨다.
            // Right 방향으로 이동중일 때 mustMoveCount 만큼 이동했다면 Down으로 방향을 바꾼다.
            switch (dir)
            {
                case MoveDir.Down:
                    if (currMoveCount == mustMoveCount)
                    {
                        currMoveCount = 1;
                        mustMoveCount++;
                        dir = MoveDir.Left;
                        x--;
                    }
                    else
                    {
                        currMoveCount++;
                        y--;
                    }
                    break;
                case MoveDir.Left:
                    if (currMoveCount == mustMoveCount)
                    {
                        currMoveCount = 1;
                        dir = MoveDir.Up;
                        y++;
                    }
                    else
                    {
                        currMoveCount++;
                        x--;
                    }
                    break;
                case MoveDir.Up:
                    if (currMoveCount == mustMoveCount)
                    {
                        currMoveCount = 1;
                        mustMoveCount++;
                        dir = MoveDir.Right;
                        x++;
                    }
                    else
                    {
                        currMoveCount++;
                        y++;
                    }
                    break;
                case MoveDir.Right:
                    if (currMoveCount == mustMoveCount)
                    {
                        currMoveCount = 1;
                        dir = MoveDir.Down;
                        y--;
                    }
                    else
                    {
                        currMoveCount++;
                        x++;
                    }
                    break;
            }
        }
    }





    public bool CollisionDetection(Vector2 pos, Vector2 dest, out Vector2 intersection)
    {
        // reference : https://www.youtube.com/watch?v=NbSee-XM7WA
        const float zeroReplacement = 0.00000001f;
        intersection = new Vector2(0, 0);

        // cell 크기 1 x 1 을 기준으로 pos와 dest를 보정한다.
        Vector2 srcCellSize = new Vector2(CellWidth, CellHeight);
        Vector2 posAdj = pos / srcCellSize;
        Vector2 destAdj = dest / srcCellSize;
        Vector2 posMax = new Vector2(PosMaxX, PosMaxY) / srcCellSize;

        // 방향 계산
        Vector2 rayStart = posAdj;
        Vector2 rayDir = (destAdj - posAdj).normalized;
        if (rayDir.x == 0) rayDir.x = zeroReplacement;
        if (rayDir.y == 0) rayDir.y = zeroReplacement;

        // step size 계산
        Vector2 rayUnitStepSize = new Vector2((float)Math.Sqrt(1f + (rayDir.y / rayDir.x) * (rayDir.y / rayDir.x)), (float)Math.Sqrt(1f + (rayDir.x / rayDir.y) * (rayDir.x / rayDir.y)));
        Vector2 mapCheck = rayStart;
        Vector2 rayLength1D;
        Vector2 step;

        // 초기 조건 설정
        if (rayDir.x < 0)
        {
            step.x = -1;
            rayLength1D.x = (rayStart.x - mapCheck.x) * rayUnitStepSize.x;
        }
        else
        {
            step.x = 1;
            rayLength1D.x = ((mapCheck.x + 1) - rayStart.x) * rayUnitStepSize.x;
        }

        if (rayDir.y < 0)
        {
            step.y = -1;
            rayLength1D.y = (rayStart.y - mapCheck.y) * rayUnitStepSize.y;
        }
        else
        {
            step.y = 1;
            rayLength1D.y = ((mapCheck.y + 1) - rayStart.y) * rayUnitStepSize.y;
        }

        // collider에 부딪히거나 dest에 도착 시 종료
        bool bCollision = false;
        float maxDistance = (destAdj - posAdj).magnitude;
        float distance = 0;
        while (!bCollision && distance < maxDistance)
        {
            // Walk along shortest path
            if (rayLength1D.x < rayLength1D.y)
            {
                if (rayLength1D.x > maxDistance)
                    break;

                mapCheck.x += step.x;
                distance = rayLength1D.x;
                rayLength1D.x += rayUnitStepSize.x;
            }
            else
            {
                if (rayLength1D.y > maxDistance)
                    break;

                mapCheck.y += step.y;
                distance = rayLength1D.y;
                rayLength1D.y += rayUnitStepSize.y;
            }

            // Test tile at new test point
            if (mapCheck.x >= 0 && mapCheck.x < posMax.x && mapCheck.y >= 0 && mapCheck.y < posMax.y)
            {
                Vector2Int cell = PosToCell(mapCheck * srcCellSize);
                if (_collision[cell.y, cell.x] == true || _objects[cell.y, cell.x] != null)
                {
                    bCollision = true;
                }
            }
        }

        // 충돌 지점 계산
        if (bCollision)
        {
            intersection = rayStart + rayDir * distance;
            intersection *= srcCellSize;
        }

        return bCollision;
    }






    #region A* PathFinding

    //// U D L R
    //int[] _deltaY = new int[] { 1, -1, 0, 0 };
    //int[] _deltaX = new int[] { 0, 0, -1, 1 };
    //int[] _cost = new int[] { 10, 10, 10, 10 };

    //public List<Vector3Int> FindPath(Vector3Int startCellPos, Vector3Int destCellPos, bool ignoreDestCollision = false)
    //{
    //    List<Pos> path = new List<Pos>();

    //    // 점수 매기기
    //    // F = G + H
    //    // F = 최종 점수 (작을 수록 좋음, 경로에 따라 달라짐)
    //    // G = 시작점에서 해당 좌표까지 이동하는데 드는 비용 (작을 수록 좋음, 경로에 따라 달라짐)
    //    // H = 목적지에서 얼마나 가까운지 (작을 수록 좋음, 고정)

    //    // (y, x) 이미 방문했는지 여부 (방문 = closed 상태)
    //    bool[,] closed = new bool[SizeY, SizeX]; // CloseList

    //    // (y, x) 가는 길을 한 번이라도 발견했는지
    //    // 발견X => MaxValue
    //    // 발견O => F = G + H
    //    int[,] open = new int[SizeY, SizeX]; // OpenList
    //    for (int y = 0; y < SizeY; y++)
    //        for (int x = 0; x < SizeX; x++)
    //            open[y, x] = Int32.MaxValue;

    //    Pos[,] parent = new Pos[SizeY, SizeX];

    //    // 오픈리스트에 있는 정보들 중에서, 가장 좋은 후보를 빠르게 뽑아오기 위한 도구
    //    PriorityQueue<PQNode> pq = new PriorityQueue<PQNode>();

    //    // CellPos -> ArrayPos
    //    Pos pos = Cell2Pos(startCellPos);
    //    Pos dest = Cell2Pos(destCellPos);

    //    // 시작점 발견 (예약 진행)
    //    open[pos.Y, pos.X] = 10 * (Math.Abs(dest.Y - pos.Y) + Math.Abs(dest.X - pos.X));
    //    pq.Push(new PQNode() { F = 10 * (Math.Abs(dest.Y - pos.Y) + Math.Abs(dest.X - pos.X)), G = 0, Y = pos.Y, X = pos.X });
    //    parent[pos.Y, pos.X] = new Pos(pos.Y, pos.X);

    //    while (pq.Count > 0)
    //    {
    //        // 제일 좋은 후보를 찾는다
    //        PQNode node = pq.Pop();
    //        // 동일한 좌표를 여러 경로로 찾아서, 더 빠른 경로로 인해서 이미 방문(closed)된 경우 스킵
    //        if (closed[node.Y, node.X])
    //            continue;

    //        // 방문한다
    //        closed[node.Y, node.X] = true;
    //        // 목적지 도착했으면 바로 종료
    //        if (node.Y == dest.Y && node.X == dest.X)
    //            break;

    //        // 상하좌우 등 이동할 수 있는 좌표인지 확인해서 예약(open)한다
    //        for (int i = 0; i < _deltaY.Length; i++)
    //        {
    //            Pos next = new Pos(node.Y + _deltaY[i], node.X + _deltaX[i]);

    //            // 유효 범위를 벗어났으면 스킵
    //            // 벽으로 막혀서 갈 수 없으면 스킵
    //            if (!ignoreDestCollision || next.Y != dest.Y || next.X != dest.X)
    //            {
    //                if (CanGo(Pos2Cell(next)) == false) // CellPos
    //                    continue;
    //            }

    //            // 이미 방문한 곳이면 스킵
    //            if (closed[next.Y, next.X])
    //                continue;

    //            // 비용 계산
    //            int g = 0;// node.G + _cost[i];
    //            int h = 10 * ((dest.Y - next.Y) * (dest.Y - next.Y) + (dest.X - next.X) * (dest.X - next.X));
    //            // 다른 경로에서 더 빠른 길 이미 찾았으면 스킵
    //            if (open[next.Y, next.X] < g + h)
    //                continue;

    //            // 예약 진행
    //            open[dest.Y, dest.X] = g + h;
    //            pq.Push(new PQNode() { F = g + h, G = g, Y = next.Y, X = next.X });
    //            parent[next.Y, next.X] = new Pos(node.Y, node.X);
    //        }
    //    }

    //    return CalcCellPathFromParent(parent, dest);
    //}

    //List<Vector3Int> CalcCellPathFromParent(Pos[,] parent, Pos dest)
    //{
    //    List<Vector3Int> cells = new List<Vector3Int>();

    //    int y = dest.Y;
    //    int x = dest.X;
    //    while (parent[y, x].Y != y || parent[y, x].X != x)
    //    {
    //        cells.Add(Pos2Cell(new Pos(y, x)));
    //        Pos pos = parent[y, x];
    //        y = pos.Y;
    //        x = pos.X;
    //    }
    //    if(!(x == 0 && y == 0))
    //        cells.Add(Pos2Cell(new Pos(y, x)));
    //    cells.Reverse();
    //    return cells;
    //}

    //Pos Cell2Pos(Vector3Int cell)
    //{
    //    // CellPos -> ArrayPos
    //    return new Pos(MaxY - cell.y, cell.x - MinX);
    //}

    //Vector3Int Pos2Cell(Pos pos)
    //{
    //    // ArrayPos -> CellPos
    //    return new Vector3Int(pos.X + MinX, MaxY - pos.Y, 0);
    //}

    #endregion
}
