using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using Google.Protobuf.Protocol;
using Server.Data;
using Server.Game;
using ServerCore;

namespace Server.Game
{
    // Map 데이터를 가지는 컴포넌트. Map 데이터가 필요한 게임룸은 이 컴포넌트를 가진다.
    // 현재 맵의 grid 데이터를 관리하는 클래스
    // 유니티의 Tools -> Generate Map 메뉴에 의해 생성된 map collision 파일 데이터를 가지고 있는다.
    // map collision 데이터를 참조하여 grid 상에서 특정 좌표가 이동 가능한 좌표인지를 판단해준다.
    public class Map
    {
        public int Id { get; private set; }

        // cell 개수를 몇배수로 할지 결정함
        public int CellMultiple { get { return Config.CellMultiple; } }

        // 1개 cell 크기
        public float CellWidth { get; private set; }    // cell 1개 너비
        public float CellHeight { get; private set; }   // cell 1개 높이

        // min,max 좌표
        public float PosMaxX { get; private set; }    // x 좌표 최대값
        public float PosMaxY { get; private set; }    // y 좌표 최대값
        public int CellMaxX { get; private set; }  // cell index x 최대값
        public int CellMaxY { get; private set; }  // cell index y 최대값
        public int TotalCellCount { get { return CellMaxX * CellMaxY; } }  // cell 개수

        public Vector2 PosCenter { get { return CellToCenterPos(CellCenter); } }
        public Vector2Int CellCenter { get { return new Vector2Int(CellMaxX / 2, CellMaxY / 2); } }

        // grid
        public Cell[,] _cells;

        // map data
        MapData _mapData;


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



        // check
        public bool IsInvalidPos(Vector2 pos)
        {
            return (pos.x < 0 || pos.x >= PosMaxX || pos.y < 0 || pos.y >= PosMaxY);
        }
        public bool IsInvalidCell(Vector2Int cell)
        {
            return (cell.x < 0 || cell.x >= CellMaxX || cell.y < 0 || cell.y >= CellMaxY);
        }
        public Vector2 GetValidPos(Vector2 pos)
        {
            return new Vector2(Math.Clamp(pos.x, 0f, PosMaxX), Math.Clamp(pos.y, 0f, PosMaxY));
        }
        public Vector2Int GetValidCell(Vector2Int cell)
        {
            return new Vector2Int(Math.Clamp(cell.x, 0, CellMaxX), Math.Clamp(cell.y, 0, CellMaxY));
        }
        // 비어있는 cell인지 확인
        public bool IsEmptyCell(Vector2Int cell, bool checkObjects = true)
        {
            if (IsInvalidCell(cell))
                return false;

            if (checkObjects)
                return (_cells[cell.y, cell.x].Collider == false) && (_cells[cell.y, cell.x].Object == null);
            else
                return _cells[cell.y, cell.x].Collider == false;
        }
        public bool IsEmptyCell(Vector2 pos, bool checkObjects = true)
        {
            return IsEmptyCell(PosToCell(pos), checkObjects);
        }


        // 맵 데이터 로드
        public void LoadMap(int mapId)
        {
            // 맵 데이터 가져오기
            MapData mapData;
            if(DataManager.MapDict.TryGetValue(mapId, out mapData) == false)
            {
                Logger.WriteLog(LogLevel.Error, $"Map.LoadMap. Invalid mapId. {this}");
                return;
            }
            _mapData = mapData;
            Id = mapData.id;

            // cell 크기 얻기
            float cellWidth = mapData.cellWidth;
            float cellHeight = mapData.cellHeight;
            CellWidth = 1f / (float)CellMultiple;   // Cell 크기는 1/CellMultiple 로 고정
            CellHeight = 1f / (float)CellMultiple;

            // grid의 min, max 위치 얻기
            int minX = mapData.cellBoundMinX;
            int maxX = mapData.cellBoundMaxX + 1;
            int minY = mapData.cellBoundMinY;
            int maxY = mapData.cellBoundMaxY + 1;
            CellMaxX = (maxX - minX) * CellMultiple;
            CellMaxY = (maxY - minY) * CellMultiple;
            PosMaxX = CellMaxX * CellWidth - 0.01f; ;
            PosMaxY = CellMaxY * CellHeight - 0.01f; ;


            // grid 생성
            _cells = new Cell[CellMaxY, CellMaxX];
            for (int y = 0; y < CellMaxY; y++)
                for (int x = 0; x < CellMaxX; x++)
                    _cells[y, x] = new Cell();

            // grid 상에서 갈 수 있는 위치 얻기
            int xCount = maxX - minX;
            int yCount = maxY - minY;
            for (int y = 0; y < yCount; y++)
            {
                string line = mapData.collisions[y];
                for (int x = 0; x < xCount; x++)
                {
                    if (line[x] == '1')
                    {
                        _cells[y * CellMultiple, x * CellMultiple].Collider = true;
                        _cells[y * CellMultiple, x * CellMultiple + 1].Collider = true;
                        _cells[y * CellMultiple + 1, x * CellMultiple].Collider = true;
                        _cells[y * CellMultiple + 1, x * CellMultiple + 1].Collider = true;
                    }
                    else
                    {
                        _cells[y * CellMultiple, x * CellMultiple].Collider = false;
                        _cells[y * CellMultiple, x * CellMultiple + 1].Collider = false;
                        _cells[y * CellMultiple + 1, x * CellMultiple].Collider = false;
                        _cells[y * CellMultiple + 1, x * CellMultiple + 1].Collider = false;
                    }

                }
            }
        }






        // 특정 위치의 오브젝트 얻기
        public GameObject Find(Vector2Int cell)
        {
            if (IsInvalidCell(cell))
                return null;

            return _cells[cell.y, cell.x].Object;
        }


        // 오브젝트가 위치한 텔레포트 얻기
        public TeleportData GetTeleportUnderObject(GameObject obj)
        {
            foreach (TeleportData teleport in _mapData.teleports)
            {
                Vector2 pos = new Vector2(obj.Pos.x - teleport.posX, obj.Pos.y - teleport.posY);
                if (pos.x > -teleport.width / 2 && pos.x < teleport.width / 2 && pos.y > -teleport.height / 2 && pos.y < teleport.height / 2)
                    return teleport;
            }
            return null;
        }

        // enter zone 위치 얻기
        public Vector2 GetPosEnterZone(int zoneNumber)
        {
            if (zoneNumber < 0 || zoneNumber >= _mapData.enterZones.Count)
                return PosCenter;
            return new Vector2(_mapData.enterZones[zoneNumber].posX, _mapData.enterZones[zoneNumber].posY);
        }



        // 맵에 오브젝트 추가
        public bool Add(GameObject gameObject)
        {
            if (IsInvalidCell(gameObject.Cell))
            {
                Logger.WriteLog(LogLevel.Error, $"Map.Add. Invalid Cell. {this}, {gameObject.ToString(InfoLevel.Position)}");
                return false;
            }

            int x = gameObject.Cell.x;
            int y = gameObject.Cell.y;
            if (IsEmptyCell(gameObject.Cell) == false)
            {
                Logger.WriteLog(LogLevel.Error, $"Map.Add. Cell already occupied. " +
                    $"{this}, cell:{gameObject.Cell}, collider:{_cells[y, x].Collider}, occupied:[{_cells[y, x].Object}], me:[{gameObject}]");
                return false;
            }

            _cells[y, x].Object = gameObject;
            Logger.WriteLog(LogLevel.Debug, $"Map.Add. {gameObject.ToString(InfoLevel.Position)}");
            return true;
        }



        // 맵에서 오브젝트 제거
        public bool Remove(GameObject gameObject)
        {
            if (IsInvalidCell(gameObject.Cell))
                return false;

            int x = gameObject.Cell.x;
            int y = gameObject.Cell.y;
            if (_cells[y,x].RemoveObject(gameObject) == false)
            {
                Logger.WriteLog(LogLevel.Error, $"Map.Remove. No object in the cell. {gameObject.ToString(InfoLevel.Position)}");
                return false;
            }

            Logger.WriteLog(LogLevel.Debug, $"Map.Remove. {gameObject.ToString(InfoLevel.Position)}");
            return true;
        }


        // object를 dest 위치에 MovingObject로 이동시킨다.
        public bool TryMoving(GameObject obj, Vector2Int dest)
        {
            // 오류체크
            Vector2Int cell = obj.Cell;
            if (IsInvalidCell(cell))
            {
                Logger.WriteLog(LogLevel.Error, $"Map.TryMoving. Invalid cell. {obj.ToString(InfoLevel.Position)}");
                return false;
            }

            Cell currCell = _cells[cell.y, cell.x];
            if (currCell.HasObject(obj) == false)
            {
                Logger.WriteLog(LogLevel.Error, $"Map.TryMoving. No object in the cell. {obj.ToString(InfoLevel.Position)}");
                return false;
            }

            dest = GetValidCell(dest);
            Cell destCell = _cells[dest.y, dest.x];

            // dest에 collider가 있을 경우 실패
            if (destCell.Collider == true)
                return false;

            // cell 이동
            if (currCell.Object == obj)
            {
                currCell.Object = null;
                destCell.AddMovingObject(obj);
            }
            else if (cell == dest)
            {
                return true;
            }
            else
            {
                currCell.RemoveMovingObject(obj);
                destCell.AddMovingObject(obj);
            }

            return true;
        }
        public bool TryMoving(GameObject obj, Vector2 dest)
        {
            return TryMoving(obj, PosToCell(dest));
        }



        // object를 dest에 위치시킨다.
        // dest에 위치할 수 없을 경우 가장 가까운 cell에 위치시킨다.
        // 현재 cell과 dest의 위치가 같아도 오류가 없으면 true를 리턴한다.
        // 이동할 수 없을 경우 false를 리턴한다. 이 때는 stopPos 값을 사용하면 안된다.
        public bool TryStop(GameObject obj, Vector2 targetPos, out Vector2 stopPos)
        {
            // 오류체크
            Vector2Int cell = obj.Cell;
            Vector2Int dest = PosToCell(targetPos);

            if (IsInvalidCell(cell))
            {
                ServerCore.Logger.WriteLog(LogLevel.Error, $"Map.Stop. Invalid cell. {obj.ToString(InfoLevel.Position)}");
                stopPos = obj.Pos;
                return false;
            }

            Cell currCell = _cells[cell.y, cell.x];
            bool isMovingobject = currCell.HasMovingObject(obj);
            if (currCell.HasObject(obj) == false)
            {
                ServerCore.Logger.WriteLog(LogLevel.Error, $"Map.Stop. No object in the cell. {obj.ToString(InfoLevel.Position)}");
                stopPos = obj.Pos;
                return false;
            }

            dest = GetValidCell(dest);
            Cell destCell = _cells[dest.y, dest.x];

            // dest 위치가 비어있을 경우 성공
            if (IsEmptyCell(dest))
            {
                if (isMovingobject)
                    currCell.RemoveMovingObject(obj);
                else
                    currCell.Object = null;
                destCell.Object = obj;
                stopPos = targetPos;
                return true;
            }

            // dest가 비어있지 않지만 dest에 내가 위치해 있을 경우 성공
            if(dest == cell && destCell.Object == obj)
            {
                stopPos = targetPos;
                return true;
            }

            // dest 위치가 비어있지 않을 경우 비어있는 가장 가까운 위치를 찾는다.
            Vector2Int emptyCell;
            if (FindEmptyCellOfDirection(obj, Util.GetOppositeDirection(obj.Dir), dest, out emptyCell) == false)
            {
                stopPos = obj.Pos;
                return false;
            }

            // 비어있는 가장 가까운 cell로 이동
            if (isMovingobject)
                currCell.RemoveMovingObject(obj);
            else
                currCell.Object = null;
            _cells[emptyCell.y, emptyCell.x].Object = obj;
            stopPos = CellToCenterPos(emptyCell);
            return true;
        }


        // origin을 기준으로 dir 방향의 cell을 찾는다.
        public Vector2Int FindCellOfDirection(MoveDir dir, Vector2Int origin)
        {
            Vector2Int cell;
            switch (dir)
            {
                case MoveDir.Up:
                    cell = new Vector2Int(origin.x + 1, origin.y - 1);
                    break;
                case MoveDir.Down:
                    cell = new Vector2Int(origin.x - 1, origin.y + 1);
                    break;
                case MoveDir.Left:
                    cell = new Vector2Int(origin.x - 1, origin.y - 1);
                    break;
                case MoveDir.Right:
                    cell = new Vector2Int(origin.x + 1, origin.y + 1);
                    break;
                case MoveDir.LeftUp:
                    cell = new Vector2Int(origin.x, origin.y - 1);
                    break;
                case MoveDir.LeftDown:
                    cell = new Vector2Int(origin.x - 1, origin.y);
                    break;
                case MoveDir.RightUp:
                    cell = new Vector2Int(origin.x + 1, origin.y);
                    break;
                case MoveDir.RightDown:
                    cell = new Vector2Int(origin.x, origin.y + 1);
                    break;
                default:
                    cell = origin;
                    break;
            }
            return cell;
        }

        // origin을 기준으로 dir 방향의 비어있는 cell을 찾는다.
        public bool FindEmptyCellOfDirection(GameObject me, MoveDir dir, Vector2Int origin, out Vector2Int emptyCell)
        {
            Vector2Int cell = origin;
            while (true)
            {
                cell = FindCellOfDirection(dir, cell);
                if (IsInvalidCell(cell))
                {
                    emptyCell = cell;
                    return false;
                }
                if (IsEmptyCell(cell))
                {
                    emptyCell = cell;
                    return true;
                }
                if (me != null && _cells[cell.y, cell.x].Object == me)
                {
                    emptyCell = cell;
                    return true;
                }
            }
        }






        // center를 기준으로 가장 가까운 빈 공간을 찾아준다.
        // 찾았으면 true, 찾지 못했으면 false를 리턴함
        public bool FindEmptyCell(Vector2Int center, bool checkObjects, out Vector2Int emptyCell)
        {
            center = GetValidCell(center);

            int x = center.x;
            int y = center.y;
            int searchCount = 0;
            MoveDir dir = MoveDir.Down;

            int mustMoveCount = 0;  // 현재 방향으로 반드시 이동해야 하는 횟수
            int currMoveCount = 0;  // 현재 방향으로 지금까지 이동한 횟수
            while(true)
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
                        if (!_cells[y, x].Collider && _cells[y, x].Object == null)
                        {
                            emptyCell = new Vector2Int(x, y);
                            return true;
                        }
                    }
                    else
                    {
                        if (!_cells[y, x].Collider)
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




        // pos에서 dest 위치로 갈 수 있는지 확인한다.
        // collider와의 충돌만 고려하고 object와의 충돌은 무시한다.
        // collider와 충돌했을 경우 false를 리턴하고 intersection에 최종 목적지를 입력한다.
        public bool CanGo(Vector2 pos, Vector2 dest, out Vector2 intersection)
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
                    if (_cells[cell.y, cell.x].Collider == true)
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

            return !bCollision;
        }







        // ToString
        public override string ToString()
        {
            return $"map:{Id}";
        }






        #region A* PathFinding
        public List<Vector2Int> FindPath(Vector2Int startCellPos, Vector2Int destCellPos, bool checkObjects = true)
        {
            return null;
        }
        //public struct Pos
        //{
        //    public Pos(int y, int x) { Y = y; X = x; }
        //    public int Y;
        //    public int X;
        //}

        //public struct PQNode : IComparable<PQNode>
        //{
        //    public int F;
        //    public int G;
        //    public int Y;
        //    public int X;

        //    public int CompareTo(PQNode other)
        //    {
        //        if (F == other.F)
        //            return 0;
        //        return F < other.F ? 1 : -1;
        //    }
        //}


        //// U D L R
        //int[] _deltaY = new int[] { 1, -1, 0, 0 };
        //int[] _deltaX = new int[] { 0, 0, -1, 1 };
        //int[] _cost = new int[] { 10, 10, 10, 10 };

        //public List<Vector2Int> FindPath(Vector2Int startCellPos, Vector2Int destCellPos, bool checkObjects = true)
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
        //            if (next.Y != dest.Y || next.X != dest.X)
        //            {
        //                if (CanGo(Pos2Cell(next), checkObjects) == false) // CellPos
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

        //List<Vector2Int> CalcCellPathFromParent(Pos[,] parent, Pos dest)
        //{
        //    List<Vector2Int> cells = new List<Vector2Int>();

        //    int y = dest.Y;
        //    int x = dest.X;
        //    while (parent[y, x].Y != y || parent[y, x].X != x)
        //    {
        //        cells.Add(Pos2Cell(new Pos(y, x)));
        //        Pos pos = parent[y, x];
        //        y = pos.Y;
        //        x = pos.X;
        //    }
        //    if (!(x == 0 && y == 0))
        //        cells.Add(Pos2Cell(new Pos(y, x)));
        //    cells.Reverse();
        //    return cells;
        //}

        //Pos Cell2Pos(Vector2Int cell)
        //{
        //    // CellPos -> ArrayPos
        //    return new Pos(MaxY - cell.y, cell.x - MinX);
        //}

        //Vector2Int Pos2Cell(Pos pos)
        //{
        //    // ArrayPos -> CellPos
        //    return new Vector2Int(pos.X + MinX, MaxY - pos.Y);
        //}

        #endregion
    }

}
