#include <windows.h>

#include "CCaveGenerator.h"
#include "utils.h"
#include "CGrid.h"


CGrid::CGrid(POINT ptStart, POINT ptSize, int cellSize)
    : _ptStart(ptStart)
    , _ptSize{ 0, }
    , _cellSize(0)
    , _numRows(0)
    , _numCols(0)
    , _rcStartPoint{ 0, }
    , _rcEndPoint{ 0, }
    , _ptSize_old{ 0, }
    , _cellSize_old(0)
    , _numRows_old(0)
    , _numCols_old(0)
{
    ResizeGrid(cellSize, ptSize.x, ptSize.y);
    ClearGrid();
}

CGrid::~CGrid()
{
    if (_grid)
    {
        delete[] _grid[0];
        delete[] _grid;
    }
}

void CGrid::ResizeGrid(int cellSize, int gridWidth, int gridHeight)
{
    if (_cellSize == cellSize && _ptSize.x == gridWidth && _ptSize.y == gridHeight)
        return;

    _cellSize_old = _cellSize;
    _cellSize = min(max(cellSize, GRID_CELL_SIZE_MIN), GRID_CELL_SIZE_MAX);
    ResizeGrid(gridWidth, gridHeight);
}

void CGrid::ResizeGrid(int gridWidth, int gridHeight)
{
    if (_cellSize_old == _cellSize && _ptSize.x == gridWidth && _ptSize.y == gridHeight)
        return;

    // 그리드영역 크기 설정
    _ptSize_old.x = _ptSize.x;
    _ptSize_old.y = _ptSize.y;
    _ptSize.x = min(max(gridWidth, GRID_SIZE_MIN_X), GRID_SIZE_MAX_X);
    _ptSize.y = min(max(gridHeight, GRID_SIZE_MIN_Y), GRID_SIZE_MAX_Y);

    // 그리드영역 크기가 _cellSize 의 배수가 되도록 조정함
    int remainderX = _ptSize.x % _cellSize;
    if (remainderX < _cellSize / 2)
        _ptSize.x = _ptSize.x - remainderX;
    else
        _ptSize.x = _ptSize.x - remainderX + _cellSize;

    int remainderY = _ptSize.y % _cellSize;
    if (remainderY < _cellSize / 2)
        _ptSize.y = _ptSize.y - remainderY;
    else
        _ptSize.y = _ptSize.y - remainderY + _cellSize;

    // 그리드 행, 열 수 계산
    _numRows_old = _numRows;
    _numCols_old = _numCols;
    _numRows = _ptSize.y / _cellSize;
    _numCols = _ptSize.x / _cellSize;

    // 시작점, 도착점이 그리기영역을 벗어났을 경우 지움
    if (_rcStartPoint.col > _numCols - 1 || _rcStartPoint.row > _numRows - 1)
    {
        ClearStartRC();
    }
    if (_rcEndPoint.col > _numCols - 1 || _rcEndPoint.row > _numRows - 1)
    {
        ClearEndRC();
    }

    // grid를 다시 만들고 이전 내용을 복사한다.
        // new grid 만들기
    EGridState** gridNew = new EGridState * [_numRows];
    gridNew[0] = new EGridState[_numRows * _numCols];
    memset(gridNew[0], 0, _numRows * _numCols);
    for (int i = 1; i < _numRows; i++)
    {
        gridNew[i] = gridNew[0] + (i * _numCols);
    }

    // 이전 값 복사
    if (_grid)
    {
        for (int row = 0; row < _numRows; row++)
        {
            if (row >= _numRows_old)
                break;

            for (int col = 0; col < _numCols; col++)
            {
                if (col >= _numCols_old)
                    break;
                gridNew[row][col] = _grid[row][col];
            }
        }
    }
    // grid 교체
    if (_grid)
    {
        delete[] _grid[0];
        delete[] _grid;
    }
    _grid = gridNew;
}

// grid 내용을 모두 지운다.
void CGrid::ClearGrid()
{
    if (_grid)
        memset(_grid[0], 0, _numRows * _numCols);

    ClearStartRC();
    ClearEndRC();
}



void CGrid::MakeMaze()
{
    utils::MakeMaze(_numCols, _numRows, reinterpret_cast<char**>(_grid));
}

int CGrid::MakeCave()
{
    cavegen::CCaveGenerator cave;
    cave.Generate(reinterpret_cast<char**>(_grid), _numCols, _numRows, 0.45f, 3);
    return cave.GetNumOfWays();
}







EGridState CGrid::GetGridState(int row, int col) const
{
    if (!_grid)
        return EGridState::WALL;
    if (row < 0 || row >= _numRows || col < 0 || col >= _numCols)
        return EGridState::WALL;
    return _grid[row][col];
}

void CGrid::SetGridState(int row, int col, EGridState state)
{
    if (!_grid)
        return;
    if (row < 0 || row >= _numRows || col < 0 || col >= _numCols)
        return;
    _grid[row][col] = state;
}

void CGrid::SetStartRC(int row, int col)
{
    _rcStartPoint.row = min(max(row, 0), _numRows - 1);
    _rcStartPoint.col = min(max(col, 0), _numCols - 1);
}

void CGrid::SetEndRC(int row, int col)
{
    _rcEndPoint.row = min(max(row, 0), _numRows - 1);
    _rcEndPoint.col = min(max(col, 0), _numCols - 1);
}

void CGrid::SetStartRCRandom()
{
    while (true)
    {
        _rcStartPoint.row = rand() % _numRows;
        _rcStartPoint.col = rand() % _numCols;
        if (_grid[_rcStartPoint.row][_rcStartPoint.col] == EGridState::WAY
            && (_rcStartPoint.row != _rcEndPoint.row || _rcStartPoint.col != _rcEndPoint.col))
            break;
    }
}

void CGrid::SetEndRCRandom()
{
    while (true)
    {
        _rcEndPoint.row = rand() % _numRows;
        _rcEndPoint.col = rand() % _numCols;
        if (_grid[_rcEndPoint.row][_rcEndPoint.col] == EGridState::WAY
            && (_rcEndPoint.row != _rcStartPoint.row || _rcEndPoint.col != _rcStartPoint.col))
            break;
    }
}

void CGrid::ClearStartRC()
{
    _rcStartPoint.col = -1;
    _rcStartPoint.row = -1;
}

void CGrid::ClearEndRC()
{
    _rcEndPoint.col = -1;
    _rcEndPoint.row = -1;
}

bool CGrid::IsMouseOnGridArea(int mouseX, int mouseY) const
{
    return (mouseX >= _ptStart.x && mouseX < _ptStart.x + _ptSize.x
        && mouseY >= _ptStart.y && mouseY < _ptStart.y + _ptSize.y);
}

bool CGrid::IsMouseOnLeftDownResizeObject(int mouseX, int mouseY) const
{
    int left = _ptStart.x + _ptSize.x / 2 - 2;
    int down = _ptStart.y + _ptSize.y;
    return (mouseX >= left && mouseX < left + 5
        && mouseY >= down && mouseY < down + 5);
}
bool CGrid::IsMouseOnRightDownResizeObject(int mouseX, int mouseY) const
{
    int left = _ptStart.x + _ptSize.x;
    int down = _ptStart.y + _ptSize.y;
    return (mouseX >= left && mouseX < left + 5
        && mouseY >= down && mouseY < down + 5);
}
bool CGrid::IsMouseOnRightUpResizeObject(int mouseX, int mouseY) const
{
    int right = _ptStart.x + _ptSize.x;
    int up = _ptStart.y + _ptSize.y / 2 - 2;
    return (mouseX >= right && mouseX < right + 5
        && mouseY >= up && mouseY < up + 5);
}