#include "framework.h"
#include "resource.h"

#include "CWinRBTree.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    rbtree::CWinRBTree winRBTree(hInstance, nCmdShow);
    winRBTree.Run();

    return 0;
}
