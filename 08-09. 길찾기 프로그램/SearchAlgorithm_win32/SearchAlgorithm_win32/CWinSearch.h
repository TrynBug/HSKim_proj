#pragma once

#include "framework.h"
#include "resource.h"

#include <stdio.h>
#include <process.h>
#include <conio.h>
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <ctime>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>

#include "CGrid.h"
#include "CAStarSearch.h"
#include "CJumpPointSearch.h"

namespace search
{
    // 색상
    #define dfCOLOR_BACKGROUND    RGB(199, 209, 225)
    #define dfCOLOR_GRID          RGB(150, 150, 150)
    #define dfCOLOR_WALL          DKGRAY_BRUSH
    #define dfCOLOR_START_POINT   RGB(0, 128, 64)
    #define dfCOLOR_END_POINT     RGB(237, 28, 36)
    #define dfCOLOR_RESIZE_BORDER RGB(50, 50, 50)
    #define dfCOLOR_OPEN_NODE     RGB(100, 130, 255)
    #define dfCOLOR_CLOSE_NODE    RGB(255, 255, 66)
    #define dfCOLOR_PATH_LINE     RGB(60, 60, 60)
    #define dfCOLOR_SMOOTH_PATH_LINE  RGB(163, 73, 164)
    #define dfCOLOR_BRESENHAM     RGB(255, 151, 185)

    // 마우스 좌표얻기 매크로
    #define mcGET_MOUSE_X_LPARAM(lParam) GET_X_LPARAM((lParam)) + _scroll.hPos
    #define mcGET_MOUSE_Y_LPARAM(lParam) GET_Y_LPARAM((lParam)) + _scroll.vPos
    // 마우스 좌표를 grid row, col 로 변환
    #define mcMOUSE_Y_TO_GRID_ROW(mouseY)   (((mouseY) - _grid.GetGridStartPos().y) / _grid.GetCellSize())
    #define mcMOUSE_X_TO_GRID_COL(mouseX)   (((mouseX) - _grid.GetGridStartPos().x) / _grid.GetCellSize())

    // 그라데이션 그림자 단계
    constexpr int SHADOW_STEP = 10;

    // jump point search에서 cell 색칠에 사용할 색상 갯수
    constexpr int NUM_OF_CELL_COLOR_GROUP = 6;

	class CWinSearch
	{
    public:
        CWinSearch(_In_ HINSTANCE hInst, _In_ int nCmdShow);
        ~CWinSearch();

        CWinSearch(const CWinSearch&) = delete;
        CWinSearch(CWinSearch&&) = delete;

    public:
        int Run();
        void Close();

    private:
        /* Windows */
        HWND CreateToolbar(HINSTANCE hInstance, HWND hWnd);

        static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
        static INT_PTR CALLBACK WndProcDialogSizeAdj(HWND, UINT, WPARAM, LPARAM);


    private:
        /* Utils */
        void ResizeWindow();  // 창의 크기가 바뀔 때 bitmap, dc를 크기에 맞게 다시 만듬
        void ReadjustScroll();  // 스크롤을 재조정한다.
        static unsigned WINAPI ThreadProcQuitRunTest(PVOID vParam);  // 테스트 중단 키보드 입력 감지 스레드


        /* Message Processing */
        void MsgProc_LBUTTONDOWN(WPARAM wParam, LPARAM lParam);  // 마우스 왼쪽 클릭 시
        void MsgProc_LBUTTONUP(WPARAM wParam, LPARAM lParam);    // 마우스 왼쪽버튼을 뗐을 때
        void MsgProc_MOUSEMOVE(WPARAM wParam, LPARAM lParam);    // 마우스를 움직일 때
        void MsgProc_MOUSEWHEEL(WPARAM wParam, LPARAM lParam);   // 마우스 휠을 굴릴 때
        void MsgProc_SIZE(WPARAM wParam, LPARAM lParam);         // 창의 크기가 바뀔 때
        LRESULT MsgProc_SETCURSOR(UINT message, WPARAM wParam, LPARAM lParam);    // 커서 교체 (마우스를 움직일 때마다 시행됨)
        LRESULT MsgProc_COMMAND(UINT message, WPARAM wParam, LPARAM lParam);      // menu 또는 toolbar 클릭
        void MsgProc_KEYDOWN(WPARAM wParam, LPARAM lParam);      // 키보드 입력
        void MsgProc_HSCROLL(WPARAM wParam, LPARAM lParam);      // 수평 스크롤바 이벤트
        void MsgProc_VSCROLL(WPARAM wParam, LPARAM lParam);      // 수직 스크롤바 이벤트
        void MsgProc_TIMER(WPARAM wParam, LPARAM lParam);        // search 타이머
        void MsgProc_PAINT(WPARAM wParam, LPARAM lParam);        // WM_PAINT
        void MsgProc_DESTROYT(WPARAM wParam, LPARAM lParam);     // WM_DESTROY


	private:
		HINSTANCE _hInst;
        int _nCmdShow;

        // handle
		HWND _hWnd;
		HDC _hDC;
		HWND _hToolbar; // toolbar

		// memory DC
		HBITMAP _hMemBit;
		HDC _hMemDC;
        HDC _hWallDC;      // memDC에 wall 그리는 용도의 DC
        HBITMAP _hWallBit; // memDC에 wall 그리는 용도의 bitmap
        int _gridSize_old;
        int _gridCols_old;

		// 클라이언트 크기
		RECT _rtClientSize;
		POINT _ptMemBitSize;  // 메모리 비트맵 크기

        // 길찾기 객체
        CAStarSearch _AStarSearch;          // A* search
        CJumpPointSearch _jumpPointSearch;  // Jump Point Search

        // grid
        CGrid _grid;

        struct GDI
        {
            HBRUSH hBrushBackground;
            HPEN hPenGrid;
            HBRUSH hBrushOpenNode;
            HBRUSH hBrushCloseNode;
            HBRUSH hBrushStartPoint;
            HBRUSH hBrushEndPoint;
            HBRUSH hBrushWall;
            HPEN hPenPath;
            HPEN hPenSmoothPath;
            HPEN arrHPenShadow[SHADOW_STEP];
            HPEN hPenResizeObject;
            HBRUSH hBrushResizeObject;
            HPEN hPenResizeBorder;
            HBRUSH hBrushResizeBorder;
            HBRUSH arrHBrushCellColor[NUM_OF_CELL_COLOR_GROUP];
            HBRUSH hBrushBresenhamLine;

            HFONT hFontGridInfo;

            void Init();
        };
        GDI _GDI;


		// 타이머
		struct Timer
		{
			int id;                // ID
			int frequency;         // 주기
			int frequencyIncrease; // 주기 증감치
			bool bResetTimer;      // 타이머 재설정 여부
			int searchSpeedLevel;  // 속도(값이 작으면 빠름, 음수 가능)
		};
		Timer _timer;

		// 커서
		struct Cursor
		{
			bool isChanged;       // 커서가 변경되어야하는지 여부
			HCURSOR hDefault;     // 기본 커서
			HCURSOR hCurrent;     // 현재 커서
            void Init();
		};
		Cursor _cursor;

        // 스크롤 관련
        struct Scroll
        {
            int hPos = 0;  // 수평 스크롤 위치
            int vPos = 0;  // 수직 스크롤 위치
            int hMax = 0;  // 수평 스크롤 최대위치
            int vMax = 0;  // 수직 스크롤 최대위치
            bool isRenderFromScrolling = false; // 렌더링이 스크롤링에 의한 것인지 여부. 스크롤링에 의한 것이면 object를 다시 그리지 않는다.

        };
        Scroll _scroll;

        // 툴바
        struct Toolbar
        {
            int btnTool = IDM_TOOLBAR_WALL;                   // 현재 눌려진 그리기도구
            int btnAlgorithm = IDM_TOOLBAR_JUMP_POINT_SEARCH; // 현재 눌려진 길찾기 알고리즘
            bool btnIsFValueChecked = false;                  // F-Value 버튼이 눌려졌는지 여부
            int height = 28;                                  // 툴바 높이
        };
        Toolbar _toolbar;

        // 마우스 관련 변수
        struct Mouse
        {
            int  x;
            int  y;
            int  clickX;
            int  clickY;
            bool clickPainting = false;
            bool erase = false;
            bool clickResizeVertical = false;
            bool clickResizeDiagonal = false;
            bool clickResizeHorizontal = false;
        };
        Mouse _mouse;

        // 테스트
        struct Test
        {
            bool isRunning = false;
            bool isDoRendering = true;
            bool isPrintInfo = false;
            unsigned int randomSeed;
            int numOfCaveWays;
        };
        Test _test;

	};

}

