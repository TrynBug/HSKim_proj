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
    // ����
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

    // ���콺 ��ǥ��� ��ũ��
    #define mcGET_MOUSE_X_LPARAM(lParam) GET_X_LPARAM((lParam)) + _scroll.hPos
    #define mcGET_MOUSE_Y_LPARAM(lParam) GET_Y_LPARAM((lParam)) + _scroll.vPos
    // ���콺 ��ǥ�� grid row, col �� ��ȯ
    #define mcMOUSE_Y_TO_GRID_ROW(mouseY)   (((mouseY) - _grid.GetGridStartPos().y) / _grid.GetCellSize())
    #define mcMOUSE_X_TO_GRID_COL(mouseX)   (((mouseX) - _grid.GetGridStartPos().x) / _grid.GetCellSize())

    // �׶��̼� �׸��� �ܰ�
    constexpr int SHADOW_STEP = 10;

    // jump point search���� cell ��ĥ�� ����� ���� ����
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
        void ResizeWindow();  // â�� ũ�Ⱑ �ٲ� �� bitmap, dc�� ũ�⿡ �°� �ٽ� ����
        void ReadjustScroll();  // ��ũ���� �������Ѵ�.
        static unsigned WINAPI ThreadProcQuitRunTest(PVOID vParam);  // �׽�Ʈ �ߴ� Ű���� �Է� ���� ������


        /* Message Processing */
        void MsgProc_LBUTTONDOWN(WPARAM wParam, LPARAM lParam);  // ���콺 ���� Ŭ�� ��
        void MsgProc_LBUTTONUP(WPARAM wParam, LPARAM lParam);    // ���콺 ���ʹ�ư�� ���� ��
        void MsgProc_MOUSEMOVE(WPARAM wParam, LPARAM lParam);    // ���콺�� ������ ��
        void MsgProc_MOUSEWHEEL(WPARAM wParam, LPARAM lParam);   // ���콺 ���� ���� ��
        void MsgProc_SIZE(WPARAM wParam, LPARAM lParam);         // â�� ũ�Ⱑ �ٲ� ��
        LRESULT MsgProc_SETCURSOR(UINT message, WPARAM wParam, LPARAM lParam);    // Ŀ�� ��ü (���콺�� ������ ������ �����)
        LRESULT MsgProc_COMMAND(UINT message, WPARAM wParam, LPARAM lParam);      // menu �Ǵ� toolbar Ŭ��
        void MsgProc_KEYDOWN(WPARAM wParam, LPARAM lParam);      // Ű���� �Է�
        void MsgProc_HSCROLL(WPARAM wParam, LPARAM lParam);      // ���� ��ũ�ѹ� �̺�Ʈ
        void MsgProc_VSCROLL(WPARAM wParam, LPARAM lParam);      // ���� ��ũ�ѹ� �̺�Ʈ
        void MsgProc_TIMER(WPARAM wParam, LPARAM lParam);        // search Ÿ�̸�
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
        HDC _hWallDC;      // memDC�� wall �׸��� �뵵�� DC
        HBITMAP _hWallBit; // memDC�� wall �׸��� �뵵�� bitmap
        int _gridSize_old;
        int _gridCols_old;

		// Ŭ���̾�Ʈ ũ��
		RECT _rtClientSize;
		POINT _ptMemBitSize;  // �޸� ��Ʈ�� ũ��

        // ��ã�� ��ü
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


		// Ÿ�̸�
		struct Timer
		{
			int id;                // ID
			int frequency;         // �ֱ�
			int frequencyIncrease; // �ֱ� ����ġ
			bool bResetTimer;      // Ÿ�̸� �缳�� ����
			int searchSpeedLevel;  // �ӵ�(���� ������ ����, ���� ����)
		};
		Timer _timer;

		// Ŀ��
		struct Cursor
		{
			bool isChanged;       // Ŀ���� ����Ǿ���ϴ��� ����
			HCURSOR hDefault;     // �⺻ Ŀ��
			HCURSOR hCurrent;     // ���� Ŀ��
            void Init();
		};
		Cursor _cursor;

        // ��ũ�� ����
        struct Scroll
        {
            int hPos = 0;  // ���� ��ũ�� ��ġ
            int vPos = 0;  // ���� ��ũ�� ��ġ
            int hMax = 0;  // ���� ��ũ�� �ִ���ġ
            int vMax = 0;  // ���� ��ũ�� �ִ���ġ
            bool isRenderFromScrolling = false; // �������� ��ũ�Ѹ��� ���� ������ ����. ��ũ�Ѹ��� ���� ���̸� object�� �ٽ� �׸��� �ʴ´�.

        };
        Scroll _scroll;

        // ����
        struct Toolbar
        {
            int btnTool = IDM_TOOLBAR_WALL;                   // ���� ������ �׸��⵵��
            int btnAlgorithm = IDM_TOOLBAR_JUMP_POINT_SEARCH; // ���� ������ ��ã�� �˰���
            bool btnIsFValueChecked = false;                  // F-Value ��ư�� ���������� ����
            int height = 28;                                  // ���� ����
        };
        Toolbar _toolbar;

        // ���콺 ���� ����
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

        // �׽�Ʈ
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

