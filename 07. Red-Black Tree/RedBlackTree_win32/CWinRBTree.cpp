#include "CWinRBTree.h"

#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>
#include <stdexcept>

using namespace rbtree;


CWinRBTree::CWinRBTree(_In_ HINSTANCE hInst, _In_ int nCmdShow)
    : _hInst(hInst)
    , _nCmdShow(nCmdShow)
    , _szTitle{ 0, }
    , _szWindowClass{ 0, }
    , _hWnd(0)
    , _hDC(0)
    , _hMemBit(0)
    , _hMemDC(0)
    , _GDI{ 0, }
    , _rtClientSize{ 0, }
    , _ptMemBitSize{ 0, }
    , _nodeFullSize(50.f)
    , _action()
    , _scroll{ 0, }
{
    _nodeFullSize = 50.f;
}

CWinRBTree::~CWinRBTree()
{
    Close();
}


int CWinRBTree::Run()
{
    // 전역 문자열을 초기화합니다.
    LoadStringW(_hInst, IDS_APP_TITLE, _szTitle, MAX_LOADSTRING);
    LoadStringW(_hInst, IDC_MY0530REDBLACKTREEWIN32, _szWindowClass, MAX_LOADSTRING);
    ATOM retResister = MyRegisterClass(_hInst);
    if (retResister == 0)
    {
        std::string msg = std::string("MyRegisterClass Failed. system error:") + std::to_string(GetLastError()) + "\n";
        throw std::runtime_error(msg.c_str());
        return FALSE;
    }

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance(_hInst, _nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(_hInst, MAKEINTRESOURCE(IDC_MY0530REDBLACKTREEWIN32));

    MSG msg;

    // 기본 메시지 루프입니다:
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

void CWinRBTree::Close()
{
    if(_bClosed == false)
        DestroyWindow(_hWnd);
}


//  함수: MyRegisterClass()
//  용도: 창 클래스를 등록합니다.
ATOM CWinRBTree::MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { 0, };

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_MY0530REDBLACKTREEWIN32));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY0530REDBLACKTREEWIN32);
    wcex.lpszClassName = _szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}


//   함수: InitInstance(HINSTANCE, int)
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//   주석:
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
BOOL CWinRBTree::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd = CreateWindowW(_szWindowClass, _szTitle, WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, (LPVOID)this);
    _hWnd = hWnd;

    if (!hWnd)
    {
        std::string msg = std::string("CreateWindowEx Failed. system error:") + std::to_string(GetLastError()) + "\n";
        throw std::runtime_error(msg.c_str());
        return FALSE;
    }

    // 이중버퍼링 DC, 비트맵 생성
    _hDC = GetDC(_hWnd);
    GetClientRect(_hWnd, &_rtClientSize);
    _ptMemBitSize.x = _rtClientSize.right;
    _ptMemBitSize.y = _rtClientSize.bottom;
    _hMemBit = CreateCompatibleBitmap(_hDC, _ptMemBitSize.x, _ptMemBitSize.y);
    _hMemDC = CreateCompatibleDC(_hDC);
    HBITMAP hOldBit = (HBITMAP)SelectObject(_hMemDC, _hMemBit);
    DeleteObject(hOldBit);
    PatBlt(_hMemDC, 0, 0, _ptMemBitSize.x, _ptMemBitSize.y, WHITENESS);  // memDC를 흰색으로 초기화
    ReadjustScroll();

    // GDI 생성
    _GDI.hPenLine = CreatePen(PS_SOLID, 1, RGB(50, 50, 50));
    _GDI.hPenRedNode = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    _GDI.hPenBlackNode = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    _GDI.hPenRedNode = (HPEN)GetStockObject(NULL_PEN);
    _GDI.hPenBlackNode = (HPEN)GetStockObject(NULL_PEN);
    _GDI.hBrushRedNode = CreateSolidBrush(RGB(255, 0, 0));
    _GDI.hBrushBlackNode = CreateSolidBrush(RGB(0, 0, 0));
    _GDI.hBrushBackground = CreateSolidBrush(RGB(199, 209, 225));
    _GDI.hPenOld = (HPEN)SelectObject(_hMemDC, _GDI.hPenRedNode);
    _GDI.hBrushOld = (HBRUSH)SelectObject(_hMemDC, _GDI.hBrushRedNode);

    // random seed
    srand(time(0));

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}





/* utils::memory bitmap 크기를 조정한다. */
void CWinRBTree::ResizeWindow()
{
    HBITMAP hNewBit = CreateCompatibleBitmap(_hMemDC, _ptMemBitSize.x, _ptMemBitSize.y);
    SelectObject(_hMemDC, hNewBit);
    DeleteObject(_hMemBit);
    _hMemBit = hNewBit;
}


/* utils::스크롤을 재조정한다. */
void CWinRBTree::ReadjustScroll()
{
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


/* tree 그리기  :: tree 그리기 */
void CWinRBTree::DrawRBTree(CRedBlackTree<int, int>& tree)
{
    // root 노드 얻기
    auto iterRoot = tree.GetRoot();
    if (iterRoot.IsNil())
        return;

    // 전체 leaf 수 계산하기
    int numOfLeaf = _getNumOfLeaf(iterRoot);

    // 노드 그림 크기 계산하기
    float nodeSize = _nodeFullSize * 0.8f;
    float nodeSpace = _nodeFullSize * 0.2f;

    // bitmap 크기를 tree 너비에 맞추기
    int minTreeWidth = (int)(_nodeFullSize * (float)numOfLeaf);
    int maxTreeWidth = (int)(_nodeFullSize * (float)numOfLeaf * 1.2f);
    int minTreeHeight = (int)(_nodeFullSize * 2.0 * log(tree.Size() + 1));
    int maxTreeHeight = (int)(_nodeFullSize * 2.0 * log(tree.Size() + 1) * 1.2);
    if (_ptMemBitSize.x < minTreeWidth || _ptMemBitSize.x > maxTreeWidth
        || _ptMemBitSize.y < minTreeHeight || _ptMemBitSize.y > maxTreeHeight)
    {
        if (_ptMemBitSize.x != max(_rtClientSize.right, maxTreeWidth)
            || _ptMemBitSize.y != max(_rtClientSize.bottom, maxTreeHeight))
        {
            _ptMemBitSize.x = max(_rtClientSize.right, maxTreeWidth);
            _ptMemBitSize.y = max(_rtClientSize.bottom, maxTreeHeight);

            ResizeWindow();
            SelectObject(_hMemDC, _GDI.hBrushBackground);
            PatBlt(_hMemDC, 0, 0, _ptMemBitSize.x, _ptMemBitSize.y, PATCOPY);
            ReadjustScroll();
        }
    }


    // 노드 그리기
    std::map<int, Point> mapPos;
    Point pos = { (float)_ptMemBitSize.x / 2.f, 0.f };
    _drawNode(iterRoot, mapPos, pos, nodeSize, nodeSpace);
}



/* tree 그리기 :: 노드 그리기
@parameters
iter: 그리기 대상 노드, mapPos: 지금까지 그린 노드들의 좌표를 저장해두는 map,
posParent: 부모 좌표, nodeSize: 노드 그림 크기, nodeSpace: 노드 그림 사이의 간격 */
void CWinRBTree::_drawNode(const CRedBlackTree<int, int>::iterator& iter
    , std::map<int, Point>& mapPos, Point posParent
    , float nodeSize, float nodeSpace)
{
    /* Nil 일 경우 return */
    if (iter.IsNil())
        return;

    /* 자신 왼쪽 subtree에서 Nil 수 구하기 */
    float leftNumOfNil = (float)_getNumOfNil(iter.GetLeftChild());
    leftNumOfNil = max(1.f, leftNumOfNil);
    /* 자신 오른쪽 subtree에서 Nil 수 구하기 */
    float rightNumOfNil = (float)_getNumOfNil(iter.GetRightChild());
    rightNumOfNil = max(1.f, rightNumOfNil);


    /* 자신의 왼쪽 경계 x좌표 계산하기 */
    float leftBoundaryX;
    // root 라면 화면 왼쪽 끝이 경계
    if (iter.IsRoot())
    {
        leftBoundaryX = 0.f;
    }
    // 왼쪽 자식이라면 조상 중 오른쪽 자식인 것의 부모의 x 좌표가 왼쪽 경계임
    // 조상 중 오른쪽 자식을 찾지 못하면 화면 왼쪽 끝이 경계
    else if (iter.IsLeftChild())
    {
        auto iterParent = iter.GetParent();
        while (true)
        {
            if (iterParent.IsRoot())
            {
                leftBoundaryX = 0.f;
                break;
            }
            if (iterParent.IsRightChild())
            {
                iterParent = iterParent.GetParent();
                auto iterPos = mapPos.find((*iterParent).first);
                leftBoundaryX = (*iterPos).second.x;
                break;
            }
            iterParent = iterParent.GetParent();
        }
    }
    // 오른쪽 자식이라면 부모의 x좌표가 왼쪽 경계임
    else
    {
        leftBoundaryX = posParent.x;
    }
    // 왼쪽 경계의 최소값은 왼쪽 subtree의 Nil 수에 따라 조정됨
    leftBoundaryX = max(leftBoundaryX, posParent.x - leftNumOfNil * (nodeSize + nodeSpace * 2));


    /* 자신의 오른쪽 경계 x좌표 계산하기 */
    float rightBoundaryX;
    // root 라면 화면 오른쪽 끝이 경계
    if (iter.IsRoot())
    {
        rightBoundaryX = (float)_ptMemBitSize.x;
    }
    // 오른쪽 자식이라면 조상 중 왼쪽 자식인 것의 부모의 x 좌표가 오른쪽 경계임
    // 조상 중 왼쪽 자식을 찾지 못하면 화면 오른쪽 끝이 경계
    else if (iter.IsRightChild())
    {
        auto iterParent = iter.GetParent();
        while (true)
        {
            if (iterParent.IsRoot())
            {
                rightBoundaryX = (float)_ptMemBitSize.x;
                break;
            }
            if (iterParent.IsLeftChild())
            {
                iterParent = iterParent.GetParent();
                auto iterPos = mapPos.find((*iterParent).first);
                rightBoundaryX = (*iterPos).second.x;
                break;
            }
            iterParent = iterParent.GetParent();
        }
    }
    // 왼쪽 자식이라면 부모의 x좌표가 오른쪽 경계임
    else
    {
        rightBoundaryX = posParent.x;
    }
    // 오른쪽 경계의 최대값은 오른쪽 subtree의 Nil 수에 따라 조정됨
    rightBoundaryX = min(rightBoundaryX, posParent.x + rightNumOfNil * (nodeSize + nodeSpace * 2));




    /* 자신의 위치 계산하기. x 좌표는 왼쪽경계와 오른쪽경계 사이에서 계산됨 */
    Point pos;
    pos.x = leftBoundaryX + (rightBoundaryX - leftBoundaryX) * (leftNumOfNil / (leftNumOfNil + rightNumOfNil));
    pos.y = posParent.y + nodeSize + nodeSpace * 2.f;
    // 위치를 map에 저장해둠
    mapPos.insert(std::make_pair((*iter).first, pos));


    /* 노드 색에 따른 GDI 변경 */
    if (iter.IsBlack())
    {
        SelectObject(_hMemDC, _GDI.hPenBlackNode);
        SelectObject(_hMemDC, _GDI.hBrushBlackNode);
    }
    else
    {
        SelectObject(_hMemDC, _GDI.hPenRedNode);
        SelectObject(_hMemDC, _GDI.hBrushRedNode);
    }


    /* 그리기 */
    // 노드 그리기
    Ellipse(_hMemDC
        , (int)(pos.x - nodeSize / 2.f)
        , (int)(pos.y - nodeSize / 2.f)
        , (int)(pos.x + nodeSize / 2.f)
        , (int)(pos.y + nodeSize / 2.f));

    // 값 텍스트 그리기
    RECT rtText = { (int)(pos.x - nodeSize / 2.f)
        , (int)(pos.y - nodeSize / 2.f)
        , (int)(pos.x + nodeSize / 2.f)
        , (int)(pos.y + nodeSize / 2.f) };
    SetBkMode(_hMemDC, TRANSPARENT);
    SetTextColor(_hMemDC, RGB(255, 255, 255));
    WCHAR szValue[10];
    swprintf_s(szValue, 10, L"%d", (*iter).first);
    DrawText(_hMemDC, szValue, lstrlen(szValue), &rtText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

    // 선 그리기
    if (!iter.IsRoot())
    {
        SelectObject(_hMemDC, _GDI.hPenLine);
        MoveToEx(_hMemDC, (int)posParent.x, (int)(posParent.y + nodeSize / 2.f), NULL);
        LineTo(_hMemDC, (int)pos.x, (int)(pos.y - nodeSize / 2.f));
    }

    // 다음 노드 그리기 
    _drawNode(iter.GetLeftChild(), mapPos, pos, nodeSize, nodeSpace);
    _drawNode(iter.GetRightChild(), mapPos, pos, nodeSize, nodeSpace);
}


/* tree 그리기 :: 현재 노드 아래의 Nil 수 구하기 */
int CWinRBTree::_getNumOfNil(const CRedBlackTree<int, int>::iterator& iter)
{
    int numOfNil = 0;

    if (iter.IsNil())
    {
        return 1;
    }
    else
    {
        numOfNil += _getNumOfNil(iter.GetLeftChild());
        numOfNil += _getNumOfNil(iter.GetRightChild());
    }

    return numOfNil;
}


/* tree 그리기 :: 현재 노드 아래의 leaf(Nil아님) 수 구하기 */
int CWinRBTree::_getNumOfLeaf(const CRedBlackTree<int, int>::iterator& iter)
{
    int numOfLeaf = 0;

    if (iter.IsLeaf())
    {
        return 1;
    }
    else if (iter.IsNil())
    {
        return 0;
    }
    else
    {
        numOfLeaf += _getNumOfLeaf(iter.GetLeftChild());
        numOfLeaf += _getNumOfLeaf(iter.GetRightChild());
    }

    return numOfLeaf;
}




/* message processing */
void CWinRBTree::MsgProc_KEYDOWN(WPARAM wParam, LPARAM lParam)
{
    // tree에 insert
    if (wParam == 'I')
    {
        while (true)
        {
            if (_rbTree.Size() == 2000)
                break;

            int rInt = rand() % 10000;
            //static int rInt = 0;
            //rInt++;
            bool isInsert = _rbTree.Insert(rInt, rInt);
            if (isInsert == true)
            {
                _action.vecValues.push_back(rInt);
                _action.isInsert = true;
                _action.isDelete = false;
                _action.value = rInt;
                InvalidateRect(_hWnd, NULL, FALSE);
                break;
            }
        }
    }
    // tree에서 delete
    else if (wParam == 'D')
    {
        if (_action.vecValues.size() == 0)
            return;

        auto rng = std::default_random_engine{};
        std::shuffle(std::begin(_action.vecValues), std::end(_action.vecValues), rng);
        int key = _action.vecValues.back();
        _action.vecValues.pop_back();

        auto iter = _rbTree.Find(key);
        _rbTree.Erase(iter);
        _action.isInsert = false;
        _action.isDelete = true;
        _action.value = key;
        InvalidateRect(_hWnd, NULL, FALSE);
        return;
    }
}

void CWinRBTree::MsgProc_MOUSEWHEEL(WPARAM wParam, LPARAM lParam)
{
    int fwKeys = GET_KEYSTATE_WPARAM(wParam);
    int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    // 현재 Ctrl 키가 눌려져 있다면 노드 크기조정
    if (fwKeys & MK_CONTROL)
    {
        // 마우스 휠을 위로 굴렸을 때 노드 크기 +
        if (zDelta > 0)
        {
            _nodeFullSize += 2.f;
            InvalidateRect(_hWnd, NULL, FALSE);
        }
        // 마우스 휠을 아래로 굴렸을 때 노드 크기 -
        else if (zDelta < 0)
        {
            _nodeFullSize -= 2.f;
            InvalidateRect(_hWnd, NULL, FALSE);
        }
    }
}


void CWinRBTree::MsgProc_SIZE(WPARAM wParam, LPARAM lParam)
{
    GetClientRect(_hWnd, &_rtClientSize);
    if (_ptMemBitSize.x < _rtClientSize.right
        || _ptMemBitSize.y < _rtClientSize.bottom)
    {
        _ptMemBitSize.x = _rtClientSize.right;
        _ptMemBitSize.y = _rtClientSize.bottom;
        ResizeWindow();
        ReadjustScroll();
        InvalidateRect(_hWnd, NULL, FALSE);
    }
    else
    {
        ReadjustScroll();
    }
}


void CWinRBTree::MsgProc_HSCROLL(WPARAM wParam, LPARAM lParam)
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
        _scroll.isRenderFromScrolling = true;  // 렌더링이 스크롤링에 의한 것임을 표시
        InvalidateRect(_hWnd, NULL, FALSE);
    }
    // 현재 스크롤바 위치 갱신
    _scroll.hPos = si.nPos;
}


void CWinRBTree::MsgProc_VSCROLL(WPARAM wParam, LPARAM lParam)
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
        _scroll.isRenderFromScrolling = true;  // 렌더링이 스크롤링에 의한 것임을 표시
        InvalidateRect(_hWnd, NULL, FALSE);
    }
    // 현재 스크롤바 위치 갱신
    _scroll.vPos = si.nPos;
}


void CWinRBTree::MsgProc_PAINT(WPARAM wParam, LPARAM lParam)
{

    // memDC 배경색 초기화
    SelectObject(_hMemDC, _GDI.hBrushBackground);
    PatBlt(_hMemDC, 0, 0, _ptMemBitSize.x, _ptMemBitSize.y, PATCOPY);

    // tree 그리기
    DrawRBTree(_rbTree);


    // insert, delete 텍스트 그리기
    SetBkMode(_hMemDC, TRANSPARENT);
    SetTextColor(_hMemDC, RGB(0, 0, 0));

    RECT rtUI = { 10 + _scroll.hPos, 10 + _scroll.vPos, 100 + _scroll.hPos, 30 + _scroll.vPos };
    WCHAR szUI[] = L"Insert: I, Delete: D";
    DrawText(_hMemDC, szUI, lstrlen(szUI), &rtUI, DT_LEFT | DT_NOCLIP);

    RECT rtInsert = { 10 + _scroll.hPos, 30 + _scroll.vPos, 100 + _scroll.hPos, 50 + _scroll.vPos };
    RECT rtDelete = { 10 + _scroll.hPos, 50 + _scroll.vPos, 100 + _scroll.hPos, 70 + _scroll.vPos };
    WCHAR szInsert[50];
    WCHAR szDelete[50];
    if (_action.isInsert)
    {
        swprintf_s(szInsert, 50, L"Inserted: %d", _action.value);
        swprintf_s(szDelete, 50, L"Deleted:");
    }
    else if (_action.isDelete)
    {
        swprintf_s(szInsert, 50, L"Inserted:");
        swprintf_s(szDelete, 50, L"Deleted: %d", _action.value);
    }
    else
    {
        swprintf_s(szInsert, 50, L"Inserted:");
        swprintf_s(szDelete, 50, L"Deleted:");
    }
    DrawText(_hMemDC, szInsert, lstrlen(szInsert), &rtInsert, DT_LEFT | DT_NOCLIP);
    DrawText(_hMemDC, szDelete, lstrlen(szDelete), &rtDelete, DT_LEFT | DT_NOCLIP);

    RECT rtNumNodes = { 10 + _scroll.hPos, 70 + _scroll.vPos, 100 + _scroll.hPos, 90 + _scroll.vPos };
    WCHAR szNumNodes[50];
    swprintf_s(szNumNodes, 50, L"number of nodes: %d", (int)_rbTree.Size());
    DrawText(_hMemDC, szNumNodes, lstrlen(szNumNodes), &rtNumNodes, DT_LEFT | DT_NOCLIP);


    // BitBlt
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(_hWnd, &ps);

    BitBlt(_hDC, 0, 0
        , _ptMemBitSize.x, _ptMemBitSize.y
        , _hMemDC, _scroll.hPos, _scroll.vPos, SRCCOPY);

    EndPaint(_hWnd, &ps);



}


void CWinRBTree::MsgProc_DESTROY(WPARAM wParam, LPARAM lParam)
{
    if (_bClosed)
        return;
    _bClosed = true;

    DeleteObject(_GDI.hPenLine);
    DeleteObject(_GDI.hPenRedNode);
    DeleteObject(_GDI.hPenBlackNode);
    DeleteObject(_GDI.hPenRedNode);
    DeleteObject(_GDI.hPenBlackNode);
    DeleteObject(_GDI.hBrushRedNode);
    DeleteObject(_GDI.hBrushBlackNode);
    DeleteObject(_GDI.hBrushBackground);
    DeleteObject(_GDI.hPenOld);
    DeleteObject(_GDI.hBrushOld);

    ReleaseDC(_hWnd, _hDC);
    DeleteDC(_hMemDC);
    DeleteObject(_hMemBit);
    PostQuitMessage(0);
}






// windows procedure
LRESULT CALLBACK CWinRBTree::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CWinRBTree* pThis;
    pThis = reinterpret_cast<CWinRBTree*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_NCCREATE:
    {
        pThis = static_cast<CWinRBTree*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);

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
    }
    break;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 메뉴 선택을 구문 분석합니다:
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    /* 키보드 입력 */
    case WM_KEYDOWN:
        pThis->MsgProc_KEYDOWN(wParam, lParam);
        break;
    /* 마우스 휠을 굴릴 때 */
    case WM_MOUSEWHEEL:
        pThis->MsgProc_MOUSEWHEEL(wParam, lParam);
        break;

    /* 창의 크기가 바뀔 때 */
    case WM_SIZE:
        pThis->MsgProc_SIZE(wParam, lParam);
        break;
    /* 수평 스크롤바 이벤트 */
    case WM_HSCROLL:
        pThis->MsgProc_HSCROLL(wParam, lParam);
        break;
    /* 수직 스크롤바 이벤트 */
    case WM_VSCROLL:
        pThis->MsgProc_VSCROLL(wParam, lParam);
        break;
    break;
    /* WM_PAINT */
    case WM_PAINT:
        pThis->MsgProc_PAINT(wParam, lParam);
        break;
    case WM_DESTROY:
        pThis->MsgProc_DESTROY(wParam, lParam);
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
