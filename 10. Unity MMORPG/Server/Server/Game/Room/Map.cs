using System;
using System.Collections.Generic;
using System.Diagnostics;
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
        GameRoom _room;
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

        // search algorithm
        JumpPointSearch _search = new JumpPointSearch();

        // etc
        Random _random = new Random();



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

        // 1개의 cell 내에서, cell의 중심을 기준으로 dir 방향의 좌표를 구한다.
        public Vector2 CellToDirectionPos(Vector2Int cell, MoveDir moveDir)
        {
            Vector2 pos = CellToCenterPos(cell);
            Vector2 dir = Util.GetDirectionVector(moveDir);
            float distance = 0;
            float distanceRate = 0.9f;
            switch (moveDir)
            {
                case MoveDir.Up:
                case MoveDir.Down:
                case MoveDir.Left:
                case MoveDir.Right:
                    distance = (float)Math.Sqrt((CellWidth / 2f) * (CellWidth / 2f) + (CellHeight / 2f) * (CellHeight / 2f));
                    break;
                case MoveDir.LeftDown:
                case MoveDir.RightUp:
                    distance = CellWidth / 2f;
                    break;
                case MoveDir.LeftUp:
                case MoveDir.RightDown:
                    distance = CellHeight / 2f;
                    break;
            }
            return pos + dir * (distance * distanceRate);
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
            return new Vector2Int(Math.Clamp(cell.x, 0, CellMaxX - 1), Math.Clamp(cell.y, 0, CellMaxY - 1));
        }

        // 비어있는 cell인지 확인
        // checkObjects : true일 경우 collider와 object 둘다 검사, false일 경우 collider만 검사함
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

        // cell이 비어있거나 내가 위치해있는지 확인함
        public bool IsEmptyCellOrMe(Vector2Int cell, GameObject me)
        {
            if (IsInvalidCell(cell))
                return false;
            if (_cells[cell.y, cell.x].Collider == true)
                return false;
            if (_cells[cell.y, cell.x].Object != null && _cells[cell.y, cell.x].Object != me)
                return false;
            return true;
        }
        public bool IsEmptyCellOrMe(Vector2 pos, GameObject me)
        {
            return IsEmptyCellOrMe(PosToCell(pos), me);
        }




        // 비어있는 랜덤 포지션 얻기 (텔레포트 위치 제외)
        public Vector2 GetRandomEmptyPos()
        {
            Vector2 pos;
            int loopCount = 0;
            while (true)
            {
                loopCount++;
                if(loopCount > 1000)
                {
                    Debug.Assert(loopCount < 1000);
                    return PosCenter;
                }    

                pos.x = PosMaxX * (float)_random.NextDouble();
                pos.y = PosMaxY * (float)_random.NextDouble();

                if (GetTeleportAtPos(pos) != null)
                    continue;

                if (IsEmptyCell(pos, checkObjects: true) == false)
                    continue;

                return pos;    
            }
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
            PosMaxX = CellMaxX * CellWidth - 0.001f; ;
            PosMaxY = CellMaxY * CellHeight - 0.001f; ;


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


            // search 알고리즘 초기화
            _search.Init(this, _cells, CellMaxX, CellMaxY);
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

        // pos 위치의 텔레포트 얻기
        public TeleportData GetTeleportAtPos(Vector2 pos)
        {
            foreach (TeleportData teleport in _mapData.teleports)
            {
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
        // checkCollider = false 일 경우 collider와 상관없이 이동시킨다. 이것은 Auto Moving을 할 때 사용한다.
        public bool TryMoving(GameObject obj, Vector2Int dest, bool checkCollider = true)
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
            if (checkCollider == true && destCell.Collider == true)
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
        public bool TryMoving(GameObject obj, Vector2 dest, bool checkCollider = true)
        {
            return TryMoving(obj, PosToCell(dest), checkCollider);
        }





        // object를 dest에 위치시킨다.
        // dest에 위치할 수 없을 경우 가장 가까운 cell에 위치시킨다.
        // 현재 cell과 dest의 위치가 같아도 오류가 없으면 true를 리턴한다.
        // 이동할 수 없을 경우 false를 리턴한다.
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

            // 주변의 비어있는 다른 위치를 찾는다
            Vector2Int emptyCell;
            if (destCell.Object == null)  // destCell에 collider만 있을 경우가 있음
                emptyCell = FindStopCell(obj, obj.Pos, targetPos, out stopPos);
            else
                emptyCell = FindStopCell(obj, targetPos, destCell.Object.Pos, out stopPos);

            // 비어있는 가장 가까운 cell로 이동
            if (isMovingobject)
                currCell.RemoveMovingObject(obj);
            else
                currCell.Object = null;
            _cells[emptyCell.y, emptyCell.x].Object = obj;
            return true;
        }


        // 내가 posMe 위치에 있고 posOther 위치에 있는 상대방과 부딪혔다고 할 때, 상대방과의 방향을 고려하여 posOther 위치 근처에 멈출 적절한 cell을 찾는다.
        // return : 정지할 cell
        // out param stopPos : 정지할 position
        struct _NearCell : IComparable<_NearCell>
        {
            public float distance;
            public Vector2Int cell;
            public int CompareTo(_NearCell other) { return this.distance < other.distance ? -1 : 1; }
        }
        List<_NearCell> _nearCells = new List<_NearCell>();
        public Vector2Int FindStopCell(GameObject me, Vector2 posMe, Vector2 posOther, out Vector2 stopPos)
        {
            // 상대방이 나를 바라볼때의 방향 계산
            MoveDir direction = Util.GetDirectionToDest(posOther, posMe);
            // 계산한 방향의 cell을 찾음
            Vector2Int cellOther = PosToCell(posOther);
            Vector2Int stopCell = Util.FindCellOfDirection(direction, cellOther);
            // cell이 비어있다면 성공
            if(IsEmptyCellOrMe(stopCell, me))
            {
                stopPos = CellToDirectionPos(stopCell, Util.GetOppositeDirection(direction));
                return stopCell;
            }

            // cell이 비어있지 않다면 주변 cell을 더 탐색한다. (우선 주변 +- 2 범위 만큼의 cell만 탐색함)
            for (int range = 1; range < 3; range++)
            {
                _nearCells.Clear();
                // 상대방이 위치한 cell 주변에서 비어있는 cell을 찾는다.
                // 그런다음 cell과 나와의 거리를 계산하여 _nearCells 에 저장한다.
                for (int y = cellOther.y - range; y < cellOther.y + range; y++)
                {
                    for (int x = cellOther.x - range; x < cellOther.x + range; x++)
                    {
                        Vector2Int cell = new Vector2Int(x, y);
                        if (IsEmptyCellOrMe(cell, me))
                        {
                            Vector2 posCenter = CellToCenterPos(cell);
                            float distance = (posMe - posCenter).squareMagnitude;
                            _nearCells.Add(new _NearCell { distance = distance, cell = cell });
                        }
                    }
                }

                if (_nearCells.Count == 0)
                    continue;
                
                // 비어있는 주변 cell을 거리가 가까운 순으로 정렬한다.
                _nearCells.Sort();
                // 거리가 가장 가까운 cell을 멈출 cell로 선택한다.
                stopCell = _nearCells[0].cell;
                MoveDir dirFromCell = Util.GetDirectionToDest(CellToCenterPos(stopCell), posMe);
                stopPos = CellToDirectionPos(stopCell, Util.GetOppositeDirection(dirFromCell));
                return stopCell;
            }

            // 아직 비어있는 cell을 찾지 못했다면 주변 아무 cell이나 비어있는 곳에 멈춘다.
            FindEmptyCell(cellOther, checkObjects: true, out stopCell);
            stopPos = CellToCenterPos(stopCell);
            return stopCell;
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


        // center를 기준으로 가장 가까운 살아있는 오브젝트를 찾는다.
        // 찾지 못했으면 null을 리턴함
        public GameObject FindAliveObjectNearbyCell(Vector2Int center, GameObject exceptObject = null)
        {
            center = GetValidCell(center);

            int loopCount = -1;
            while (true)
            {
                loopCount++;
                if (loopCount > 1000)
                {
                    Debug.Assert(loopCount < 1000);
                    return null;
                }

                // 현재 루프에서 찾을 범위 계산
                Vector2Int from = center - new Vector2Int(loopCount, loopCount);
                Vector2Int to = center + new Vector2Int(loopCount, loopCount);

                // 범위가 전체맵을 벗어나면 실패
                if (from.x < 0 && to.x >= CellMaxX && from.y < 0 && to.y >= CellMaxX)
                    return null;

                // 찾을 범위를 맵에 맞춤
                from = GetValidCell(from);
                to = GetValidCell(to);

                // 범위의 왼쪽위 -> 오른쪽위 검색
                for (int x = from.x; x <= to.x; x++)
                {
                    GameObject obj = _cells[from.y, x].GetObject();
                    if (obj != null && obj != exceptObject && obj.IsAlive)
                        return obj;
                }

                // 범위의 왼쪽, 오른쪽 검색
                for (int y = from.y + 1; y <= to.y - 1; y++)
                {
                    GameObject obj = _cells[y, from.x].GetObject();
                    if (obj != null && obj != exceptObject && obj.IsAlive)
                        return obj;
                    obj = _cells[y, to.x].GetObject();
                    if (obj != null && obj != exceptObject && obj.IsAlive)
                        return obj;
                }

                // 범위의 왼쪽아래 -> 오른쪽아래 검색
                for (int x = from.x; x <= to.x; x++)
                {
                    GameObject obj = _cells[to.y, x].GetObject();
                    if (obj != null && obj != exceptObject && obj.IsAlive)
                        return obj;
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






        // pos 위치를 기준으로 look 방향의 사각형 range 범위에 있는 살아있는 오브젝트를 찾아 리턴한다.
        public List<GameObject> FindObjectsInRect(Vector2 pos, Vector2 range, LookDir look, GameObject except)
        {
            List<GameObject> listObjects = new List<GameObject>();

            // 공격범위를 조사할 cell 찾기
            // 공격범위: pos 위치에서 look 방향으로 너비 range.x, 높이 range.y 사각형
            Vector2 targetPosMin;
            Vector2 targetPosMax;
            if (look == LookDir.LookLeft)
            {
                targetPosMin.x = pos.x + (Util.GetDirectionVector(MoveDir.Left) * range.x + Util.GetDirectionVector(MoveDir.Down) * range.y / 2f).x;
                targetPosMax.x = pos.x + (Util.GetDirectionVector(MoveDir.Up) * range.y / 2f).x;
                targetPosMin.y = pos.y + (Util.GetDirectionVector(MoveDir.Left) * range.x + Util.GetDirectionVector(MoveDir.Up) * range.y / 2f).y;
                targetPosMax.y = pos.y + (Util.GetDirectionVector(MoveDir.Down) * range.y / 2f).y;
            }
            else
            {
                targetPosMin.x = pos.x + (Util.GetDirectionVector(MoveDir.Down) * range.y / 2f).x;
                targetPosMax.x = pos.x + (Util.GetDirectionVector(MoveDir.Right) * range.x + Util.GetDirectionVector(MoveDir.Up) * range.y / 2f).x;
                targetPosMin.y = pos.y + (Util.GetDirectionVector(MoveDir.Up) * range.y / 2f).y;
                targetPosMax.y = pos.y + (Util.GetDirectionVector(MoveDir.Right) * range.x + Util.GetDirectionVector(MoveDir.Down) * range.y / 2f).y;
            }
            targetPosMin = GetValidPos(targetPosMin);
            targetPosMax = GetValidPos(targetPosMax);
            Vector2Int targetCellMin = PosToCell(targetPosMin);
            Vector2Int targetCellMax = PosToCell(targetPosMax);

            // 범위 내의 object 찾기
            for (int y = targetCellMin.y; y <= targetCellMax.y; y++)
            {
                for (int x = targetCellMin.x; x <= targetCellMax.x; x++)
                {
                    Cell cell = _cells[y, x];
                    if (cell.Object != null)
                    {
                        if (cell.Object == except)
                            continue;
                        if (cell.Object.IsAlive == false)
                            continue;
                        if (Util.IsTargetInRectRange(pos, look, range, cell.Object.Pos) == true)
                            listObjects.Add(cell.Object);
                    }
                    foreach (GameObject objMove in cell.MovingObjects)
                    {
                        if (objMove == except)
                            continue;
                        if (objMove.IsAlive == false)
                            continue;
                        if (Util.IsTargetInRectRange(pos, look, range, objMove.Pos) == true)
                            listObjects.Add(objMove);
                    }
                }
            }

            return listObjects;
        }


        // 길찾기
        public List<Vector2> SearchPath(Vector2 start, Vector2 end)
        {
            // 목적지까지 곧바로 갈 수 있다면 경로를 바로 return
            Vector2 intersection;
            if(CanGo(start, end, out intersection) == true)
            {
                return new List<Vector2>() { start, end };
            }

            // 경로 찾기
            Vector2Int startCell = PosToCell(start);
            Vector2Int endCell = PosToCell(end);
            List<Vector2> path = _search.SearchPath(startCell, endCell);
            if (path.Count > 0)
                path[0] = start;  // 경로 첫 좌표는 시작좌표로 교체한다.
            return path;
        }




        // ToString
        public override string ToString()
        {
            return $"map:{Id}";
        }

    }

}
