#include <Windows.h>
#include <vector>

#include "CCaveGenerator.h"
#include "CBresenhamPath.h"


int CCaveGenerator::GetNumOfWays()
{
    int numOfWays = 0;
    for (size_t i = 0; i < _vecRoom.size(); i++)
    {
        numOfWays += (int)_vecRoom[i]->_vecTiles.size();
    }

    return numOfWays;
}


// grid �� cave�� �����Ѵ�.
void CCaveGenerator::Generate(char** grid, int width, int height, float randomFillPercent, int smoothCount)
{
    if (randomFillPercent < 0.f || randomFillPercent > 1.f)
        return;

    _grid = grid;
    _width = width;
    _height = height;

    // generate random wall
    int randomCriterion = (int)(32767.f * randomFillPercent);
    for (int h = 0; h < _height; h++)
    {
        for (int w = 0; w < _width; w++)
        {
            _grid[h][w] = rand() > randomCriterion ? WAY : WALL;
        }
    }

    // smooth map
    for (int i = 0; i < smoothCount; i++)
    {
        SmoothMap();
    }

    // room �� ã�´�.
    GetRooms();

    // ����� room�� �����Ѵ�.
    ConnectCloseRooms();

    // ����� room ���̿� ���� �����.
    MakePathBetweenRooms();

}


// cellular automata �˰������� cave�� �����.
void CCaveGenerator::SmoothMap()
{
    int neighbourWallCount = 0;
    for (int h = 0; h < _height; h++)
    {
        for (int w = 0; w < _width; w++)
        {
            // �ֺ� wall count ���
            neighbourWallCount = GetSurroundingWallCount(w, h);

            // smoothing
            if (neighbourWallCount > 4)
            {
                _grid[h][w] = WALL;
            }
            else if (neighbourWallCount < 4)
            {
                _grid[h][w] = WAY;
            }

        }
    }
}

// (x, y) ��ǥ �ֺ� 8�� ���⿡�� wall ���� ����Ѵ�.
int CCaveGenerator::GetSurroundingWallCount(int x, int y)
{
    int wallCount = 0;
    for (int neighbourY = y - 1; neighbourY <= y + 1; neighbourY++)
    {
        for (int neighbourX = x - 1; neighbourX <= x + 1; neighbourX++)
        {
            if (neighbourY < 0 || neighbourY >= _height || neighbourX < 0 || neighbourX >= _width)
            {
                wallCount++;
            }
            else if (neighbourX != x || neighbourY != y)
            {
                wallCount += _grid[neighbourY][neighbourX];
            }
        }
    }

    return wallCount;
}


// room�� �����Ѵ�.
void CCaveGenerator::GetRooms()
{
    // �湮���� �迭 ����
    char** arr2Visited = new char* [_height];
    arr2Visited[0] = new char[_height * _width];
    for (int i = 1; i < _height; i++)
    {
        arr2Visited[i] = arr2Visited[0] + (i * _width);
    }

    // wall�� �湮�Ѱ����� ����
    memcpy(arr2Visited[0], _grid[0], _height * _width);

    // room ã��
    std::vector<Point> vecNextTile;
    for (int y = 0; y < _height; y++)
    {
        for (int x = 0; x < _width; x++)
        {
            if (arr2Visited[y][x] == 1)
                continue;

            CRoom* pRoom = new CRoom;
            _vecRoom.push_back(pRoom);

            vecNextTile.clear();
            vecNextTile.push_back(Point(x, y));

            while (vecNextTile.size() != 0)
            {
                Point tileXY = vecNextTile.back();
                int tileX = tileXY.x;
                int tileY = tileXY.y;
                vecNextTile.pop_back();

                if (arr2Visited[tileY][tileX] == 0)
                {
                    arr2Visited[tileY][tileX] = 1;
                    pRoom->_vecTiles.push_back(tileXY);  // room�� tile list�� ��ǥ �߰�

                    if (tileY - 1 < 0 || tileY + 1 >= _height || tileX - 1 < 0 || tileX + 1 >= _width
                        || _grid[tileY - 1][tileX] == WALL || _grid[tileY + 1][tileX] == WALL
                        || _grid[tileY][tileX - 1] == WALL || _grid[tileY][tileX + 1] == WALL)
                    {
                        pRoom->_vecEdgeTiles.push_back(tileXY);  // room�� edge list�� edge��ǥ �߰�
                    }

                    // ���� ��ġ ���� ��, �Ʒ�, ����, ������ 4�� ������ tile�� Ž���Ѵ�.
                    if (tileY - 1 >= 0)
                        vecNextTile.push_back(Point(tileX, tileY - 1));
                    if (tileY + 1 < _height)
                        vecNextTile.push_back(Point(tileX, tileY + 1));
                    if (tileX - 1 >= 0)
                        vecNextTile.push_back(Point(tileX - 1, tileY));
                    if (tileX + 1 < _width)
                        vecNextTile.push_back(Point(tileX + 1, tileY));
                }
            }

        }
    }

    delete[] arr2Visited[0];
    delete[] arr2Visited;
}


// �ּҰŸ��� ������ 2�� room�� �����Ѵ�.
void CCaveGenerator::ConnectCloseRooms()
{
    int dist = 0;
    int minDist = INT_MAX;
    CRoom* pRoomA;
    CRoom* pRoomB;
    CRoom* pMinDistRoomA = nullptr;
    CRoom* pMinDistRoomB = nullptr;
    Point tileA;
    Point tileB;
    Point minDistTileA;
    Point minDistTileB;
    bool isConnectionFound = false;
    for (size_t roomA = 0; roomA < _vecRoom.size(); roomA++)
    {
        isConnectionFound = false;

        pRoomA = _vecRoom[roomA];

        // roomA �� ��� edge tile�� �ٸ� ��� room�� edge tile ������ �Ÿ��� ����Ͽ� �ּҰŸ��� ã�´�.
        for (size_t roomB = roomA + 1; roomB < _vecRoom.size(); roomB++)
        {
            pRoomB = _vecRoom[roomB];
            for (size_t tileIndexA = 0; tileIndexA < pRoomA->_vecEdgeTiles.size(); tileIndexA++)
            {
                tileA = pRoomA->_vecEdgeTiles[tileIndexA];
                for (size_t tileIndexB = 0; tileIndexB < pRoomB->_vecEdgeTiles.size(); tileIndexB++)
                {
                    tileB = pRoomB->_vecEdgeTiles[tileIndexB];
                    dist = abs(tileA.x - tileB.x) + abs(tileA.y - tileB.y);
                    if (dist < minDist)
                    {
                        minDist = dist;
                        pMinDistRoomA = pRoomA;
                        pMinDistRoomB = pRoomB;
                        minDistTileA = tileA;
                        minDistTileB = tileB;
                        isConnectionFound = true;
                    }

                }
            }
        }

        // �ּҰŸ��� ���� 2�� room�� ã�Ҵٸ� ������ connectedRoom ����Ʈ�� room�� ����Ѵ�.
        if (isConnectionFound)
        {
            pMinDistRoomA->_vecConnectedRooms.push_back(pMinDistRoomB);
            pMinDistRoomB->_vecConnectedRooms.push_back(pMinDistRoomA);

            pMinDistRoomA->_vecConnectFrom.push_back(minDistTileA);
            pMinDistRoomA->_vecConnectTo.push_back(minDistTileB);

            pMinDistRoomB->_vecConnectFrom.push_back(minDistTileB);
            pMinDistRoomB->_vecConnectTo.push_back(minDistTileA);

        }
    }


    // room �ϳ��� ��ǥ room���� ���ϰ� ��� room�� ��ǥ room�� ������ �� �ִ����� �˻��Ѵ�.
    // ��� room�� ��ǥ room�� ������ �� ���� ������ �ݺ��Ѵ�.
    while (_inspectAccessibilityToMasterRoom())
    {
        // ��ǥ room�� ������ �� ���� room �ϳ��� ��ǥ room�� ������ �� �ִ� room �� ���� ����� room�� �����Ѵ�.
        _connectToMasterAccessibleRoom();
    }

}


// room �ϳ��� ��ǥ room���� ���ϰ� ��� room�� ��ǥ room�� ������ �� �ִ����� �˻��Ѵ�.
bool CCaveGenerator::_inspectAccessibilityToMasterRoom()
{
    if (_vecRoom.size() == 0)
        return false;

    CRoom* pMasterRoom = _vecRoom[0];
    pMasterRoom->_isAccessibleToMasterRoom = true;

    CRoom* pRoom;
    CRoom* pPrevRoom;
    bool isFoundTargetRoom;
    bool isThereNonAccessibleRoom = false;
    std::vector<std::pair<CRoom*, CRoom*>> vecPath;
    for (size_t room = 1; room < _vecRoom.size(); room++)
    {
        if (_vecRoom[room]->_isAccessibleToMasterRoom == true)
            continue;

        isFoundTargetRoom = false;
        vecPath.clear();
        vecPath.push_back(std::make_pair(_vecRoom[room], _vecRoom[room]));

        while (vecPath.size() != 0)
        {
            pRoom = vecPath.back().first;
            pPrevRoom = vecPath.back().second;
            vecPath.pop_back();

            // master room�� ������
            if (pRoom == pMasterRoom)
            {
                isFoundTargetRoom = true;
                _vecRoom[room]->_isAccessibleToMasterRoom = true;
                break;
            }

            // ���� Ž���� room�� vector�� �Է���
            for (size_t i = 0; i < pRoom->_vecConnectedRooms.size(); i++)
            {
                if (pRoom->_vecConnectedRooms[i] != pPrevRoom)
                    vecPath.push_back(std::make_pair(pRoom->_vecConnectedRooms[i], pRoom));
            }
        }

        if (isFoundTargetRoom == false)
        {
            _vecRoom[room]->_isAccessibleToMasterRoom = false;
            isThereNonAccessibleRoom = true;
        }
    }

    return isThereNonAccessibleRoom;
}


// ��ǥ room�� ������ �� ���� room �ϳ��� ��ǥ room�� ������ �� �ִ� room �� ���� ����� room�� �����Ѵ�.
void CCaveGenerator::_connectToMasterAccessibleRoom()
{
    int dist = 0;
    int minDist = INT_MAX;
    CRoom* pRoomA;
    CRoom* pRoomB;
    CRoom* pMinDistRoomA = nullptr;
    CRoom* pMinDistRoomB = nullptr;
    Point tileA;
    Point tileB;
    Point minDistTileA;
    Point minDistTileB;
    bool isConnectionFound = false;
    for (size_t roomA = 0; roomA < _vecRoom.size(); roomA++)
    {
        // roomA �� ��ǥ room�� ������ �� ���� room�̴�.
        isConnectionFound = false;
        pRoomA = _vecRoom[roomA];
        if (pRoomA->_isAccessibleToMasterRoom == true)
            continue;

        for (size_t roomB = 0; roomB < _vecRoom.size(); roomB++)
        {
            // roomB �� ��ǥ room�� ������ �� �ִ� room�̴�.
            pRoomB = _vecRoom[roomB];
            if (pRoomB->_isAccessibleToMasterRoom == false)
                continue;

            // roomA�� ��� edge tile�� ��� roomB�� ��� edge tile ������ �Ÿ��� ����Ͽ� �ּҰŸ��� ã�´�.
            for (size_t tileIndexA = 0; tileIndexA < pRoomA->_vecEdgeTiles.size(); tileIndexA++)
            {
                tileA = pRoomA->_vecEdgeTiles[tileIndexA];
                for (size_t tileIndexB = 0; tileIndexB < pRoomB->_vecEdgeTiles.size(); tileIndexB++)
                {
                    tileB = pRoomB->_vecEdgeTiles[tileIndexB];
                    dist = abs(tileA.x - tileB.x) + abs(tileA.y - tileB.y);
                    if (dist < minDist)
                    {
                        minDist = dist;
                        pMinDistRoomA = pRoomA;
                        pMinDistRoomB = pRoomB;
                        minDistTileA = tileA;
                        minDistTileB = tileB;
                        isConnectionFound = true;
                    }

                }
            }
        }

        // �ּҰŸ��� ã�Ҵٸ� ������ connected room ����Ʈ�� room�� ����Ѵ�.
        if (isConnectionFound)
        {
            pMinDistRoomA->_isAccessibleToMasterRoom = true;

            pMinDistRoomA->_vecConnectedRooms.push_back(pMinDistRoomB);
            pMinDistRoomB->_vecConnectedRooms.push_back(pMinDistRoomA);

            pMinDistRoomA->_vecConnectFrom.push_back(minDistTileA);
            pMinDistRoomA->_vecConnectTo.push_back(minDistTileB);

            pMinDistRoomB->_vecConnectFrom.push_back(minDistTileB);
            pMinDistRoomB->_vecConnectTo.push_back(minDistTileA);

            // �ϳ��� ���������� �����Ѵ�.
            return;
        }
    }
}


// ����� room ���̿� ���� �����.
void CCaveGenerator::MakePathBetweenRooms()
{
    CRoom* pRoom;
    CRoom* pPrevRoom;
    std::vector<std::pair<CRoom*, CRoom*>> vecPath;
    CBresenhamPath BPath;

    for (size_t room = 0; room < _vecRoom.size(); room++)
    {
        vecPath.push_back(std::make_pair(_vecRoom[room], _vecRoom[room]));

        while (vecPath.size() != 0)
        {
            pRoom = vecPath.back().first;
            pPrevRoom = vecPath.back().second;
            vecPath.pop_back();

            for (size_t i = 0; i < pRoom->_vecConnectedRooms.size(); i++)
            {
                if (pRoom->_vecConnectedRooms[i] != pPrevRoom)
                {
                    // ���� Ž���� room ��� 
                    vecPath.push_back(std::make_pair(pRoom->_vecConnectedRooms[i], pRoom));

                    // room ���̿� ���� �׸���.
                    POINT ptStart = { pRoom->_vecConnectFrom[i].x,  pRoom->_vecConnectFrom[i].y };
                    POINT ptEnd = { pRoom->_vecConnectTo[i].x,  pRoom->_vecConnectTo[i].y };
                    BPath.SetParam(ptStart, ptEnd);
                    while (BPath.Next())
                    {
                        int tileX = BPath.GetX();
                        int tileY = BPath.GetY();
                        int r = 1;
                        for (int x = -r; x <= r; x++)
                        {
                            for (int y = -r; y <= r; y++)
                            {
                                if (x * x + y * y <= r * r)
                                {
                                    int drawX = tileX + x;
                                    int drawY = tileY + y;
                                    if (drawX < 0 || drawX >= _width || drawY < 0 || drawY >= _height)
                                        continue;

                                    _grid[drawY][drawX] = WAY;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}