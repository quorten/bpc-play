/* Central windows startup stuff */
/* Windows GUI will be used for now */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0510
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
/* #include <ErrorRep.h> */
#include "resource.h"

/* #include "FileSysInterface.h" */
#include "TextEdit.h"

static enum {false, true} bool_t;
typedef enum bool_t bool;

HINSTANCE g_hInstance;
TCHAR stringTable[29][100];
bool draggingTree = false; /* This is up here for a strange reason (see "Message loop") */

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CalculateChildWindowSizes(HWND hwnd, bool resize);
void SizeChildWindows();
LRESULT CALLBACK DividerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutBoxProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LinGraphProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
	for (i = M_NEW; i < M_NEW + 29; i++)
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
static float treeFrac = 0.25f; /* Fraction of width that tree divider is at */
static float tmlnFrac = 0.75f; /* Fraction of height that timeline divider is at */
static HWND dataWin;
static HWND treeWin;
static HWND tmlnWin;
static HWND tmlnDivider;
static HWND treeDivider;
static HWND statusWin;
static HWND childFocus; /* Child window with keyboard focus */
static bool draggingTmln = false;
static bool statbVis = true;
static bool treeVis = true;
static bool tmlnVis = false;
static bool keySplit = false;
static POINT oldCursorPos;

/* Variables that reduce function calling */
static int dragClickX;
static int dragClickY;
static HBITMAP hbmOld; /* Used for window divider dragging */

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

		GetWindowRect(statusWin, &rt);
		statusHeight = rt.bottom - rt.top;
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
			SetCapture(hwnd);
			{
				RECT rt;
				HBITMAP hbm;
				HBRUSH hBr;
				HDC hDC;
				HDC hDCMem;
				const char data[4] = {(char)0x40, (char)0x00, (char)0x80, (char)0x00}; /* 0100 ... 1000 */
				rt.left = treeDivXPos; rt.top = 0;
				rt.right = treeDivXPos + dividerWidth; rt.bottom = treeHeight;
				hbm = CreateBitmap(2, 2, 1, 1, &data);
				hBr = CreatePatternBrush(hbm);
				hDC = GetDC(hwnd);
				hbmOld = CreateCompatibleBitmap(hDC, rt.right - rt.left, rt.bottom - rt.top);
				hDCMem = CreateCompatibleDC(hDC);
				SelectObject(hDCMem, hbmOld);
				BitBlt(hDCMem, 0, 0, rt.right - rt.left, rt.bottom - rt.top, hDC, rt.left, rt.top, SRCCOPY);
				DeleteDC(hDCMem);
				SelectObject(hDC, hBr);
				PatBlt(hDC, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
				DeleteObject(hBr);
				DeleteObject(hbm);
				ReleaseDC(hwnd, hDC);
			}
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
					CheckMenuItem(hSub, 4, MF_BYPOSITION | MF_UNCHECKED);
					ShowWindow(treeWin, SW_HIDE);
					ShowWindow(treeDivider, SW_HIDE);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				else
				{
					treeVis = true;
					CheckMenuItem(hSub, 4, MF_BYPOSITION | MF_CHECKED);
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
					CheckMenuItem(hSub, 5, MF_BYPOSITION | MF_UNCHECKED);
					ShowWindow(tmlnWin, SW_HIDE);
					ShowWindow(tmlnDivider, SW_HIDE);
					CalculateChildWindowSizes(hwnd, false);
					SizeChildWindows();
				}
				else
				{
					tmlnVis = true;
					CheckMenuItem(hSub, 5, MF_BYPOSITION | MF_CHECKED);
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
				RECT rt;
				HBITMAP hbm;
				HBRUSH hBr;
				HDC hDC;
				HDC hDCMem;
				const char data[4] = {(char)0x40, (char)0x00, (char)0x80, (char)0x00}; /* 0100 ... 1000 */
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				pt.x = treeDivXPos + divDragPt;
				pt.y = treeHeight / 2 - 1;
				ClientToScreen(hwnd, &pt);
				SetCursorPos(pt.x, pt.y);
				GetClassInfoEx(g_hInstance, "VdividerWindow", &wcex);
				SetCursor(wcex.hCursor);
			SetCapture(hwnd);
				rt.left = treeDivXPos; rt.top = 0;
				rt.right = treeDivXPos + dividerWidth; rt.bottom = treeHeight;
				hbm = CreateBitmap(2, 2, 1, 1, &data);
				hBr = CreatePatternBrush(hbm);
				hDC = GetDC(hwnd);
				hbmOld = CreateCompatibleBitmap(hDC, rt.right - rt.left, rt.bottom - rt.top);
				hDCMem = CreateCompatibleDC(hDC);
				SelectObject(hDCMem, hbmOld);
				BitBlt(hDCMem, 0, 0, rt.right - rt.left, rt.bottom - rt.top, hDC, rt.left, rt.top, SRCCOPY);
				DeleteDC(hDCMem);
				SelectObject(hDC, hBr);
				PatBlt(hDC, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, PATINVERT);
				DeleteObject(hBr);
				DeleteObject(hbm);
				ReleaseDC(hwnd, hDC);
				/* RegisterHotKey(hwnd, VK_ESCAPE, NULL, VK_ESCAPE); */
				/* RegisterHotKey(hwnd, VK_LEFT, NULL, VK_LEFT); */
				/* RegisterHotKey(hwnd, VK_RIGHT, NULL, VK_RIGHT); */
				/* RegisterHotKey(hwnd, VK_RETURN, NULL, VK_RETURN); */
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
		if (IsClipboardFormatAvailable(CF_TEXT))
			EnableMenuItem((HMENU)wParam, M_PASTE, MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem((HMENU)wParam, M_PASTE, MF_BYCOMMAND | MF_GRAYED);
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
		pnmtv = (NMTREEVIEW*)lParam;
		if (pnmtv->hdr.code == TVN_SELCHANGED)
		{
			/* MessageBox(NULL, "BOO!", NULL, MB_OK); */
			/* FSSOnChangeSelection(pnmtv->itemNew.pszText, dataWin); */
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
		HDC hDC;
		HDC hDCMem;
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
				{
				keySplit = false;
			draggingTree = false;
			hDC = GetDC(hwnd);
			hDCMem = CreateCompatibleDC(hDC);
			SelectObject(hDCMem, hbmOld);
			BitBlt(hDC, treeDivXPos, 0, dividerWidth, treeHeight, hDCMem, 0, 0, SRCCOPY);
			DeleteDC(hDCMem);
			ReleaseDC(hwnd, hDC);
			DeleteObject(hbmOld);
			SetCursorPos(oldCursorPos.x, oldCursorPos.y);
			ReleaseCapture();
			MoveWindow(treeDivider, treeDivXPos, 0, dividerWidth, treeHeight, TRUE);
			CalculateChildWindowSizes(hwnd, false);
			SizeChildWindows();
			treeFrac = (float)(treeDivXPos + divDragPt) / winWidth;
			/* UnregisterHotKey(hwnd, VK_ESCAPE); */
			/* UnregisterHotKey(hwnd, VK_LEFT); */
			/* UnregisterHotKey(hwnd, VK_RIGHT); */
			/* UnregisterHotKey(hwnd, VK_RETURN); */
				}
				break;
			}
		}
		if (wParam == VK_ESCAPE)
		{
			/* UnregisterHotKey(hwnd, VK_ESCAPE); */
			if (keySplit == true)
			{
				keySplit = false;
				/* UnregisterHotKey(hwnd, VK_LEFT); */
				/* UnregisterHotKey(hwnd, VK_RIGHT); */
				/* UnregisterHotKey(hwnd, VK_RETURN); */
			}
			if (keySplit == true)
				SetCursorPos(oldCursorPos.x, oldCursorPos.y);
			ReleaseCapture();
		}
		break;
	}
	case WM_LBUTTONDOWN:
		if (keySplit == true)
		{
			HDC hDC;
			HDC hDCMem;
			keySplit = false;
			draggingTree = false;
			hDC = GetDC(hwnd);
			hDCMem = CreateCompatibleDC(hDC);
			SelectObject(hDCMem, hbmOld);
			BitBlt(hDC, treeDivXPos, 0, dividerWidth, treeHeight, hDCMem, 0, 0, SRCCOPY);
			DeleteDC(hDCMem);
			ReleaseDC(hwnd, hDC);
			DeleteObject(hbmOld);
			SetCursorPos(oldCursorPos.x, oldCursorPos.y);
			ReleaseCapture();
			MoveWindow(treeDivider, treeDivXPos, 0, dividerWidth, treeHeight, TRUE);
			CalculateChildWindowSizes(hwnd, false);
			SizeChildWindows();
			treeFrac = (float)(treeDivXPos + divDragPt) / winWidth;
			/* UnregisterHotKey(hwnd, VK_ESCAPE); */
			/* UnregisterHotKey(hwnd, VK_LEFT); */
			/* UnregisterHotKey(hwnd, VK_RIGHT); */
			/* UnregisterHotKey(hwnd, VK_RETURN); */
		}
		break;
	case WM_RBUTTONDOWN:
		break;
	case WM_CAPTURECHANGED:
		if ((HWND)lParam != hwnd)
		{
			if (keySplit == true)
			{
				keySplit = false;
				/* UnregisterHotKey(hwnd, VK_LEFT); */
				/* UnregisterHotKey(hwnd, VK_RIGHT); */
				/* UnregisterHotKey(hwnd, VK_RETURN); */
			}
			if (draggingTree == true)
			{
				draggingTree = false;
				if (keySplit == true)
					SetCursorPos(oldCursorPos.x, oldCursorPos.y);
				treeDivXPos = treeWidth;
				DeleteObject(hbmOld);
				InvalidateRect(hwnd, NULL, TRUE);
				/* UnregisterHotKey(hwnd, VK_ESCAPE); */
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
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			GetClientRect(hwnd, &rt);
			rt.left += divDragPt + vScrollWidth * 2;
			rt.top += divDragPt + vScrollWidth * 2;
			rt.right -= divDragPt + vScrollWidth * 2;
			rt.bottom -= divDragPt + vScrollWidth * 2;
			if (rt.left <= pt.x && pt.x < rt.right)
			{
				HDC hDC;
				HBITMAP hbm;
				HBRUSH hBr;
				unsigned resBmSize;
				const char data[4] = {(char)0x40, (char)0x00, (char)0x80, (char)0x00}; /* 0100 ... 1000 */
				static RECT rt = {0, 0, 0, 0};
				rt.left -= (int)treeWidth + (int)dividerWidth;
				rt.right -= (int)treeWidth + (int)dividerWidth;
				rt.left += (int)dividerWidth;
				rt.right += (int)dividerWidth;
				treeDivXPos = LOWORD(lParam) - divDragPt;
				/* MoveWindow(treeDivider, treeDivXPos, 0, dividerWidth, treeHeight, TRUE); */
				rt.left = treeDivXPos; rt.top = 0;
				rt.right = treeDivXPos + dividerWidth; rt.bottom = treeHeight;
				/* InvalidateRect(tmlnWin, &rt, TRUE); */
				/* UpdateWindow(tmlnWin); */
				hDC = GetDC(hwnd);
				hbm = CreateBitmap(2, 2, 1, 1, &data);
				hBr = CreatePatternBrush(hbm);
				if (oldTreeDivXPos < treeDivXPos)
				{
					HBITMAP hRes;
					HDC hDCMem;
					HDC hDCRes;
					resBmSize = treeDivXPos + dividerWidth - oldTreeDivXPos;
					hRes = CreateCompatibleBitmap(hDC, resBmSize, treeHeight);
					hDCMem = CreateCompatibleDC(hDC);
					SelectObject(hDCMem, hbmOld);
					hDCRes = CreateCompatibleDC(hDC);
					SelectObject(hDCRes, hRes);
					SelectObject(hDCRes, hBr);
					BitBlt(hDCRes, 0, 0, resBmSize, treeHeight, hDC, oldTreeDivXPos, 0, SRCCOPY);
					BitBlt(hDCRes, 0, 0, dividerWidth, treeHeight, hDCMem, 0, 0, SRCCOPY);
					BitBlt(hDCMem, 0, 0, dividerWidth, treeHeight, hDCRes, resBmSize - dividerWidth, 0, SRCCOPY);
					DeleteDC(hDCMem);
					PatBlt(hDCRes, resBmSize - dividerWidth, 0, dividerWidth, treeHeight, PATINVERT);
					BitBlt(hDC, oldTreeDivXPos, 0, resBmSize, treeHeight, hDCRes, 0, 0, SRCCOPY);
					DeleteDC(hDCRes);
					DeleteObject(hRes);
				}
				else
				{
					HBITMAP hRes;
					HDC hDCMem;
					HDC hDCRes;
					resBmSize = oldTreeDivXPos + dividerWidth - treeDivXPos;
					hRes = CreateCompatibleBitmap(hDC, resBmSize, treeHeight);
					hDCMem = CreateCompatibleDC(hDC);
					SelectObject(hDCMem, hbmOld);
					hDCRes = CreateCompatibleDC(hDC);
					SelectObject(hDCRes, hRes);
					SelectObject(hDCRes, hBr);
					BitBlt(hDCRes, 0, 0, resBmSize, treeHeight, hDC, treeDivXPos, 0, SRCCOPY);
					BitBlt(hDCRes, resBmSize - dividerWidth, 0, dividerWidth, treeHeight, hDCMem, 0, 0, SRCCOPY);
					BitBlt(hDCMem, 0, 0, dividerWidth, treeHeight, hDCRes, 0, 0, SRCCOPY);
					DeleteDC(hDCMem);
					PatBlt(hDCRes, 0, 0, dividerWidth, treeHeight, PATINVERT);
					BitBlt(hDC, treeDivXPos, 0, resBmSize, treeHeight, hDCRes, 0, 0, SRCCOPY);
					DeleteDC(hDCRes);
					DeleteObject(hRes);
				}
				DeleteObject(hBr);
				DeleteObject(hbm);
				ReleaseDC(hwnd, hDC);
			}
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
			HDC hDC;
			HDC hDCMem;
			draggingTree = false;
			/* UnregisterHotKey(hwnd, VK_ESCAPE); */
			hDC = GetDC(hwnd);
			hDCMem = CreateCompatibleDC(hDC);
			SelectObject(hDCMem, hbmOld);
			BitBlt(hDC, treeDivXPos, 0, dividerWidth, treeHeight, hDCMem, 0, 0, SRCCOPY);
			DeleteDC(hDCMem);
			ReleaseDC(hwnd, hDC);
			DeleteObject(hbmOld);
			ReleaseCapture();
			MoveWindow(treeDivider, treeDivXPos, 0, dividerWidth, treeHeight, TRUE);
			CalculateChildWindowSizes(hwnd, false);
			SizeChildWindows();
			treeFrac = (float)(treeDivXPos + divDragPt) / winWidth;
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

LRESULT CALLBACK DividerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int xBordSize; /* x Border size */
	static int yBordSize;
	static int xEdgeSize;
	static int yEdgeSize;
	switch (uMsg)
	{
	case WM_CREATE:
		xBordSize = GetSystemMetrics(SM_CXBORDER);
		yBordSize = GetSystemMetrics(SM_CYBORDER);
		xEdgeSize = GetSystemMetrics(SM_CXEDGE);
		yEdgeSize = GetSystemMetrics(SM_CYEDGE);
		break;
	case WM_DESTROY:
		break;
	case WM_LBUTTONDOWN:
		dragClickX = LOWORD(lParam) + treeDivXPos;
		dragClickY = HIWORD(lParam) + tmlnDivYPos;
		SendMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), (LPARAM)hwnd);
		break;
	/*case WM_MOUSEMOVE:
		int x, y;
		LPARAM newlParam;
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		if (GetWindowLong(hwnd, GWL_ID) == TREE_DIVIDER)
			x += treeDivXPos;
		if (GetWindowLong(hwnd, GWL_ID) == TMLN_DIVIDER)
			y += tmlnDivYPos;
		newlParam = x + (y << 16);
		PostMessage((HWND)GetWindowLongPtr(hwnd, GWLP_HWNDPARENT), WM_MOUSEMOVE, wParam, newlParam);
		break;*/
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hDC;
		/* RECT rt, rtClient; */
		hDC = BeginPaint(hwnd, &ps);
		/*GetClientRect(hwnd, &rtClient);
		/* Go simple
		TCHAR windType[3];
		GetClassName(hwnd, windType, 2);
		if (windType[0] == 'V')
		{
			HBRUSH hBr = GetSysColorBrush(COLOR_3DSHADOW);
			rt.left = 0; rt.right = xBordSize;
			rt.top = 0; rt.bottom = rtClient.bottom;
			FillRect(hDC, &rt, hBr);
			rt.left = rtClient.right - xBordSize; rt.right = rtClient.right;
			FillRect(hDC, &rt, hBr);
		}
		else
		{
			HBRUSH hBr = GetSysColorBrush(COLOR_3DSHADOW);
			rt.left = 0; rt.right = rtClient.right;
			rt.top = 0; rt.bottom = yBordSize;
			FillRect(hDC, &rt, hBr);
			rt.top = rtClient.bottom - yBordSize; rt.bottom = rtClient.right;
			FillRect(hDC, &rt, hBr);
		}*/
		/*
		/* Draw 3D look
		/* Draw innermost highlights
		HBRUSH hBr = GetSysColorBrush(COLOR_3DLIGHT);
		rt.left = 0; rt.top = 0;
		rt.right = rtClient.right; rt.bottom = yEdgeSize;
		FillRect(hDC, &rt, hBr);
		rt.right = xEdgeSize; rt.bottom = rtClient.bottom;
		FillRect(hDC, &rt, hBr);
		hBr = GetSysColorBrush(COLOR_3DSHADOW);
		rt.left = rtClient.right - xEdgeSize; rt.top = 0;
		rt.right = rtClient.right; rt.bottom = rtClient.bottom;
		FillRect(hDC, &rt, hBr);
		rt.left = 0; rt.top = rtClient.bottom - yEdgeSize;
		rt.right = rtClient.right; rt.bottom = rtClient.bottom;
		FillRect(hDC, &rt, hBr);
		/* Draw outermost highlights
		hBr = GetSysColorBrush(COLOR_3DHIGHLIGHT);
		rt.left = 0; rt.top = 0;
		rt.right = rtClient.right - xBordSize; rt.bottom = yBordSize;
		FillRect(hDC, &rt, hBr);
		rt.right = xBordSize; rt.bottom = rtClient.bottom - yBordSize;
		FillRect(hDC, &rt, hBr);
		hBr = GetSysColorBrush(COLOR_3DDKSHADOW);
		rt.left = rtClient.right - xBordSize; rt.top = 0;
		rt.right = rtClient.right; rt.bottom = rtClient.bottom;
		FillRect(hDC, &rt, hBr);
		rt.left = 0; rt.top = rtClient.bottom - yBordSize;
		rt.right = rtClient.right; rt.bottom = rtClient.bottom;
		FillRect(hDC, &rt, hBr);
		*/
		EndPaint(hwnd, &ps);
		break;
	}
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
