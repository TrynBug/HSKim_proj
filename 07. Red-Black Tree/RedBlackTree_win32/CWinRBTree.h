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
		void ResizeWindow();  // memory bitmap �����
		void ReadjustScroll(); // ��ũ���� �������Ѵ�.

		/* tree �׸��� */
		void DrawRBTree(CRedBlackTree<int, int>& tree);  // tree �׸���
		void _drawNode(const CRedBlackTree<int, int>::iterator& iter
			, std::map<int, Point>& mapPos, Point posParent
			, float nodeSize, float nodeSpace);  // ��� �׸���
		int _getNumOfNil(const CRedBlackTree<int, int>::iterator& iter);  // ���� ��� �Ʒ��� Nil �� ���ϱ�
		int _getNumOfLeaf(const CRedBlackTree<int, int>::iterator& iter); // ���� ��� �Ʒ��� leaf(Nil�ƴ�) �� ���ϱ�


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
			HPEN hPenLine;           // ��� ���� ��
			HPEN hPenRedNode;        // ������ �׵θ�
			HPEN hPenBlackNode;      // ����� �׵θ�
			HBRUSH hBrushRedNode;    // ������ ��
			HBRUSH hBrushBlackNode;  // ����� ��
			HBRUSH hBrushBackground; // ����
			HPEN hPenOld;
			HBRUSH hBrushOld;
		};
		GDI _GDI;

		// size
		RECT _rtClientSize;      // Ŭ���̾�Ʈ ���� ũ��
		POINT _ptMemBitSize;     // �޸� ��Ʈ�� ũ��
		float _nodeFullSize;     // ��� ũ�� + ��� �翷 ���� ũ��

		// tree
		CRedBlackTree<int, int> _rbTree;  // red-black tree

		// ����� �Է¿� ���� Ŭ���� ���� ��ȭ ����
		struct Action
		{
			std::vector<int> vecValues;  // insert�� ���� ��Ƴ��¿뵵
			bool isInsert = false;       // ������ ����� �Է��� insert��
			bool isDelete = false;       // ������ ����� �Է��� delete��
			int value;                   // ������ ����� �Է¿� ���� insert/delete ��
		};
		Action _action;


		struct Scroll
		{
			int hPos = 0;  // ���� ��ũ�� ��ġ
			int vPos = 0;  // ���� ��ũ�� ��ġ
			int hMax = 0;  // ���� ��ũ�� �ִ���ġ
			int vMax = 0;  // ���� ��ũ�� �ִ���ġ
			bool isRenderFromScrolling = false; // �������� ��ũ�Ѹ��� ���� ������ ����. ��ũ�Ѹ��� ���� ���̸� object�� �ٽ� �׸��� �ʴ´�.
		};
		Scroll _scroll;


	};



}