#pragma once

#include "framework.h"
#include "Resource.h"

#include <map>
#include <vector>
#include "CRedBlackTree.h"

namespace rbtree
{
	constexpr int MAX_LOADSTRING = 100;

	struct Point
	{
		float x;
		float y;
	};

	class CWinRBTree
	{
	public:
		CWinRBTree(_In_ HINSTANCE hInst, _In_ int nCmdShow);
		~CWinRBTree();

		CWinRBTree(const CWinRBTree&) = delete;
		CWinRBTree(CWinRBTree&&) = delete;

	public:
		int Run();
		void Close();

	private:
		/* Windows */
		ATOM MyRegisterClass(HINSTANCE hInstance);
		BOOL InitInstance(HINSTANCE, int);
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

	private:
		/* message processing */
		void MsgProc_KEYDOWN(WPARAM wParam, LPARAM lParam);
		void MsgProc_MOUSEWHEEL(WPARAM wParam, LPARAM lParam);
		void MsgProc_SIZE(WPARAM wParam, LPARAM lParam);
		void MsgProc_HSCROLL(WPARAM wParam, LPARAM lParam);
		void MsgProc_VSCROLL(WPARAM wParam, LPARAM lParam);
		void MsgProc_PAINT(WPARAM wParam, LPARAM lParam);
		void MsgProc_DESTROY(WPARAM wParam, LPARAM lParam);

	private:
		/* utils */
		void ResizeWindow();  // memory bitmap 재생성
		void ReadjustScroll(); // 스크롤을 재조정한다.

		/* tree 그리기 */
		void DrawRBTree(CRedBlackTree<int, int>& tree);  // tree 그리기
		void _drawNode(const CRedBlackTree<int, int>::iterator& iter
			, std::map<int, Point>& mapPos, Point posParent
			, float nodeSize, float nodeSpace);  // 노드 그리기
		int _getNumOfNil(const CRedBlackTree<int, int>::iterator& iter);  // 현재 노드 아래의 Nil 수 구하기
		int _getNumOfLeaf(const CRedBlackTree<int, int>::iterator& iter); // 현재 노드 아래의 leaf(Nil아님) 수 구하기


	private:
		HINSTANCE _hInst;
		int _nCmdShow;
		WCHAR _szTitle[MAX_LOADSTRING];
		WCHAR _szWindowClass[MAX_LOADSTRING];
		bool _bClosed; 

		// handle
		HWND _hWnd;
		HDC _hDC;
		HBITMAP _hMemBit;
		HDC _hMemDC;

		struct GDI
		{
			HPEN hPenLine;           // 노드 연결 선
			HPEN hPenRedNode;        // 레드노드 테두리
			HPEN hPenBlackNode;      // 블랙노드 테두리
			HBRUSH hBrushRedNode;    // 레드노드 색
			HBRUSH hBrushBlackNode;  // 블랙노드 색
			HBRUSH hBrushBackground; // 배경색
			HPEN hPenOld;
			HBRUSH hBrushOld;
		};
		GDI _GDI;

		// size
		RECT _rtClientSize;      // 클라이언트 영역 크기
		POINT _ptMemBitSize;     // 메모리 비트맵 크기
		float _nodeFullSize;     // 노드 크기 + 노드 양옆 공간 크기

		// tree
		CRedBlackTree<int, int> _rbTree;  // red-black tree

		// 사용자 입력에 대한 클래스 상태 변화 값들
		struct Action
		{
			std::vector<int> vecValues;  // insert한 값들 모아놓는용도
			bool isInsert = false;       // 마지막 사용자 입력이 insert임
			bool isDelete = false;       // 마지막 사용자 입력이 delete임
			int value;                   // 마지막 사용자 입력에 대한 insert/delete 값
		};
		Action _action;


		struct Scroll
		{
			int hPos = 0;  // 수평 스크롤 위치
			int vPos = 0;  // 수직 스크롤 위치
			int hMax = 0;  // 수평 스크롤 최대위치
			int vMax = 0;  // 수직 스크롤 최대위치
			bool isRenderFromScrolling = false; // 렌더링이 스크롤링에 의한 것인지 여부. 스크롤링에 의한 것이면 object를 다시 그리지 않는다.
		};
		Scroll _scroll;


	};



}