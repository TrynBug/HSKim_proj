using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using Google.Protobuf.Protocol;
using ServerCore;

namespace Server.Game
{
    public struct Vector2Int
    {
        public int x;
        public int y;

        public Vector2Int(int x, int y) { this.x = x; this.y = y; }

        public static Vector2Int up { get { return new Vector2Int(0, 1); } }
        public static Vector2Int down { get { return new Vector2Int(0, -1); } }
        public static Vector2Int left { get { return new Vector2Int(-1, 0); } }
        public static Vector2Int right { get { return new Vector2Int(1, 0); } }

        public static Vector2Int operator+(Vector2Int a, Vector2Int b)
        {
            return new Vector2Int(a.x + b.x, a.y + b.y);
        }

        public static Vector2Int operator -(Vector2Int a, Vector2Int b)
        {
            return new Vector2Int(a.x - b.x, a.y - b.y);
        }

        public int sqrtMagnitude { get { return (x * x + y * y); } }
        public float magnitude { get { return (float)Math.Sqrt(sqrtMagnitude); } }
        public int cellDistFromZero { get { return Math.Abs(x) + Math.Abs(y); } }

        public override string? ToString() { return $"({x},{y})"; }
    }

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

    // Map 데이터를 가지는 컴포넌트. Map 데이터가 필요한 게임룸은 이 컴포넌트를 가진다.
    // 현재 맵의 grid 데이터를 관리하는 클래스
    // 유니티의 Tools -> Generate Map 메뉴에 의해 생성된 map collision 파일 데이터를 가지고 있는다.
    // map collision 데이터를 참조하여 grid 상에서 특정 좌표가 이동 가능한 좌표인지를 판단해준다.
    public class Map
    {
        // grid min,max 좌표
        public int MaxX { get; set; }
        public int MaxY { get; set; }
        public int MinX { get; set; }
        public int MinY { get; set; }

        public int SizeX { get { return MaxX - MinX + 1; } }
        public int SizeY { get { return MaxY - MinY + 1; } }
        public int TotalCellCount { get { return SizeX * SizeY; } }

        // grid 좌표별 갈수있는 위치인지 여부
        bool[,] _collision;
        GameObject[,] _objects;

        // 맵 데이터 로드
        public void LoadMap(int mapId, string path)
        {
            string mapName = "Map_" + mapId.ToString("000");   // mapId를 00# 형태의 string으로 변경
            string text = File.ReadAllText($"{path}/{mapName}.txt");         // map 파일 읽기

            // map collision 텍스트 데이터로 StringReader 생성
            StringReader reader = new StringReader(text);

            // grid의 min, max 위치 얻기
            MinX = int.Parse(reader.ReadLine());
            MaxX = int.Parse(reader.ReadLine());
            MinY = int.Parse(reader.ReadLine());
            MaxY = int.Parse(reader.ReadLine());

            // grid 상에서 갈 수 있는 위치 얻기
            int xCount = MaxX - MinX + 1;
            int yCount = MaxY - MinY + 1;
            _collision = new bool[yCount, xCount];
            _objects = new GameObject[yCount, xCount];
            for (int y = 0; y < yCount; y++)
            {
                string line = reader.ReadLine();
                for (int x = 0; x < xCount; x++)
                {
                    if (line[x] == '1')
                        _collision[y, x] = true;
                    else
                        _collision[y, x] = false;

                }
            }
        }

        // 특정 위치의 오브젝트 얻기
        public GameObject Find(Vector2Int cellPos)
        {
            if (cellPos.x < MinX || cellPos.x > MaxX)
                return null;
            if (cellPos.y < MinY || cellPos.y > MaxY)
                return null;

            int x = cellPos.x - MinX;
            int y = MaxY - cellPos.y;
            return _objects[y, x];
        }

        // 맵에 오브젝트 추가
        public bool Add(GameObject gameObject)
        {
            if (CanGo(gameObject.CellPos, true) == false)
            {
                Logger.WriteLog(LogLevel.Error, $"Map.Add. Invalid position. " +
                    $"objectId:{gameObject.Info.ObjectId}, pos:({gameObject.CellPos.x},{gameObject.CellPos.y})");
                return false;
            }

            int x = gameObject.CellPos.x - MinX;
            int y = MaxY - gameObject.CellPos.y;
            if (_objects[y, x] != null)
            {
                Logger.WriteLog(LogLevel.Error, $"Map.Add. Cell already occupied " +
                    $"occupiedId:{_objects[y, x].Id}, myId:{gameObject.Info.ObjectId}");
                return false;
            }
            _objects[y, x] = gameObject;
            Logger.WriteLog(LogLevel.Debug, $"Map.Add. Id:{gameObject.Info.ObjectId}, pos:{gameObject.CellPos}, cell:({x},{y})");
            return true;
        }

        // 맵에서 오브젝트 제거
        public bool Remove(GameObject gameObject)
        {
            PositionInfo posInfo = gameObject.Info.PosInfo;
            if (posInfo.PosX < MinX || posInfo.PosX > MaxX)
                return false;
            if (posInfo.PosY < MinY || posInfo.PosY > MaxY)
                return false;

            int x = posInfo.PosX - MinX;
            int y = MaxY - posInfo.PosY;
            if (_objects[y, x] == null)
            {
                Logger.WriteLog(LogLevel.Error, $"Map.Remove. No object at position. " +
                    $"objectId:{gameObject.Info.ObjectId}, pos:({posInfo.PosX},{posInfo.PosY})");
                return false;
            }
            if (_objects[y,x] != gameObject)
            {
                Logger.WriteLog(LogLevel.Error, $"Map.Remove. Object mismatch. " +
                    $"occupiedId:{_objects[y,x].Id}, myId:{gameObject.Info.ObjectId}");
                return false;
            }

            _objects[y, x] = null;
            Logger.WriteLog(LogLevel.Debug, $"Map.Remove. Id:{gameObject.Info.ObjectId}, pos:{gameObject.CellPos}, cell:({x},{y})");
            return true;
        }

        // grid 상에서 해당 좌표가 갈 수 있는지 여부 확인
        public bool CanGo(Vector2Int cellPos, bool checkObjects = true)
        {
            if (cellPos.x < MinX || cellPos.x > MaxX)
                return false;
            if (cellPos.y < MinY || cellPos.y > MaxY)
                return false;

            int x = cellPos.x - MinX;
            int y = MaxY - cellPos.y;

            bool noWall = !_collision[y, x];
            bool noObject = checkObjects ? (_objects[y, x] == null) : true;
            return noWall && noObject;
        }

        // object를 위치로 이동
        public bool ApplyMove(GameObject gameObject, Vector2Int dest)
        {
            PositionInfo posInfo = gameObject.Info.PosInfo;
            if (posInfo.PosX < MinX || posInfo.PosX > MaxX)
                return false;
            if (posInfo.PosY < MinY || posInfo.PosY > MaxY)
                return false;

            if (posInfo.PosX == dest.x && posInfo.PosY == dest.y)
                return true;

            if (CanGo(dest, true) == false)
            {
                //Logger.WriteLog(LogLevel.Error, $"Map.ApplyMove. Object can't go to position. " +
                //    $"objectId:{gameObject.Info.ObjectId}, state:{posInfo.State}, dir:{posInfo.MoveDir}, pos:({posInfo.PosX},{posInfo.PosY}), dest:({dest.x},{dest.y})");
                return false;
            }

            // grid 이동
            int fromX = posInfo.PosX - MinX;
            int fromY = MaxY - posInfo.PosY;
            int toX = dest.x - MinX;
            int toY = MaxY - dest.y;
            if (_objects[fromY, fromX] != gameObject || _objects[toY, toX] != null)
            {
                Logger.WriteLog(LogLevel.Error, $"Map.ApplyMove. Invalid grid operation. objectId:{gameObject.Info.ObjectId}, " +
                    $"grid from:[objectId:{_objects[fromY, fromX]?.Info.ObjectId}, grid:({fromX},{fromY})], " +
                    $"grid to:[objectId:{_objects[toY, toX]?.Info.ObjectId}, grid:({toX},{toY})]");
                return false;
            }
            _objects[fromY, fromX] = null;
            _objects[toY, toX] = gameObject;

            // 위치 이동
            posInfo.PosX = dest.x;
            posInfo.PosY = dest.y;

            return true;
        }

        // center를 기준으로 가장 가까운 빈 공간을 찾아준다.
        // 찾았으면 true, 찾지 못했으면 false를 리턴함
        public bool GetEmptyCell(Vector2Int center, bool checkObjects, out Vector2Int emptyCell)
        {
            Pos pos = Cell2Pos(center);
            int x = pos.X;
            int y = pos.Y;
            int searchCount = 0;
            MoveDir dir = MoveDir.Down;

            int mustMoveCount = 0;  // 현재 방향으로 반드시 이동해야 하는 횟수
            int currMoveCount = 0;  // 현재 방향으로 지금까지 이동한 횟수
            while(true)
            {
                // 찾아본 cell의 수가 전체 cell의 수보다 크면 실패
                if (searchCount >= TotalCellCount)
                {
                    emptyCell = new Vector2Int(0, 0);
                    return false;
                }

                // 좌표가 grid 내일 경우 오브젝트를 검색한다.
                if (!(x < 0 || x >= SizeX || y < 0 || y >= SizeY))
                {
                    if (checkObjects)
                    {
                        if (!_collision[y, x] && _objects[y, x] == null)
                        {
                            emptyCell = Pos2Cell(new Pos(y, x));
                            return true;
                        }
                    }
                    else
                    {
                        if (!_collision[y, x])
                        {
                            emptyCell = Pos2Cell(new Pos(y, x));
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
                        if(currMoveCount == mustMoveCount)
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



        #region A* PathFinding

        // U D L R
        int[] _deltaY = new int[] { 1, -1, 0, 0 };
        int[] _deltaX = new int[] { 0, 0, -1, 1 };
        int[] _cost = new int[] { 10, 10, 10, 10 };

        public List<Vector2Int> FindPath(Vector2Int startCellPos, Vector2Int destCellPos, bool checkObjects = true)
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
                    if (next.Y != dest.Y || next.X != dest.X)
                    {
                        if (CanGo(Pos2Cell(next), checkObjects) == false) // CellPos
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

        List<Vector2Int> CalcCellPathFromParent(Pos[,] parent, Pos dest)
        {
            List<Vector2Int> cells = new List<Vector2Int>();

            int y = dest.Y;
            int x = dest.X;
            while (parent[y, x].Y != y || parent[y, x].X != x)
            {
                cells.Add(Pos2Cell(new Pos(y, x)));
                Pos pos = parent[y, x];
                y = pos.Y;
                x = pos.X;
            }
            if (!(x == 0 && y == 0))
                cells.Add(Pos2Cell(new Pos(y, x)));
            cells.Reverse();
            return cells;
        }

        Pos Cell2Pos(Vector2Int cell)
        {
            // CellPos -> ArrayPos
            return new Pos(MaxY - cell.y, cell.x - MinX);
        }

        Vector2Int Pos2Cell(Pos pos)
        {
            // ArrayPos -> CellPos
            return new Vector2Int(pos.X + MinX, MaxY - pos.Y);
        }

        #endregion
    }

}
