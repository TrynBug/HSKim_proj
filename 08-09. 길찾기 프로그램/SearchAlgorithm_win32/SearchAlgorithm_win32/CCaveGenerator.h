#pragma once

// cellular automata �˰������� cave�� �����.
// https://www.youtube.com/playlist?list=PLFt_AvWsXl0eZgMK_DT5_biRkWXftAOf9
struct Point
{
    int x;
    int y;

    Point(int x, int y)
        :x(x), y(y)
    {}
    Point()
        :x(0), y(0)
    {}
};

class CRoom
{
    friend class CCaveGenerator;

    std::vector<Point> _vecTiles;            // room�� ���Ե� tile
    std::vector<Point> _vecEdgeTiles;        // room�� �����ڸ� tile
    std::vector<CRoom*> _vecConnectedRooms;  // ����� room
    std::vector<Point> _vecConnectFrom;      // room ���� ������ǥ
    std::vector<Point> _vecConnectTo;        // room ���� ������ǥ
    bool _isAccessibleToMasterRoom;          // master room�� ���� ���� ����

    CRoom()
        : _isAccessibleToMasterRoom(false)
    {}
};

class CCaveGenerator
{
    char** _grid;
    int    _width;
    int    _height;

    std::vector<CRoom*> _vecRoom;

public:
    CCaveGenerator()
        :_grid(nullptr), _width(0), _height(0)
    {}

    ~CCaveGenerator()
    {
        for (size_t i = 0; i < _vecRoom.size(); i++)
        {
            delete _vecRoom[i];
        }
    }

public:
    // grid �� cave�� �����Ѵ�.
    void Generate(char** grid, int width, int height, float randomFillPercent, int smoothCount);

    // get
    int GetNumOfRooms() { return (int)_vecRoom.size(); }
    int GetNumOfWays();

private:
    enum eWall
    {
        WAY = 0,
        WALL = 1
    };

    // cellular automata �˰������� cave�� �����.
    void SmoothMap();
    // (x, y) ��ǥ �ֺ� 8�� ���⿡�� wall ���� ����Ѵ�.
    int GetSurroundingWallCount(int x, int y);

    // room�� �����Ѵ�.
    void GetRooms();
    // �ּҰŸ��� ������ 2�� room���� �����Ѵ�.
    void ConnectCloseRooms();
    // ����� room ���̿� ���� �����.
    void MakePathBetweenRooms();


    // room �ϳ��� ��ǥ room���� ���ϰ� ��� room�� ��ǥ room�� ������ �� �ִ����� �˻��Ѵ�.
    bool _inspectAccessibilityToMasterRoom();
    // ��ǥ room�� ������ �� ���� room �ϳ��� ��ǥ room�� ������ �� �ִ� room �� ���� ����� room�� �����Ѵ�.
    void _connectToMasterAccessibleRoom();
};



