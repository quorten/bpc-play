/* Panel interface */
/* This is platform dependent code: include windows.h before this
   header. */
/* Explain to users how to build a panel layout.  */

#ifndef PANEL_H
#define PANEL_H

typedef struct Panel_t Panel;

/* This should be SUBPROP_* */
enum PanelSubProps
{
	SUBPAN_NORMAL = -1, /* No specials */
	SUBPAN_LOCK1, /* Size lock "sub1" */
	SUBPAN_LOCK2 /* Size lock "sub2" */
};

struct Panel_t
{
	RECT dims;
	BOOL terminal;
	Panel* parent;
	HWND panWin; /* Only applicable for terminal panels */
	/* Variables below are for non-terminal panels only */
	BOOL horzDiv; /* dividing line runs horizontal */
	HWND divWin;
	long divPos; /* Specifies the absolute (not offset) position of
				    the divider. */
	float divFrac; /* Fractional offset relative to current panel
				      dimensions, used to avoid precision loss while
					  resizing. */
	int subProps; /* See PanelSubProps */
	Panel* sub1;
	Panel* sub2;
};

Panel* CreateFramePanel(HWND frameWin, RECT* dims, BOOL horzDiv, int subProps, HINSTANCE hInstance);
void InsertPanel(HWND frameWin, Panel* pBefore, BOOL horzDiv, int subProps, unsigned oldMoveTo, HINSTANCE hInstance);
void AddTerminalPanel(Panel* parent);
void DeletePanel(Panel* panel, unsigned subToKeep);
void PanelSizeAdjust(Panel* subPanel);
void UpdatePanelSizes(Panel* panel);
void ScalePanels(Panel* panel);
int CalcRtWidth(RECT* rt);
int CalcRtHeight(RECT* rt);
void FreePanels(Panel* frame);

void SizePanelWindows(Panel* panel);
void BeginDivDrag(HWND frameWin, Panel* panel);
void DrawDivDrag(HWND frameWin);
void EndDivDrag(HWND frameWin, BOOL cancel);

BOOL RegisterDivWinClasses(HINSTANCE hInstance);
LRESULT CALLBACK DividerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL DispatchDividerDrag(HWND frameWin, HWND divWin);
void ProcDivDrag(HWND frameWin, int xpos, int ypos);

#endif
