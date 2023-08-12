#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Winmm.lib")

#include "CWinSearch.h"

using namespace search;




CWinSearch::CWinSearch(_In_ HINSTANCE hInst, _In_ int nCmdShow)
    : _hInst(hInst)
    , _nCmdShow(nCmdShow)
{



    _timer.id = 1;                   // ID
    _timer.frequency = 300;          // 주기
    _timer.frequencyIncrease = 100;  // 주기 증감치
    _timer.bResetTimer = false;     // 타이머 재설정 여부
    _timer.searchSpeedLevel = 0;     // 속도(값이 작으면 빠름, 음수 가능)

    _cursor.isChanged = false;
    _cursor.hDefault = 0;
    _cursor.hCurrent = 0;


}

CWinSearch::~CWinSearch()
{

}




void CWinSearch::GDI::Init()
{
    hBrushBackground = CreateSolidBrush(dfCOLOR_BACKGROUND);
    hPenGrid = CreatePen(PS_SOLID, 1, dfCOLOR_GRID);
    hBrushOpenNode = CreateSolidBrush(dfCOLOR_OPEN_NODE);
    hBrushCloseNode = CreateSolidBrush(dfCOLOR_CLOSE_NODE);
    hBrushStartPoint = CreateSolidBrush(dfCOLOR_START_POINT);
    hBrushEndPoint = CreateSolidBrush(dfCOLOR_END_POINT);
    hBrushWall = (HBRUSH)GetStockObject(dfCOLOR_WALL);
    hPenPath = CreatePen(PS_SOLID, 3, dfCOLOR_PATH_LINE);
    hPenSmoothPath = CreatePen(PS_SOLID, 3, dfCOLOR_SMOOTH_PATH_LINE);

    int r0 = GetRValue(dfCOLOR_BACKGROUND) - 28;   // 그림자 시작 색
    int g0 = GetGValue(dfCOLOR_BACKGROUND) - 25;
    int b0 = GetBValue(dfCOLOR_BACKGROUND) - 23;
    int r1 = GetRValue(dfCOLOR_BACKGROUND);        // 그림자 종료 색
    int g1 = GetGValue(dfCOLOR_BACKGROUND);
    int b1 = GetBValue(dfCOLOR_BACKGROUND);
    for (int i = 0; i < SHADOW_STEP; i++)
    {
        int r = r0 + ((r1 - r0) * i / SHADOW_STEP);
        int g = g0 + ((g1 - g0) * i / SHADOW_STEP);
        int b = b0 + ((b1 - b0) * i / SHADOW_STEP);

        arrHPenShadow[i] = CreatePen(PS_SOLID, 1, RGB(r, g, b));
    }

    hPenResizeObject = (HPEN)GetStockObject(BLACK_PEN);
    hBrushResizeObject = (HBRUSH)GetStockObject(WHITE_BRUSH);
    hPenResizeBorder = CreatePen(PS_DOT, 1, dfCOLOR_RESIZE_BORDER);
    hBrushResizeBorder = (HBRUSH)GetStockObject(HOLLOW_BRUSH);

    COLORREF arrRGB[NUM_OF_CELL_COLOR_GROUP];
    ggColorSlice(NUM_OF_CELL_COLOR_GROUP, arrRGB, 0.3);
    for (int i = 0; i < NUM_OF_CELL_COLOR_GROUP; i++)
    {
        arrHBrushCellColor[i] = CreateSolidBrush(arrRGB[i]);
    }

    hBrushBresenhamLine = CreateSolidBrush(dfCOLOR_BRESENHAM);

    hFontGridInfo = CreateFont(15, 0, 0, 0, 0
        , false, false, false
        , ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY
        , DEFAULT_PITCH | FF_DONTCARE
        , NULL);
}


void CWinSearch::Cursor::Init()
{
    hDefault = LoadCursor(nullptr, IDC_ARROW);
    hCurrent = hDefault;
    isChanged = false;
}



void CWinSearch::Painting::SetSize(int sizeGrid, int sizeX, int sizeY)
{
    gridSize = min(max(sizeGrid, GRID_SIZE_MIN), GRID_SIZE_MAX);
    SetSize(sizeX, sizeY);
}

void CWinSearch::Painting::SetSize(int sizeGrid, int sizeX, int sizeY)
{
    gridRows_old = gridRows;
    gridCols_old = gridCols;

    // 그리기영역 크기 설정
    ptSize.x = min(max(sizeX, PAINT_SIZE_MIN_X), PAINT_SIZE_MAX_X);
    ptSize.y = min(max(sizeY, PAINT_SIZE_MIN_Y), PAINT_SIZE_MAX_Y);

    // 그리기영역 크기가 gridSize 의 배수가 되도록 조정함
    int remainderX = ptSize.x % gridSize;
    if (remainderX < gridSize / 2)
        ptSize.x = ptSize.x - remainderX;
    else
        ptSize.x = ptSize.x - remainderX + gridSize;

    int remainderY = ptSize.y % gridSize;
    if (remainderY < gridSize / 2)
        ptSize.y = ptSize.y - remainderY;
    else
        ptSize.y = ptSize.y - remainderY + gridSize;

    // 그리드 행, 열 수 계산
    gridRows = ptSize.y / gridSize;
    gridCols = ptSize.x / gridSize;

    // 시작점, 도착점이 그리기영역을 벗어났을 경우 지움
    if (startCol > gridCols - 1 || startRow > gridRows - 1)
    {
        startCol = -1;
        startRow = -1;
    }
    if (endCol > gridCols - 1 || endRow > gridRows - 1)
    {
        endCol = -1;
        endRow = -1;
    }
}








int CWinSearch::Run()
{
    srand((unsigned int)time(0));

    // 커서 생성
    _cursor.Init();

    // GDI 생성
    _GDI.Init();

    // 창 클래스를 등록합니다.
    WNDCLASSEXW wcex = { 0, };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = _hInst;
    wcex.hIcon = LoadIcon(_hInst, MAKEINTRESOURCE(IDC_MY0608ASTARSEARCH));
    wcex.hCursor = _cursor.hDefault;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY0608ASTARSEARCH);
    wcex.lpszClassName = L"WinClient";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);


    // window 생성
    _hWnd = CreateWindowW(L"WinClient", L"Client", WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, _hInst, (LPVOID)this);
    if (!_hWnd)
    {
        std::string msg = std::string("CreateWindowEx Failed. system error:") + std::to_string(GetLastError()) + "\n";
        throw std::runtime_error(msg.c_str());
        return FALSE;
    }

    // Initialize common controls.
    INITCOMMONCONTROLSEX stInitCommonControls;
    stInitCommonControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    stInitCommonControls.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&stInitCommonControls);

    // toolbar 만들기.
    _hToolbar = CreateToolbar(_hInst, _hWnd);

    // 메인 윈도우의 device context를 얻는다.
    _hDC = GetDC(_hWnd);
    // 현재 윈도우 크기를 얻는다.
    GetClientRect(_hWnd, &_rtClientSize);

    // 이중 버퍼링 용도의 비트맵과 DC를 만든다.
    _ptMemBitSize.x = _rtClientSize.right;
    _ptMemBitSize.y = _rtClientSize.bottom;
    _hMemBit = CreateCompatibleBitmap(_hDC, _ptMemBitSize.x, _ptMemBitSize.y);
    _hMemDC = CreateCompatibleDC(_hDC);
    HBITMAP hOldBit = (HBITMAP)SelectObject(_hMemDC, _hMemBit);
    DeleteObject(hOldBit);

    // memDC에 wall 그리는 용도의 DC
    _hWallDC = CreateCompatibleDC(_hDC);
    _gridSize_old = -1;
    _gridCols_old = -1;

    // grid 초기화
    InitGrid();

    // Show Window
    ShowWindow(_hWnd, _nCmdShow);  // nCmdShow 는 윈도우를 최대화로 생성할 것인지, 최소화로 생성할 것인지 등을 지정하는 옵션이다.
    UpdateWindow(_hWnd);          // 윈도우를 한번 갱신시킨다.


#ifdef _DEBUG
    // DEBUG 모드일 경우 콘솔 생성
    //AllocConsole();
    //FILE* stream;
    //freopen_s(&stream, "CONIN$", "r", stdin);
    //freopen_s(&stream, "CONOUT$", "w", stdout);
    //freopen_s(&stream, "CONOUT$", "w", stderr);
#endif

    // 단축키 사용 지정
    HACCEL hAccelTable = LoadAccelerators(_hInst, MAKEINTRESOURCE(IDC_MY0608ASTARSEARCH));

    // main loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;

}


void CWinSearch::Close()
{

}





/* Toolbar :: toolbar 만들기 */
HWND CWinSearch::CreateToolbar(HINSTANCE hInstance, HWND hWnd)
{
    // Declare and initialize local constants
    HIMAGELIST hImageList = NULL;
    const int imageListID = 0;
    const int numButtons = 21;
    const int numCustonButtons = 16;
    const int bitmapSize = 16;
    const DWORD buttonStyles = BTNS_AUTOSIZE;

    // Create the toolbar.
    HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL
        , WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | CCS_TOP | BTNS_AUTOSIZE
        , 0, 0, 0, 0
        , hWnd
        , NULL //(HMENU)IDC_MAIN_TOOL
        , GetModuleHandle(NULL), NULL);
    SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(hWndToolbar, TB_SETEXTENDEDSTYLE, 0, (LPARAM)TBSTYLE_EX_HIDECLIPPEDBUTTONS);

    // set button size
    //SendMessage(hWndToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(32, 32));

    TBADDBITMAP tbab;
    tbab.hInst = HINST_COMMCTRL;
    tbab.nID = IDB_STD_SMALL_COLOR;
    SendMessage(hWndToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbab);


    // Create the image list.
    hImageList = ImageList_Create(bitmapSize, bitmapSize   // Dimensions of individual bitmaps.
        , ILC_COLOR16 | ILC_MASK  // Ensures transparent background.
        , numCustonButtons, 0);
    HBITMAP hBitSizeAdj = (HBITMAP)LoadImage(nullptr, L"toolbar_sizeAdj.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitWall = (HBITMAP)LoadImage(nullptr, L"toolbar_wall.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitStartpoint = (HBITMAP)LoadImage(nullptr, L"toolbar_startpoint.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitEndpoint = (HBITMAP)LoadImage(nullptr, L"toolbar_endpoint.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitBresenhamLine = (HBITMAP)LoadImage(nullptr, L"toolbar_bresenhamLine.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitAStarSearch = (HBITMAP)LoadImage(nullptr, L"toolbar_AStar.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitJumpPointSearch = (HBITMAP)LoadImage(nullptr, L"toolbar_JumpPoint.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitSearch = (HBITMAP)LoadImage(nullptr, L"toolbar_search.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitFValue = (HBITMAP)LoadImage(nullptr, L"toolbar_FValue.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitSpeedUp = (HBITMAP)LoadImage(nullptr, L"toolbar_speedUp.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitSpeedDown = (HBITMAP)LoadImage(nullptr, L"toolbar_speedDown.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitGoToEnd = (HBITMAP)LoadImage(nullptr, L"toolbar_goToEnd.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitMaze = (HBITMAP)LoadImage(nullptr, L"toolbar_maze.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitCave = (HBITMAP)LoadImage(nullptr, L"toolbar_cave.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitRandom = (HBITMAP)LoadImage(nullptr, L"toolbar_random.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    HBITMAP hBitTest = (HBITMAP)LoadImage(nullptr, L"toolbar_test.bmp", IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    ImageList_Add(hImageList, hBitSizeAdj, NULL);
    ImageList_Add(hImageList, hBitWall, NULL);
    ImageList_Add(hImageList, hBitStartpoint, NULL);
    ImageList_Add(hImageList, hBitEndpoint, NULL);
    ImageList_Add(hImageList, hBitBresenhamLine, NULL);
    ImageList_Add(hImageList, hBitAStarSearch, NULL);
    ImageList_Add(hImageList, hBitJumpPointSearch, NULL);
    ImageList_Add(hImageList, hBitSearch, NULL);
    ImageList_Add(hImageList, hBitFValue, NULL);
    ImageList_Add(hImageList, hBitSpeedUp, NULL);
    ImageList_Add(hImageList, hBitSpeedDown, NULL);
    ImageList_Add(hImageList, hBitGoToEnd, NULL);
    ImageList_Add(hImageList, hBitMaze, NULL);
    ImageList_Add(hImageList, hBitCave, NULL);
    ImageList_Add(hImageList, hBitRandom, NULL);
    ImageList_Add(hImageList, hBitTest, NULL);
    DeleteObject(hBitSizeAdj);
    DeleteObject(hBitWall);
    DeleteObject(hBitStartpoint);
    DeleteObject(hBitEndpoint);
    DeleteObject(hBitBresenhamLine);
    DeleteObject(hBitAStarSearch);
    DeleteObject(hBitJumpPointSearch);
    DeleteObject(hBitSearch);
    DeleteObject(hBitFValue);
    DeleteObject(hBitSpeedUp);
    DeleteObject(hBitSpeedDown);
    DeleteObject(hBitGoToEnd);
    DeleteObject(hBitMaze);
    DeleteObject(hBitCave);
    DeleteObject(hBitRandom);
    DeleteObject(hBitTest);

    // Set the image list.
    SendMessage(hWndToolbar, TB_SETIMAGELIST, (WPARAM)imageListID, (LPARAM)hImageList);
    // Load the button images.
    SendMessage(hWndToolbar, TB_LOADIMAGES, (WPARAM)IDB_STD_SMALL_COLOR, (LPARAM)HINST_COMMCTRL);
    // for tooltip
    SendMessage(hWndToolbar, TB_SETMAXTEXTROWS, (WPARAM)0, (LPARAM)0);
    // Initialize button info.
    TBBUTTON tbButtons[numButtons] =
    {
        { MAKELONG(STD_FILENEW + numCustonButtons, imageListID),  IDM_TOOLBAR_NEW,  TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"New"},
        { MAKELONG(0, imageListID), IDM_TOOLBAR_SIZE_ADJ,   TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Resize"},
        { 10, 0, 0, BTNS_SEP, {0}, 0, 0},
        { MAKELONG(1, imageListID), IDM_TOOLBAR_WALL,       TBSTATE_ENABLED | TBSTATE_CHECKED, BTNS_AUTOSIZE | BTNS_CHECKGROUP, {0}, 0, (INT_PTR)L"Wall"},
        { MAKELONG(2, imageListID), IDM_TOOLBAR_STARTPOINT, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_CHECKGROUP, {0}, 0, (INT_PTR)L"Start Point"},
        { MAKELONG(3, imageListID), IDM_TOOLBAR_ENDPOINT,   TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_CHECKGROUP, {0}, 0, (INT_PTR)L"End Point"},
        { MAKELONG(4, imageListID), IDM_TOOLBAR_BRESENHAM_LINE,   TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_CHECKGROUP, {0}, 0, (INT_PTR)L"Bresenham Line"},
        { 10, 0, 0, BTNS_SEP, {0}, 0, 0},
        { MAKELONG(5, imageListID), IDM_TOOLBAR_ASTAR_SEARCH, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_CHECKGROUP, {0}, 0, (INT_PTR)L"A* Search"},
        { MAKELONG(6, imageListID), IDM_TOOLBAR_JUMP_POINT_SEARCH,   TBSTATE_ENABLED | TBSTATE_CHECKED, BTNS_AUTOSIZE | BTNS_CHECKGROUP, {0}, 0, (INT_PTR)L"Jump Point Search"},
        { 10, 0, 0, BTNS_SEP, {0}, 0, 0},
        { MAKELONG(7, imageListID), IDM_TOOLBAR_SEARCH,     TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Search"},
        { MAKELONG(8, imageListID), IDM_TOOLBAR_FVALUE,     TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_CHECK, {0}, 0, (INT_PTR)L"Show F-value"},
        { MAKELONG(9, imageListID), IDM_TOOLBAR_SPEEDUP,    TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Speed Up"},
        { MAKELONG(10, imageListID), IDM_TOOLBAR_SPEEDDOWN,  TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Speed Down"},
        { MAKELONG(11, imageListID), IDM_TOOLBAR_GOTOEND,   TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Go to End"},
        { 10, 0, 0, BTNS_SEP, {0}, 0, 0},
        { MAKELONG(12, imageListID), IDM_TOOLBAR_MAKE_MAZE, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Make Maze"},
        { MAKELONG(13, imageListID), IDM_TOOLBAR_MAKE_CAVE, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Make Cave"},
        { MAKELONG(14, imageListID), IDM_TOOLBAR_MAKE_RANDOM, TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Make Random"},
        { MAKELONG(15, imageListID), IDM_TOOLBAR_RUN_TEST,  TBSTATE_ENABLED, BTNS_AUTOSIZE | TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"Run Test"},
    };
    // Add buttons.
    SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)numButtons, (LPARAM)&tbButtons);
    // show toolbar
    SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);

    return hWndToolbar;
}









// Utils :: memory bitmap 크기를 조정한다.
void CWinSearch::ResizeWindow()
{
    // 그리기영역 전체를 포함할 수 있는 크기로 만든다.
    GetClientRect(_hWnd, &_rtClientSize);
    int newBitSizeX = max(_rtClientSize.right, _painting.ptSize.x + _painting.ptPos.x + 50);
    int newBitSizeY = max(_rtClientSize.bottom, _painting.ptSize.y + _painting.ptPos.y + _toolbar.height + 50);

    // 만약 새로운 크기가 이전 크기와 같다면 종료
    if (newBitSizeX == _ptMemBitSize.x && newBitSizeY == _ptMemBitSize.y)
        return;

    // 비트맵 재생성
    _ptMemBitSize.x = newBitSizeX;
    _ptMemBitSize.y = newBitSizeY;
    HBITMAP hNewBit = CreateCompatibleBitmap(_hMemDC, _ptMemBitSize.x, _ptMemBitSize.y);
    SelectObject(_hMemDC, hNewBit);
    DeleteObject(_hMemBit);
    _hMemBit = hNewBit;

}

// Utils :: grid를 초기화한다.
void CWinSearch::InitGrid()
{
    if (_arr2Grid)
    {
        delete[] _arr2Grid[0];
        delete[] _arr2Grid;
    }

    _arr2Grid = new char* [_painting.gridRows];
    _arr2Grid[0] = new char[_painting.gridRows * _painting.gridCols];
    memset(_arr2Grid[0], 0, _painting.gridRows * _painting.gridCols);
    for (int i = 1; i < _painting.gridRows; i++)
    {
        _arr2Grid[i] = _arr2Grid[0] + (i * _painting.gridCols);
    }

    _painting.startCol = -1;
    _painting.startRow = -1;
    _painting.endCol = -1;
    _painting.endRow = -1;
}

// Utils :: grid 내용을 모두 지운다.
void CWinSearch::ClearGrid()
{
    memset(_arr2Grid[0], 0, _painting.gridRows * _painting.gridCols);

    _painting.startCol = -1;
    _painting.startRow = -1;
    _painting.endCol = -1;
    _painting.endRow = -1;
}

// Utils :: grid를 다시 만들고 이전 내용을 복사한다.
void CWinSearch::ResizeGrid()
{
    // new grid 만들기
    char** gridNew = new char* [_painting.gridRows];
    gridNew[0] = new char[_painting.gridRows * _painting.gridCols];
    memset(gridNew[0], 0, _painting.gridRows * _painting.gridCols);
    for (int i = 1; i < _painting.gridRows; i++)
    {
        gridNew[i] = gridNew[0] + (i * _painting.gridCols);
    }

    // 이전 값 복사
    for (int row = 0; row < _painting.gridRows; row++)
    {
        if (row >= _painting.gridRows_old)
            break;

        for (int col = 0; col < _painting.gridCols; col++)
        {
            if (col >= _painting.gridCols_old)
                break;
            gridNew[row][col] = _arr2Grid[row][col];
        }
    }

    delete[] _arr2Grid[0];
    delete[] _arr2Grid;
    _arr2Grid = gridNew;
}

// Utils :: 스크롤을 재조정한다.
void CWinSearch::ReadjustScroll()
{
    GetClientRect(_hWnd, &_rtClientSize);
    // 수평 스크롤링 범위는 (비트맵너비 - 클라이언트너비) 로 정해진다.
    // 현재 수평 스크롤 위치는 수평 스크롤링 범위 내에 유지된다.
    _scroll.hMax = max(_ptMemBitSize.x - _rtClientSize.right, 0);
    _scroll.hPos = min(_scroll.hPos, _scroll.hMax);
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = _ptMemBitSize.x;
    si.nPage = _rtClientSize.right;
    si.nPos = _scroll.hPos;
    SetScrollInfo(_hWnd, SB_HORZ, &si, TRUE);

    // 수직 스크롤링 범위는 (비트맵높이 - 클라이언트높이) 로 정해진다.
    // 현재 수직 스크롤 위치는 수직 스크롤링 범위 내에 유지된다.
    _scroll.vMax = max(_ptMemBitSize.y - _rtClientSize.bottom, 0);
    _scroll.vPos = min(_scroll.vPos, _scroll.vMax);
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = _ptMemBitSize.y;
    si.nPage = _rtClientSize.bottom;
    si.nPos = _scroll.vPos;
    SetScrollInfo(_hWnd, SB_VERT, &si, TRUE);
}


// Utils :: 테스트 중단 키보드 입력 감지 스레드
unsigned WINAPI CWinSearch::ThreadProcQuitRunTest(PVOID vParam)
{
    CWinSearch* pThis = reinterpret_cast<CWinSearch*>(vParam);
    while (1)
    {
        if (pThis->_test.isRunning == false)
            return 0;

        int ch = _getch();
        if (ch == 'q')
        {
            pThis->_test.isRunning = false;
            return 0;
        }
        else if (ch == 'r')
        {
            pThis->_test.isDoRendering = pThis->_test.isDoRendering ? false : true;
        }
        Sleep(50);
    }
}







/* Message Processing */
// 마우스 왼쪽 클릭 시
void CWinSearch::MsgProc_LBUTTONDOWN(WPARAM wParam, LPARAM lParam)
{
    int xPos = mcGET_MOUSE_X_LPARAM(lParam);
    int yPos = mcGET_MOUSE_Y_LPARAM(lParam);
    _mouse.clickX = xPos;
    _mouse.clickY = yPos;

#ifdef _DEBUG
    printf("WM_LBUTTONDOWN  xPos %d, yPos %d, _scroll.hPos %d, _scroll.vPos %d\n", xPos, yPos, _scroll.hPos, _scroll.vPos);
#endif
    // 현재 마우스 좌표가 그리기영역 위일 경우 click 판정
    if (xPos >= _painting.ptPos.x && xPos < _painting.ptPos.x + _painting.ptSize.x
        && yPos >= _painting.ptPos.y && yPos < _painting.ptPos.y + _painting.ptSize.y)
    {
        _mouse.clickPainting = true;
        int row = mcMOUSE_Y_TO_GRID_ROW(yPos);
        int col = mcMOUSE_X_TO_GRID_COL(xPos);

        // 현재 선택된 toolbar 버튼이 Wall 일 경우 grid 상태를 바꿈
        if (_toolbar.btnTool == IDM_TOOLBAR_WALL)
        {
            if (_arr2Grid[row][col] == GRID_WALL)
            {
                _mouse.erase = true;
                _arr2Grid[row][col] = GRID_WAY;
            }
            else
            {
                _mouse.erase = false;
                _arr2Grid[row][col] = GRID_WALL;
            }
        }

        // 현재 선택된 toolbar 버튼이 Start Point 일 경우 start point 좌표를 설정함
        else if (_toolbar.btnTool == IDM_TOOLBAR_STARTPOINT)
        {
            if (row == _painting.startRow && col == _painting.startCol)
            {
                _painting.startRow = -1;
                _painting.startCol = -1;
            }
            else
            {
                _painting.startRow = row;
                _painting.startCol = col;
            }
        }

        // 현재 선택된 toolbar 버튼이 End Point 일 경우 end point 좌표를 설정함
        else if (_toolbar.btnTool == IDM_TOOLBAR_ENDPOINT)
        {

            if (row == _painting.endRow && col == _painting.endCol)
            {
                _painting.endRow = -1;
                _painting.endCol = -1;
            }
            else
            {
                _painting.endRow = row;
                _painting.endCol = col;
            }
        }

        InvalidateRect(_hWnd, NULL, FALSE);
        return;
    }


    int resizeObjectLeft[3] = { _painting.ptPos.x + _painting.ptSize.x / 2 - 2, _painting.ptPos.x + _painting.ptSize.x, _painting.ptPos.x + _painting.ptSize.x };
    int resizeObjectTop[3] = { _painting.ptPos.y + _painting.ptSize.y, _painting.ptPos.y + _painting.ptSize.y, _painting.ptPos.y + _painting.ptSize.y / 2 - 2 };
    // 마우스가 중간아래 그리기영역 크기조정 object 위에 있을 경우 해당 object 클릭 체크
    if (xPos >= resizeObjectLeft[0] && xPos < resizeObjectLeft[0] + 5
        && yPos >= resizeObjectTop[0] && yPos < resizeObjectTop[0] + 5)
    {
        _mouse.clickResizeVertical = true;
    }
    // 마우스가 오른쪽아래 그리기영역 크기조정 object 위에 있을 경우 해당 object 클릭 체크
    else if (xPos >= resizeObjectLeft[1] && xPos < resizeObjectLeft[1] + 5
        && yPos >= resizeObjectTop[1] && yPos < resizeObjectTop[1] + 5)
    {
        _mouse.clickResizeDiagonal = true;
    }
    // 마우스가 오른쪽중간 그리기영역 크기조정 object 위에 있을 경우 해당 object 클릭 체크
    else if (xPos >= resizeObjectLeft[2] && xPos < resizeObjectLeft[2] + 5
        && yPos >= resizeObjectTop[2] && yPos < resizeObjectTop[2] + 5)
    {
        _mouse.clickResizeHorizontal = true;
    }
}


// 마우스 왼쪽버튼을 뗐을 때
void CWinSearch::MsgProc_LBUTTONUP(WPARAM wParam, LPARAM lParam)
{
    _mouse.clickPainting = false;

    // 그리기영역 크기조정 object를 클릭 중일 때 마우스를 뗐으면 memory bitmap을 크기에 맞게 다시 만든다.
    if (_mouse.clickResizeVertical || _mouse.clickResizeDiagonal || _mouse.clickResizeHorizontal)
    {
        // 마우스 좌표와 그리기영역 크기를 계산한다.
        int xPos = mcGET_MOUSE_X_LPARAM(lParam);
        int yPos = mcGET_MOUSE_Y_LPARAM(lParam);
        int newSizeX = min(max(xPos - _painting.ptPos.x, PAINT_SIZE_MIN_X), PAINT_SIZE_MAX_X);
        int newSizeY = min(max(yPos - _painting.ptPos.y, PAINT_SIZE_MIN_Y), PAINT_SIZE_MAX_Y);
        if (_mouse.clickResizeVertical)
            newSizeX = _painting.ptSize.x;
        if (_mouse.clickResizeHorizontal)
            newSizeY = _painting.ptSize.y;

        // _painting 에 크기 저장
        _painting.SetSize(newSizeX, newSizeY);
        ResizeGrid(); // 그리드 재생성

        // bitmap 다시 만들기
        ResizeWindow();
        ReadjustScroll();
        InvalidateRect(_hWnd, NULL, FALSE);
    }
    _mouse.clickResizeVertical = false;
    _mouse.clickResizeHorizontal = false;
    _mouse.clickResizeDiagonal = false;



}


// 마우스를 움직일 때
void CWinSearch::MsgProc_MOUSEMOVE(WPARAM wParam, LPARAM lParam)
{
    _cursor.isChanged = false;
    _cursor.hCurrent = _cursor.hDefault;

    int xPos = mcGET_MOUSE_X_LPARAM(lParam);
    int yPos = mcGET_MOUSE_Y_LPARAM(lParam);
    _mouse.x = xPos;
    _mouse.y = yPos;

    // 마우스가 그리기영역에 있을 경우
    if (xPos >= _painting.ptPos.x && xPos < _painting.ptPos.x + _painting.ptSize.x
        && yPos >= _painting.ptPos.y && yPos < _painting.ptPos.y + _painting.ptSize.y)
    {
        // 클릭 중일 경우
        if (_mouse.clickPainting)
        {
            // 현재 toolbar의 wall 버튼이 눌려져 있을 경우
            if (_toolbar.btnTool == IDM_TOOLBAR_WALL)
            {
                int row = mcMOUSE_Y_TO_GRID_ROW(yPos);
                int col = mcMOUSE_X_TO_GRID_COL(xPos);
                // grid의 상태를 바꾼다.
                if (_mouse.erase == true)
                    _arr2Grid[row][col] = GRID_WAY;
                else
                    _arr2Grid[row][col] = GRID_WALL;

                InvalidateRect(_hWnd, NULL, FALSE);
            }
            // 현재 toolbar의 Bresenham Line 버튼이 눌려져 있을 경우 WM_PAIN 에서 선을 그림
            if (_toolbar.btnTool == IDM_TOOLBAR_BRESENHAM_LINE)
            {
                InvalidateRect(_hWnd, NULL, FALSE);
            }
        }
    }


    int resizeObjectLeft[3] = { _painting.ptPos.x + _painting.ptSize.x / 2 - 2, _painting.ptPos.x + _painting.ptSize.x, _painting.ptPos.x + _painting.ptSize.x };
    int resizeObjectTop[3] = { _painting.ptPos.y + _painting.ptSize.y, _painting.ptPos.y + _painting.ptSize.y, _painting.ptPos.y + _painting.ptSize.y / 2 - 2 };
    // 마우스가 중간아래 그리기영역 크기조정 object 위에 있거나, 마우스를 누르고 있을 경우
    if (xPos >= resizeObjectLeft[0] && xPos < resizeObjectLeft[0] + 5
        && yPos >= resizeObjectTop[0] && yPos < resizeObjectTop[0] + 5
        || _mouse.clickResizeVertical == true)
    {
        _cursor.hCurrent = LoadCursor(nullptr, IDC_SIZENS);  // 커서 변경
        _cursor.isChanged = true;
        if (_mouse.clickResizeVertical == true)                    // 마우스를 클릭한채로 움직이면 화면 갱신
            InvalidateRect(_hWnd, NULL, FALSE);
    }
    // 마우스가 오른쪽아래 그리기영역 크기조정 object 위에 있을 경우
    else if (xPos >= resizeObjectLeft[1] && xPos < resizeObjectLeft[1] + 5
        && yPos >= resizeObjectTop[1] && yPos < resizeObjectTop[1] + 5
        || _mouse.clickResizeDiagonal == true)
    {
        _cursor.hCurrent = LoadCursor(nullptr, IDC_SIZENWSE);  // 커서 변경
        _cursor.isChanged = true;
        if (_mouse.clickResizeDiagonal == true)                      // 마우스를 클릭한채로 움직이면 화면 갱신
            InvalidateRect(_hWnd, NULL, FALSE);
    }
    // 마우스가 오른쪽중간 그리기영역 크기조정 object 위에 있을 경우
    else if (xPos >= resizeObjectLeft[2] && xPos < resizeObjectLeft[2] + 5
        && yPos >= resizeObjectTop[2] && yPos < resizeObjectTop[2] + 5
        || _mouse.clickResizeHorizontal == true)
    {
        _cursor.hCurrent = LoadCursor(nullptr, IDC_SIZEWE);  // 커서 변경
        _cursor.isChanged = true;
        if (_mouse.clickResizeHorizontal == true)                 // 마우스를 클릭한채로 움직이면 화면 갱신
            InvalidateRect(_hWnd, NULL, FALSE);
    }
}


// 마우스 휠을 굴릴 때
void CWinSearch::MsgProc_MOUSEWHEEL(WPARAM wParam, LPARAM lParam)
{
    int fwKeys = GET_KEYSTATE_WPARAM(wParam);
    int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    // 현재 Ctrl 키가 눌려져 있다면 grid 크기조정
    if (fwKeys & MK_CONTROL)
    {
        // 마우스 휠을 위로 굴렸을 때 grid 크기 + 1
        if (zDelta > 0)
        {
            // grid 크기 + 1
            int newGridSize = _painting.gridSize + 1;
            if (newGridSize > GRID_SIZE_MAX)
                return;

            int newSizeX = newGridSize * _painting.gridCols;
            int newSizeY = newGridSize * _painting.gridRows;
            _painting.SetSize(newGridSize, newSizeX, newSizeY);
            ResizeGrid(); // 그리드 재생성

            // bitmap 다시 만들기, 스크롤바 조정
            ResizeWindow();
            ReadjustScroll();
            InvalidateRect(_hWnd, NULL, FALSE);
        }
        // 마우스 휠을 아래로 굴렸을 때 grid 크기 - 1
        else if (zDelta < 0)
        {
            // grid 크기 - 1
            int newGridSize = _painting.gridSize - 1;
            if (newGridSize < GRID_SIZE_MIN)
                return;

            int newSizeX = newGridSize * _painting.gridCols;
            int newSizeY = newGridSize * _painting.gridRows;
            _painting.SetSize(newGridSize, newSizeX, newSizeY);
            ResizeGrid(); // 그리드 재생성

            // bitmap 다시 만들기, 스크롤바 조정
            ResizeWindow();
            ReadjustScroll();
            InvalidateRect(_hWnd, NULL, FALSE);
        }
    }

    // 현재 Ctrl 키가 눌려져 있지 않다면 수직 스크롤링
    else
    {
        // 마우스 휠을 위로 굴렸을 때 위로 스크롤링
        if (zDelta > 0)
        {
            PostMessage(_hWnd, WM_VSCROLL, (WPARAM)MAKELONG(SB_LINEUP, 0), (LPARAM)NULL);
        }
        // 마우스 휠을 위로 굴렸을 때 아래로 스크롤링
        else if (zDelta < 0)
        {
            PostMessage(_hWnd, WM_VSCROLL, (WPARAM)MAKELONG(SB_LINEDOWN, 0), (LPARAM)NULL);
        }
    }
}


// 창의 크기가 바뀔 때
void CWinSearch::MsgProc_SIZE(WPARAM wParam, LPARAM lParam)
{
    // mem bitmap, 스크롤, toolbar를 화면 크기에 맞게 다시 만든다.
    ResizeWindow();
    ReadjustScroll();


    // invalid rect
    InvalidateRect(_hWnd, NULL, FALSE);
    SendMessage(_hToolbar, TB_AUTOSIZE, 0, 0);

}


// 커서 교체 (마우스를 움직일 때마다 시행됨)
LRESULT CWinSearch::MsgProc_SETCURSOR(UINT message, WPARAM wParam, LPARAM lParam)
{
    // 커서가 변경되어야 할 때만 custom cursor로 변경함
    if (_cursor.isChanged)
    {
        SetCursor(_cursor.hCurrent);
        return 0;
    }
    // 그렇지 않으면 윈도우 기본 동작에 맡김
    else
    {
        return DefWindowProc(_hWnd, message, wParam, lParam);
    }
}


// menu 또는 toolbar 클릭
LRESULT CWinSearch::MsgProc_COMMAND(UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);

    switch (wmId)
    {
    case IDM_EXIT:
        DestroyWindow(_hWnd);
        break;

        /* toolbar :: 새로그리기 버튼 클릭 */
    case IDM_TOOLBAR_NEW:
        ClearGrid();
        _AStarSearch.Clear();
        _jumpPointSearch.Clear();
        InvalidateRect(_hWnd, NULL, FALSE);
        break;

        /* toolbar :: 크기조정 버튼 클릭 */
    case IDM_TOOLBAR_SIZE_ADJ:
    {
        // IDD_SIZE_ADJ 다이얼로그를 띄운다.
        // IDD_SIZE_ADJ 다이얼로그 에서는 _painting 구조체의 값을 변경한다.
        bool isOk = (bool)DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SIZE_ADJ), _hWnd, WndProcDialogSizeAdj, (LPARAM)this);
        // 다이얼로그에서 OK를 눌렀을 경우 memory bitmap과 grid를 새로운 크기로 다시 만든다.
        if (isOk)
        {
            ResizeGrid();
            ResizeWindow();
            ReadjustScroll();
            InvalidateRect(_hWnd, NULL, FALSE);
        }
    }
    break;

    /* toolbar :: Wall 그리기 버튼 클릭 */
    case IDM_TOOLBAR_WALL:
        _toolbar.btnTool = IDM_TOOLBAR_WALL;
        break;
        /* toolbar :: StartPoint 그리기 버튼 클릭 */
    case IDM_TOOLBAR_STARTPOINT:
        _toolbar.btnTool = IDM_TOOLBAR_STARTPOINT;
        break;
        /* toolbar :: EndPoint 그리기 버튼 클릭 */
    case IDM_TOOLBAR_ENDPOINT:
        _toolbar.btnTool = IDM_TOOLBAR_ENDPOINT;
        break;
        /* toolbar :: Bresenham Line 그리기 버튼 클릭 */
    case IDM_TOOLBAR_BRESENHAM_LINE:
        _toolbar.btnTool = IDM_TOOLBAR_BRESENHAM_LINE;
        break;


        /* toolbar :: A* Search 버튼 클릭 */
    case IDM_TOOLBAR_ASTAR_SEARCH:
        _timer.frequency = 55;
        _timer.frequencyIncrease = 20;
        _toolbar.btnAlgorithm = IDM_TOOLBAR_ASTAR_SEARCH;
        break;
        /* toolbar :: Jump Point Search 버튼 클릭 */
    case IDM_TOOLBAR_JUMP_POINT_SEARCH:
        _timer.frequency = 300;
        _timer.frequencyIncrease = 100;
        _toolbar.btnAlgorithm = IDM_TOOLBAR_JUMP_POINT_SEARCH;
        break;


        /* toolbar :: Search 버튼 클릭 */
    case IDM_TOOLBAR_SEARCH:
    {
        // 시작점, 도착점 존재 여부 체크
        if (_painting.startCol < 0
            || _painting.startRow < 0
            || _painting.startCol >= _painting.gridCols
            || _painting.startRow >= _painting.gridRows
            || _painting.endCol < 0
            || _painting.endRow < 0
            || _painting.endCol >= _painting.gridCols
            || _painting.endRow >= _painting.gridRows)
        {
            break;
        }

        if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
        {
            // A* search 파라미터 설정
            _AStarSearch.SetParam(_painting.startRow, _painting.startCol
                , _painting.endRow, _painting.endCol
                , _arr2Grid
                , _painting.gridRows
                , _painting.gridCols
                , _painting.gridSize);

            // step by step 길찾기 시작
            _AStarSearch.SearchStepByStep();
        }
        else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
        {
            // jump point search 파라미터 설정
            _jumpPointSearch.SetParam(_painting.startRow, _painting.startCol
                , _painting.endRow, _painting.endCol
                , _arr2Grid
                , _painting.gridRows
                , _painting.gridCols);

            // step by step 길찾기 시작
            _jumpPointSearch.SearchStepByStep();
        }

        // 타이머 설정
        SetTimer(_hWnd, _timer.id, max(0, _timer.frequency + _timer.searchSpeedLevel * _timer.frequencyIncrease), NULL);
        InvalidateRect(_hWnd, NULL, FALSE);
        break;
    }

    /* toolbar :: F-Value 버튼 클릭 */
    case IDM_TOOLBAR_FVALUE:
    {
        // F-Value 버튼 상태 확인
        LRESULT result = SendMessage(_hToolbar, TB_GETSTATE, (WPARAM)IDM_TOOLBAR_FVALUE, 0);
        // 버튼이 check 상태인지 여부 기록
        _toolbar.btnIsFValueChecked = result & TBSTATE_CHECKED;
        SleepEx(0, TRUE);
        InvalidateRect(_hWnd, NULL, FALSE);
    }
    break;

    /* toolbar :: Speed Up 버튼 클릭 */
    case IDM_TOOLBAR_SPEEDUP:
    {
        _timer.searchSpeedLevel--;
        if (_timer.searchSpeedLevel < -3)
            _timer.searchSpeedLevel = -3;

        _timer.bResetTimer = true;
    }
    break;

    /* toolbar :: Speed Down 버튼 클릭 */
    case IDM_TOOLBAR_SPEEDDOWN:
    {
        _timer.searchSpeedLevel++;
        if (_timer.searchSpeedLevel > 10)
            _timer.searchSpeedLevel = 10;

        _timer.bResetTimer = true;
    }
    break;

    /* toolbar :: Go to End 버튼 클릭 */
    case IDM_TOOLBAR_GOTOEND:
    {
        // 타이머 kill
        KillTimer(_hWnd, _timer.id);

        // 길찾기가 끝날 때까지 길찾기 수행
        if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
        {
            while (!_AStarSearch.SearchNextStep())
            {
                continue;
            }
        }
        else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
        {
            while (!_jumpPointSearch.SearchNextStep())
            {
                continue;
            }
        }

        InvalidateRect(_hWnd, NULL, FALSE);
    }
    break;


    /* toolbar :: Make Maze 버튼 클릭 */
    case IDM_TOOLBAR_MAKE_MAZE:
    {
        _AStarSearch.Clear();
        _jumpPointSearch.Clear();

        // maze 생성
        MakeMaze(_painting.gridCols, _painting.gridRows, _arr2Grid);

        // start, end point 지정
        int startRow = rand() % _painting.gridRows;
        int startCol = rand() % _painting.gridCols;
        int endRow = rand() % _painting.gridRows;
        int endCol = rand() % _painting.gridCols;
        while (_arr2Grid[startRow][startCol] == GRID_WALL)
        {
            startRow = rand() % _painting.gridRows;
            startCol = rand() % _painting.gridCols;
        }
        while (_arr2Grid[endRow][endCol] == GRID_WALL
            || (endRow == startRow && endCol == startCol))
        {
            endRow = rand() % _painting.gridRows;
            endCol = rand() % _painting.gridCols;
        }
        _painting.startCol = startCol;
        _painting.startRow = startRow;
        _painting.endCol = endCol;
        _painting.endRow = endRow;


        InvalidateRect(_hWnd, NULL, FALSE);
    }
    break;


    /* toolbar :: Make Cave 버튼 클릭 */
    case IDM_TOOLBAR_MAKE_CAVE:
    {
        _AStarSearch.Clear();
        _jumpPointSearch.Clear();

        // cave 생성
        CCaveGenerator cave;
        cave.Generate(_arr2Grid, _painting.gridCols, _painting.gridRows, 0.45f, 3);

        // start, end point 지정
        _test.numOfCaveWays = cave.GetNumOfWays();
        if (_test.numOfCaveWays > 1)
        {
            int startRow = rand() % _painting.gridRows;
            int startCol = rand() % _painting.gridCols;
            int endRow = rand() % _painting.gridRows;
            int endCol = rand() % _painting.gridCols;
            while (_arr2Grid[startRow][startCol] == GRID_WALL)
            {
                startRow = rand() % _painting.gridRows;
                startCol = rand() % _painting.gridCols;
            }
            while (_arr2Grid[endRow][endCol] == GRID_WALL
                || (endRow == startRow && endCol == startCol))
            {
                endRow = rand() % _painting.gridRows;
                endCol = rand() % _painting.gridCols;
            }
            _painting.startCol = startCol;
            _painting.startRow = startRow;
            _painting.endCol = endCol;
            _painting.endRow = endRow;
        }
        else
        {
            _painting.startCol = -1;
            _painting.startRow = -1;
            _painting.endCol = -1;
            _painting.endRow = -1;
        }

        InvalidateRect(_hWnd, NULL, FALSE);
    }
    break;


    /* toolbar :: Make Random 버튼 클릭 */
    case IDM_TOOLBAR_MAKE_RANDOM:
    {
        _AStarSearch.Clear();
        _jumpPointSearch.Clear();

        // random wall 생성
        for (int row = 0; row < _painting.gridRows; row++)
        {
            for (int col = 0; col < _painting.gridCols; col++)
            {
                _arr2Grid[row][col] = rand() > 16384 ? GRID_WAY : GRID_WALL;
            }
        }

        // start, end point 지정
        _painting.startCol = rand() % _painting.gridCols;
        _painting.startRow = rand() % _painting.gridRows;
        _painting.endCol = rand() % _painting.gridCols;
        _painting.endRow = rand() % _painting.gridRows;
        while (_painting.endRow == _painting.startRow
            && _painting.endCol == _painting.startCol)
        {
            _painting.endRow = rand() % _painting.gridRows;
            _painting.endCol = rand() % _painting.gridCols;
        }

        // 길뚫기
        _arr2Grid[_painting.startRow][_painting.startCol] = GRID_WAY;
        _arr2Grid[_painting.endRow][_painting.endCol] = GRID_WAY;
        int numOfWayPoint = (int)(pow(log(min(_painting.gridRows, _painting.gridCols)), 2.0) / 2.0);
        CBresenhamPath BPath;
        POINT ptStart;
        POINT ptEnd;
        for (int i = 0; i < numOfWayPoint; i++)
        {
            if (i == 0)
            {
                ptStart.x = _painting.startCol;
                ptStart.y = _painting.startRow;
            }
            else
            {
                ptStart.x = ptEnd.x;
                ptStart.y = ptEnd.y;
            }

            if (i == numOfWayPoint - 1)
            {
                ptEnd.x = _painting.endCol;
                ptEnd.y = _painting.endRow;
            }
            else
            {
                ptEnd.x = rand() % _painting.gridCols;
                ptEnd.y = rand() % _painting.gridRows;
                while (ptEnd.x == ptStart.x && ptEnd.y == ptStart.y)
                {
                    ptEnd.x = rand() % _painting.gridCols;
                    ptEnd.y = rand() % _painting.gridRows;
                }
            }

            BPath.SetParam(ptStart, ptEnd);
            while (BPath.Next())
            {
                _arr2Grid[BPath.GetY()][BPath.GetX()] = GRID_WAY;
            }
        }


        InvalidateRect(_hWnd, NULL, FALSE);
    }
    break;


    /* toolbar :: Run Test 버튼 클릭 */
    case IDM_TOOLBAR_RUN_TEST:
        static int sTestCount;                // 테스트 loop 제어용 카운트
        static int sTestTotalSearchCount;     // 길찾기를 수행한 횟수
        static int sTestPrevSearchCount;      // 이전 테스트 진행상황 출력 때의 길찾기 수행 횟수
        static ULONGLONG sTestStartTime;      // 테스트 시작 시간
        static ULONGLONG sTestPrevPrintTime;  // 이전 테스트 진행상황 출력 시간
        static LARGE_INTEGER sllFrequency;
        static LARGE_INTEGER sllTotalTime;  // 길찾기에 소요된 총 시간
        {
            QueryPerformanceFrequency(&sllFrequency);
            sllTotalTime.QuadPart = 28185624038i64;//0;

            if (_test.isRunning == false)
            {
                _test.isRunning = true;
                _test.isDoRendering = true;
                _test.isPrintInfo = true;
                sTestCount = 0;
                sTestTotalSearchCount = 8598420;// -1;
                sTestPrevSearchCount = 0;

                // 콘솔 생성
                AllocConsole();
                FILE* stream;
                freopen_s(&stream, "CONIN$", "r", stdin);
                freopen_s(&stream, "CONOUT$", "w", stdout);
                freopen_s(&stream, "CONOUT$", "w", stderr);

                // 콘솔에서 키보드 입력을 감지하는 스레드 실행
                _beginthreadex(NULL, 0, ThreadProcQuitRunTest, (PVOID)this, 0, NULL);
            }

            _test.randomSeed = (unsigned int)time(0);
            srand(_test.randomSeed);

            sTestStartTime = GetTickCount64() - (27360 * 1000); // GetTickCount64();
            sTestPrevPrintTime = sTestStartTime;


            // 첫 맵 생성
            PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_MAKE_CAVE, 0), (LPARAM)NULL);

            // IDM_TOOLBAR_RUN_TEST_INTERNAL 메시지 생성
            PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_RUN_TEST_INTERNAL, 0), (LPARAM)NULL);
        }
        break;

        /* toolbar :: Run Test 버튼 클릭 후 발생되는 테스트 무한반복 메시지 */
    case IDM_TOOLBAR_RUN_TEST_INTERNAL:
    {
        if (_test.isRunning)
        {
            sTestCount++;
            sTestTotalSearchCount++;

            // 1초마다 출력
            if (GetTickCount64() - sTestPrevPrintTime > 1000)
            {
                printf("Total test time: %lld sec, Test per sec: %d, Total search count: %d, average search time(ms): %f\n"
                    , (GetTickCount64() - sTestStartTime) / 1000
                    , sTestTotalSearchCount - sTestPrevSearchCount
                    , sTestTotalSearchCount
                    , (double)sllTotalTime.QuadPart / (double)sTestTotalSearchCount / (double)sllFrequency.QuadPart * 1000.0);

                sTestPrevPrintTime = GetTickCount64();
                sTestPrevSearchCount = sTestTotalSearchCount;
            }


            // 이전 결과 확인
            bool searchResult = true;
            if (sTestTotalSearchCount != 8598421)//0)
            {
                if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
                {
                    searchResult = _AStarSearch.IsSucceeded();
                }
                else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
                {
                    searchResult = _jumpPointSearch.IsSucceeded();
                }
            }

            // 목적지를 찾지 못했다면 오류이므로 테스트 종료
            if (searchResult == false)
            {
                ReadjustScroll();
                PostMessage(_hWnd, WM_PAINT, (WPARAM)MAKEWPARAM(0, 0), (LPARAM)NULL);
                _test.isRunning = false;
                printf("!!ERROR!! Cannot find the destination.\n");
                printf("Seed : %u, Total search : %d\n", _test.randomSeed, sTestTotalSearchCount);

                break;
            }

            // 만약 이전에 생성한 cave에서 way 수가 2보다 작을 경우 다시 만듬
            if (_test.numOfCaveWays < 2)
            {
                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_MAKE_CAVE, 0), (LPARAM)NULL);
                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_RUN_TEST_INTERNAL, 0), (LPARAM)NULL);
                sTestTotalSearchCount--;
                break;
            }

            // 맵 생성, 그리드 크기 조정
            if (sTestCount == 50)
            {
                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_MAKE_MAZE, 0), (LPARAM)NULL);
                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_RUN_TEST_INTERNAL, 0), (LPARAM)NULL);
                sTestTotalSearchCount--;
                break;
            }
            else if (sTestCount == 80)
            {
                int gridSize = min(40, _painting.gridSize);
                int randomWidth = (rand() % 130 + 20) * gridSize;
                int randomHeight = (rand() % 80 + 20) * gridSize;

                _painting.SetSize(gridSize, randomWidth, randomHeight);
                ResizeGrid();
                ResizeWindow();
                if (_test.isDoRendering)
                    ReadjustScroll();

                sTestCount = -1;

                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_MAKE_CAVE, 0), (LPARAM)NULL);
                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_RUN_TEST_INTERNAL, 0), (LPARAM)NULL);
                sTestTotalSearchCount--;
                break;
            }
            //PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_MAKE_RANDOM, 0), (LPARAM)NULL);

            // start, end point 지정
            int startRow = rand() % _painting.gridRows;
            int startCol = rand() % _painting.gridCols;
            int endRow = rand() % _painting.gridRows;
            int endCol = rand() % _painting.gridCols;
            while (_arr2Grid[startRow][startCol] == GRID_WALL)
            {
                startRow = rand() % _painting.gridRows;
                startCol = rand() % _painting.gridCols;
            }
            while (_arr2Grid[endRow][endCol] == GRID_WALL
                || (endRow == startRow && endCol == startCol))
            {
                endRow = rand() % _painting.gridRows;
                endCol = rand() % _painting.gridCols;
            }
            _painting.startCol = startCol;
            _painting.startRow = startRow;
            _painting.endCol = endCol;
            _painting.endRow = endRow;


            // 실패 테스트
            //if (sTestTotalSearchCount == 10000)
            //{
            //    _arr2Grid[endRow + 1][endCol + 1] = GRID_WALL;
            //    _arr2Grid[endRow + 1][endCol] = GRID_WALL;
            //    _arr2Grid[endRow + 1][endCol - 1] = GRID_WALL;
            //    _arr2Grid[endRow][endCol + 1] = GRID_WALL;
            //    _arr2Grid[endRow][endCol - 1] = GRID_WALL;
            //    _arr2Grid[endRow - 1][endCol + 1] = GRID_WALL;
            //    _arr2Grid[endRow - 1][endCol] = GRID_WALL;
            //    _arr2Grid[endRow - 1][endCol - 1] = GRID_WALL;
            //}

            // search 
            LARGE_INTEGER llSearchStartTime;
            LARGE_INTEGER llSearchEndTime;
            QueryPerformanceCounter(&llSearchStartTime);
            if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
            {
                // A* search 파라미터 설정
                _AStarSearch.SetParam(_painting.startRow, _painting.startCol
                    , _painting.endRow, _painting.endCol
                    , _arr2Grid
                    , _painting.gridRows
                    , _painting.gridCols
                    , _painting.gridSize);

                // 길찾기
                _AStarSearch.Search();
            }
            else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
            {
                // jump point search 파라미터 설정
                _jumpPointSearch.SetParam(_painting.startRow, _painting.startCol
                    , _painting.endRow, _painting.endCol
                    , _arr2Grid
                    , _painting.gridRows
                    , _painting.gridCols);

                // 길찾기
                _jumpPointSearch.Search();
            }
            QueryPerformanceCounter(&llSearchEndTime);
            sllTotalTime.QuadPart += llSearchEndTime.QuadPart - llSearchStartTime.QuadPart;

            // rendering
            if (_test.isDoRendering)
            {
                PostMessage(_hWnd, WM_PAINT, (WPARAM)MAKEWPARAM(0, 0), (LPARAM)NULL);
            }

            // 계속 테스트 수행
            PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_RUN_TEST_INTERNAL, 0), (LPARAM)NULL);
        }

    }
    break;

    default:
        return DefWindowProc(_hWnd, message, wParam, lParam);
    }

    return 0;
}


// 키보드 입력
void CWinSearch::MsgProc_KEYDOWN(WPARAM wParam, LPARAM lParam)
{

    switch (wParam)
    {
        // 방향키를 누르면 스크롤 이동
    case VK_LEFT:
        PostMessage(_hWnd, WM_HSCROLL, (WPARAM)MAKELONG(SB_LINEUP, 0), (LPARAM)NULL);
        break;

    case VK_RIGHT:
        PostMessage(_hWnd, WM_HSCROLL, (WPARAM)MAKELONG(SB_LINEDOWN, 0), (LPARAM)NULL);
        break;

    case VK_UP:
        PostMessage(_hWnd, WM_VSCROLL, (WPARAM)MAKELONG(SB_LINEUP, 0), (LPARAM)NULL);
        break;

    case VK_DOWN:
        PostMessage(_hWnd, WM_VSCROLL, (WPARAM)MAKELONG(SB_LINEDOWN, 0), (LPARAM)NULL);
        break;
    }
}


// 수평 스크롤바 이벤트
void CWinSearch::MsgProc_HSCROLL(WPARAM wParam, LPARAM lParam)
{
    // 수평 스크롤바 현재 정보 얻기
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo(_hWnd, SB_HORZ, &si);

    // 스크롤바 현재 위치 저장
    _scroll.hPos = si.nPos;

    // 스크롤바 이벤트 처리
    switch (LOWORD(wParam))
    {
    case SB_LINEUP:  // 스크롤바 왼쪽 화살표를 누름
        si.nPos -= 15;
        break;

    case SB_LINEDOWN:  // 스크롤바 오른쪽 화살표를 누름
        si.nPos += 15;
        break;

    case SB_PAGEUP:  // 스크롤바 왼쪽 shaft를 누름
        si.nPos -= si.nPage;
        break;

    case SB_PAGEDOWN:  // 스크롤바 오른쪽 shaft를 누름
        si.nPos += si.nPage;
        break;

    case SB_THUMBPOSITION:  // 스크롤바를 잡고 드래그함
        si.nPos = si.nTrackPos;
        break;
    }
    si.nPos = min(max(0, si.nPos), _scroll.hMax);  // min, max 조정

    // 스크롤바의 위치를 설정하고 정보를 다시 얻음
    si.fMask = SIF_POS;
    SetScrollInfo(_hWnd, SB_HORZ, &si, TRUE);
    GetScrollInfo(_hWnd, SB_HORZ, &si);

    // 만약 스크롤바의 위치가 변경됐다면 윈도우와 스크롤을 갱신한다.
    if (si.nPos != _scroll.hPos)
    {
        ScrollWindow(_hWnd, 0, _scroll.hPos - si.nPos, NULL, NULL);

        //UpdateWindow(_hWnd);
        _scroll.isRenderFromScrolling = true;  // 렌더링이 스크롤링에 의한 것임을 표시
        InvalidateRect(_hWnd, NULL, FALSE);
        SendMessage(_hToolbar, TB_AUTOSIZE, 0, 0);
    }
    // 현재 스크롤바 위치 갱신
    _scroll.hPos = si.nPos;

}


// 수직 스크롤바 이벤트
void CWinSearch::MsgProc_VSCROLL(WPARAM wParam, LPARAM lParam)
{
    // 수직 스크롤바 현재 정보 얻기
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo(_hWnd, SB_VERT, &si);

    // 스크롤바 현재 위치 저장
    _scroll.vPos = si.nPos;

    // 스크롤바 이벤트 처리
    switch (LOWORD(wParam))
    {
    case SB_LINEUP:  // 스크롤바 위 화살표를 누름
        si.nPos -= 15;
        break;

    case SB_LINEDOWN:  // 스크롤바 아래 화살표를 누름
        si.nPos += 15;
        break;

    case SB_PAGEUP:  // 스크롤바 위 shaft를 누름
        si.nPos -= si.nPage;
        break;

    case SB_PAGEDOWN:  // 스크롤바 아래 shaft를 누름
        si.nPos += si.nPage;
        break;

    case SB_THUMBPOSITION:  // 스크롤바를 잡고 드래그함
        si.nPos = si.nTrackPos;
        break;
    }
    si.nPos = min(max(0, si.nPos), _scroll.vMax);  // min, max 조정

    // 스크롤바의 위치를 설정하고 정보를 다시 얻음
    si.fMask = SIF_POS;
    SetScrollInfo(_hWnd, SB_VERT, &si, TRUE);
    GetScrollInfo(_hWnd, SB_VERT, &si);

    // 만약 스크롤바의 위치가 변경됐다면 윈도우와 스크롤을 갱신한다.
    if (si.nPos != _scroll.vPos)
    {
        ScrollWindow(_hWnd, 0, _scroll.vPos - si.nPos, NULL, NULL);
        //UpdateWindow(_hWnd);
        _scroll.isRenderFromScrolling = true;  // 렌더링이 스크롤링에 의한 것임을 표시
        InvalidateRect(_hWnd, NULL, FALSE);
        SendMessage(_hToolbar, TB_AUTOSIZE, 0, 0);
    }
    // 현재 스크롤바 위치 갱신
    _scroll.vPos = si.nPos;
}


// search 타이머
void CWinSearch::MsgProc_TIMER(WPARAM wParam, LPARAM lParam)
{
    // speed up 또는 speed down 버튼을 눌러 타이머를 다시설정해야함
    if (_timer.bResetTimer)
    {

        SetTimer(_hWnd, _timer.id, max(0, _timer.frequency + _timer.searchSpeedLevel * _timer.frequencyIncrease), NULL);
        _timer.bResetTimer = false;
    }

    // 길찾기 다음 step 실행
    bool isCompleted = false;
    if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
    {
        isCompleted = _AStarSearch.SearchNextStep();
    }
    else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
    {
        isCompleted = _jumpPointSearch.SearchNextStep();
    }


    // 완료되면 타이머 kill
    if (isCompleted)
    {
        KillTimer(_hWnd, _timer.id);
    }

    InvalidateRect(_hWnd, NULL, FALSE);
}


// WM_PAINT
void CWinSearch::MsgProc_PAINT(WPARAM wParam, LPARAM lParam)
{
    // 스크롤링에 의한 렌더링이 아닐 경우 전체를 다시 그린다.
    if (!_scroll.isRenderFromScrolling)
    {
        /* 그리기 영역 초기화 */
        PatBlt(_hMemDC, _painting.ptPos.x, _painting.ptPos.y, _painting.ptSize.x, _painting.ptSize.y, WHITENESS);


        /* 격자 그리기 */
        SelectObject(_hMemDC, _GDI.hPenGrid);
        for (int col = 0; col <= _painting.gridCols; col++)
        {
            MoveToEx(_hMemDC, col * _painting.gridSize + _painting.ptPos.x, _painting.ptPos.y, NULL);
            LineTo(_hMemDC, col * _painting.gridSize + _painting.ptPos.x, _painting.ptSize.y + _painting.ptPos.y);
        }
        for (int row = 0; row <= _painting.gridRows; row++)
        {
            MoveToEx(_hMemDC, _painting.ptPos.x, row * _painting.gridSize + _painting.ptPos.y, NULL);
            LineTo(_hMemDC, _painting.ptSize.x + _painting.ptPos.x, row * _painting.gridSize + _painting.ptPos.y);
        }

        /* 노드 정보 얻기 */
        // A* 노드 정보 얻기
        auto umapAStarNodeInfo = _AStarSearch.GetNodeInfo();

        // jump point search 노드 정보 얻기
        auto mmapJPSOpenList = _jumpPointSearch.GetOpenList();
        auto vecJPSCloseList = _jumpPointSearch.GetCloseList();
        decltype(vecJPSCloseList) vecJPSNodeInfo;
        vecJPSNodeInfo.reserve(mmapJPSOpenList.size() + vecJPSCloseList.size());
        for (auto iter = mmapJPSOpenList.begin(); iter != mmapJPSOpenList.end(); ++iter)
        {
            vecJPSNodeInfo.push_back((*iter).second);
        }
        for (size_t i = 0; i < vecJPSCloseList.size(); i++)
        {
            vecJPSNodeInfo.push_back(vecJPSCloseList[i]);
        }



        /* 길찾기 결과 그리기 */
        // A* search 결과 그리기
        if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
        {
            // A* 노드 정보 얻기

            // open 노드, close 노드 색칠하기
            auto iterNodeInfo = umapAStarNodeInfo.begin();
            for (; iterNodeInfo != umapAStarNodeInfo.end(); ++iterNodeInfo)
            {
                const AStarNode* pNode = iterNodeInfo->second;
                int col = pNode->_x;
                int row = pNode->_y;
                if (col < 0 || col > _painting.gridCols || row < 0 || row > _painting.gridRows)
                {
                    continue;;
                }

                if (pNode->_type == eAStarNodeType::OPEN)
                {
                    SelectObject(_hMemDC, _GDI.hBrushOpenNode);
                }
                else if (pNode->_type == eAStarNodeType::CLOSE)
                {
                    SelectObject(_hMemDC, _GDI.hBrushCloseNode);
                }
                Rectangle(_hMemDC
                    , col * _painting.gridSize + _painting.ptPos.x
                    , row * _painting.gridSize + _painting.ptPos.y
                    , (col + 1) * _painting.gridSize + _painting.ptPos.x
                    , (row + 1) * _painting.gridSize + _painting.ptPos.y);
            }
        }
        // Jump Point Search 결과 그리기
        else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
        {
            // cell 색칠
            const unsigned char* const* arr2CellColorGroup = _jumpPointSearch.GetCellColorGroup();
            for (int row = 0; row < _jumpPointSearch.GetGridRows(); row++)
            {
                for (int col = 0; col < _jumpPointSearch.GetGridCols(); col++)
                {
                    if (arr2CellColorGroup[row][col] != 0)
                    {
                        SelectObject(_hMemDC, _GDI.arrHBrushCellColor[(arr2CellColorGroup[row][col] - 1) % NUM_OF_CELL_COLOR_GROUP]);
                        Rectangle(_hMemDC
                            , col * _painting.gridSize + _painting.ptPos.x
                            , row * _painting.gridSize + _painting.ptPos.y
                            , (col + 1) * _painting.gridSize + _painting.ptPos.x
                            , (row + 1) * _painting.gridSize + _painting.ptPos.y);
                    }
                }
            }

            // open 노드, close 노드 색칠하기
            for (size_t i = 0; i < vecJPSNodeInfo.size(); i++)
            {
                const JPSNode* pNode = vecJPSNodeInfo[i];
                int col = pNode->_x;
                int row = pNode->_y;
                if (col < 0 || col > _painting.gridCols || row < 0 || row > _painting.gridRows)
                {
                    continue;;
                }

                if (pNode->_type == eJPSNodeType::OPEN)
                {
                    SelectObject(_hMemDC, _GDI.hBrushOpenNode);
                }
                else if (pNode->_type == eJPSNodeType::CLOSE)
                {
                    SelectObject(_hMemDC, _GDI.hBrushCloseNode);
                }
                Rectangle(_hMemDC
                    , col * _painting.gridSize + _painting.ptPos.x
                    , row * _painting.gridSize + _painting.ptPos.y
                    , (col + 1) * _painting.gridSize + _painting.ptPos.x
                    , (row + 1) * _painting.gridSize + _painting.ptPos.y);
            }
        }

        /* start point 색칠하기 */
        if (_painting.startCol >= 0 || _painting.startRow >= 0)
        {
            SelectObject(_hMemDC, _GDI.hBrushStartPoint);
            Rectangle(_hMemDC
                , _painting.startCol * _painting.gridSize + _painting.ptPos.x
                , _painting.startRow * _painting.gridSize + _painting.ptPos.y
                , (_painting.startCol + 1) * _painting.gridSize + _painting.ptPos.x
                , (_painting.startRow + 1) * _painting.gridSize + _painting.ptPos.y);
        }

        /* end point 색칠하기 */
        if (_painting.endCol >= 0 || _painting.endRow >= 0)
        {
            SelectObject(_hMemDC, _GDI.hBrushEndPoint);
            Rectangle(_hMemDC
                , _painting.endCol * _painting.gridSize + _painting.ptPos.x
                , _painting.endRow * _painting.gridSize + _painting.ptPos.y
                , (_painting.endCol + 1) * _painting.gridSize + _painting.ptPos.x
                , (_painting.endRow + 1) * _painting.gridSize + _painting.ptPos.y);
        }


        /* wall 색칠하기 */
        // 하나의 row 전체를 커버하는 wall bitmap을 만든다.
        if (_painting.gridSize != _gridSize_old || _painting.gridCols != _gridCols_old)
        {
            _hWallBit = CreateCompatibleBitmap(_hDC, _painting.gridSize * _painting.gridCols, _painting.gridSize);
            HBITMAP hWallBit_old = (HBITMAP)SelectObject(_hWallDC, _hWallBit);
            DeleteObject(hWallBit_old);

            SelectObject(_hWallDC, _GDI.hPenGrid);
            SelectObject(_hWallDC, _GDI.hBrushWall);
            for (int col = 0; col < _painting.gridCols; col++)
            {
                Rectangle(_hWallDC
                    , col * _painting.gridSize
                    , 0
                    , (col + 1) * _painting.gridSize
                    , _painting.gridSize);
            }

            _gridSize_old = _painting.gridSize;
            _gridCols_old = _painting.gridCols;
        }

        // 연속된 wall을 wall bitmap에서 복사해 붙여넣는다.
        int wallStartCol = -1;
        for (int row = 0; row < _painting.gridRows; row++)
        {
            for (int col = 0; col < _painting.gridCols; col++)
            {
                if (_arr2Grid[row][col] == GRID_WALL)
                {
                    // col이 처음이 아니고 col 이전이 WAY 인 경우,
                    // 또는 col이 처음인 경우 wall start
                    if ((col != 0 && _arr2Grid[row][col - 1] == GRID_WAY)
                        || col == 0)
                    {
                        if (wallStartCol == -1)
                            wallStartCol = col;
                    }

                    // col이 마지막이 아니고 col 다음이 WAY 인 경우
                    // 또는 col이 마지막인 경우 wall end, painting
                    if ((col != _painting.gridCols - 1 && _arr2Grid[row][col + 1] == GRID_WAY)
                        || col == _painting.gridCols - 1)
                    {
                        BitBlt(_hMemDC
                            , wallStartCol * _painting.gridSize + _painting.ptPos.x
                            , row * _painting.gridSize + _painting.ptPos.y
                            , (col + 1) * _painting.gridSize + _painting.ptPos.x
                            , (row + 1) * _painting.gridSize + _painting.ptPos.y
                            , _hWallDC
                            , _painting.gridSize * (_painting.gridCols - (col + 1 - wallStartCol))
                            , 0
                            , SRCCOPY);

                        wallStartCol = -1;
                    }
                }

            }
        }


        /* 길찾기 경로 그리기 */
        std::vector<POINT> vecPath;
        if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
        {
            vecPath = _AStarSearch.GetPath();
        }
        else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
        {
            vecPath = _jumpPointSearch.GetPath();
        }

        if (vecPath.size() > 0)
        {
            SelectObject(_hMemDC, _GDI.hPenPath);
            for (int i = 0; i < vecPath.size() - 1; i++)
            {
                MoveToEx(_hMemDC
                    , vecPath[i].x * _painting.gridSize + _painting.gridSize / 2 + _painting.ptPos.x
                    , vecPath[i].y * _painting.gridSize + _painting.gridSize / 2 + _painting.ptPos.y, NULL);
                LineTo(_hMemDC
                    , vecPath[i + 1].x * _painting.gridSize + _painting.gridSize / 2 + _painting.ptPos.x
                    , vecPath[i + 1].y * _painting.gridSize + _painting.gridSize / 2 + _painting.ptPos.y);
            }
        }


        /* Bresenham 알고리즘으로 보정된 경로 그리기 */
        std::vector<POINT> vecSmoothPath;
        if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
        {
            vecSmoothPath = _jumpPointSearch.GetSmoothPath();
            if (vecSmoothPath.size() > 0)
            {
                SelectObject(_hMemDC, _GDI.hPenSmoothPath);
                for (int i = 0; i < vecSmoothPath.size() - 1; i++)
                {
                    MoveToEx(_hMemDC
                        , vecSmoothPath[i].x * _painting.gridSize + _painting.gridSize / 2 + _painting.ptPos.x
                        , vecSmoothPath[i].y * _painting.gridSize + _painting.gridSize / 2 + _painting.ptPos.y, NULL);
                    LineTo(_hMemDC
                        , vecSmoothPath[i + 1].x * _painting.gridSize + _painting.gridSize / 2 + _painting.ptPos.x
                        , vecSmoothPath[i + 1].y * _painting.gridSize + _painting.gridSize / 2 + _painting.ptPos.y);
                }
            }
        }


        /* F-value, 부모 방향 화살표 그리기 */
        if (_toolbar.btnIsFValueChecked)
        {
            // 텍스트 배경색, 글자색 설정
            SetBkMode(_hMemDC, TRANSPARENT);
            SetTextColor(_hMemDC, RGB(0, 0, 0));

            // F-value font 생성
            int FValuefontSize = _painting.gridSize / 4;
            HFONT hFValueFont = CreateFont(FValuefontSize, 0, 0, 0, 0
                , false, false, false
                , ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY
                , DEFAULT_PITCH | FF_DONTCARE
                , NULL);

            // 화살표 font 생성
            int arrowFontSize = _painting.gridSize / 2;
            HFONT hArrowFont = CreateFont(arrowFontSize, 0, 0, 0, FW_BOLD
                , false, false, false
                , ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY
                , DEFAULT_PITCH | FF_DONTCARE
                , NULL);

            HFONT hFontOld = (HFONT)SelectObject(_hMemDC, hFValueFont);

            // A* F-value, 부모 방향 화살표 그리기
            if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
            {
                auto iterNodeInfo = umapAStarNodeInfo.begin();
                for (; iterNodeInfo != umapAStarNodeInfo.end(); ++iterNodeInfo)
                {
                    const AStarNode* pNode = iterNodeInfo->second;
                    int col = pNode->_x;
                    int row = pNode->_y;

                    SelectObject(_hMemDC, hFValueFont);

                    // G-value 그리기
                    WCHAR szGValue[10];
                    int lenSzGValue = swprintf_s(szGValue, 10, L"G %.1f", pNode->_valG);
                    TextOut(_hMemDC
                        , col * _painting.gridSize + _painting.ptPos.x + 1
                        , row * _painting.gridSize + _painting.ptPos.y
                        , szGValue, lenSzGValue);

                    // H-value 그리기
                    WCHAR szHValue[10];
                    int lenSzHValue = swprintf_s(szHValue, 10, L"H %.1f", pNode->_valH);
                    TextOut(_hMemDC
                        , col * _painting.gridSize + _painting.ptPos.x + 1
                        , row * _painting.gridSize + FValuefontSize + _painting.ptPos.y
                        , szHValue, lenSzHValue);

                    // F-value 그리기
                    WCHAR szFValue[10];
                    int lenSzFValue = swprintf_s(szFValue, 10, L"F %.1f", pNode->_valF);
                    TextOut(_hMemDC
                        , col * _painting.gridSize + _painting.ptPos.x + 1
                        , row * _painting.gridSize + FValuefontSize * 2 + _painting.ptPos.y
                        , szFValue, lenSzFValue);

                    // 부모 방향 그리기
                    SelectObject(_hMemDC, hArrowFont);
                    if (pNode->_pParent != nullptr)
                    {
                        WCHAR chArrow;
                        int diffX = pNode->_pParent->_x - pNode->_x;
                        int diffY = pNode->_pParent->_y - pNode->_y;
                        if (diffX > 0)
                        {
                            if (diffY > 0)
                                chArrow = L'↘';
                            else if (diffY == 0)
                                chArrow = L'→';
                            else
                                chArrow = L'↗';
                        }
                        else if (diffX == 0)
                        {
                            if (diffY > 0)
                                chArrow = L'↓';
                            else if (diffY == 0)
                                chArrow = L'?';
                            else
                                chArrow = L'↑';
                        }
                        else
                        {
                            if (diffY > 0)
                                chArrow = L'↙';
                            else if (diffY == 0)
                                chArrow = L'←';
                            else
                                chArrow = L'↖';
                        }

                        TextOut(_hMemDC
                            , (col + 1) * _painting.gridSize - arrowFontSize + _painting.ptPos.x
                            , (row + 1) * _painting.gridSize - arrowFontSize + _painting.ptPos.y
                            , (LPCWSTR)&chArrow, 1);
                    }
                }
            }

            // Jump Point Search F-value, 부모 방향 화살표 그리기
            else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
            {
                for (size_t i = 0; i < vecJPSNodeInfo.size(); i++)
                {
                    const JPSNode* pNode = vecJPSNodeInfo[i];
                    int col = pNode->_x;
                    int row = pNode->_y;

                    SelectObject(_hMemDC, hFValueFont);

                    // G-value 그리기
                    WCHAR szGValue[10];
                    int lenSzGValue = swprintf_s(szGValue, 10, L"G %.1f", pNode->_valG);
                    TextOut(_hMemDC
                        , col * _painting.gridSize + _painting.ptPos.x + 1
                        , row * _painting.gridSize + _painting.ptPos.y
                        , szGValue, lenSzGValue);

                    // H-value 그리기
                    WCHAR szHValue[10];
                    int lenSzHValue = swprintf_s(szHValue, 10, L"H %.1f", pNode->_valH);
                    TextOut(_hMemDC
                        , col * _painting.gridSize + _painting.ptPos.x + 1
                        , row * _painting.gridSize + FValuefontSize + _painting.ptPos.y
                        , szHValue, lenSzHValue);

                    // F-value 그리기
                    WCHAR szFValue[10];
                    int lenSzFValue = swprintf_s(szFValue, 10, L"F %.1f", pNode->_valF);
                    TextOut(_hMemDC
                        , col * _painting.gridSize + _painting.ptPos.x + 1
                        , row * _painting.gridSize + FValuefontSize * 2 + _painting.ptPos.y
                        , szFValue, lenSzFValue);

                    // 부모 방향 그리기
                    SelectObject(_hMemDC, hArrowFont);
                    if (pNode->_pParent != nullptr)
                    {
                        WCHAR chArrow;
                        int diffX = pNode->_pParent->_x - pNode->_x;
                        int diffY = pNode->_pParent->_y - pNode->_y;
                        if (diffX > 0)
                        {
                            if (diffY > 0)
                                chArrow = L'↘';
                            else if (diffY == 0)
                                chArrow = L'→';
                            else
                                chArrow = L'↗';
                        }
                        else if (diffX == 0)
                        {
                            if (diffY > 0)
                                chArrow = L'↓';
                            else if (diffY == 0)
                                chArrow = L'?';
                            else
                                chArrow = L'↑';
                        }
                        else
                        {
                            if (diffY > 0)
                                chArrow = L'↙';
                            else if (diffY == 0)
                                chArrow = L'←';
                            else
                                chArrow = L'↖';
                        }

                        TextOut(_hMemDC
                            , (col + 1) * _painting.gridSize - arrowFontSize + _painting.ptPos.x
                            , (row + 1) * _painting.gridSize - arrowFontSize + _painting.ptPos.y
                            , (LPCWSTR)&chArrow, 1);
                    }
                }
            }

            SelectObject(_hMemDC, hFontOld);
            DeleteObject(hFValueFont);
            DeleteObject(hArrowFont);
        }  // end of F-value, 부모 방향 화살표 그리기


        /* Bresenham Line 그리기 */
        // Bresenham Line 버튼이 눌려져 있고, 현재 마우스를 드래그 중일 경우 선을 그림
        if (_toolbar.btnTool == IDM_TOOLBAR_BRESENHAM_LINE
            && _mouse.clickPainting)
        {
            SelectObject(_hMemDC, _GDI.hPenGrid);
            SelectObject(_hMemDC, _GDI.hBrushBresenhamLine);

            int startRow = mcMOUSE_Y_TO_GRID_ROW(_mouse.clickY);
            int startCol = mcMOUSE_X_TO_GRID_COL(_mouse.clickX);
            int endRow = mcMOUSE_Y_TO_GRID_ROW(_mouse.y);
            int endCol = mcMOUSE_X_TO_GRID_COL(_mouse.x);
            Rectangle(_hMemDC
                , startCol * _painting.gridSize + _painting.ptPos.x
                , startRow * _painting.gridSize + _painting.ptPos.y
                , (startCol + 1) * _painting.gridSize + _painting.ptPos.x
                , (startRow + 1) * _painting.gridSize + _painting.ptPos.y);

            CBresenhamPath BPath;
            BPath.SetParam(POINT{ startCol, startRow }, POINT{ endCol, endRow });
            while (BPath.Next())
            {
                Rectangle(_hMemDC
                    , BPath.GetX() * _painting.gridSize + _painting.ptPos.x
                    , BPath.GetY() * _painting.gridSize + _painting.ptPos.y
                    , (BPath.GetX() + 1) * _painting.gridSize + _painting.ptPos.x
                    , (BPath.GetY() + 1) * _painting.gridSize + _painting.ptPos.y);
            }
        }





        /* 그리기영역을 제외한 memDC 영역 초기화 */
        // memDC 초기화를 이곳에서 하는 이유는 그리기영역 밖으로 삐져나와 그려진 것들을 없애기 위해서임.
        SelectObject(_hMemDC, _GDI.hBrushBackground);
        PatBlt(_hMemDC, 0, 0, _ptMemBitSize.x, _painting.ptPos.y, PATCOPY);
        PatBlt(_hMemDC, 0, 0, _painting.ptPos.x, _ptMemBitSize.y, PATCOPY);
        PatBlt(_hMemDC, _painting.ptPos.x + _painting.ptSize.x, 0, _ptMemBitSize.x, _ptMemBitSize.y, PATCOPY);
        PatBlt(_hMemDC, 0, _painting.ptPos.y + _painting.ptSize.y, _ptMemBitSize.x, _ptMemBitSize.y, PATCOPY);


        /* Mem DC에 그라데이션 그림자 그리기 */
        for (int i = 0; i < SHADOW_STEP; i++)
        {
            SelectObject(_hMemDC, _GDI.arrHPenShadow[i]);

            MoveToEx(_hMemDC, _painting.ptPos.x + _painting.ptSize.x + i, _painting.ptPos.y + 10, NULL);
            LineTo(_hMemDC, _painting.ptPos.x + _painting.ptSize.x + i, _painting.ptPos.y + _painting.ptSize.y + SHADOW_STEP);
            MoveToEx(_hMemDC, _painting.ptPos.x + 10, _painting.ptPos.y + _painting.ptSize.y + i, NULL);
            LineTo(_hMemDC, _painting.ptPos.x + _painting.ptSize.x + SHADOW_STEP, _painting.ptPos.y + _painting.ptSize.y + i);
        }


        /* 그리기영역 크기조정 object 그리기 */
        {
            SelectObject(_hMemDC, _GDI.hPenResizeObject);
            SelectObject(_hMemDC, _GDI.hBrushResizeObject);
            int left[3] = { _painting.ptPos.x + _painting.ptSize.x / 2 - 2, _painting.ptPos.x + _painting.ptSize.x, _painting.ptPos.x + _painting.ptSize.x };
            int top[3] = { _painting.ptPos.y + _painting.ptSize.y, _painting.ptPos.y + _painting.ptSize.y, _painting.ptPos.y + _painting.ptSize.y / 2 - 2 };
            for (int i = 0; i < 3; i++)
            {
                Rectangle(_hMemDC, left[i], top[i], left[i] + 5, top[i] + 5);
            }
        }


        /* 그리기영역 크기조정 object를 클릭했을 때 크기조정 테두리 그리기 */
        if (_mouse.clickResizeVertical || _mouse.clickResizeDiagonal || _mouse.clickResizeHorizontal)
        {
            SelectObject(_hMemDC, _GDI.hPenResizeBorder);
            SelectObject(_hMemDC, _GDI.hBrushResizeBorder);

            // 수직 그리기영역 크기조정 object를 클릭했을 때 크기조정 테두리 그리기
            if (_mouse.clickResizeVertical)
            {
                Rectangle(_hMemDC
                    , _painting.ptPos.x
                    , _painting.ptPos.y
                    , _painting.ptPos.x + _painting.ptSize.x
                    , max(_mouse.y, _painting.ptPos.y + PAINT_SIZE_MIN_Y));
            }
            // 대각 그리기영역 크기조정 object를 클릭했을 때 크기조정 테두리 그리기
            else if (_mouse.clickResizeDiagonal)
            {
                Rectangle(_hMemDC
                    , _painting.ptPos.x
                    , _painting.ptPos.y
                    , max(_mouse.x, _painting.ptPos.x + PAINT_SIZE_MIN_X)
                    , max(_mouse.y, _painting.ptPos.y + PAINT_SIZE_MIN_Y));
            }
            // 수평 그리기영역 크기조정 object를 클릭했을 때 크기조정 테두리 그리기
            else if (_mouse.clickResizeHorizontal)
            {
                Rectangle(_hMemDC
                    , _painting.ptPos.x
                    , _painting.ptPos.y
                    , max(_mouse.x, _painting.ptPos.x + PAINT_SIZE_MIN_X)
                    , _painting.ptPos.y + _painting.ptSize.y);
            }
        }


        /* 그리드 row, col 수 표시문자 출력 */
        SelectObject(_hMemDC, _GDI.hFontGridInfo);
        SetBkMode(_hMemDC, TRANSPARENT);
        SetTextColor(_hMemDC, RGB(0, 0, 0));

        WCHAR szGridInfo[30];
        int lenSzGridInfo = swprintf_s(szGridInfo, 30, L"%d x %d cells", _painting.gridCols, _painting.gridRows);
        TextOut(_hMemDC
            , _painting.ptPos.x
            , _painting.ptPos.y - 15
            , szGridInfo, lenSzGridInfo);


        /* 테스트 수행중일 때 관련 텍스트 출력 */
        WCHAR szTestInfo[150];
        int lenSzTestInfo;
        if (_test.isRunning || _test.isPrintInfo)
        {
            lenSzTestInfo = swprintf_s(szTestInfo, 150, L"Test is running...  seed: %u.  Press 'Q' on the console window to quit the test or 'R' to stop rendering.\n", _test.randomSeed);
            TextOut(_hMemDC
                , _painting.ptPos.x + 100
                , _painting.ptPos.y - 15
                , szTestInfo, lenSzTestInfo);

            if (_test.isRunning == false && _test.isPrintInfo == true)
                _test.isPrintInfo = false;
        }


        // begin paint
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(_hWnd, &ps);

        /* MemDC에서 MainDC로 bit block transfer */
        BitBlt(_hDC
            , 0, _toolbar.height
            , _ptMemBitSize.x, _ptMemBitSize.y
            , _hMemDC, _scroll.hPos, _toolbar.height + _scroll.vPos, SRCCOPY);
        EndPaint(_hWnd, &ps);

    } // end of if(!_scroll.isRenderFromScrolling)


    // 스크롤링에 의한 렌더링일 경우 BitBlt만 다시 한다.
    else
    {
        _scroll.isRenderFromScrolling = false;

        // begin paint
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(_hWnd, &ps);

        /* MemDC에서 MainDC로 bit block transfer */
        BitBlt(_hDC
            , 0, _toolbar.height
            , _ptMemBitSize.x, _ptMemBitSize.y
            , _hMemDC, _scroll.hPos, _toolbar.height + _scroll.vPos, SRCCOPY);

        EndPaint(_hWnd, &ps);
    }


}


// WM_DESTROY
void CWinSearch::MsgProc_DESTROYT(WPARAM wParam, LPARAM lParam)
{
    // KillTimer
    // select old dc
    // release dc
    // delete dc
    KillTimer(_hWnd, _timer.id);

    PostQuitMessage(0);     // 이 작업을 하지 않으면 윈도우만 닫히고 프로세스는 종료되지 않게 된다.
}














// 크기 조정 다이얼로그
INT_PTR CALLBACK CWinSearch::WndProcDialogSizeAdj(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CWinSearch* pThis = reinterpret_cast<CWinSearch*>(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        SetDlgItemInt(hDlg, IDC_SIZE_ADJ_SIZE, pThis->_painting.gridSize, TRUE);
        SetDlgItemInt(hDlg, IDC_SIZE_ADJ_WIDTH, pThis->_painting.ptSize.x, TRUE);
        SetDlgItemInt(hDlg, IDC_SIZE_ADJ_HEIGHT, pThis->_painting.ptSize.y, TRUE);
    }
    return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch LOWORD(wParam)
        {
        case IDOK:
        {
            // OK를 눌렀을 경우 _painting 에 크기 값을 저장한다.
            BOOL translated;
            int gridSize = (int)GetDlgItemInt(hDlg, IDC_SIZE_ADJ_SIZE, &translated, TRUE);
            if (!translated)
                break;

            int width = (int)GetDlgItemInt(hDlg, IDC_SIZE_ADJ_WIDTH, &translated, TRUE);
            if (!translated)
                break;

            int height = (int)GetDlgItemInt(hDlg, IDC_SIZE_ADJ_HEIGHT, &translated, TRUE);
            if (!translated)
                break;

            pThis->_painting.SetSize(gridSize, width, height);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        }
        break;
    }
    return (INT_PTR)FALSE;
}










// windows procedure
LRESULT CALLBACK CWinSearch::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
    // 디버깅 모드 중에 visual studio 출력 창에 메시지 값 출력하기
    char DebugLog[50];
    sprintf_s(DebugLog, "%llu, %u, %llu, %llu\n", (unsigned __int64)hWnd, message, (unsigned __int64)wParam, (unsigned __int64)lParam);
    OutputDebugStringA(DebugLog);
#endif


    CWinSearch* pThis;
    pThis = reinterpret_cast<CWinSearch*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_NCCREATE:
    {
        pThis = static_cast<CWinSearch*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);

        SetLastError(0);
        if (!SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis)))
        {
            if (GetLastError() != 0)
            {
                std::string msg = std::string("Failed to set 'this' pointer to USERDATA. system error : ") + std::to_string(GetLastError()) + "\n";
                throw std::runtime_error(msg.c_str());
                return FALSE;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    break;

    /* 마우스 왼쪽 클릭 시 */
    case WM_LBUTTONDOWN:
        pThis->MsgProc_LBUTTONDOWN(wParam, lParam);
        break;
    /* 마우스 왼쪽버튼을 뗐을 때 */
    case WM_LBUTTONUP:
        pThis->MsgProc_LBUTTONUP(wParam, lParam);
        break;
    /* 마우스를 움직일 때 */
    case WM_MOUSEMOVE:
        pThis->MsgProc_MOUSEMOVE(wParam, lParam);
        break;
    /* 마우스 휠을 굴릴 때 */
    case WM_MOUSEWHEEL:
        pThis->MsgProc_MOUSEWHEEL(wParam, lParam);
        break;
    /* 창의 크기가 바뀔 때 */
    case WM_SIZE:
        pThis->MsgProc_SIZE(wParam, lParam);
        break;
    /* 커서 교체 (마우스를 움직일 때마다 시행됨) */
    case WM_SETCURSOR:
        return pThis->MsgProc_SETCURSOR(message, wParam, lParam);
    /* menu 또는 toolbar 클릭 */
    case WM_COMMAND:
        return pThis->MsgProc_COMMAND(message, wParam, lParam);
    /* 키보드 입력 */
    case WM_KEYDOWN:
        pThis->MsgProc_KEYDOWN(wParam, lParam);
        break;
    /* 수평 스크롤바 이벤트 */
    case WM_HSCROLL:
        pThis->MsgProc_HSCROLL(wParam, lParam);
        break;
    /* 수직 스크롤바 이벤트 */
    case WM_VSCROLL:
        pThis->MsgProc_VSCROLL(wParam, lParam);
        break;
    /* search 타이머 */
    case WM_TIMER:
        pThis->MsgProc_TIMER(wParam, lParam);
        break;
    /* WM_PAINT */
    case WM_PAINT:
        pThis->MsgProc_PAINT(wParam, lParam);
        break;
    case WM_DESTROY:
        pThis->MsgProc_DESTROYT(wParam, lParam);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


