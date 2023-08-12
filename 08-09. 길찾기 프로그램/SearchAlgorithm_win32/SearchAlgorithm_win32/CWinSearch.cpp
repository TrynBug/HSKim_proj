#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Winmm.lib")


#include "CWinSearch.h"
#include "utils.h"
#include "CBresenhamPath.h"

using namespace search;




CWinSearch::CWinSearch(_In_ HINSTANCE hInst, _In_ int nCmdShow)
    : _hInst(hInst)
    , _nCmdShow(nCmdShow)
    , _grid({ 30, 48 }, { 1280, 720 }, 20)
{



    _timer.id = 1;                   // ID
    _timer.frequency = 300;          // �ֱ�
    _timer.frequencyIncrease = 100;  // �ֱ� ����ġ
    _timer.bResetTimer = false;     // Ÿ�̸� �缳�� ����
    _timer.searchSpeedLevel = 0;     // �ӵ�(���� ������ ����, ���� ����)

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

    int r0 = GetRValue(dfCOLOR_BACKGROUND) - 28;   // �׸��� ���� ��
    int g0 = GetGValue(dfCOLOR_BACKGROUND) - 25;
    int b0 = GetBValue(dfCOLOR_BACKGROUND) - 23;
    int r1 = GetRValue(dfCOLOR_BACKGROUND);        // �׸��� ���� ��
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
    utils::ggColorSlice(NUM_OF_CELL_COLOR_GROUP, arrRGB, 0.3);
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



int CWinSearch::Run()
{
    srand((unsigned int)time(0));

    // Ŀ�� ����
    _cursor.Init();

    // GDI ����
    _GDI.Init();

    // â Ŭ������ ����մϴ�.
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


    // window ����
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

    // toolbar �����.
    _hToolbar = CreateToolbar(_hInst, _hWnd);

    // ���� �������� device context�� ��´�.
    _hDC = GetDC(_hWnd);
    // ���� ������ ũ�⸦ ��´�.
    GetClientRect(_hWnd, &_rtClientSize);

    // ���� ���۸� �뵵�� ��Ʈ�ʰ� DC�� �����.
    _ptMemBitSize.x = _rtClientSize.right;
    _ptMemBitSize.y = _rtClientSize.bottom;
    _hMemBit = CreateCompatibleBitmap(_hDC, _ptMemBitSize.x, _ptMemBitSize.y);
    _hMemDC = CreateCompatibleDC(_hDC);
    HBITMAP hOldBit = (HBITMAP)SelectObject(_hMemDC, _hMemBit);
    DeleteObject(hOldBit);

    // memDC�� wall �׸��� �뵵�� DC
    _hWallDC = CreateCompatibleDC(_hDC);

    // Show Window
    ShowWindow(_hWnd, _nCmdShow);  // nCmdShow �� �����츦 �ִ�ȭ�� ������ ������, �ּ�ȭ�� ������ ������ ���� �����ϴ� �ɼ��̴�.
    UpdateWindow(_hWnd);          // �����츦 �ѹ� ���Ž�Ų��.


#ifdef _DEBUG
    // DEBUG ����� ��� �ܼ� ����
    //AllocConsole();
    //FILE* stream;
    //freopen_s(&stream, "CONIN$", "r", stdin);
    //freopen_s(&stream, "CONOUT$", "w", stdout);
    //freopen_s(&stream, "CONOUT$", "w", stderr);
#endif

    // ����Ű ��� ����
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





/* Toolbar :: toolbar ����� */
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









// Utils :: memory bitmap ũ�⸦ �����Ѵ�.
void CWinSearch::ResizeWindow()
{
    // �׸��念�� ��ü�� ������ �� �ִ� ũ��� �����.
    GetClientRect(_hWnd, &_rtClientSize);
    POINT ptGridSize = _grid.GetGridSize();
    POINT ptGridStart = _grid.GetGridStartPos();
    int newBitSizeX = max(_rtClientSize.right, ptGridSize.x + ptGridStart.x + 50);
    int newBitSizeY = max(_rtClientSize.bottom, ptGridSize.y + ptGridStart.y + _toolbar.height + 50);

    // ���� ���ο� ũ�Ⱑ ���� ũ��� ���ٸ� ����
    if (newBitSizeX == _ptMemBitSize.x && newBitSizeY == _ptMemBitSize.y)
        return;

    // ��Ʈ�� �����
    _ptMemBitSize.x = newBitSizeX;
    _ptMemBitSize.y = newBitSizeY;
    HBITMAP hNewBit = CreateCompatibleBitmap(_hMemDC, _ptMemBitSize.x, _ptMemBitSize.y);
    SelectObject(_hMemDC, hNewBit);
    DeleteObject(_hMemBit);
    _hMemBit = hNewBit;

}






// Utils :: ��ũ���� �������Ѵ�.
void CWinSearch::ReadjustScroll()
{
    GetClientRect(_hWnd, &_rtClientSize);
    // ���� ��ũ�Ѹ� ������ (��Ʈ�ʳʺ� - Ŭ���̾�Ʈ�ʺ�) �� ��������.
    // ���� ���� ��ũ�� ��ġ�� ���� ��ũ�Ѹ� ���� ���� �����ȴ�.
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

    // ���� ��ũ�Ѹ� ������ (��Ʈ�ʳ��� - Ŭ���̾�Ʈ����) �� ��������.
    // ���� ���� ��ũ�� ��ġ�� ���� ��ũ�Ѹ� ���� ���� �����ȴ�.
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


// Utils :: �׽�Ʈ �ߴ� Ű���� �Է� ���� ������
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
// ���콺 ���� Ŭ�� ��
void CWinSearch::MsgProc_LBUTTONDOWN(WPARAM wParam, LPARAM lParam)
{
    int xPos = mcGET_MOUSE_X_LPARAM(lParam);
    int yPos = mcGET_MOUSE_Y_LPARAM(lParam);
    _mouse.clickX = xPos;
    _mouse.clickY = yPos;

#ifdef _DEBUG
    printf("WM_LBUTTONDOWN  xPos %d, yPos %d, _scroll.hPos %d, _scroll.vPos %d\n", xPos, yPos, _scroll.hPos, _scroll.vPos);
#endif
    // ���� ���콺 ��ǥ�� �׸��⿵�� ���� ��� click ����
    if (_grid.IsMouseOnGridArea(xPos, yPos))
    {
        _mouse.clickPainting = true;
        int row = mcMOUSE_Y_TO_GRID_ROW(yPos);
        int col = mcMOUSE_X_TO_GRID_COL(xPos);

        // ���� ���õ� toolbar ��ư�� Wall �� ��� grid ���¸� �ٲ�
        if (_toolbar.btnTool == IDM_TOOLBAR_WALL)
        {
            if (_grid.GetGridState(row, col) == EGridState::WALL)
            {
                _mouse.erase = true;
                _grid.SetGridState(row, col, EGridState::WAY);
            }
            else
            {
                _mouse.erase = false;
                _grid.SetGridState(row, col, EGridState::WALL);
            }
        }

        // ���� ���õ� toolbar ��ư�� Start Point �� ��� start point ��ǥ�� ������
        else if (_toolbar.btnTool == IDM_TOOLBAR_STARTPOINT)
        {
            RC rcStart = _grid.GetStartRC();
            if (row == rcStart.row && col == rcStart.col)
            {
                _grid.ClearStartRC();
            }
            else
            {
                _grid.SetStartRC(row, col);
            }
        }

        // ���� ���õ� toolbar ��ư�� End Point �� ��� end point ��ǥ�� ������
        else if (_toolbar.btnTool == IDM_TOOLBAR_ENDPOINT)
        {
            RC rcEnd = _grid.GetEndRC();
            if (row == rcEnd.row && col == rcEnd.col)
            {
                _grid.ClearEndRC();
            }
            else
            {
                _grid.SetEndRC(row, col);
            }
        }

        InvalidateRect(_hWnd, NULL, FALSE);
        return;
    }

    // ���콺�� �߰��Ʒ� �׸��⿵�� ũ������ object ���� ���� ��� �ش� object Ŭ�� üũ
    if (_grid.IsMouseOnLeftDownResizeObject(xPos, yPos))
    {
        _mouse.clickResizeVertical = true;
    }
    // ���콺�� �����ʾƷ� �׸��⿵�� ũ������ object ���� ���� ��� �ش� object Ŭ�� üũ
    else if (_grid.IsMouseOnRightDownResizeObject(xPos, yPos))
    {
        _mouse.clickResizeDiagonal = true;
    }
    // ���콺�� �������߰� �׸��⿵�� ũ������ object ���� ���� ��� �ش� object Ŭ�� üũ
    else if (_grid.IsMouseOnRightUpResizeObject(xPos, yPos))
    {
        _mouse.clickResizeHorizontal = true;
    }
}


// ���콺 ���ʹ�ư�� ���� ��
void CWinSearch::MsgProc_LBUTTONUP(WPARAM wParam, LPARAM lParam)
{
    _mouse.clickPainting = false;

    // �׸��念�� ũ������ object�� Ŭ�� ���� �� ���콺�� ������ memory bitmap�� ũ�⿡ �°� �ٽ� �����.
    if (_mouse.clickResizeVertical || _mouse.clickResizeDiagonal || _mouse.clickResizeHorizontal)
    {
        // ���콺 ��ǥ�� �׸��念�� ũ�⸦ ����Ѵ�.
        int xPos = mcGET_MOUSE_X_LPARAM(lParam);
        int yPos = mcGET_MOUSE_Y_LPARAM(lParam);
        POINT ptStart = _grid.GetGridStartPos();
        POINT ptSize = _grid.GetGridSize();
        int newSizeX = min(max(xPos - ptStart.x, GRID_SIZE_MIN_X), GRID_SIZE_MAX_X);
        int newSizeY = min(max(yPos - ptStart.y, GRID_SIZE_MIN_Y), GRID_SIZE_MAX_Y);
        if (_mouse.clickResizeVertical)
            newSizeX = ptSize.x;
        if (_mouse.clickResizeHorizontal)
            newSizeY = ptSize.y;

        // �׸��� �����
        _grid.ResizeGrid(newSizeX, newSizeY); 

        // bitmap �ٽ� �����
        ResizeWindow();
        ReadjustScroll();
        InvalidateRect(_hWnd, NULL, FALSE);
    }
    _mouse.clickResizeVertical = false;
    _mouse.clickResizeHorizontal = false;
    _mouse.clickResizeDiagonal = false;



}


// ���콺�� ������ ��
void CWinSearch::MsgProc_MOUSEMOVE(WPARAM wParam, LPARAM lParam)
{
    _cursor.isChanged = false;
    _cursor.hCurrent = _cursor.hDefault;

    int xPos = mcGET_MOUSE_X_LPARAM(lParam);
    int yPos = mcGET_MOUSE_Y_LPARAM(lParam);
    _mouse.x = xPos;
    _mouse.y = yPos;

    // ���콺�� �׸��⿵���� ���� ���
    if (_grid.IsMouseOnGridArea(xPos, yPos))
    {
        // Ŭ�� ���� ���
        if (_mouse.clickPainting)
        {
            // ���� toolbar�� wall ��ư�� ������ ���� ���
            if (_toolbar.btnTool == IDM_TOOLBAR_WALL)
            {
                int row = mcMOUSE_Y_TO_GRID_ROW(yPos);
                int col = mcMOUSE_X_TO_GRID_COL(xPos);
                // grid�� ���¸� �ٲ۴�.
                if (_mouse.erase == true)
                    _grid.SetGridState(row, col, EGridState::WAY);
                else
                    _grid.SetGridState(row, col, EGridState::WALL);

                InvalidateRect(_hWnd, NULL, FALSE);
            }
            // ���� toolbar�� Bresenham Line ��ư�� ������ ���� ��� WM_PAIN ���� ���� �׸�
            if (_toolbar.btnTool == IDM_TOOLBAR_BRESENHAM_LINE)
            {
                InvalidateRect(_hWnd, NULL, FALSE);
            }
        }
    }


    // ���콺�� �߰��Ʒ� �׸��⿵�� ũ������ object ���� �ְų�, ���콺�� ������ ���� ���
    if (_grid.IsMouseOnLeftDownResizeObject(xPos, yPos) || _mouse.clickResizeVertical == true)
    {
        _cursor.hCurrent = LoadCursor(nullptr, IDC_SIZENS);  // Ŀ�� ����
        _cursor.isChanged = true;
        if (_mouse.clickResizeVertical == true)                    // ���콺�� Ŭ����ä�� �����̸� ȭ�� ����
            InvalidateRect(_hWnd, NULL, FALSE);
    }
    // ���콺�� �����ʾƷ� �׸��⿵�� ũ������ object ���� ���� ���
    else if (_grid.IsMouseOnRightDownResizeObject(xPos, yPos) || _mouse.clickResizeDiagonal == true)
    {
        _cursor.hCurrent = LoadCursor(nullptr, IDC_SIZENWSE);  // Ŀ�� ����
        _cursor.isChanged = true;
        if (_mouse.clickResizeDiagonal == true)                      // ���콺�� Ŭ����ä�� �����̸� ȭ�� ����
            InvalidateRect(_hWnd, NULL, FALSE);
    }
    // ���콺�� �������߰� �׸��⿵�� ũ������ object ���� ���� ���
    else if (_grid.IsMouseOnRightUpResizeObject(xPos, yPos) || _mouse.clickResizeHorizontal == true)
    {
        _cursor.hCurrent = LoadCursor(nullptr, IDC_SIZEWE);  // Ŀ�� ����
        _cursor.isChanged = true;
        if (_mouse.clickResizeHorizontal == true)                 // ���콺�� Ŭ����ä�� �����̸� ȭ�� ����
            InvalidateRect(_hWnd, NULL, FALSE);
    }
}


// ���콺 ���� ���� ��
void CWinSearch::MsgProc_MOUSEWHEEL(WPARAM wParam, LPARAM lParam)
{
    int fwKeys = GET_KEYSTATE_WPARAM(wParam);
    int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    // ���� Ctrl Ű�� ������ �ִٸ� grid ũ������
    if (fwKeys & MK_CONTROL)
    {
        // ���콺 ���� ���� ������ �� grid ũ�� + 1
        if (zDelta > 0)
        {
            // grid ũ�� + 1
            int newCellSize = _grid.GetCellSize() + 1;
            if (newCellSize > GRID_CELL_SIZE_MAX)
                return;

            int newSizeX = newCellSize * _grid.GetNumCols();
            int newSizeY = newCellSize * _grid.GetNumRows();
            _grid.ResizeGrid(newCellSize, newSizeX, newSizeY); // �׸��� �����

            // bitmap �ٽ� �����, ��ũ�ѹ� ����
            ResizeWindow();
            ReadjustScroll();
            InvalidateRect(_hWnd, NULL, FALSE);
        }
        // ���콺 ���� �Ʒ��� ������ �� grid ũ�� - 1
        else if (zDelta < 0)
        {
            // grid ũ�� - 1
            int newCellSize = _grid.GetCellSize() - 1;
            if (newCellSize < GRID_CELL_SIZE_MIN)
                return;

            int newSizeX = newCellSize * _grid.GetNumCols();
            int newSizeY = newCellSize * _grid.GetNumRows();
            _grid.ResizeGrid(newCellSize, newSizeX, newSizeY); // �׸��� �����

            // bitmap �ٽ� �����, ��ũ�ѹ� ����
            ResizeWindow();
            ReadjustScroll();
            InvalidateRect(_hWnd, NULL, FALSE);
        }
    }

    // ���� Ctrl Ű�� ������ ���� �ʴٸ� ���� ��ũ�Ѹ�
    else
    {
        // ���콺 ���� ���� ������ �� ���� ��ũ�Ѹ�
        if (zDelta > 0)
        {
            PostMessage(_hWnd, WM_VSCROLL, (WPARAM)MAKELONG(SB_LINEUP, 0), (LPARAM)NULL);
        }
        // ���콺 ���� ���� ������ �� �Ʒ��� ��ũ�Ѹ�
        else if (zDelta < 0)
        {
            PostMessage(_hWnd, WM_VSCROLL, (WPARAM)MAKELONG(SB_LINEDOWN, 0), (LPARAM)NULL);
        }
    }
}


// â�� ũ�Ⱑ �ٲ� ��
void CWinSearch::MsgProc_SIZE(WPARAM wParam, LPARAM lParam)
{
    // mem bitmap, ��ũ��, toolbar�� ȭ�� ũ�⿡ �°� �ٽ� �����.
    ResizeWindow();
    ReadjustScroll();


    // invalid rect
    InvalidateRect(_hWnd, NULL, FALSE);
    SendMessage(_hToolbar, TB_AUTOSIZE, 0, 0);

}


// Ŀ�� ��ü (���콺�� ������ ������ �����)
LRESULT CWinSearch::MsgProc_SETCURSOR(UINT message, WPARAM wParam, LPARAM lParam)
{
    // Ŀ���� ����Ǿ�� �� ���� custom cursor�� ������
    if (_cursor.isChanged)
    {
        SetCursor(_cursor.hCurrent);
        return 0;
    }
    // �׷��� ������ ������ �⺻ ���ۿ� �ñ�
    else
    {
        return DefWindowProc(_hWnd, message, wParam, lParam);
    }
}


// menu �Ǵ� toolbar Ŭ��
LRESULT CWinSearch::MsgProc_COMMAND(UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);

    switch (wmId)
    {
    case IDM_EXIT:
        DestroyWindow(_hWnd);
        break;

        /* toolbar :: ���α׸��� ��ư Ŭ�� */
    case IDM_TOOLBAR_NEW:
        _grid.ClearGrid();
        _AStarSearch.Clear();
        _jumpPointSearch.Clear();
        InvalidateRect(_hWnd, NULL, FALSE);
        break;

        /* toolbar :: ũ������ ��ư Ŭ�� */
    case IDM_TOOLBAR_SIZE_ADJ:
    {
        // IDD_SIZE_ADJ ���̾�α׸� ����.
        // IDD_SIZE_ADJ ���̾�α� ������ _grid ����ü�� ���� �����Ѵ�.
        bool isOk = (bool)DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SIZE_ADJ), _hWnd, WndProcDialogSizeAdj, (LPARAM)this);
        // ���̾�α׿��� OK�� ������ ��� memory bitmap�� grid�� ���ο� ũ��� �ٽ� �����.
        if (isOk)
        {
            ResizeWindow();
            ReadjustScroll();
            InvalidateRect(_hWnd, NULL, FALSE);
        }
    }
    break;

    /* toolbar :: Wall �׸��� ��ư Ŭ�� */
    case IDM_TOOLBAR_WALL:
        _toolbar.btnTool = IDM_TOOLBAR_WALL;
        break;
        /* toolbar :: StartPoint �׸��� ��ư Ŭ�� */
    case IDM_TOOLBAR_STARTPOINT:
        _toolbar.btnTool = IDM_TOOLBAR_STARTPOINT;
        break;
        /* toolbar :: EndPoint �׸��� ��ư Ŭ�� */
    case IDM_TOOLBAR_ENDPOINT:
        _toolbar.btnTool = IDM_TOOLBAR_ENDPOINT;
        break;
        /* toolbar :: Bresenham Line �׸��� ��ư Ŭ�� */
    case IDM_TOOLBAR_BRESENHAM_LINE:
        _toolbar.btnTool = IDM_TOOLBAR_BRESENHAM_LINE;
        break;


        /* toolbar :: A* Search ��ư Ŭ�� */
    case IDM_TOOLBAR_ASTAR_SEARCH:
        _timer.frequency = 55;
        _timer.frequencyIncrease = 20;
        _toolbar.btnAlgorithm = IDM_TOOLBAR_ASTAR_SEARCH;
        break;
        /* toolbar :: Jump Point Search ��ư Ŭ�� */
    case IDM_TOOLBAR_JUMP_POINT_SEARCH:
        _timer.frequency = 300;
        _timer.frequencyIncrease = 100;
        _toolbar.btnAlgorithm = IDM_TOOLBAR_JUMP_POINT_SEARCH;
        break;


        /* toolbar :: Search ��ư Ŭ�� */
    case IDM_TOOLBAR_SEARCH:
    {
        RC rcStart = _grid.GetStartRC();
        RC rcEnd = _grid.GetEndRC();

        // ������, ������ ���� ���� üũ
        if (rcStart.col < 0
            || rcStart.row < 0
            || rcStart.col >= _grid.GetNumCols()
            || rcStart.row >= _grid.GetNumRows()
            || rcEnd.col < 0
            || rcEnd.row < 0
            || rcEnd.col >= _grid.GetNumCols()
            || rcEnd.row >= _grid.GetNumRows())
        {
            break;
        }

        if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
        {
            // A* search �Ķ���� ����
            _AStarSearch.SetParam(&_grid);

            // step by step ��ã�� ����
            _AStarSearch.SearchStepByStep();
        }
        else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
        {
            // jump point search �Ķ���� ����
            _jumpPointSearch.SetParam(&_grid);

            // step by step ��ã�� ����
            _jumpPointSearch.SearchStepByStep();
        }

        // Ÿ�̸� ����
        SetTimer(_hWnd, _timer.id, max(0, _timer.frequency + _timer.searchSpeedLevel * _timer.frequencyIncrease), NULL);
        InvalidateRect(_hWnd, NULL, FALSE);
        break;
    }

    /* toolbar :: F-Value ��ư Ŭ�� */
    case IDM_TOOLBAR_FVALUE:
    {
        // F-Value ��ư ���� Ȯ��
        LRESULT result = SendMessage(_hToolbar, TB_GETSTATE, (WPARAM)IDM_TOOLBAR_FVALUE, 0);
        // ��ư�� check �������� ���� ���
        _toolbar.btnIsFValueChecked = result & TBSTATE_CHECKED;
        SleepEx(0, TRUE);
        InvalidateRect(_hWnd, NULL, FALSE);
    }
    break;

    /* toolbar :: Speed Up ��ư Ŭ�� */
    case IDM_TOOLBAR_SPEEDUP:
    {
        _timer.searchSpeedLevel--;
        if (_timer.searchSpeedLevel < -3)
            _timer.searchSpeedLevel = -3;

        _timer.bResetTimer = true;
    }
    break;

    /* toolbar :: Speed Down ��ư Ŭ�� */
    case IDM_TOOLBAR_SPEEDDOWN:
    {
        _timer.searchSpeedLevel++;
        if (_timer.searchSpeedLevel > 10)
            _timer.searchSpeedLevel = 10;

        _timer.bResetTimer = true;
    }
    break;

    /* toolbar :: Go to End ��ư Ŭ�� */
    case IDM_TOOLBAR_GOTOEND:
    {
        // Ÿ�̸� kill
        KillTimer(_hWnd, _timer.id);

        // ��ã�Ⱑ ���� ������ ��ã�� ����
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


    /* toolbar :: Make Maze ��ư Ŭ�� */
    case IDM_TOOLBAR_MAKE_MAZE:
    {
        _AStarSearch.Clear();
        _jumpPointSearch.Clear();

        // maze ����
        _grid.MakeMaze();

        // start, end point ���� ����
        _grid.SetStartRCRandom();
        _grid.SetEndRCRandom();

        InvalidateRect(_hWnd, NULL, FALSE);
    }
    break;


    /* toolbar :: Make Cave ��ư Ŭ�� */
    case IDM_TOOLBAR_MAKE_CAVE:
    {
        _AStarSearch.Clear();
        _jumpPointSearch.Clear();

        // cave ����
        _test.numOfCaveWays = _grid.MakeCave();
        // start, end point ����
        if (_test.numOfCaveWays > 1)
        {
            _grid.SetStartRCRandom();
            _grid.SetEndRCRandom();
        }
        else
        {
            _grid.ClearStartRC();
            _grid.ClearEndRC();
        }

        InvalidateRect(_hWnd, NULL, FALSE);
    }
    break;


    /* toolbar :: Make Random ��ư Ŭ�� */
    case IDM_TOOLBAR_MAKE_RANDOM:
    {
        _AStarSearch.Clear();
        _jumpPointSearch.Clear();

        // random wall ����
        int gridNumRows = _grid.GetNumRows();
        int gridNumCols = _grid.GetNumCols();
        RC rcStart = _grid.GetStartRC();
        RC rcEnd = _grid.GetEndRC();
        for (int row = 0; row < gridNumRows; row++)
        {
            for (int col = 0; col < gridNumCols; col++)
            {
                _grid.SetGridState(row, col, rand() > SHRT_MAX / 2 ? EGridState::WAY : EGridState::WALL);
            }
        }

        // start, end point ���� ����
        _grid.SetStartRCRandom();
        _grid.SetEndRCRandom();

        // ��ձ�
        _grid.SetGridState(rcStart.row, rcStart.col, EGridState::WAY);
        _grid.SetGridState(rcEnd.row, rcEnd.col, EGridState::WAY);
        int numOfWayPoint = (int)(pow(log(min(gridNumRows, gridNumCols)), 2.0) / 2.0);
        CBresenhamPath BPath;
        POINT ptStart;
        POINT ptEnd;
        for (int i = 0; i < numOfWayPoint; i++)
        {
            if (i == 0)
            {
                ptStart.x = rcStart.col;
                ptStart.y = rcStart.row;
            }
            else
            {
                ptStart.x = ptEnd.x;
                ptStart.y = ptEnd.y;
            }

            if (i == numOfWayPoint - 1)
            {
                ptEnd.x = rcEnd.col;
                ptEnd.y = rcEnd.row;
            }
            else
            {
                ptEnd.x = rand() % gridNumCols;
                ptEnd.y = rand() % gridNumRows;
                while (ptEnd.x == ptStart.x && ptEnd.y == ptStart.y)
                {
                    ptEnd.x = rand() % gridNumCols;
                    ptEnd.y = rand() % gridNumRows;
                }
            }

            BPath.SetParam(ptStart, ptEnd);
            while (BPath.Next())
            {
                _grid.SetGridState(BPath.GetY(), BPath.GetX(), EGridState::WAY);
            }
        }


        InvalidateRect(_hWnd, NULL, FALSE);
    }
    break;


    /* toolbar :: Run Test ��ư Ŭ�� */
    case IDM_TOOLBAR_RUN_TEST:
        static int sTestCount;                // �׽�Ʈ loop ����� ī��Ʈ
        static int sTestTotalSearchCount;     // ��ã�⸦ ������ Ƚ��
        static int sTestPrevSearchCount;      // ���� �׽�Ʈ �����Ȳ ��� ���� ��ã�� ���� Ƚ��
        static ULONGLONG sTestStartTime;      // �׽�Ʈ ���� �ð�
        static ULONGLONG sTestPrevPrintTime;  // ���� �׽�Ʈ �����Ȳ ��� �ð�
        static LARGE_INTEGER sllFrequency;
        static LARGE_INTEGER sllTotalTime;  // ��ã�⿡ �ҿ�� �� �ð�
        {
            QueryPerformanceFrequency(&sllFrequency);
            sllTotalTime.QuadPart = 0;

            if (_test.isRunning == false)
            {
                _test.isRunning = true;
                _test.isDoRendering = true;
                _test.isPrintInfo = true;
                sTestCount = 0;
                sTestTotalSearchCount = -1;
                sTestPrevSearchCount = 0;

                // �ܼ� ����
                AllocConsole();
                FILE* stream;
                freopen_s(&stream, "CONIN$", "r", stdin);
                freopen_s(&stream, "CONOUT$", "w", stdout);
                freopen_s(&stream, "CONOUT$", "w", stderr);

                // �ֿܼ��� Ű���� �Է��� �����ϴ� ������ ����
                _beginthreadex(NULL, 0, ThreadProcQuitRunTest, (PVOID)this, 0, NULL);
            }

            _test.randomSeed = (unsigned int)time(0);
            srand(_test.randomSeed);

            sTestStartTime = GetTickCount64();
            sTestPrevPrintTime = sTestStartTime;


            // ù �� ����
            PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_MAKE_CAVE, 0), (LPARAM)NULL);

            // IDM_TOOLBAR_RUN_TEST_INTERNAL �޽��� ����
            PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_RUN_TEST_INTERNAL, 0), (LPARAM)NULL);
        }
        break;

        /* toolbar :: Run Test ��ư Ŭ�� �� �߻��Ǵ� �׽�Ʈ ���ѹݺ� �޽��� */
    case IDM_TOOLBAR_RUN_TEST_INTERNAL:
    {
        if (_test.isRunning)
        {
            sTestCount++;
            sTestTotalSearchCount++;

            // 1�ʸ��� ���
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


            // ���� ��� Ȯ��
            bool searchResult = true;
            if (sTestTotalSearchCount != 0)
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

            // �������� ã�� ���ߴٸ� �����̹Ƿ� �׽�Ʈ ����
            if (searchResult == false)
            {
                ReadjustScroll();
                PostMessage(_hWnd, WM_PAINT, (WPARAM)MAKEWPARAM(0, 0), (LPARAM)NULL);
                _test.isRunning = false;
                printf("!!ERROR!! Cannot find the destination.\n");
                printf("Seed : %u, Total search : %d\n", _test.randomSeed, sTestTotalSearchCount);

                break;
            }

            // ���� ������ ������ cave���� way ���� 2���� ���� ��� �ٽ� ����
            if (_test.numOfCaveWays < 2)
            {
                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_MAKE_CAVE, 0), (LPARAM)NULL);
                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_RUN_TEST_INTERNAL, 0), (LPARAM)NULL);
                sTestTotalSearchCount--;
                break;
            }

            // �� ����, �׸��� ũ�� ����
            if (sTestCount == 50)
            {
                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_MAKE_MAZE, 0), (LPARAM)NULL);
                PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_RUN_TEST_INTERNAL, 0), (LPARAM)NULL);
                sTestTotalSearchCount--;
                break;
            }
            else if (sTestCount == 80)
            {
                int cellSize = min(40, _grid.GetCellSize());
                int randomWidth = (rand() % 130 + 20) * cellSize;
                int randomHeight = (rand() % 80 + 20) * cellSize;

                _grid.ResizeGrid(cellSize, randomWidth, randomHeight);
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

            // start, end point ���� ����
            _grid.SetStartRCRandom();
            _grid.SetEndRCRandom();

            // ���� �׽�Ʈ
            //if (sTestTotalSearchCount == 10000)
            //{
            //    _arr2Grid[endRow + 1][endCol + 1] = EGridState::WALL;
            //    _arr2Grid[endRow + 1][endCol] = EGridState::WALL;
            //    _arr2Grid[endRow + 1][endCol - 1] = EGridState::WALL;
            //    _arr2Grid[endRow][endCol + 1] = EGridState::WALL;
            //    _arr2Grid[endRow][endCol - 1] = EGridState::WALL;
            //    _arr2Grid[endRow - 1][endCol + 1] = EGridState::WALL;
            //    _arr2Grid[endRow - 1][endCol] = EGridState::WALL;
            //    _arr2Grid[endRow - 1][endCol - 1] = EGridState::WALL;
            //}

            // search 
            RC rcStart = _grid.GetStartRC();
            RC rcEnd = _grid.GetEndRC();
            LARGE_INTEGER llSearchStartTime;
            LARGE_INTEGER llSearchEndTime;
            QueryPerformanceCounter(&llSearchStartTime);
            if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
            {
                // A* search �Ķ���� ����
                _AStarSearch.SetParam(&_grid);

                // ��ã��
                _AStarSearch.Search();
            }
            else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
            {
                // jump point search �Ķ���� ����
                _jumpPointSearch.SetParam(&_grid);

                // ��ã��
                _jumpPointSearch.Search();
            }
            QueryPerformanceCounter(&llSearchEndTime);
            sllTotalTime.QuadPart += llSearchEndTime.QuadPart - llSearchStartTime.QuadPart;

            // rendering
            if (_test.isDoRendering)
            {
                PostMessage(_hWnd, WM_PAINT, (WPARAM)MAKEWPARAM(0, 0), (LPARAM)NULL);
            }

            // ��� �׽�Ʈ ����
            PostMessage(_hWnd, WM_COMMAND, (WPARAM)MAKEWPARAM(IDM_TOOLBAR_RUN_TEST_INTERNAL, 0), (LPARAM)NULL);
        }

    }
    break;

    default:
        return DefWindowProc(_hWnd, message, wParam, lParam);
    }

    return 0;
}


// Ű���� �Է�
void CWinSearch::MsgProc_KEYDOWN(WPARAM wParam, LPARAM lParam)
{

    switch (wParam)
    {
        // ����Ű�� ������ ��ũ�� �̵�
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


// ���� ��ũ�ѹ� �̺�Ʈ
void CWinSearch::MsgProc_HSCROLL(WPARAM wParam, LPARAM lParam)
{
    // ���� ��ũ�ѹ� ���� ���� ���
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo(_hWnd, SB_HORZ, &si);

    // ��ũ�ѹ� ���� ��ġ ����
    _scroll.hPos = si.nPos;

    // ��ũ�ѹ� �̺�Ʈ ó��
    switch (LOWORD(wParam))
    {
    case SB_LINEUP:  // ��ũ�ѹ� ���� ȭ��ǥ�� ����
        si.nPos -= 15;
        break;

    case SB_LINEDOWN:  // ��ũ�ѹ� ������ ȭ��ǥ�� ����
        si.nPos += 15;
        break;

    case SB_PAGEUP:  // ��ũ�ѹ� ���� shaft�� ����
        si.nPos -= si.nPage;
        break;

    case SB_PAGEDOWN:  // ��ũ�ѹ� ������ shaft�� ����
        si.nPos += si.nPage;
        break;

    case SB_THUMBPOSITION:  // ��ũ�ѹٸ� ��� �巡����
        si.nPos = si.nTrackPos;
        break;
    }
    si.nPos = min(max(0, si.nPos), _scroll.hMax);  // min, max ����

    // ��ũ�ѹ��� ��ġ�� �����ϰ� ������ �ٽ� ����
    si.fMask = SIF_POS;
    SetScrollInfo(_hWnd, SB_HORZ, &si, TRUE);
    GetScrollInfo(_hWnd, SB_HORZ, &si);

    // ���� ��ũ�ѹ��� ��ġ�� ����ƴٸ� ������� ��ũ���� �����Ѵ�.
    if (si.nPos != _scroll.hPos)
    {
        ScrollWindow(_hWnd, 0, _scroll.hPos - si.nPos, NULL, NULL);

        //UpdateWindow(_hWnd);
        _scroll.isRenderFromScrolling = true;  // �������� ��ũ�Ѹ��� ���� ������ ǥ��
        InvalidateRect(_hWnd, NULL, FALSE);
        SendMessage(_hToolbar, TB_AUTOSIZE, 0, 0);
    }
    // ���� ��ũ�ѹ� ��ġ ����
    _scroll.hPos = si.nPos;

}


// ���� ��ũ�ѹ� �̺�Ʈ
void CWinSearch::MsgProc_VSCROLL(WPARAM wParam, LPARAM lParam)
{
    // ���� ��ũ�ѹ� ���� ���� ���
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo(_hWnd, SB_VERT, &si);

    // ��ũ�ѹ� ���� ��ġ ����
    _scroll.vPos = si.nPos;

    // ��ũ�ѹ� �̺�Ʈ ó��
    switch (LOWORD(wParam))
    {
    case SB_LINEUP:  // ��ũ�ѹ� �� ȭ��ǥ�� ����
        si.nPos -= 15;
        break;

    case SB_LINEDOWN:  // ��ũ�ѹ� �Ʒ� ȭ��ǥ�� ����
        si.nPos += 15;
        break;

    case SB_PAGEUP:  // ��ũ�ѹ� �� shaft�� ����
        si.nPos -= si.nPage;
        break;

    case SB_PAGEDOWN:  // ��ũ�ѹ� �Ʒ� shaft�� ����
        si.nPos += si.nPage;
        break;

    case SB_THUMBPOSITION:  // ��ũ�ѹٸ� ��� �巡����
        si.nPos = si.nTrackPos;
        break;
    }
    si.nPos = min(max(0, si.nPos), _scroll.vMax);  // min, max ����

    // ��ũ�ѹ��� ��ġ�� �����ϰ� ������ �ٽ� ����
    si.fMask = SIF_POS;
    SetScrollInfo(_hWnd, SB_VERT, &si, TRUE);
    GetScrollInfo(_hWnd, SB_VERT, &si);

    // ���� ��ũ�ѹ��� ��ġ�� ����ƴٸ� ������� ��ũ���� �����Ѵ�.
    if (si.nPos != _scroll.vPos)
    {
        ScrollWindow(_hWnd, 0, _scroll.vPos - si.nPos, NULL, NULL);
        //UpdateWindow(_hWnd);
        _scroll.isRenderFromScrolling = true;  // �������� ��ũ�Ѹ��� ���� ������ ǥ��
        InvalidateRect(_hWnd, NULL, FALSE);
        SendMessage(_hToolbar, TB_AUTOSIZE, 0, 0);
    }
    // ���� ��ũ�ѹ� ��ġ ����
    _scroll.vPos = si.nPos;
}


// search Ÿ�̸�
void CWinSearch::MsgProc_TIMER(WPARAM wParam, LPARAM lParam)
{
    // speed up �Ǵ� speed down ��ư�� ���� Ÿ�̸Ӹ� �ٽü����ؾ���
    if (_timer.bResetTimer)
    {

        SetTimer(_hWnd, _timer.id, max(0, _timer.frequency + _timer.searchSpeedLevel * _timer.frequencyIncrease), NULL);
        _timer.bResetTimer = false;
    }

    // ��ã�� ���� step ����
    bool isCompleted = false;
    if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
    {
        isCompleted = _AStarSearch.SearchNextStep();
    }
    else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
    {
        isCompleted = _jumpPointSearch.SearchNextStep();
    }


    // �Ϸ�Ǹ� Ÿ�̸� kill
    if (isCompleted)
    {
        KillTimer(_hWnd, _timer.id);
    }

    InvalidateRect(_hWnd, NULL, FALSE);
}


// WM_PAINT
void CWinSearch::MsgProc_PAINT(WPARAM wParam, LPARAM lParam)
{
    POINT ptGridSize = _grid.GetGridSize();
    POINT ptGridStart = _grid.GetGridStartPos();
    int gridCellSize = _grid.GetCellSize();
    int gridNumRows = _grid.GetNumRows();
    int gridNumCols = _grid.GetNumCols();
    RC rcStart = _grid.GetStartRC();
    RC rcEnd = _grid.GetEndRC();

    // ��ũ�Ѹ��� ���� �������� �ƴ� ��� ��ü�� �ٽ� �׸���.
    if (!_scroll.isRenderFromScrolling)
    {
        /* �׸��� ���� �ʱ�ȭ */
        PatBlt(_hMemDC, ptGridStart.x, ptGridStart.y, ptGridSize.x, ptGridSize.y, WHITENESS);


        /* ���� �׸��� */
        SelectObject(_hMemDC, _GDI.hPenGrid);
        for (int col = 0; col <= gridNumCols; col++)
        {
            MoveToEx(_hMemDC, col * gridCellSize + ptGridStart.x, ptGridStart.y, NULL);
            LineTo(_hMemDC, col * gridCellSize + ptGridStart.x, ptGridSize.y + ptGridStart.y);
        }
        for (int row = 0; row <= gridNumRows; row++)
        {
            MoveToEx(_hMemDC, ptGridStart.x, row * gridCellSize + ptGridStart.y, NULL);
            LineTo(_hMemDC, ptGridSize.x + ptGridStart.x, row * gridCellSize + ptGridStart.y);
        }

        /* ��� ���� ��� */
        // A* ��� ���� ���
        auto umapAStarNodeInfo = _AStarSearch.GetNodeInfo();

        // jump point search ��� ���� ���
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



        /* ��ã�� ��� �׸��� */
        // A* search ��� �׸���
        if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
        {
            // A* ��� ���� ���

            // open ���, close ��� ��ĥ�ϱ�
            auto iterNodeInfo = umapAStarNodeInfo.begin();
            for (; iterNodeInfo != umapAStarNodeInfo.end(); ++iterNodeInfo)
            {
                const AStarNode* pNode = iterNodeInfo->second;
                int col = pNode->_x;
                int row = pNode->_y;
                if (col < 0 || col > gridNumCols || row < 0 || row > gridNumRows)
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
                    , col * gridCellSize + ptGridStart.x
                    , row * gridCellSize + ptGridStart.y
                    , (col + 1) * gridCellSize + ptGridStart.x
                    , (row + 1) * gridCellSize + ptGridStart.y);
            }
        }
        // Jump Point Search ��� �׸���
        else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
        {
            // cell ��ĥ
            const unsigned char* const* arr2CellColorGroup = _jumpPointSearch.GetCellColorGroup();
            for (int row = 0; row < _jumpPointSearch.GetGridRows(); row++)
            {
                for (int col = 0; col < _jumpPointSearch.GetGridCols(); col++)
                {
                    if (arr2CellColorGroup[row][col] != 0)
                    {
                        SelectObject(_hMemDC, _GDI.arrHBrushCellColor[(arr2CellColorGroup[row][col] - 1) % NUM_OF_CELL_COLOR_GROUP]);
                        Rectangle(_hMemDC
                            , col * gridCellSize + ptGridStart.x
                            , row * gridCellSize + ptGridStart.y
                            , (col + 1) * gridCellSize + ptGridStart.x
                            , (row + 1) * gridCellSize + ptGridStart.y);
                    }
                }
            }

            // open ���, close ��� ��ĥ�ϱ�
            for (size_t i = 0; i < vecJPSNodeInfo.size(); i++)
            {
                const JPSNode* pNode = vecJPSNodeInfo[i];
                int col = pNode->_x;
                int row = pNode->_y;
                if (col < 0 || col > gridNumCols || row < 0 || row > gridNumRows)
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
                    , col * gridCellSize + ptGridStart.x
                    , row * gridCellSize + ptGridStart.y
                    , (col + 1) * gridCellSize + ptGridStart.x
                    , (row + 1) * gridCellSize + ptGridStart.y);
            }
        }

        /* start point ��ĥ�ϱ� */
        if (rcStart.col >= 0 || rcStart.row >= 0)
        {
            SelectObject(_hMemDC, _GDI.hBrushStartPoint);
            Rectangle(_hMemDC
                , rcStart.col * gridCellSize + ptGridStart.x
                , rcStart.row * gridCellSize + ptGridStart.y
                , (rcStart.col + 1) * gridCellSize + ptGridStart.x
                , (rcStart.row + 1) * gridCellSize + ptGridStart.y);
        }

        /* end point ��ĥ�ϱ� */
        if (rcEnd.col >= 0 || rcEnd.row >= 0)
        {
            SelectObject(_hMemDC, _GDI.hBrushEndPoint);
            Rectangle(_hMemDC
                , rcEnd.col * gridCellSize + ptGridStart.x
                , rcEnd.row * gridCellSize + ptGridStart.y
                , (rcEnd.col + 1) * gridCellSize + ptGridStart.x
                , (rcEnd.row + 1) * gridCellSize + ptGridStart.y);
        }


        /* wall ��ĥ�ϱ� */
        // �ϳ��� row ��ü�� Ŀ���ϴ� wall bitmap�� �����.
        static int sPrevGridCellSize = -1;
        static int sPrevGridNumCols = -1;
        if (gridCellSize != sPrevGridCellSize || gridNumCols != sPrevGridNumCols)
        {
            _hWallBit = CreateCompatibleBitmap(_hDC, gridCellSize * gridNumCols, gridCellSize);
            HBITMAP hWallBit_old = (HBITMAP)SelectObject(_hWallDC, _hWallBit);
            DeleteObject(hWallBit_old);

            SelectObject(_hWallDC, _GDI.hPenGrid);
            SelectObject(_hWallDC, _GDI.hBrushWall);
            for (int col = 0; col < gridNumCols; col++)
            {
                Rectangle(_hWallDC
                    , col * gridCellSize
                    , 0
                    , (col + 1) * gridCellSize
                    , gridCellSize);
            }

            sPrevGridCellSize = gridCellSize;
            sPrevGridNumCols = gridNumCols;
        }

        // ���ӵ� wall�� wall bitmap���� ������ �ٿ��ִ´�.
        int wallStartCol = -1;
        for (int row = 0; row < gridNumRows; row++)
        {
            for (int col = 0; col < gridNumCols; col++)
            {
                if (_grid.GetGridState(row, col) == EGridState::WALL)
                {
                    // col�� ó���� �ƴϰ� col ������ WAY �� ���,
                    // �Ǵ� col�� ó���� ��� wall start
                    if ((col != 0 && _grid.GetGridState(row, col - 1) == EGridState::WAY)
                        || col == 0)
                    {
                        if (wallStartCol == -1)
                            wallStartCol = col;
                    }

                    // col�� �������� �ƴϰ� col ������ WAY �� ���
                    // �Ǵ� col�� �������� ��� wall end, painting
                    if ((col != gridNumCols - 1 && _grid.GetGridState(row, col + 1) == EGridState::WAY)
                        || col == gridNumCols - 1)
                    {
                        BitBlt(_hMemDC
                            , wallStartCol * gridCellSize + ptGridStart.x
                            , row * gridCellSize + ptGridStart.y
                            , (col + 1) * gridCellSize + ptGridStart.x
                            , (row + 1) * gridCellSize + ptGridStart.y
                            , _hWallDC
                            , gridCellSize * (gridNumCols - (col + 1 - wallStartCol))
                            , 0
                            , SRCCOPY);

                        wallStartCol = -1;
                    }
                }

            }
        }


        /* ��ã�� ��� �׸��� */
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
                    , vecPath[i].x * gridCellSize + gridCellSize / 2 + ptGridStart.x
                    , vecPath[i].y * gridCellSize + gridCellSize / 2 + ptGridStart.y, NULL);
                LineTo(_hMemDC
                    , vecPath[i + 1].x * gridCellSize + gridCellSize / 2 + ptGridStart.x
                    , vecPath[i + 1].y * gridCellSize + gridCellSize / 2 + ptGridStart.y);
            }
        }


        /* Bresenham �˰������� ������ ��� �׸��� */
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
                        , vecSmoothPath[i].x * gridCellSize + gridCellSize / 2 + ptGridStart.x
                        , vecSmoothPath[i].y * gridCellSize + gridCellSize / 2 + ptGridStart.y, NULL);
                    LineTo(_hMemDC
                        , vecSmoothPath[i + 1].x * gridCellSize + gridCellSize / 2 + ptGridStart.x
                        , vecSmoothPath[i + 1].y * gridCellSize + gridCellSize / 2 + ptGridStart.y);
                }
            }
        }


        /* F-value, �θ� ���� ȭ��ǥ �׸��� */
        if (_toolbar.btnIsFValueChecked)
        {
            // �ؽ�Ʈ ����, ���ڻ� ����
            SetBkMode(_hMemDC, TRANSPARENT);
            SetTextColor(_hMemDC, RGB(0, 0, 0));

            // F-value font ����
            int FValuefontSize = gridCellSize / 4;
            HFONT hFValueFont = CreateFont(FValuefontSize, 0, 0, 0, 0
                , false, false, false
                , ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY
                , DEFAULT_PITCH | FF_DONTCARE
                , NULL);

            // ȭ��ǥ font ����
            int arrowFontSize = gridCellSize / 2;
            HFONT hArrowFont = CreateFont(arrowFontSize, 0, 0, 0, FW_BOLD
                , false, false, false
                , ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY
                , DEFAULT_PITCH | FF_DONTCARE
                , NULL);

            HFONT hFontOld = (HFONT)SelectObject(_hMemDC, hFValueFont);

            // A* F-value, �θ� ���� ȭ��ǥ �׸���
            if (_toolbar.btnAlgorithm == IDM_TOOLBAR_ASTAR_SEARCH)
            {
                auto iterNodeInfo = umapAStarNodeInfo.begin();
                for (; iterNodeInfo != umapAStarNodeInfo.end(); ++iterNodeInfo)
                {
                    const AStarNode* pNode = iterNodeInfo->second;
                    int col = pNode->_x;
                    int row = pNode->_y;

                    SelectObject(_hMemDC, hFValueFont);

                    // G-value �׸���
                    WCHAR szGValue[10];
                    int lenSzGValue = swprintf_s(szGValue, 10, L"G %.1f", pNode->_valG);
                    TextOut(_hMemDC
                        , col * gridCellSize + ptGridStart.x + 1
                        , row * gridCellSize + ptGridStart.y
                        , szGValue, lenSzGValue);

                    // H-value �׸���
                    WCHAR szHValue[10];
                    int lenSzHValue = swprintf_s(szHValue, 10, L"H %.1f", pNode->_valH);
                    TextOut(_hMemDC
                        , col * gridCellSize + ptGridStart.x + 1
                        , row * gridCellSize + FValuefontSize + ptGridStart.y
                        , szHValue, lenSzHValue);

                    // F-value �׸���
                    WCHAR szFValue[10];
                    int lenSzFValue = swprintf_s(szFValue, 10, L"F %.1f", pNode->_valF);
                    TextOut(_hMemDC
                        , col * gridCellSize + ptGridStart.x + 1
                        , row * gridCellSize + FValuefontSize * 2 + ptGridStart.y
                        , szFValue, lenSzFValue);

                    // �θ� ���� �׸���
                    SelectObject(_hMemDC, hArrowFont);
                    if (pNode->_pParent != nullptr)
                    {
                        WCHAR chArrow;
                        int diffX = pNode->_pParent->_x - pNode->_x;
                        int diffY = pNode->_pParent->_y - pNode->_y;
                        if (diffX > 0)
                        {
                            if (diffY > 0)
                                chArrow = L'��';
                            else if (diffY == 0)
                                chArrow = L'��';
                            else
                                chArrow = L'��';
                        }
                        else if (diffX == 0)
                        {
                            if (diffY > 0)
                                chArrow = L'��';
                            else if (diffY == 0)
                                chArrow = L'?';
                            else
                                chArrow = L'��';
                        }
                        else
                        {
                            if (diffY > 0)
                                chArrow = L'��';
                            else if (diffY == 0)
                                chArrow = L'��';
                            else
                                chArrow = L'��';
                        }

                        TextOut(_hMemDC
                            , (col + 1) * gridCellSize - arrowFontSize + ptGridStart.x
                            , (row + 1) * gridCellSize - arrowFontSize + ptGridStart.y
                            , (LPCWSTR)&chArrow, 1);
                    }
                }
            }

            // Jump Point Search F-value, �θ� ���� ȭ��ǥ �׸���
            else if (_toolbar.btnAlgorithm == IDM_TOOLBAR_JUMP_POINT_SEARCH)
            {
                for (size_t i = 0; i < vecJPSNodeInfo.size(); i++)
                {
                    const JPSNode* pNode = vecJPSNodeInfo[i];
                    int col = pNode->_x;
                    int row = pNode->_y;

                    SelectObject(_hMemDC, hFValueFont);

                    // G-value �׸���
                    WCHAR szGValue[10];
                    int lenSzGValue = swprintf_s(szGValue, 10, L"G %.1f", pNode->_valG);
                    TextOut(_hMemDC
                        , col * gridCellSize + ptGridStart.x + 1
                        , row * gridCellSize + ptGridStart.y
                        , szGValue, lenSzGValue);

                    // H-value �׸���
                    WCHAR szHValue[10];
                    int lenSzHValue = swprintf_s(szHValue, 10, L"H %.1f", pNode->_valH);
                    TextOut(_hMemDC
                        , col * gridCellSize + ptGridStart.x + 1
                        , row * gridCellSize + FValuefontSize + ptGridStart.y
                        , szHValue, lenSzHValue);

                    // F-value �׸���
                    WCHAR szFValue[10];
                    int lenSzFValue = swprintf_s(szFValue, 10, L"F %.1f", pNode->_valF);
                    TextOut(_hMemDC
                        , col * gridCellSize + ptGridStart.x + 1
                        , row * gridCellSize + FValuefontSize * 2 + ptGridStart.y
                        , szFValue, lenSzFValue);

                    // �θ� ���� �׸���
                    SelectObject(_hMemDC, hArrowFont);
                    if (pNode->_pParent != nullptr)
                    {
                        WCHAR chArrow;
                        int diffX = pNode->_pParent->_x - pNode->_x;
                        int diffY = pNode->_pParent->_y - pNode->_y;
                        if (diffX > 0)
                        {
                            if (diffY > 0)
                                chArrow = L'��';
                            else if (diffY == 0)
                                chArrow = L'��';
                            else
                                chArrow = L'��';
                        }
                        else if (diffX == 0)
                        {
                            if (diffY > 0)
                                chArrow = L'��';
                            else if (diffY == 0)
                                chArrow = L'?';
                            else
                                chArrow = L'��';
                        }
                        else
                        {
                            if (diffY > 0)
                                chArrow = L'��';
                            else if (diffY == 0)
                                chArrow = L'��';
                            else
                                chArrow = L'��';
                        }

                        TextOut(_hMemDC
                            , (col + 1) * gridCellSize - arrowFontSize + ptGridStart.x
                            , (row + 1) * gridCellSize - arrowFontSize + ptGridStart.y
                            , (LPCWSTR)&chArrow, 1);
                    }
                }
            }

            SelectObject(_hMemDC, hFontOld);
            DeleteObject(hFValueFont);
            DeleteObject(hArrowFont);
        }  // end of F-value, �θ� ���� ȭ��ǥ �׸���


        /* Bresenham Line �׸��� */
        // Bresenham Line ��ư�� ������ �ְ�, ���� ���콺�� �巡�� ���� ��� ���� �׸�
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
                , startCol * gridCellSize + ptGridStart.x
                , startRow * gridCellSize + ptGridStart.y
                , (startCol + 1) * gridCellSize + ptGridStart.x
                , (startRow + 1) * gridCellSize + ptGridStart.y);

            CBresenhamPath BPath;
            BPath.SetParam(POINT{ startCol, startRow }, POINT{ endCol, endRow });
            while (BPath.Next())
            {
                Rectangle(_hMemDC
                    , BPath.GetX() * gridCellSize + ptGridStart.x
                    , BPath.GetY() * gridCellSize + ptGridStart.y
                    , (BPath.GetX() + 1) * gridCellSize + ptGridStart.x
                    , (BPath.GetY() + 1) * gridCellSize + ptGridStart.y);
            }
        }





        /* �׸��⿵���� ������ memDC ���� �ʱ�ȭ */
        // memDC �ʱ�ȭ�� �̰����� �ϴ� ������ �׸��⿵�� ������ �������� �׷��� �͵��� ���ֱ� ���ؼ���.
        SelectObject(_hMemDC, _GDI.hBrushBackground);
        PatBlt(_hMemDC, 0, 0, _ptMemBitSize.x, ptGridStart.y, PATCOPY);
        PatBlt(_hMemDC, 0, 0, ptGridStart.x, _ptMemBitSize.y, PATCOPY);
        PatBlt(_hMemDC, ptGridStart.x + ptGridSize.x, 0, _ptMemBitSize.x, _ptMemBitSize.y, PATCOPY);
        PatBlt(_hMemDC, 0, ptGridStart.y + ptGridSize.y, _ptMemBitSize.x, _ptMemBitSize.y, PATCOPY);


        /* Mem DC�� �׶��̼� �׸��� �׸��� */
        for (int i = 0; i < SHADOW_STEP; i++)
        {
            SelectObject(_hMemDC, _GDI.arrHPenShadow[i]);

            MoveToEx(_hMemDC, ptGridStart.x + ptGridSize.x + i, ptGridStart.y + 10, NULL);
            LineTo(_hMemDC, ptGridStart.x + ptGridSize.x + i, ptGridStart.y + ptGridSize.y + SHADOW_STEP);
            MoveToEx(_hMemDC, ptGridStart.x + 10, ptGridStart.y + ptGridSize.y + i, NULL);
            LineTo(_hMemDC, ptGridStart.x + ptGridSize.x + SHADOW_STEP, ptGridStart.y + ptGridSize.y + i);
        }


        /* �׸��⿵�� ũ������ object �׸��� */
        {
            SelectObject(_hMemDC, _GDI.hPenResizeObject);
            SelectObject(_hMemDC, _GDI.hBrushResizeObject);
            int left[3] = { ptGridStart.x + ptGridSize.x / 2 - 2, ptGridStart.x + ptGridSize.x, ptGridStart.x + ptGridSize.x };
            int top[3] = { ptGridStart.y + ptGridSize.y, ptGridStart.y + ptGridSize.y, ptGridStart.y + ptGridSize.y / 2 - 2 };
            for (int i = 0; i < 3; i++)
            {
                Rectangle(_hMemDC, left[i], top[i], left[i] + 5, top[i] + 5);
            }
        }


        /* �׸��⿵�� ũ������ object�� Ŭ������ �� ũ������ �׵θ� �׸��� */
        if (_mouse.clickResizeVertical || _mouse.clickResizeDiagonal || _mouse.clickResizeHorizontal)
        {
            SelectObject(_hMemDC, _GDI.hPenResizeBorder);
            SelectObject(_hMemDC, _GDI.hBrushResizeBorder);

            // ���� �׸��⿵�� ũ������ object�� Ŭ������ �� ũ������ �׵θ� �׸���
            if (_mouse.clickResizeVertical)
            {
                Rectangle(_hMemDC
                    , ptGridStart.x
                    , ptGridStart.y
                    , ptGridStart.x + ptGridSize.x
                    , max(_mouse.y, ptGridStart.y + GRID_SIZE_MIN_Y));
            }
            // �밢 �׸��⿵�� ũ������ object�� Ŭ������ �� ũ������ �׵θ� �׸���
            else if (_mouse.clickResizeDiagonal)
            {
                Rectangle(_hMemDC
                    , ptGridStart.x
                    , ptGridStart.y
                    , max(_mouse.x, ptGridStart.x + GRID_SIZE_MIN_X)
                    , max(_mouse.y, ptGridStart.y + GRID_SIZE_MIN_Y));
            }
            // ���� �׸��⿵�� ũ������ object�� Ŭ������ �� ũ������ �׵θ� �׸���
            else if (_mouse.clickResizeHorizontal)
            {
                Rectangle(_hMemDC
                    , ptGridStart.x
                    , ptGridStart.y
                    , max(_mouse.x, ptGridStart.x + GRID_SIZE_MIN_X)
                    , ptGridStart.y + ptGridSize.y);
            }
        }


        /* �׸��� row, col �� ǥ�ù��� ��� */
        SelectObject(_hMemDC, _GDI.hFontGridInfo);
        SetBkMode(_hMemDC, TRANSPARENT);
        SetTextColor(_hMemDC, RGB(0, 0, 0));

        WCHAR szGridInfo[30];
        int lenSzGridInfo = swprintf_s(szGridInfo, 30, L"%d x %d cells", gridNumCols, gridNumRows);
        TextOut(_hMemDC
            , ptGridStart.x
            , ptGridStart.y - 15
            , szGridInfo, lenSzGridInfo);


        /* �׽�Ʈ �������� �� ���� �ؽ�Ʈ ��� */
        WCHAR szTestInfo[150];
        int lenSzTestInfo;
        if (_test.isRunning || _test.isPrintInfo)
        {
            lenSzTestInfo = swprintf_s(szTestInfo, 150, L"Test is running...  seed: %u.  Press 'Q' on the console window to quit the test or 'R' to stop rendering.\n", _test.randomSeed);
            TextOut(_hMemDC
                , ptGridStart.x + 100
                , ptGridStart.y - 15
                , szTestInfo, lenSzTestInfo);

            if (_test.isRunning == false && _test.isPrintInfo == true)
                _test.isPrintInfo = false;
        }


        // begin paint
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(_hWnd, &ps);

        /* MemDC���� MainDC�� bit block transfer */
        BitBlt(_hDC
            , 0, _toolbar.height
            , _ptMemBitSize.x, _ptMemBitSize.y
            , _hMemDC, _scroll.hPos, _toolbar.height + _scroll.vPos, SRCCOPY);
        EndPaint(_hWnd, &ps);

    } // end of if(!_scroll.isRenderFromScrolling)


    // ��ũ�Ѹ��� ���� �������� ��� BitBlt�� �ٽ� �Ѵ�.
    else
    {
        _scroll.isRenderFromScrolling = false;

        // begin paint
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(_hWnd, &ps);

        /* MemDC���� MainDC�� bit block transfer */
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

    PostQuitMessage(0);     // �� �۾��� ���� ������ �����츸 ������ ���μ����� ������� �ʰ� �ȴ�.
}














// ũ�� ���� ���̾�α�
INT_PTR CALLBACK CWinSearch::WndProcDialogSizeAdj(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CWinSearch* pThis = reinterpret_cast<CWinSearch*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
    switch (message)
    {
    case WM_INITDIALOG:
    {
        pThis = reinterpret_cast<CWinSearch*>(lParam);

        SetLastError(0);
        if (!SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis)))
        {
            if (GetLastError() != 0)
            {
                std::string msg = std::string("Failed to set 'this' pointer to DialogSizeAdj's USERDATA. system error : ") + std::to_string(GetLastError()) + "\n";
                throw std::runtime_error(msg.c_str());
                return FALSE;
            }
        }

        SetDlgItemInt(hDlg, IDC_SIZE_ADJ_SIZE, pThis->_grid.GetCellSize(), TRUE);
        POINT ptGridSize = pThis->_grid.GetGridSize();
        SetDlgItemInt(hDlg, IDC_SIZE_ADJ_WIDTH, ptGridSize.x, TRUE);
        SetDlgItemInt(hDlg, IDC_SIZE_ADJ_HEIGHT, ptGridSize.y, TRUE);
    }
    return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch LOWORD(wParam)
        {
        case IDOK:
        {
            // OK�� ������ ��� _grid �� ũ�� ���� �����Ѵ�.
            BOOL translated;
            int cellSize = (int)GetDlgItemInt(hDlg, IDC_SIZE_ADJ_SIZE, &translated, TRUE);
            if (!translated)
                break;

            int width = (int)GetDlgItemInt(hDlg, IDC_SIZE_ADJ_WIDTH, &translated, TRUE);
            if (!translated)
                break;

            int height = (int)GetDlgItemInt(hDlg, IDC_SIZE_ADJ_HEIGHT, &translated, TRUE);
            if (!translated)
                break;

            pThis->_grid.ResizeGrid(cellSize, width, height);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)FALSE;
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
    // ����� ��� �߿� visual studio ��� â�� �޽��� �� ����ϱ�
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

    /* ���콺 ���� Ŭ�� �� */
    case WM_LBUTTONDOWN:
        pThis->MsgProc_LBUTTONDOWN(wParam, lParam);
        break;
    /* ���콺 ���ʹ�ư�� ���� �� */
    case WM_LBUTTONUP:
        pThis->MsgProc_LBUTTONUP(wParam, lParam);
        break;
    /* ���콺�� ������ �� */
    case WM_MOUSEMOVE:
        pThis->MsgProc_MOUSEMOVE(wParam, lParam);
        break;
    /* ���콺 ���� ���� �� */
    case WM_MOUSEWHEEL:
        pThis->MsgProc_MOUSEWHEEL(wParam, lParam);
        break;
    /* â�� ũ�Ⱑ �ٲ� �� */
    case WM_SIZE:
        pThis->MsgProc_SIZE(wParam, lParam);
        break;
    /* Ŀ�� ��ü (���콺�� ������ ������ �����) */
    case WM_SETCURSOR:
        return pThis->MsgProc_SETCURSOR(message, wParam, lParam);
    /* menu �Ǵ� toolbar Ŭ�� */
    case WM_COMMAND:
        return pThis->MsgProc_COMMAND(message, wParam, lParam);
    /* Ű���� �Է� */
    case WM_KEYDOWN:
        pThis->MsgProc_KEYDOWN(wParam, lParam);
        break;
    /* ���� ��ũ�ѹ� �̺�Ʈ */
    case WM_HSCROLL:
        pThis->MsgProc_HSCROLL(wParam, lParam);
        break;
    /* ���� ��ũ�ѹ� �̺�Ʈ */
    case WM_VSCROLL:
        pThis->MsgProc_VSCROLL(wParam, lParam);
        break;
    /* search Ÿ�̸� */
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


