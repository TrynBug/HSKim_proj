#pragma once

// 그리드, 그리기 영역 최대크기, 최소크기
constexpr int GRID_CELL_SIZE_MAX = 100;
constexpr int GRID_CELL_SIZE_MIN = 5;
constexpr int GRID_SIZE_MAX_X = 3000;
constexpr int GRID_SIZE_MAX_Y = 3000;
constexpr int GRID_SIZE_MIN_X = 100;
constexpr int GRID_SIZE_MIN_Y = 100;


// 그리드에서 WAY, WALL 을 나타내는 값
enum class EGridState : unsigned char
{
    WAY = 0,
    WALL = 1
};

struct RC
{
    int row;
    int col;
};

// 그리드 
class CGrid
{
public:
    CGrid(POINT ptStart, POINT ptSize, int cellSize);
    ~CGrid();

public:
    void ResizeGrid(int cellSize, int gridWidth, int gridHeight);
    void ResizeGrid(int gridWidth, int gridHeight);
    void ClearGrid();

public:
    void MakeMaze();
    int MakeCave();

public:
    EGridState GetGridState(int row, int col) const;
    POINT GetGridStartPos() const { return _ptStart; }
    POINT GetGridSize()     const { return _ptSize; }
    POINT GetGridSize_Old() const { return _ptSize_old; }
    RC GetStartRC()         const { return _rcStartPoint; }
    RC GetEndRC()           const { return _rcEndPoint; }
    int GetCellSize()       const { return _cellSize; }
    int GetCellSize_Old()   const { return _cellSize_old; }
    int GetNumRows()        const { return _numRows; }
    int GetNumRows_Old()    const { return _numRows_old; }
    int GetNumCols()        const { return _numCols; }
    int GetNumCols_Old()    const { return _numCols_old; }

    void SetGridState(int row, int col, EGridState state);
    void SetStartRC(int row, int col);
    void SetEndRC(int row, int col);
    void SetStartRCRandom();
    void SetEndRCRandom();
    void ClearStartRC();
    void ClearEndRC();

public:
    bool IsMouseOnGridArea(int mouseX, int mouseY) const;
    bool IsMouseOnLeftDownResizeObject(int mouseX, int mouseY) const;
    bool IsMouseOnRightDownResizeObject(int mouseX, int mouseY) const;
    bool IsMouseOnRightUpResizeObject(int mouseX, int mouseY) const;

public:
    const EGridState* const* _getGrid() const { return _grid; }

private:
    EGridState** _grid = nullptr;  // 그리드 2차원배열. 각 cell 마다의 EGridState 값을 가지고있음

    POINT _ptStart; // 그리드 영역 시작좌표 (x,y)
    POINT _ptSize;  // 그리드 영역 크기 (width, height)
    int _cellSize;  // cell 1개의 크기. 범위:[GRID_CELL_SIZE_MIN, GRID_CELL_SIZE_MAX]

    int _numRows;  // grid 행 수
    int _numCols;  // grid 열 수

    RC _rcStartPoint;  // 길찾기 시작점 (row, col)
    RC _rcEndPoint;    // 길찾기 목적지 (row, col)

    POINT _ptSize_old;
    int _cellSize_old;
    int _numRows_old;
    int _numCols_old;
};
