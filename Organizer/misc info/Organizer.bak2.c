/* Central windows startup stuff */
/* Windows GUI will be used for now */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0510
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
/* #include <ErrorRep.h> */
#include "resource.h"

#include <crtdbg.h>

#include "core.h"
#include "Panel.h"
/* #include "FileSysInterface.h" */
#include "TextEdit.h"

HINSTANCE g_hInstance;
TCHAR stringTable[NUM_STRS][100];
bool draggingTree = false; /* This is up here for a strange reason (see "Message loop") */

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CalculateChildWindowSizes(HWND hwnd, bool resize);
void SizeChildWindows();
void BeginDivDrag();
void DrawDivDrag(LPARAM lParam, unsigned oldTreeDivXPos);
void EndDivDrag();
void InitPopups(HMENU popup);
LRESULT CALLBACK DividerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutBoxProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LinGraphProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HWND CreateToolBar(HWND hWndParent);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	HWND hwnd;			/* Window handle */
	WNDCLASSEX wcex;	/* Window class */
	MSG msg;			/* Message structure */
	HACCEL hAccel;		/* Accelerator table */
	/* Temporary variables */
	unsigned i;

	/* Don't allow Windows Error Reporting */
	/* AddERExcludedApplication("Organizer.exe"); */
	//_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));

	/* Register the main window class */
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = MainWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)MAIN_ICON);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = (LPCTSTR)MAINMENU;
	wcex.lpszClassName = "mainWindow";
	wcex.hIconSm = NULL;

	if (!RegisterClassEx(&wcex))
		return 0;

	/* Register the FileSysInterface custom window class */
	wcex.lpszMenuName = NULL;
	/* wcex.lpfnWndProc = FSwindowProc; */
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszClassName = "FScustomWindow";

	/*if (!RegisterClassEx(&wcex))
		return 0;*/

	/* Register newer text edit window class */
	wcex.lpfnWndProc = TextEditProc;
	wcex.lpszClassName = "CustomTextEdit";

	if (!RegisterClassEx(&wcex))
		return 0;

	/* Register the linear graph window class */
	wcex.lpfnWndProc = LinGraphProc;
	wcex.lpszClassName = "linGraphWindow";

	if (!RegisterClassEx(&wcex))
		return 0;

	/* Register a two more classes for child window dividers (vertical and horizontal) */
	wcex.lpfnWndProc = DividerWindowProc;
	wcex.hCursor = LoadCursor(NULL, IDC_SIZEWE);
	wcex.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wcex.lpszClassName = "VdividerWindow";

	if (!RegisterClassEx(&wcex))
		return 0;

	wcex.hCursor = LoadCursor(NULL, IDC_SIZENS);
	wcex.lpszClassName = "HdividerWindow";

	if (!RegisterClassEx(&wcex))
		return 0;

	/* Load the accelerator table */
	hAccel = LoadAccelerators(hInstance, (LPCTSTR)ACCTABLE);

	/* Load the string table */
	for (i = M_NEW; i < M_NEW + NUM_STRS; i++)
		LoadString(hInstance, i, stringTable[i-M_NEW], 100 / sizeof(TCHAR));

	/* Initialize tree view and status bar functionality */
	InitCommonControls();

	/* Globally save the application instance */
	g_hInstance = hInstance;

	/* Create the main window */
	hwnd = CreateWindowEx(0, "mainWindow", "Organizer",
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
						  CW_USEDEFAULT, CW_USEDEFAULT,	/* window position does not matter */
						  CW_USEDEFAULT, CW_USEDEFAULT,	/* window size does not matter */
		NULL, 0, hInstance, NULL);

	if (!hwnd)
		return 0;

	/* Show and draw (update) the window */
	ShowWindow(hwnd, nShowCmd);
	UpdateWindow(hwnd);

	/* Message loop */
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (draggingTree == true && ((msg.message >= WM_KEYFIRST &&
			msg.message <= WM_KEYLAST) ||
			(msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)))
		{
			/* In other programs that have a window divider that you can drag, you may be able
			   to hit the escape key to cancel dragging. Also, other keyboard input unrelated
			   to the dragging operation is ignored, but it seems that the child window that
			   previously had keyboard focus still use display styles as if they had keyboard
			   focus. The best way to solve this problem seems to be to disrupt the normal
			   functioning of a program's main message loop to redirect keyboard messages to
			   the main window. */
			msg.hwnd = hwnd;
			/* We don't need WM_CHAR messages (from TranslateMessage) or accelerator messages
			   (from TranslateAccelerator). */
			DispatchMessage(&msg);
		}
		else
		{
			if (!TranslateAccelerator(hwnd, hAccel, &msg))
				TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (_CrtDumpMemoryLeaks() == FALSE)
		OutputDebugString("No memory leaks occurred.\n");
	return (int)msg.wParam;
}

/* Window management variables */
static unsigned winWidth, winHeight; /* Client area dimensions */
static unsigned dataWidth, dataHeight; /* Data window dimensions */
static unsigned treeWidth, treeHeight; /* Tree window dimensions */
static unsigned tmlnWidth, tmlnHeight; /* Timeline window dimensions */
static unsigned dividerWidth, divDragPt; /* Information for window divisions */
static unsigned vScrollWidth; /* Used for dragging window dividers */
static unsigned treeDivXPos; /* Tree divider x position */
static unsigned tmlnDivYPos;
static unsigned statusHeight;
static unsigned toolBarHeight;
static float treeFrac = 0.25f; /* Fraction of width that tree divider is at */
static float tmlnFrac = 0.75f; /* Fraction of height that timeline divider is at */
static HWND dataWin;
static HWND treeWin;
static HWND tmlnWin;
static HWND tmlnDivider;
static HWND treeDivider;
static HWND statusWin;
static HWND toolBar;
static HWND childFocus; /* Child window with keyboard focus */
static bool draggingTmln = false;
static bool toolbVis = true;
static bool statbVis = true;
static bool treeVis = true;
static bool tmlnVis = false;
static bool keySplit = false;
static POINT oldCursorPos;

/* Variables that reduce function calling */
static int dragClickX;
static int dragClickY;
static HBITMAP hbmOld; /* Used for window divider dragging */
static HWND lastHwnd; /* Parent window handle used in function calls */

/*
The main window has a horizontal divider and a vertical divider in the top half
that split it into three panes. The top left pane is a tree view, the top right
pane is a purpose-specific data window, and the bottom pane is a linear data
graph (timeline) window.

The window dividers notify the main window of being clicked, but the main
window handles dragging and resizing of panes.
*/
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFont = NULL;
	lastHwnd = hwnd;
	switch (uMsg)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* cs;
		HDC hDC;
		RECT rt;
		HTREEITEM hPrev;
		TVINSERTSTRUCT tv;

		/* Set up window divider measures */
		dividerWidth = GetSystemMetrics(SM_CXFRAME);
		divDragPt = dividerWidth / 2;
		if (dividerWidth % 2 != 1)
			divDragPt--;
		vScrollWidth = GetSystemMetrics(SM_CXVSCROLL);

		cs = (CREATESTRUCT*)lParam;
		CalculateChildWindowSizes(hwnd, true);
		treeWin = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
			0, 0, treeWidth, treeHeight,
			hwnd, (HMENU)TREE_WINDOW, cs->hInstance, NULL);
		tmlnWin = CreateWindowEx(WS_EX_CLIENTEDGE, "linGraphWindow"/*"EDIT"*/, NULL,
			WS_CHILD | WS_HSCROLL/* | ES_LEFT | ES_MULTILINE | ES_WANTRETURN*/,
			0, treeHeight + dividerWidth, tmlnWidth, tmlnHeight,
			hwnd, (HMENU)TMLN_WINDOW, cs->hInstance, NULL);
		dataWin = CreateWindowEx(WS_EX_CLIENTEDGE, "CustomTextEdit"/*"EDIT"*/, NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL/* | ES_LEFT | ES_MULTILINE | ES_WANTRETURN | ES_NOHIDESEL*/,
			treeWidth + dividerWidth, 0, winWidth - treeWidth - dividerWidth, treeHeight,
			hwnd, (HMENU)DATA_WINDOW, cs->hInstance, NULL);
		/* Set a default font */
		hDC = GetDC(hwnd);
		hFont = CreateFont(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
		ReleaseDC(hwnd, hDC);
		SendMessage(dataWin, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE);
		treeDivider = CreateWindowEx(WS_EX_WINDOWEDGE,
			"VdividerWindow", NULL, WS_CHILD | WS_VISIBLE,
			treeDivXPos, 0, dividerWidth, treeHeight,
			hwnd, (HMENU)TREE_DIVIDER, cs->hInstance, NULL);
		tmlnDivider = CreateWindowEx(WS_EX_WINDOWEDGE,
			"HdividerWindow", NULL, WS_CHILD,
			0, tmlnDivYPos, tmlnWidth, dividerWidth,
			hwnd, (HMENU)TMLN_DIVIDER, cs->hInstance, NULL);
		statusWin = CreateWindowEx(0,
			STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
			hwnd, (HMENU)STATUS_BAR, cs->hInstance, NULL);
		SendMessage(statusWin, SB_SETTEXT, (WPARAM)0, (LPARAM)"Ready");
		toolBar = CreateToolBar(hwnd);

		GetWindowRect(statusWin, &rt);
		statusHeight = rt.bottom - rt.top;
		GetWindowRect(toolBar, &rt);
		toolBarHeight = rt.bottom - rt.top;
		toolBarHeight -= 1; /* I don't understand, but there is otherwise a bug */
		CalculateChildWindowSizes(hwnd, false);
		SizeChildWindows();

		/* Sample tree hierarchy */
		tv.hParent = NULL;
		tv.hInsertAfter = TVI_LAST;
		tv.item.mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_TEXT;
		tv.item.pszText = "Root";
		tv.item.cchTextMax = 5;
		tv.item.cChildren = 1;
		tv.item.lParam = 1101;
		hPrev = TreeView_InsertItem(treeWin, &tv);
		tv.hParent = hPrev;
		tv.item.pszText = "README.txt";
		tv.item.cchTextMax = 11;
		tv.item.cChildren = 0;
		tv.item.lParam = 1102;
		TreeView_InsertItem(treeWin, &tv);
		tv.hParent = hPrev;
		tv.item.pszText = "Misc";
		tv.item.cchTextMax = 5;
		tv.item.cChildren = 1;
		tv.item.lParam = 1103;
		hPrev = TreeView_InsertItem(treeWin, &tv);
		tv.hParent = hPrev;
		tv.item.pszText = "Item 1";
		tv.item.cchTextMax = 7;
		tv.item.cChildren = 0;
		tv.item.lParam = 1104;
		TreeView_InsertItem(treeWin, &tv);
		tv.item.pszText = "Item 2";
		tv.item.cchTextMax = 7;
		tv.item.lParam = 1105;
		TreeView_InsertItem(treeWin, &tv);
		tv.hParent = NULL;
		tv.item.pszText = "Pictures";
		tv.item.cchTextMax = 9;
		tv.item.lParam = 1106;
		hPrev = TreeView_InsertItem(treeWin, &tv);
		tv.hParent = NULL;

		childFocus = dataWin;
		SetFocus(childFocus);
		break;
	}
	case WM_DESTROY:
		DestroyWindow(treeWin);
		DestroyWindow(tmlnWin);
		DestroyWindow(dataWin);
		DeleteObject(hFont);
		DestroyWindow(tmlnDivider);
		DestroyWindow(treeDivider);
		DestroyWindow(statusWin);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		/*PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);*/
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			/* Window divider activation */
		case TREE_DIVIDER:
			draggingTree = true;
			/* RegisterHotKey(hwnd, VK_ESCAPE, NULL, VK_ESCAPE); */
			treeDivXPos = dragClickX - divDragPt;
			BeginDivDrag();
			break;
		case TMLN_DIVIDER:
			draggingTmln = true;
			tmlnDivYPos = dragClickY - divDragPt;
			treeHeight = tmlnDivYPos;
			SetCapture(hwnd);
			break;
		case K_SWITCHPANES:
			childFocus = GetFocus();
			if (childFocus == treeWin)
			{
				SetFocus(dataWin);
				childFocus = dataWin;
			}
			else if (childFocus == dataWin && treeVis == true)
			{
				SetFocus(treeWin);
				childFocus = treeWin;
			}
			break;
			/* File commands */
		case M_NEW:
			MessageBox(hwnd, "HEY!", NULL, MB_OK);
			break;
		case M_OPEN:
			break;
		case M_SAVE:
			break;
		case M_SAVEAS:
			break;
		case M_EXIT:
			DestroyWindow(hwnd);
			break;
			/* Edit commands */
		case M_UNDO:
			SendMessage(dataWin, WM_UNDO, (WPARAM)0, (LPARAM)0);
			break;
		case M_REDO:
			SendMessage(dataWin, EM_REDO, (WPARAM)0, (LPARAM)0);
			break;
		case M_CUT:
			SendMessage(dataWin, WM_CUT, (WPARAM)0, (LPARAM)0);
			break;
		case M_COPY:
			SendMessage(dataWin, WM_COPY, (WPARAM)0, (LPARAM)0);
			break;
		case M_PASTE:
			SendMessage(dataWin, WM_PASTE, (WPARAM)0, (LPARAM)0);
			break;
		case M_EMPTYCLIP:
			if (OpenClipboard(hwnd))
			{
				EmptyClipboard();
				CloseClipboard();
			}
			else
				MessageBeep(MB_OK);
			break;
		case M_DELETE:
			SendMessage(dataWin, WM_CLEAR, (WPARAM)0, (LPARAM)0);
			break;
		case M_SELECTALL:
			SendMessage(dataWin, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
			break;
		case M_FIND:
			break;
		case M_FINDNEXT:
			MessageBox(hwnd, "HEY!", NULL, MB_OK);
			break;
		case M_REPLACE:
			break;
		case M_FONT:
		{
			CHOOSEFONT cf;
			LOGFONT logFnt;
			GetObject(hFont, sizeof(LOGFONT), &logFnt);
			cf.lStructSize = sizeof(CHOOSEFONT);
			cf.hwndOwner = hwnd;
			cf.hDC = NULL;
			cf.lpLogFont = &logFnt;
			cf.iPointSize = 0;
			if (hFont == NULL)
				cf.Flags = CF_SCREENFONTS;
			else
				cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
			cf.rgbColors = (COLORREF)0x00000000;
			cf.lCustData = 0;
			cf.lpfnHook = NULL;
			cf.lpTemplateName = NULL;
			cf.hInstance = NULL;
			cf.lpszStyle = NULL;
			cf.nFontType = 0;
			cf.nSizeMin = 0;
			cf.nSizeMax = 0;
			if (ChooseFont(&cf))
			{
				DeleteObject(hFont);
				hFont = CreateFontIndirect(cf.lpLogFont);
				SendMessage(dataWin, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);
			}
			break;
		}
		/* View commands */
		case M_DATA:
			break;
		case M_METADATA:
			break;
		case M_STATBAR:
			{
				HMENU hMen;
				HMENU hSub;
				hMen = GetMenu(hwnd);
				hSub = GetSubMenu(hMen, 2);
				if (statbVis == true)
				{
					statbVis = false;
					CheckMenuItem(hSub, 3, MF_BYPOSITION | MF_UNCHECKED);
					ShowWindow(statusWin, SW_HIDE);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				else
				{
					statbVis = true;
					CheckMenuItem(hSub, 3, MF_BYPOSITION | MF_CHECKED);
					ShowWindow(statusWin, SW_SHOW);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				break;
			}
		case M_TOOLBAR:
			{
				HMENU hMen;
				HMENU hSub;
				hMen = GetMenu(hwnd);
				hSub = GetSubMenu(hMen, 2);
				if (toolbVis == true)
				{
					toolbVis = false;
					CheckMenuItem(hSub, 4, MF_BYPOSITION | MF_UNCHECKED);
					ShowWindow(toolBar, SW_HIDE);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				else
				{
					toolbVis = true;
					CheckMenuItem(hSub, 4, MF_BYPOSITION | MF_CHECKED);
					ShowWindow(toolBar, SW_SHOW);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				break;
			}
		case M_TREE:
			{
				HMENU hMen;
				HMENU hSub;
				hMen = GetMenu(hwnd);
				hSub = GetSubMenu(hMen, 2);
				if (treeVis == true)
				{
					treeVis = false;
					if (childFocus == treeWin)
					{
						SetFocus(dataWin);
						childFocus = dataWin;
					}
					CheckMenuItem(hSub, 5, MF_BYPOSITION | MF_UNCHECKED);
					ShowWindow(treeWin, SW_HIDE);
					ShowWindow(treeDivider, SW_HIDE);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				else
				{
					treeVis = true;
					CheckMenuItem(hSub, 5, MF_BYPOSITION | MF_CHECKED);
					ShowWindow(treeWin, SW_SHOW);
					ShowWindow(treeDivider, SW_SHOW);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				break;
			}
		case M_LINGRAPH:
			{
				HMENU hMen;
				HMENU hSub;
				hMen = GetMenu(hwnd);
				hSub = GetSubMenu(hMen, 2);
				if (tmlnVis == true)
				{
					tmlnVis = false;
					if (childFocus == tmlnWin)
					{
						SetFocus(dataWin);
						childFocus = dataWin;
					}
					CheckMenuItem(hSub, 6, MF_BYPOSITION | MF_UNCHECKED);
					ShowWindow(tmlnWin, SW_HIDE);
					ShowWindow(tmlnDivider, SW_HIDE);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				else
				{
					tmlnVis = true;
					CheckMenuItem(hSub, 6, MF_BYPOSITION | MF_CHECKED);
					ShowWindow(tmlnWin, SW_SHOW);
					ShowWindow(tmlnDivider, SW_SHOW);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				break;
			}
		case M_SPLIT:
			keySplit = true;
			draggingTree = true;
			GetCursorPos(&oldCursorPos);
			{
				POINT pt;
				WNDCLASSEX wcex;
				/* Set the cursor properly */
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				pt.x = treeDivXPos + divDragPt;
				pt.y = treeHeight / 2 - 1;
				ClientToScreen(hwnd, &pt);
				SetCursorPos(pt.x, pt.y);
				GetClassInfoEx(g_hInstance, "VdividerWindow", &wcex);
				SetCursor(wcex.hCursor);
				BeginDivDrag();
			}
			break;
			/* Entry commands */
		case M_ADDFILE:
			break;
		case M_ADDGROUP:
			break;
		case M_MAKEGROUPH:
			MessageBox(hwnd, "HEY!", NULL, MB_OK);
			break;
		case M_ENTMOVUP:
			break;
		case M_ENTMOVD:
			break;
		case M_FILTERMODE:
			break;
			/* Help commands */
		case M_ABOUT:
			DialogBox(g_hInstance, (LPCTSTR)ABOUTBOX, hwnd, AboutBoxProc);
			break;
		}
		break;
	case WM_ENTERMENULOOP:
		/* SendMessage(statusWin, SB_SIMPLE, (WPARAM)TRUE, (LPARAM)0); */
		break;
	case WM_INITMENUPOPUP:
		InitPopups((HMENU)wParam);
		break;
	case WM_MENUSELECT:
		if (LOWORD(wParam) >= M_NEW)
		{
			SendMessage(statusWin, SB_SETTEXT, (WPARAM)0, (LPARAM)stringTable[LOWORD(wParam)-M_NEW]);
		}
		else
		{
			SendMessage(statusWin, SB_SETTEXT, (WPARAM)0, (LPARAM)0);
		}
		break;
	case WM_EXITMENULOOP:
		/* SendMessage(statusWin, SB_SIMPLE, (WPARAM)FALSE, (LPARAM)0); */
		SendMessage(statusWin, SB_SETTEXT, (WPARAM)0, (LPARAM)"Ready");
		break;
	case WM_NOTIFY:
	{
		NMTREEVIEW* pnmtv;
		NMTTDISPINFO* lpnmtdi;
		pnmtv = (NMTREEVIEW*)lParam;
		lpnmtdi = (NMTTDISPINFO*)lParam;
		if (pnmtv->hdr.code == TVN_SELCHANGED)
		{
			/* MessageBox(NULL, "BOO!", NULL, MB_OK); */
			/* FSSOnChangeSelection(pnmtv->itemNew.pszText, dataWin); */
		}
		if (lpnmtdi->hdr.code == TTN_GETDISPINFO)
		{
			/* Just give the address of the pre-loaded strings */
			UINT_PTR btnId;
			unsigned strIdx;
			btnId = lpnmtdi->hdr.idFrom;
			switch (btnId)
			{
			case M_NEW: strIdx = T_NEW; break;
			case M_OPEN: strIdx = T_OPEN; break;
			case M_SAVE: strIdx = T_SAVE; break;
			case M_CUT: strIdx = T_CUT; break;
			case M_COPY: strIdx = T_COPY; break;
			case M_PASTE: strIdx = T_PASTE; break;
			case M_UNDO: strIdx = T_UNDO; break;
			case M_REDO: strIdx = T_REDO; break;
			}
			lpnmtdi->hinst = NULL;
			lpnmtdi->lpszText = stringTable[strIdx-M_NEW];
		}
		break;
	}
	case WM_SETFOCUS:
		if (treeWin == (HWND)wParam ||
			dataWin == (HWND)wParam ||
			tmlnWin == (HWND)wParam)
			childFocus = (HWND)wParam;
		SetFocus(childFocus);
		break;
	case WM_ACTIVATE:
		if (wParam == FALSE)
			childFocus = GetFocus();
		break;
	case WM_KEYDOWN:
	{
		POINT pt;
		RECT rt;
		GetCursorPos(&pt);
		if (keySplit == true)
		{
			switch (wParam)
			{
			case VK_LEFT:
				ScreenToClient(hwnd, &pt);
				pt.x -= dividerWidth / 2;
				GetClientRect(hwnd, &rt);
				rt.left += divDragPt + vScrollWidth * 2;
				rt.top += divDragPt + vScrollWidth * 2;
				rt.right -= divDragPt + vScrollWidth * 2;
				rt.bottom -= divDragPt + vScrollWidth * 2;
				if (rt.left <= pt.x && pt.x < rt.right)
				{
					ClientToScreen(hwnd, &pt);
					SetCursorPos(pt.x, pt.y);
				}
				break;
			case VK_RIGHT:
				ScreenToClient(hwnd, &pt);
				pt.x += dividerWidth / 2;
				GetClientRect(hwnd, &rt);
				rt.left += divDragPt + vScrollWidth * 2;
				rt.top += divDragPt + vScrollWidth * 2;
				rt.right -= divDragPt + vScrollWidth * 2;
				rt.bottom -= divDragPt + vScrollWidth * 2;
				if (rt.left <= pt.x && pt.x < rt.right)
				{
					ClientToScreen(hwnd, &pt);
					SetCursorPos(pt.x, pt.y);
				}
				break;
			case VK_RETURN:
				EndDivDrag();
				break;
			}
		}
		if (wParam == VK_ESCAPE)
		{
			if (keySplit == true)
			{
				keySplit = false;
				SetCursorPos(oldCursorPos.x, oldCursorPos.y);
			}
			ReleaseCapture();
		}
		break;
	}
	case WM_LBUTTONDOWN:
		if (keySplit == true)
		{
			SetCursorPos(oldCursorPos.x, oldCursorPos.y);
			EndDivDrag();
		}
		break;
	case WM_RBUTTONDOWN:
		break;
	case WM_CAPTURECHANGED:
		if ((HWND)lParam != hwnd)
		{
			if (draggingTree == true)
			{
				draggingTree = false;
				if (keySplit == true)
					SetCursorPos(oldCursorPos.x, oldCursorPos.y);
				treeDivXPos = treeWidth;
				DeleteObject(hbmOld);
				InvalidateRect(hwnd, NULL, TRUE);
			}
			if (keySplit == true)
			{
				keySplit = false;
			}
			if (draggingTmln == true)
			{
				draggingTmln = false;
				tmlnDivYPos = dataHeight;
				treeHeight = dataHeight;
				SizeChildWindows();
				DeleteObject(hbmOld);
				InvalidateRect(hwnd, NULL, TRUE);

			}
			return 0;
		}
	case WM_MOUSEMOVE:
		if (draggingTree == true)
		{
			POINT pt;
			RECT rt;
			unsigned oldTreeDivXPos;
			oldTreeDivXPos = treeDivXPos;
			GetClientRect(hwnd, &rt);
			pt.x = (short)LOWORD(lParam);
			pt.y = (short)HIWORD(lParam);
			rt.left += divDragPt + vScrollWidth * 2;
			rt.top += divDragPt + vScrollWidth * 2;
			rt.right -= divDragPt + vScrollWidth * 2;
			rt.bottom -= divDragPt + vScrollWidth * 2;
			if (pt.x < rt.left)
				pt.x = rt.left;
			if (pt.x >= rt.right)
				pt.x = rt.right - 1;
			lParam = MAKELPARAM((short)pt.x, (short)pt.y);
			DrawDivDrag(lParam, oldTreeDivXPos);
		}
		if (draggingTmln == true)
		{
			POINT pt;
			RECT rt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			GetClientRect(hwnd, &rt);
			rt.left += divDragPt + vScrollWidth * 2;
			rt.top += divDragPt + vScrollWidth * 2;
			rt.right -= divDragPt + vScrollWidth * 2;
			rt.bottom -= divDragPt + vScrollWidth * 2;
			if (PtInRect(&rt, pt) == TRUE)
			{
				tmlnDivYPos = HIWORD(lParam) - divDragPt;
				MoveWindow(tmlnDivider, 0, tmlnDivYPos, tmlnWidth, dividerWidth, TRUE);
				treeHeight = tmlnDivYPos;
				MoveWindow(treeDivider, treeDivXPos, 0, dividerWidth, treeHeight, TRUE);
			}
		}
		break;
	case WM_LBUTTONUP:
		if (draggingTree == true)
		{
			EndDivDrag();
		}
		if (draggingTmln == true)
		{
			draggingTmln = false;
			ReleaseCapture();
			MoveWindow(tmlnDivider, 0, tmlnDivYPos, tmlnWidth, dividerWidth, TRUE);
			MoveWindow(treeDivider, treeDivXPos, 0, dividerWidth, treeHeight, TRUE);
			CalculateChildWindowSizes(hwnd, false);
			SizeChildWindows();
			tmlnFrac = (float)(tmlnDivYPos + divDragPt) / winHeight;
		}
		break;
	case WM_RBUTTONUP:
		break;
	case WM_MOVE:
		SendMessage(dataWin, WM_MOVE, (WPARAM)0, (LPARAM)0);
		break;
	case WM_SIZE:
		CalculateChildWindowSizes(hwnd, true);
		SizeChildWindows();
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CalculateChildWindowSizes(HWND hwnd, bool resize)
{
	RECT rt;

	GetClientRect(hwnd, &rt);
	winWidth = rt.right - rt.left;
	winHeight = rt.bottom - rt.top;
	if (statbVis == true)
	{
		winHeight -= statusHeight;
	}
	if (toolbVis == true)
		winHeight -= toolBarHeight;
	if (resize == true)
	{
		treeDivXPos = (unsigned)(winWidth * treeFrac) - divDragPt;
		tmlnDivYPos = (unsigned)(winHeight * tmlnFrac) - divDragPt;
	}
	treeWidth = treeDivXPos;
	treeHeight = tmlnDivYPos;
	tmlnWidth = winWidth;
	tmlnHeight = winHeight - treeHeight - dividerWidth;
	dataWidth = winWidth - treeWidth - dividerWidth;
	dataHeight = winHeight - tmlnHeight - dividerWidth;
	if (tmlnVis == false)
	{
		dataHeight = winHeight;
		treeHeight = winHeight;
	}
	if (treeVis == false)
	{
		dataWidth = winWidth;
	}
}

void SizeChildWindows()
{
	if (toolbVis == true)
	{
	MoveWindow(treeWin, 0, toolBarHeight, treeWidth, treeHeight, TRUE);
	MoveWindow(tmlnWin, 0, toolBarHeight + treeHeight + dividerWidth, tmlnWidth, tmlnHeight, TRUE);
	if (treeVis == true)
		MoveWindow(dataWin, treeWidth + dividerWidth, toolBarHeight, winWidth - treeWidth - dividerWidth, treeHeight, TRUE);
	else
		MoveWindow(dataWin, 0, toolBarHeight, winWidth, treeHeight, TRUE);
	MoveWindow(treeDivider, treeDivXPos, toolBarHeight, dividerWidth, treeHeight, TRUE);
	MoveWindow(tmlnDivider, 0, toolBarHeight + tmlnDivYPos, tmlnWidth, dividerWidth, TRUE);
	if (statbVis == true)
		MoveWindow(statusWin, 0, toolBarHeight + winHeight, winWidth, statusHeight, TRUE);
	MoveWindow(toolBar, 0, 0, winWidth, toolBarHeight, TRUE);
	}
	else
	{
	MoveWindow(treeWin, 0, 0, treeWidth, treeHeight, TRUE);
	MoveWindow(tmlnWin, 0, treeHeight + dividerWidth, tmlnWidth, tmlnHeight, TRUE);
	if (treeVis == true)
		MoveWindow(dataWin, treeWidth + dividerWidth, 0, winWidth - treeWidth - dividerWidth, treeHeight, TRUE);
	else
		MoveWindow(dataWin, 0, 0, winWidth, treeHeight, TRUE);
	MoveWindow(treeDivider, treeDivXPos, 0, dividerWidth, treeHeight, TRUE);
	MoveWindow(tmlnDivider, 0, tmlnDivYPos, tmlnWidth, dividerWidth, TRUE);
	if (statbVis == true)
		MoveWindow(statusWin, 0, winHeight, winWidth, statusHeight, TRUE);
	}
}

void BeginDivDrag()
{
	RECT rt;
	HBITMAP hbm;
	HBRUSH hBr;
	HDC hDC;
	HDC hDCMem;
	long yOfs;
	const char data[4] = {0x40, 0x00, 0x80, 0x00}; /* 0100 ... 1000 */

	if (toolbVis == true)
		yOfs = toolBarHeight;
	else
		yOfs = 0;
	SetCapture(lastHwnd);
	/* Set the hatched bar size */
	rt.left = treeDivXPos; rt.top = yOfs;
	rt.right = treeDivXPos + dividerWidth; rt.bottom = rt.top + treeHeight;
	/* Create the pattern brush */
	hbm = CreateBitmap(2, 2, 1, 1, &data);
	hBr = CreatePatternBrush(hbm);
	/* Prepare the previous bits */
	hDC = GetDC(lastHwnd);
	hbmOld = CreateCompatibleBitmap(hDC, rt.right - rt.left, rt.bottom - rt.top);
	hDCMem = CreateCompatibleDC(hDC);
	SelectObject(hDCMem, hbmOld);
	BitBlt(hDCMem, 0, 0, rt.right - rt.left, rt.bottom - rt.top, hDC, rt.left, rt.top, SRCCOPY);
	DeleteDC(hDCMem);
	/* Draw the hatched bar */
	SelectObject(hDC, hBr);
	PatBlt(hDC, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
	DeleteObject(hBr);
	DeleteObject(hbm);
	ReleaseDC(lastHwnd, hDC);
}

void DrawDivDrag(LPARAM lParam, unsigned oldTreeDivXPos)
{
	unsigned resBmSize;
	unsigned resBmXPos;
	unsigned oldDivPos;
	unsigned newDivPos;
	long yOfs;

	static RECT rt = {0, 0, 0, 0};

	if (toolbVis == true)
		yOfs = toolBarHeight;
	else
		yOfs = 0;

	rt.left -= (int)treeWidth + (int)dividerWidth;
	rt.right -= (int)treeWidth + (int)dividerWidth;
	rt.left += (int)dividerWidth;
	rt.right += (int)dividerWidth;
	treeDivXPos = LOWORD(lParam) - divDragPt;
	/* MoveWindow(treeDivider, treeDivXPos, 0, dividerWidth, treeHeight, TRUE); */
	rt.left = treeDivXPos; rt.top = yOfs;
	rt.right = treeDivXPos + dividerWidth; rt.bottom = rt.top + treeHeight;
	/* InvalidateRect(tmlnWin, &rt, TRUE); */
	/* UpdateWindow(tmlnWin); */

	/* Depending on whether the new position is before or after the old
	   position, the width and positioning calculations for the back
	   buffer bitmap (hRes) will be different. */
	if (oldTreeDivXPos < treeDivXPos)
	{
		resBmSize = treeDivXPos + dividerWidth - oldTreeDivXPos;
		resBmXPos = oldTreeDivXPos;
		oldDivPos = 0;
		newDivPos = resBmSize - dividerWidth; /* Relative to back buffer bitmap */
	}
	else
	{
		resBmSize = oldTreeDivXPos + dividerWidth - treeDivXPos;
		resBmXPos = treeDivXPos;
		oldDivPos = resBmSize - dividerWidth;
		newDivPos = 0; /* Relative to back buffer bitmap */
	}

	/* Time to compute! */
	{
		HDC hDC;
		HBITMAP hbm;
		HBRUSH hBr;

		HBITMAP hRes; /* Resulting (back buffer) bitmap */
		HDC hDCMem;
		HDC hDCRes;

		const char data[4] = {(char)0x40, (char)0x00, (char)0x80, (char)0x00}; /* 0100 ... 1000 */

		hDC = GetDC(lastHwnd);
		hbm = CreateBitmap(2, 2, 1, 1, &data);
		hBr = CreatePatternBrush(hbm);

		/* Create memory DC's and bitmaps */
		hRes = CreateCompatibleBitmap(hDC, resBmSize, treeHeight);
		hDCMem = CreateCompatibleDC(hDC);
		SelectObject(hDCMem, hbmOld);
		hDCRes = CreateCompatibleDC(hDC);
		SelectObject(hDCRes, hRes);
		SelectObject(hDCRes, hBr);
		/* Get a subcopy of the screen */
		BitBlt(hDCRes, 0, 0, resBmSize, treeHeight, hDC, resBmXPos, yOfs, SRCCOPY);
		/* Erase the bits at the old position */
		BitBlt(hDCRes, oldDivPos, 0, dividerWidth, treeHeight, hDCMem, 0, 0, SRCCOPY);
		/* Save the bits about to get modified */
		BitBlt(hDCMem, 0, 0, dividerWidth, treeHeight, hDCRes, newDivPos, 0, SRCCOPY);
		DeleteDC(hDCMem);
		/* Draw the divider drag marker at the new position */
		PatBlt(hDCRes, newDivPos, 0, dividerWidth, treeHeight, PATINVERT);
		BitBlt(hDC, resBmXPos, yOfs, resBmSize, treeHeight, hDCRes, 0, 0, SRCCOPY);
		DeleteDC(hDCRes);
		DeleteObject(hRes);

		DeleteObject(hBr);
		DeleteObject(hbm);
		ReleaseDC(lastHwnd, hDC);
	}
}

void EndDivDrag()
{
	HDC hDC;
	HDC hDCMem;
	long yOfs;

	if (toolbVis == true)
		yOfs = toolBarHeight;
	else
		yOfs = 0;
	keySplit = false;
	draggingTree = false;
	/* Erase the old hatched bar */
	hDC = GetDC(lastHwnd);
	hDCMem = CreateCompatibleDC(hDC);
	SelectObject(hDCMem, hbmOld);
	BitBlt(hDC, treeDivXPos, yOfs, dividerWidth, treeHeight, hDCMem, 0, 0, SRCCOPY);
	DeleteDC(hDCMem);
	ReleaseDC(lastHwnd, hDC);
	DeleteObject(hbmOld);
	/* End the operation */
	ReleaseCapture();
	MoveWindow(treeDivider, treeDivXPos, yOfs, dividerWidth, treeHeight, TRUE);
	CalculateChildWindowSizes(lastHwnd, false);
	SizeChildWindows();
	treeFrac = (float)(treeDivXPos + divDragPt) / winWidth;
}

void InitPopups(HMENU popup)
{
	int numMenItems;
	UINT id;
	unsigned regBegin;
	unsigned regEnd;
	bool switches[8];
	const unsigned menIDs[8] =
		{M_UNDO, M_REDO, M_CUT, M_COPY, M_DELETE, M_PASTE, M_EMPTYCLIP, M_SELECTALL};
	const unsigned numSwitches = 8;
	/* Temporary variables */
	unsigned i;

	numMenItems = GetMenuItemCount(popup);
	if (numMenItems == 0)
		return;

	/* Initialize the edit menu */
	id = GetMenuItemID(popup, 0);
	if (id != M_UNDO)
		return;

	/* Undo */
	if (SendMessage(dataWin, EM_CANUNDO, 0, 0) == FALSE)
		switches[0] = false;
	else switches[0] = true;

	/* Redo */
	if (SendMessage(dataWin, EM_CANREDO, 0, 0) == FALSE)
		switches[1] = false;
	else switches[1] = true;

	/* Cut, copy, and delete */
	SendMessage(dataWin, EM_GETSEL, (WPARAM)&regBegin, (LPARAM)&regEnd);
	if (regBegin == regEnd)
	{
		switches[2] = false;
		switches[3] = false;
		switches[4] = false;
	}
	else
	{
		switches[2] = true;
		switches[3] = true;
		switches[4] = true;
	}

	/* Paste */
	if (IsClipboardFormatAvailable(CF_TEXT))
		switches[5] = true;
	else switches[5] = false;

	/* Empty clipboard */
	if (OpenClipboard(lastHwnd))
	{
		if (EnumClipboardFormats(0) == 0)
			switches[6] = false;
		else switches[6] = true;
		CloseClipboard();
	}

	/* Select all */
	if (((char*)SendMessage(dataWin, EM_GETHANDLE, 0, 0))[0] == '\0')
		switches[7] = false;
	else switches[7] = true;

	for (i = 0; i < numSwitches; i++)
	{
		UINT dispCmd;
		if (switches[i] == false)
			dispCmd = MF_GRAYED;
		else dispCmd = MF_ENABLED;
		EnableMenuItem(popup, menIDs[i], MF_BYCOMMAND | dispCmd);
	}
}

LRESULT CALLBACK DividerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		dragClickX = LOWORD(lParam) + treeDivXPos;
		dragClickY = HIWORD(lParam) + tmlnDivYPos;
		SendMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), (LPARAM)hwnd);
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK AboutBoxProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndOwner;
	RECT rc, rcDlg, rcOwner;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		/* Center the dialog in the parent window */
		/* Get the owner window and dialog box rectangles */
		if ((hwndOwner = GetParent(hDlg)) == NULL)
		{
			hwndOwner = GetDesktopWindow();
		}

		GetWindowRect(hwndOwner, &rcOwner);
		GetWindowRect(hDlg, &rcDlg);
		CopyRect(&rc, &rcOwner);

		/* Offset the owner and dialog box rectangles so that
		   right and bottom values represent the width and
		   height, and then offset the owner again to discard
		   space taken up by the dialog box. */
		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
		OffsetRect(&rc, -rc.left, -rc.top);
		OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

		/* The new position is the sum of half the remaining
		   space and the owner's original position. */
		SetWindowPos(hDlg,
			HWND_TOP,
			rcOwner.left + (rc.right / 2),
			rcOwner.top + (rc.bottom / 2),
					 0, 0,          /* ignores size arguments */
			SWP_NOSIZE);
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK LinGraphProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		break;
	case WM_DESTROY:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/* CreateToolBar creates a toolbar and adds bitmaps to it.  The
   function returns the handle to the toolbar if successful, or NULL
   otherwise. */
HWND CreateToolBar(HWND hWndParent)
{
	TBBUTTON tbButtonsCreate[10];
	/*TBBUTTON tbButtonsAdd[4];*/
	HWND hWndToolbar;
	/*TBADDBITMAP tb;
	int index, stdidx;*/

	ZeroMemory(&tbButtonsCreate, sizeof(tbButtonsCreate));
	tbButtonsCreate[0].iBitmap = STD_FILENEW;
	tbButtonsCreate[0].idCommand = M_NEW;
	tbButtonsCreate[0].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[0].fsStyle = BTNS_BUTTON;

	tbButtonsCreate[1].iBitmap = STD_FILEOPEN;
	tbButtonsCreate[1].idCommand = M_OPEN;
	tbButtonsCreate[1].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[1].fsStyle = BTNS_BUTTON;

	tbButtonsCreate[2].iBitmap = STD_FILESAVE;
	tbButtonsCreate[2].idCommand = M_SAVE;
	tbButtonsCreate[2].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[2].fsStyle = BTNS_BUTTON;

	tbButtonsCreate[3].iBitmap = 0;
	tbButtonsCreate[3].idCommand = 0;
	tbButtonsCreate[3].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[3].fsStyle = BTNS_SEP;

	tbButtonsCreate[4].iBitmap = STD_CUT;
	tbButtonsCreate[4].idCommand = M_CUT;
	tbButtonsCreate[4].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[4].fsStyle = BTNS_BUTTON;

	tbButtonsCreate[5].iBitmap = STD_COPY;
	tbButtonsCreate[5].idCommand = M_COPY;
	tbButtonsCreate[5].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[5].fsStyle = BTNS_BUTTON;

	tbButtonsCreate[6].iBitmap = STD_PASTE;
	tbButtonsCreate[6].idCommand = M_PASTE;
	tbButtonsCreate[6].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[6].fsStyle = BTNS_BUTTON;

	tbButtonsCreate[7].iBitmap = 0;
	tbButtonsCreate[7].idCommand = 0;
	tbButtonsCreate[7].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[7].fsStyle = BTNS_SEP;

	tbButtonsCreate[8].iBitmap = STD_UNDO;
	tbButtonsCreate[8].idCommand = M_UNDO;
	tbButtonsCreate[8].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[8].fsStyle = BTNS_BUTTON;

	tbButtonsCreate[9].iBitmap = STD_REDOW;
	tbButtonsCreate[9].idCommand = M_REDO;
	tbButtonsCreate[9].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[9].fsStyle = BTNS_BUTTON;

	/*tbButtonsCreate[10].iBitmap = 0;
	tbButtonsCreate[10].idCommand = 0;
	tbButtonsCreate[10].fsState = TBSTATE_ENABLED;
	tbButtonsCreate[10].fsStyle = BTNS_SEP;

	ZeroMemory(&tbButtonsAdd, sizeof(tbButtonsAdd));
	tbButtonsAdd[0].iBitmap = VIEW_LARGEICONS;
	tbButtonsAdd[0].idCommand = IDM_LARGEICON;
	tbButtonsAdd[0].fsState = TBSTATE_ENABLED;
	tbButtonsAdd[0].fsStyle = BTNS_BUTTON;

	tbButtonsAdd[1].iBitmap = VIEW_SMALLICONS;
	tbButtonsAdd[1].idCommand = IDM_SMALLICON;
	tbButtonsAdd[1].fsState = TBSTATE_ENABLED;
	tbButtonsAdd[1].fsStyle = BTNS_BUTTON;

	tbButtonsAdd[2].iBitmap = VIEW_LIST;
	tbButtonsAdd[2].idCommand = IDM_LISTVIEW;
	tbButtonsAdd[2].fsState = TBSTATE_ENABLED;
	tbButtonsAdd[2].fsStyle = BTNS_BUTTON;

	tbButtonsAdd[3].iBitmap = VIEW_DETAILS;
	tbButtonsAdd[3].idCommand = IDM_REPORTVIEW;
	tbButtonsAdd[3].fsState = TBSTATE_ENABLED;
	tbButtonsAdd[3].fsStyle = BTNS_BUTTON;*/

	/* Create a toolbar with three standard file bitmaps and one
	   separator.  There are 15 items in IDB_STD_SMALL_COLOR.
	   However, because this is a standard system-defined bitmap, the
	   parameter that specifies the number of button images in the
	   bitmap (nBitmaps) is ignored, so it is set to 0. */
	hWndToolbar = CreateToolbarEx (hWndParent, 
		WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT, 
		ID_TOOLBAR, 0, HINST_COMMCTRL, IDB_STD_SMALL_COLOR, 
		tbButtonsCreate, 10, 0, 0, 100, 30, sizeof (TBBUTTON));

	/* Add four view bitmaps. The view bitmaps are not in the same
	   file as the standard bitmaps; therefore, you must change the
	   resource identifier from IDB_STD_SMALL_COLOR to
	   IDB_VIEW_SMALL_COLOR. */
	/*tb.hInst = HINST_COMMCTRL;
	tb.nID = IDB_VIEW_SMALL_COLOR;*/
	/* There are 12 items in IDB_VIEW_SMALL_COLOR.  However, because
	   this is a standard system-defined bitmap, wParam (nButtons) is
	   ignored. */
	/*stdidx = SendMessage (hWndToolbar, TB_ADDBITMAP, 0, (LPARAM)&tb);*/

	/* Update the indexes to the view bitmaps. */
	/*for (index = 0; index < 4; index++)
		tbButtonsAdd[index].iBitmap += stdidx;*/

	/* Add the view buttons. */
	/*SendMessage (hWndToolbar, TB_ADDBUTTONS, 4, (LPARAM) &tbButtonsAdd[0]);*/

	return hWndToolbar;
} 
