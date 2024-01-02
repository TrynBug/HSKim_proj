using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using Unity.VisualScripting.FullSerializer;
using UnityEngine;

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
    // 현재 맵의 grid
    public Grid CurrentGrid { get; private set; }

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
    GameObject[,] _objects;


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


    // 맵 데이터 로드
    public void LoadMap(int mapId)
    {
        DestroyMap();

        string mapName = "Map_" + mapId.ToString("000");   // mapId를 00# 형태의 string으로 변경
        GameObject go = Managers.Resource.Instantiate($"Map/{mapName}");  // mapId에 해당하는 map 생성
        go.name = "Map";

        // Collision 타일맵을 찾은 다음, Active를 false로 하여 화면상에서는 보이지 않도록 만든다.
        GameObject collision = Util.FindChild(go, "Collision", true);
        if (collision != null)
            collision.SetActive(false);

        // grid 얻기
        CurrentGrid = go.GetComponent<Grid>();

        // map collision 파일 로드
        TextAsset txt = Managers.Resource.Load<TextAsset>($"Map/{mapName}");
        StringReader reader = new StringReader(txt.text);

        // cell 크기 얻기
        CellWidth = float.Parse(reader.ReadLine());
        CellHeight = float.Parse(reader.ReadLine());

        // grid의 min, max 위치 얻기
        int minX = int.Parse(reader.ReadLine());
        int maxX = int.Parse(reader.ReadLine()) + 1;
        int minY = int.Parse(reader.ReadLine());
        int maxY = int.Parse(reader.ReadLine()) + 1;
        PosMaxX = (maxX - minX + 1) * CellWidth;
        PosMaxY = (maxY - minY + 1) * CellWidth;
        CellMaxX = (maxX - minX + 1) * CellMultiple;
        CellMaxY = (maxY - minY + 1) * CellMultiple;


        // grid 생성
        _collision = new bool[CellMaxY, CellMaxX];
        _objects = new GameObject[CellMaxY, CellMaxX];

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

                }
                else
                {
                    _collision[y * CellMultiple, x * CellMultiple] = false;
                    _collision[y * CellMultiple, x * CellMultiple + 1] = false;
                }

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


    #region A* PathFinding

    // U D L R
    int[] _deltaY = new int[] { 1, -1, 0, 0 };
    int[] _deltaX = new int[] { 0, 0, -1, 1 };
    int[] _cost = new int[] { 10, 10, 10, 10 };

    public List<Vector3Int> FindPath(Vector3Int startCellPos, Vector3Int destCellPos, bool ignoreDestCollision = false)
    {
        List<Pos> path = new List<Pos>();

        // 점수 매기기
        // F = G + H
        // F = 최종 점수 (작을 수록 좋음, 경로에 따라 달라짐)
        // G = 시작점에서 해당 좌표까지 이동하는데 드는 비용 (작을 수록 좋음, 경로에 따라 달라짐)
        // H = 목적지에서 얼마나 가까운지 (작을 수록 좋음, 고정)

        // (y, x) 이미 방문했는지 여부 (방문 = closed 상태)
        bool[,] closed = new bool[SizeY, SizeX]; // CloseList

        // (y, x) 가는 길을 한 번이라도 발견했는지
        // 발견X => MaxValue
        // 발견O => F = G + H
        int[,] open = new int[SizeY, SizeX]; // OpenList
        for (int y = 0; y < SizeY; y++)
            for (int x = 0; x < SizeX; x++)
                open[y, x] = Int32.MaxValue;

        Pos[,] parent = new Pos[SizeY, SizeX];

        // 오픈리스트에 있는 정보들 중에서, 가장 좋은 후보를 빠르게 뽑아오기 위한 도구
        PriorityQueue<PQNode> pq = new PriorityQueue<PQNode>();

        // CellPos -> ArrayPos
        Pos pos = Cell2Pos(startCellPos);
        Pos dest = Cell2Pos(destCellPos);

        // 시작점 발견 (예약 진행)
        open[pos.Y, pos.X] = 10 * (Math.Abs(dest.Y - pos.Y) + Math.Abs(dest.X - pos.X));
        pq.Push(new PQNode() { F = 10 * (Math.Abs(dest.Y - pos.Y) + Math.Abs(dest.X - pos.X)), G = 0, Y = pos.Y, X = pos.X });
        parent[pos.Y, pos.X] = new Pos(pos.Y, pos.X);

        while (pq.Count > 0)
        {
            // 제일 좋은 후보를 찾는다
            PQNode node = pq.Pop();
            // 동일한 좌표를 여러 경로로 찾아서, 더 빠른 경로로 인해서 이미 방문(closed)된 경우 스킵
            if (closed[node.Y, node.X])
                continue;

            // 방문한다
            closed[node.Y, node.X] = true;
            // 목적지 도착했으면 바로 종료
            if (node.Y == dest.Y && node.X == dest.X)
                break;

            // 상하좌우 등 이동할 수 있는 좌표인지 확인해서 예약(open)한다
            for (int i = 0; i < _deltaY.Length; i++)
            {
                Pos next = new Pos(node.Y + _deltaY[i], node.X + _deltaX[i]);

                // 유효 범위를 벗어났으면 스킵
                // 벽으로 막혀서 갈 수 없으면 스킵
                if (!ignoreDestCollision || next.Y != dest.Y || next.X != dest.X)
                {
                    if (CanGo(Pos2Cell(next)) == false) // CellPos
                        continue;
                }

                // 이미 방문한 곳이면 스킵
                if (closed[next.Y, next.X])
                    continue;

                // 비용 계산
                int g = 0;// node.G + _cost[i];
                int h = 10 * ((dest.Y - next.Y) * (dest.Y - next.Y) + (dest.X - next.X) * (dest.X - next.X));
                // 다른 경로에서 더 빠른 길 이미 찾았으면 스킵
                if (open[next.Y, next.X] < g + h)
                    continue;

                // 예약 진행
                open[dest.Y, dest.X] = g + h;
                pq.Push(new PQNode() { F = g + h, G = g, Y = next.Y, X = next.X });
                parent[next.Y, next.X] = new Pos(node.Y, node.X);
            }
        }

        return CalcCellPathFromParent(parent, dest);
    }

    List<Vector3Int> CalcCellPathFromParent(Pos[,] parent, Pos dest)
    {
        List<Vector3Int> cells = new List<Vector3Int>();

        int y = dest.Y;
        int x = dest.X;
        while (parent[y, x].Y != y || parent[y, x].X != x)
        {
            cells.Add(Pos2Cell(new Pos(y, x)));
            Pos pos = parent[y, x];
            y = pos.Y;
            x = pos.X;
        }
        if(!(x == 0 && y == 0))
            cells.Add(Pos2Cell(new Pos(y, x)));
        cells.Reverse();
        return cells;
    }

    Pos Cell2Pos(Vector3Int cell)
    {
        // CellPos -> ArrayPos
        return new Pos(MaxY - cell.y, cell.x - MinX);
    }

    Vector3Int Pos2Cell(Pos pos)
    {
        // ArrayPos -> CellPos
        return new Vector3Int(pos.X + MinX, MaxY - pos.Y, 0);
    }

    #endregion
}
