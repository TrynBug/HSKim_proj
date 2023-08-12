#pragma once

// �׸���, �׸��� ���� �ִ�ũ��, �ּ�ũ��
constexpr int GRID_CELL_SIZE_MAX = 100;
constexpr int GRID_CELL_SIZE_MIN = 5;
constexpr int GRID_SIZE_MAX_X = 3000;
constexpr int GRID_SIZE_MAX_Y = 3000;
constexpr int GRID_SIZE_MIN_X = 100;
constexpr int GRID_SIZE_MIN_Y = 100;


// �׸��忡�� WAY, WALL �� ��Ÿ���� ��
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

// �׸��� 
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
    EGridState** _grid = nullptr;  // �׸��� 2�����迭. �� cell ������ EGridState ���� ����������

    POINT _ptStart; // �׸��� ���� ������ǥ (x,y)
    POINT _ptSize;  // �׸��� ���� ũ�� (width, height)
    int _cellSize;  // cell 1���� ũ��. ����:[GRID_CELL_SIZE_MIN, GRID_CELL_SIZE_MAX]

    int _numRows;  // grid �� ��
    int _numCols;  // grid �� ��

    RC _rcStartPoint;  // ��ã�� ������ (row, col)
    RC _rcEndPoint;    // ��ã�� ������ (row, col)

    POINT _ptSize_old;
    int _cellSize_old;
    int _numRows_old;
    int _numCols_old;
};
