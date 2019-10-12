/* Panel interface */
/* A panel is a rectangular area that may have a horizontal or
   vertical division through it dividing it into two subpanels.  A
   subpanel may either contain more panels or be a terminal panel.

   How to use:

   * To create an initial frame panel with two terminal panels, call
   CreateFramePanel().

   * To create another split through one of the two sections, first
   call InsertPanel() with "pBefore" set to the terminal panel to
   be replaced and moved down the hierarchy, then call
   AddTerminalPanel() to fill in the empty slot in the new
   non-terminal panel.

   * After calling InsertPanel() and AddTerminalPanel(), you must set
   "divPos" and call UpdatePanelSizes() for the subpanel dimensions
   to be valid.

   * Call DeletePanel() to merge and delete a section in a
   non-terminal panel.  This is effectively the reverse of
   InsertPanel(), except it will delete entire subtrees.

   * Call UpdatePanelSizes() whenever a divider was dragged.
   UpdatePanelSizes() will call ScalePanels() as necessary.

   * Call ScalePanels() whenever a resize occurred that requires
   subpanel size recalculations.

   * Call FreePanels() to delete panel hierarchies including and
   downward from the given panel.

   * Call SizePanelWindows() to update the size of the windows that
   are associated with the given panel frame. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <string.h>

#include <crtdbg.h>

#include "Panel.h"
#include "resource.h"

/********************************************************************\
 * Panel data interface												*
\********************************************************************/

/* Our functions primarily treat the panel hierarchy as a
   linked binary tree. */

static int newDivID = NEW_DIV_ID;

/* See PanelSubProps for information on "subProps".  */
Panel* CreateFramePanel(HWND frameWin, RECT* dims, BOOL horzDiv, int subProps, HINSTANCE hInstance)
{
	Panel* newPanel;
	Panel* termPanel;

	newPanel = (Panel*)malloc(sizeof(Panel));
	ZeroMemory(newPanel, sizeof(Panel));
	CopyRect(&newPanel->dims, dims);
	newPanel->terminal = FALSE;
	newPanel->horzDiv = horzDiv;
	newPanel->divFrac = 0.5f;
	newPanel->subProps = subProps;
	if (horzDiv == TRUE && subProps == SUBPAN_NORMAL)
	{
		long cyFrame;
		cyFrame = GetSystemMetrics(SM_CYFRAME);
		newPanel->divWin = CreateWindowEx(WS_EX_WINDOWEDGE,
			"HdividerWindow", NULL, WS_CHILD,
			0, 0, 0, cyFrame,
			frameWin, (HMENU)newDivID, hInstance, NULL);
		SetWindowLongPtr(newPanel->divWin, GWLP_USERDATA, (LONG)newPanel);
		newDivID++;
	}
	if (horzDiv == FALSE && subProps == SUBPAN_NORMAL)
	{
		long cxFrame;
		cxFrame = GetSystemMetrics(SM_CXFRAME);
		newPanel->divWin = CreateWindowEx(WS_EX_WINDOWEDGE,
			"VdividerWindow", NULL, WS_CHILD | WS_VISIBLE,
			0, 0, cxFrame, 0,
			frameWin, (HMENU)newDivID, hInstance, NULL);
		SetWindowLongPtr(newPanel->divWin, GWLP_USERDATA, (LONG)newPanel);
		newDivID++;
	}
	if (horzDiv == TRUE)
		newPanel->divPos = dims->top + CalcRtHeight(dims) / 2;
	else
		newPanel->divPos = dims->left + CalcRtWidth(dims) / 2;
	newPanel->subProps = subProps;

	/* Manually insert one terminal panel */
	termPanel = (Panel*)malloc(sizeof(Panel));
	ZeroMemory(termPanel, sizeof(Panel));
	termPanel->terminal = TRUE;
	termPanel->parent = newPanel;
	newPanel->sub1 = termPanel;
	PanelSizeAdjust(newPanel->sub1);
	/* Do the function call for the other */
	AddTerminalPanel(newPanel);
	UpdatePanelSizes(newPanel);

	return newPanel;
}

/* Inserts a non-terminal panel before the given panel.  "oldMoveTo"
   specifies which subpanel (0 or 1) of the new panel the previous
   panel should go in.  Linking the parent to the new panel is
   automatic except when inserting before the root panel.  The empty
   panel slot in this new non-terminal panel must be filled.
   "divPos" in the new non-terminal panel is also undefined.
   See PanelSubProps for information on "subProps".  */
void InsertPanel(HWND frameWin, Panel* pBefore, BOOL horzDiv, int subProps, unsigned oldMoveTo, HINSTANCE hInstance)
{
	Panel* newPanel;
	newPanel = (Panel*)malloc(sizeof(Panel));
	ZeroMemory(newPanel, sizeof(Panel));
	CopyRect(&newPanel->dims, &pBefore->dims);
	newPanel->terminal = FALSE;
	newPanel->horzDiv = horzDiv;
	newPanel->parent = pBefore->parent;
	newPanel->subProps = subProps;
	if (horzDiv == TRUE && subProps == SUBPAN_NORMAL)
	{
		long cyFrame;
		cyFrame = GetSystemMetrics(SM_CYFRAME);
		newPanel->divWin = CreateWindowEx(WS_EX_WINDOWEDGE,
			"HdividerWindow", NULL, WS_CHILD | WS_VISIBLE,
			0, 0, 0, cyFrame,
			frameWin, (HMENU)newDivID, hInstance, NULL);
		SetWindowLongPtr(newPanel->divWin, GWLP_USERDATA, (LONG)newPanel);
		newDivID++;
	}
	if (horzDiv == FALSE && subProps == SUBPAN_NORMAL)
	{
		long cxFrame;
		cxFrame = GetSystemMetrics(SM_CXFRAME);
		newPanel->divWin = CreateWindowEx(WS_EX_WINDOWEDGE,
			"VdividerWindow", NULL, WS_CHILD | WS_VISIBLE,
			0, 0, cxFrame, 0,
			frameWin, (HMENU)newDivID, hInstance, NULL);
		SetWindowLongPtr(newPanel->divWin, GWLP_USERDATA, (LONG)newPanel);
		newDivID++;
	}

	if (oldMoveTo == 0)
	{
		newPanel->sub1 = pBefore;
		newPanel->sub2 = NULL;
	}
	else
	{
		newPanel->sub1 = NULL;
		newPanel->sub2 = pBefore;
	}

	/* Find whether this panel is the first or second subpanel */
	if (pBefore->parent != NULL)
	{
		if (pBefore->parent->sub1 == pBefore)
			pBefore->parent->sub1 = newPanel;
		else
			pBefore->parent->sub2 = newPanel;
	}
	pBefore->parent = newPanel;
}

/* Adds a terminal panel.  Panel sizes are not changed and the
   terminal panel's width or height may be undefined. */
void AddTerminalPanel(Panel* parent)
{
	Panel* newPanel;
	newPanel = (Panel*)malloc(sizeof(Panel));
	ZeroMemory(newPanel, sizeof(Panel));
	newPanel->terminal = TRUE;
	newPanel->parent = parent;
	PanelSizeAdjust(newPanel);

	/* Figure out which parent anchor is NULL */
	if (parent != NULL)
	{
		if (parent->sub1 == NULL)
			parent->sub1 = newPanel;
		else
			parent->sub2 = newPanel;
	}
}

/* Deletes the given panel and modifies pointers as necessary.
   "subToKeep" is either zero or one.  Do not use this on terminal
   panels. */
void DeletePanel(Panel* panel, unsigned subToKeep)
{
	/* First modify the pointers, then scale the dimensions. */
	Panel* thisPan;
	Panel* otherPan;

	if (subToKeep == 0)
	{
		thisPan = panel->sub1;
		otherPan = panel->sub2;
	}
	else
	{
		thisPan = panel->sub2;
		otherPan = panel->sub1;
	}

	if (otherPan->terminal == FALSE)
		FreePanels(otherPan);
	else
		free(otherPan);

	if (subToKeep == 0)
		panel->sub2 = NULL;
	else
		panel->sub1 = NULL;

	/* Find whether this panel is the first or second subpanel */
	if (panel->parent != NULL)
	{
		if (panel->parent->sub1 == panel)
			panel->parent->sub1 = thisPan;
		else
			panel->parent->sub2 = thisPan;
	}
	thisPan->parent = panel->parent;
	ScalePanels(panel);
	free(panel);
}

/* Adjust the width and/or height of a subpanel to be consistent with
   that of the parent panel. */
void PanelSizeAdjust(Panel* subPanel)
{
	Panel* parent;
	parent = subPanel->parent;
	if (parent->horzDiv == TRUE)
	{
		subPanel->dims.left = parent->dims.left;
		subPanel->dims.right = parent->dims.right;
		/* Find whether this panel is the first or second subpanel */
		if (parent != NULL)
		{
			if (parent->sub1 == subPanel)
				subPanel->dims.top = parent->dims.top;
			else
				subPanel->dims.bottom = parent->dims.bottom;
		}
	}
	else
	{
		subPanel->dims.top = parent->dims.top;
		subPanel->dims.bottom = parent->dims.bottom;
		/* Find whether this panel is the first or second subpanel */
		if (parent != NULL)
		{
			if (parent->sub1 == subPanel)
				subPanel->dims.left = parent->dims.left;
			else
				subPanel->dims.right = parent->dims.right;
		}
	}
}

/* Working downward from the given panel, resizes the panels to
   be consistent with the new divider position of the given panel. */
void UpdatePanelSizes(Panel* panel)
{
	RECT* sub1Dims;
	RECT* sub2Dims;
	sub1Dims = &panel->sub1->dims;
	sub2Dims = &panel->sub2->dims;

	/* Update divFrac */
	if (panel->horzDiv == TRUE)
	{
		panel->divFrac = (float)(panel->divPos - panel->dims.top) /
			CalcRtHeight(&panel->dims);
	}
	else
	{
		panel->divFrac = (float)(panel->divPos - panel->dims.left) /
			CalcRtWidth(&panel->dims);
	}

	/* Update the subpanels */
	if (panel->horzDiv == TRUE)
	{
		long cyFrame;
		cyFrame = GetSystemMetrics(SM_CYFRAME);
		//sub1Dims->top = panel->dims.top;
		sub1Dims->bottom = panel->divPos;
		sub2Dims->top = panel->divPos;
		//sub2Dims->bottom = panel->dims.bottom;
		if (panel->subProps == SUBPAN_NORMAL)
		{
			sub1Dims->bottom -= cyFrame / 2;
			/* Avoid rounding errors */
			sub2Dims->top += cyFrame - cyFrame / 2;
		}
	}
	else
	{
		long cxFrame;
		cxFrame = GetSystemMetrics(SM_CXFRAME);
		//sub1Dims->left = panel->dims.left;
		sub1Dims->right = panel->divPos;
		sub2Dims->left = panel->divPos;
		//sub2Dims->right = panel->dims.right;
		if (panel->subProps == SUBPAN_NORMAL)
		{
			sub1Dims->right -= cxFrame / 2;
			/* Avoid rounding errors */
			sub2Dims->left += cxFrame - cxFrame / 2;
		}
	}
	ScalePanels(panel->sub1);
	ScalePanels(panel->sub2);
}

/* Working downward from the given panel, resizes the panels to
   maintain the previous proportions in the new dimensions. */
void ScalePanels(Panel* panel)
{
	if (panel->terminal == TRUE)
		return;
	if (panel->horzDiv == TRUE)
	{
		/* Find if a subobject got deleted */
		if (panel->sub1 == NULL)
		{
			panel->sub2->dims.top = panel->dims.top;
			panel->sub2->dims.bottom = panel->dims.bottom;
			PanelSizeAdjust(panel->sub2);
			/* Recurse into the subpanels */
			ScalePanels(panel->sub2);
		}
		else if (panel->sub2 == NULL)
		{
			panel->sub1->dims.top = panel->dims.top;
			panel->sub1->dims.bottom = panel->dims.bottom;
			PanelSizeAdjust(panel->sub1);
			/* Recurse into the subpanels */
			ScalePanels(panel->sub1);
		}
		else if (panel->subProps != SUBPAN_NORMAL)
		{
			/* Try to maintain locked panel sizes */
			/* Don't subtract divider widths */
			RECT* sub1Dims;
			RECT* sub2Dims;
			long subLen1, subLen2;
			long newSubLen1, newSubLen2;
			sub1Dims = &panel->sub1->dims;
			sub2Dims = &panel->sub2->dims;
			subLen1 = CalcRtHeight(sub1Dims);
			subLen2 = CalcRtHeight(sub2Dims);
			if (panel->subProps == SUBPAN_LOCK1)
			{
				newSubLen1 = subLen1;
				newSubLen2 = CalcRtHeight(&panel->dims) - subLen1;
			}
			if (panel->subProps == SUBPAN_LOCK2)
			{
				newSubLen1 = CalcRtHeight(&panel->dims) - subLen2;
				newSubLen2 = subLen2;
			}
			sub1Dims->top = panel->dims.top;
			sub1Dims->bottom = sub1Dims->top + newSubLen1;
			sub2Dims->top = sub1Dims->bottom;
			sub2Dims->bottom = sub2Dims->top + newSubLen2;
			PanelSizeAdjust(panel->sub1);
			PanelSizeAdjust(panel->sub2);
			/* Recurse into the subpanels */
			ScalePanels(panel->sub1);
			ScalePanels(panel->sub2);
		}
		else
		{
			/* Caclulate the new lengths */
			RECT* sub1Dims;
			RECT* sub2Dims;
			long subLen1, subLen2;
			long prevLen;
			long newSubLen1, newSubLen2;
			long cyFrame;
			sub1Dims = &panel->sub1->dims;
			sub2Dims = &panel->sub2->dims;
			subLen1 = CalcRtHeight(sub1Dims);
			subLen2 = CalcRtHeight(sub2Dims);
			prevLen = subLen1 + subLen2;
			cyFrame = GetSystemMetrics(SM_CYFRAME);
			//newSubLen1 = MulDiv(subLen1, CalcRtHeight(&panel->dims), prevLen);
			newSubLen1 = (long)(panel->divFrac * CalcRtHeight(&panel->dims));
			newSubLen2 = CalcRtHeight(&panel->dims) - newSubLen1;
			newSubLen1 -= cyFrame / 2;
			//newSubLen2 = MulDiv(subLen2, CalcRtHeight(&panel->dims), prevLen);
			newSubLen2 -= cyFrame - cyFrame / 2; /* Avoid rounding errors */
			/* Modify the dimensions */
			sub1Dims->top = panel->dims.top;
			sub1Dims->bottom = sub1Dims->top + newSubLen1;
			sub2Dims->top = sub1Dims->bottom + cyFrame;
			sub2Dims->bottom = sub2Dims->top + newSubLen2;
			panel->divPos = sub1Dims->bottom + cyFrame / 2;
			PanelSizeAdjust(panel->sub1);
			PanelSizeAdjust(panel->sub2);
			/* Recurse into the subpanels */
			ScalePanels(panel->sub1);
			ScalePanels(panel->sub2);
		}
	}
	else
	{
		/* Find if a subobject got deleted */
		if (panel->sub1 == NULL)
		{
			panel->sub2->dims.left = panel->dims.left;
			panel->sub2->dims.right = panel->dims.right;
			PanelSizeAdjust(panel->sub2);
			/* Recurse into the subpanels */
			ScalePanels(panel->sub2);
		}
		else if (panel->sub2 == NULL)
		{
			panel->sub1->dims.left = panel->dims.left;
			panel->sub1->dims.right = panel->dims.right;
			PanelSizeAdjust(panel->sub1);
			/* Recurse into the subpanels */
			ScalePanels(panel->sub1);
		}
		else if (panel->subProps != SUBPAN_NORMAL)
		{
			/* Try to maintain locked panel sizes */
			/* Don't subtract divider widths */
			RECT* sub1Dims;
			RECT* sub2Dims;
			long subLen1, subLen2;
			long newSubLen1, newSubLen2;
			sub1Dims = &panel->sub1->dims;
			sub2Dims = &panel->sub2->dims;
			subLen1 = CalcRtWidth(sub1Dims);
			subLen2 = CalcRtWidth(sub2Dims);
			if (panel->subProps == SUBPAN_LOCK1)
			{
				newSubLen1 = subLen1;
				newSubLen2 = CalcRtWidth(&panel->dims) - subLen1;
			}
			if (panel->subProps == SUBPAN_LOCK2)
			{
				newSubLen1 = CalcRtWidth(&panel->dims) - subLen2;
				newSubLen2 = subLen2;
			}
			sub1Dims->left = panel->dims.left;
			sub1Dims->right = sub1Dims->left + newSubLen1;
			sub2Dims->left = sub1Dims->right;
			sub2Dims->right = sub2Dims->left + newSubLen2;
			PanelSizeAdjust(panel->sub1);
			PanelSizeAdjust(panel->sub2);
			/* Recurse into the subpanels */
			ScalePanels(panel->sub1);
			ScalePanels(panel->sub2);
		}
		else
		{
			/* Caclulate the new lengths */
			RECT* sub1Dims;
			RECT* sub2Dims;
			long subLen1, subLen2;
			long prevLen;
			long newSubLen1, newSubLen2;
			long cxFrame;
			sub1Dims = &panel->sub1->dims;
			sub2Dims = &panel->sub2->dims;
			subLen1 = CalcRtWidth(sub1Dims);
			subLen2 = CalcRtWidth(sub2Dims);
			prevLen = subLen1 + subLen2;
			cxFrame = GetSystemMetrics(SM_CXFRAME);
			//newSubLen1 = MulDiv(subLen1, CalcRtWidth(&panel->dims), prevLen);
			newSubLen1 = (long)(panel->divFrac * CalcRtWidth(&panel->dims));
			newSubLen2 = CalcRtWidth(&panel->dims) - newSubLen1;
			newSubLen1 -= cxFrame / 2;
			//newSubLen2 = MulDiv(subLen2, CalcRtWidth(&panel->dims), prevLen);
			newSubLen2 -= cxFrame - cxFrame / 2; /* Avoid rounding errors */
			/* Modify the dimensions */
			sub1Dims->left = panel->dims.left;
			sub1Dims->right = sub1Dims->left + newSubLen1;
			sub2Dims->left = sub1Dims->right + cxFrame;
			sub2Dims->right = sub2Dims->left + newSubLen2;
			panel->divPos = sub1Dims->right + cxFrame / 2;
			PanelSizeAdjust(panel->sub1);
			PanelSizeAdjust(panel->sub2);
			/* Recurse into the subpanels */
			ScalePanels(panel->sub1);
			ScalePanels(panel->sub2);
		}
	}
}

int CalcRtWidth(RECT* rt)
{
	return rt->right - rt->left;
}

int CalcRtHeight(RECT* rt)
{
	return rt->bottom - rt->top;
}

/* Frees all dynamic memory resources consumed by the given frame.
   Divider windows that have been created by this code will be
   destroyed, but user added terminal panel HWND's will not.  */
void FreePanels(Panel* frame)
{
	if (frame->sub1->terminal == FALSE)
		FreePanels(frame->sub1);
	else
		free(frame->sub1);

	if (frame->sub2->terminal == FALSE)
		FreePanels(frame->sub2);
	else
		free(frame->sub2);

	/* Now we can free the current frame panel */
	DestroyWindow(frame->divWin);
	free(frame);
}

void SizePanelWindows(Panel* panel)
{
	if (panel->sub1 != NULL)
	{
		if (panel->sub1->terminal == TRUE)
		{
			Panel* subPan;
			subPan = panel->sub1;
			MoveWindow(subPan->panWin, subPan->dims.left, subPan->dims.top,
				CalcRtWidth(&subPan->dims), CalcRtHeight(&subPan->dims), TRUE);
		}
		else
		{
			/* Recurse into this panel */
			SizePanelWindows(panel->sub1);
		}
	}
	if (panel->sub2 != NULL)
	{
		if (panel->sub2->terminal == TRUE)
		{
			Panel* subPan;
			subPan = panel->sub2;
			MoveWindow(subPan->panWin, subPan->dims.left, subPan->dims.top,
				CalcRtWidth(&subPan->dims), CalcRtHeight(&subPan->dims), TRUE);
		}
		else
		{
			/* Recurse into this panel */
			SizePanelWindows(panel->sub2);
		}
	}
	if (panel->terminal == FALSE && panel->subProps == SUBPAN_NORMAL &&
		panel->divWin != NULL)
	{
		long px, py, pw, ph;
		if (panel->horzDiv == TRUE)
		{
			long cyFrame;
			cyFrame = GetSystemMetrics(SM_CYFRAME);
			px = panel->dims.left;
			py = panel->divPos;
			py -= cyFrame / 2;
			pw = CalcRtWidth(&panel->dims);
			ph = cyFrame;
		}
		else
		{
			long cxFrame;
			cxFrame = GetSystemMetrics(SM_CXFRAME);
			px = panel->divPos;
			px -= cxFrame / 2;
			py = panel->dims.top;
			pw = cxFrame;
			ph = CalcRtHeight(&panel->dims);
		}
		MoveWindow(panel->divWin, px, py, pw, ph, TRUE);
	}
}

/********************************************************************\
 * Divider dragging													*
\********************************************************************/

/* Dragging window dividers should always be a synchronous, single
   tasking process.  */

//static long draggingDiv;
static Panel* dragPanel;
static BOOL keySplit = FALSE;

/* Variables that reduce function calling */
static unsigned vScrollWidth;
static HBITMAP hbmOld;
static long oldDivPos;

void BeginDivDrag(HWND frameWin, Panel* panel)
{
	RECT rt;
	HBITMAP hbm;
	HBRUSH hBr;
	HDC hDC;
	HDC hDCMem;
	const char data[4] = {0x40, 0x00, 0x80, 0x00}; /* 0100 ... 1000 */

	dragPanel = panel;
	SetCapture(frameWin);
	oldDivPos = dragPanel->divPos;
	/* Set the hatched bar size */
	if (dragPanel->horzDiv == TRUE)
	{
		long cyFrame;
		cyFrame = GetSystemMetrics(SM_CYFRAME);
		rt.left = dragPanel->dims.left;
		rt.top = dragPanel->divPos - cyFrame / 2;
		rt.right = dragPanel->dims.right;
		rt.bottom = rt.top + cyFrame;
	}
	else
	{
		long cxFrame;
		cxFrame = GetSystemMetrics(SM_CXFRAME);
		rt.left = dragPanel->divPos - cxFrame / 2;
		rt.top = dragPanel->dims.top;
		rt.right = rt.left + cxFrame;
		rt.bottom = dragPanel->dims.bottom;
	}
	/* Create the pattern brush */
	hbm = CreateBitmap(2, 2, 1, 1, &data);
	hBr = CreatePatternBrush(hbm);
	/* Prepare the previous bits */
	hDC = GetDC(frameWin);
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
	ReleaseDC(frameWin, hDC);
}

/* This function redraws the divider drag graphic by first performing all
   screen operations on a back-buffer bitmap, then doing a single BitBlt()
   to the actual screen.  You should have already set dragPanel->dragPos
   to its new value.  "frameWin" is the parent window that contains the
   entire frame set.  */
void DrawDivDrag(HWND frameWin)
{
	long resBmXPos;
	long resBmYPos;
	long resBmWidth;
	long resBmHeight;
	long resDivWidth;
	long resDivHeight;
	long oldResDivPosX;
	long oldResDivPosY;
	long resDivPosX;
	long resDivPosY;

	/* Calculate horz./vert. independent measures.  */
	if (dragPanel->horzDiv == TRUE)
	{
		long cyFrame;
		cyFrame = GetSystemMetrics(SM_CYFRAME);
		resDivWidth = dragPanel->dims.right - dragPanel->dims.left;
		resDivHeight = cyFrame;

		/* Depending on whether the new position is before or after the old
		   position, the width and positioning calculations for the back
		   buffer bitmap (hRes) will be different. */
		if (oldDivPos < dragPanel->divPos)
		{
			resBmHeight = dragPanel->divPos + cyFrame - oldDivPos;
			resBmYPos = oldDivPos - cyFrame / 2;
			oldResDivPosY = 0;
			resDivPosY = resBmHeight - cyFrame; /* Relative to back buffer bitmap */
		}
		else
		{
			resBmHeight = oldDivPos + cyFrame - dragPanel->divPos;
			resBmYPos = dragPanel->divPos - cyFrame / 2;
			oldResDivPosY = resBmHeight - cyFrame;
			resDivPosY = 0; /* Relative to back buffer bitmap */
		}
		resBmXPos = dragPanel->dims.left;
		resBmWidth = CalcRtWidth(&dragPanel->dims);
		resDivWidth = resBmWidth;
		resDivHeight = cyFrame;
		oldResDivPosX = 0;
		resDivPosX = 0;
	}
	else
	{
		long cxFrame;
		cxFrame = GetSystemMetrics(SM_CXFRAME);
		resDivWidth = cxFrame;
		resDivHeight = dragPanel->dims.bottom - dragPanel->dims.top;

		/* Depending on whether the new position is before or after the old
		   position, the width and positioning calculations for the back
		   buffer bitmap (hRes) will be different. */
		if (oldDivPos < dragPanel->divPos)
		{
			resBmWidth = dragPanel->divPos + cxFrame - oldDivPos;
			resBmXPos = oldDivPos - cxFrame / 2;
			oldResDivPosX = 0;
			resDivPosX = resBmWidth - cxFrame; /* Relative to back buffer bitmap */
		}
		else
		{
			resBmWidth = oldDivPos + cxFrame - dragPanel->divPos;
			resBmXPos = dragPanel->divPos - cxFrame / 2;
			oldResDivPosX = resBmWidth - cxFrame;
			resDivPosX = 0; /* Relative to back buffer bitmap */
		}
		resBmYPos = dragPanel->dims.top;
		resBmHeight = CalcRtHeight(&dragPanel->dims);
		resDivHeight = resBmHeight;
		resDivWidth = cxFrame;
		oldResDivPosY = 0;
		resDivPosY = 0;
	}

	/* Time to rasterize! */
	{
		HDC hDC;
		HBITMAP hbm;
		HBRUSH hBr;

		HBITMAP hRes; /* Resulting (back buffer) bitmap */
		HDC hDCMem;
		HDC hDCRes;

		const char data[4] = {(char)0x40, (char)0x00, (char)0x80, (char)0x00}; /* 0100 ... 1000 */

		hDC = GetDC(frameWin);
		hbm = CreateBitmap(2, 2, 1, 1, &data);
		hBr = CreatePatternBrush(hbm);

		/* Create memory DC's and bitmaps */
		hRes = CreateCompatibleBitmap(hDC, resBmWidth, resBmHeight);
		hDCMem = CreateCompatibleDC(hDC);
		SelectObject(hDCMem, hbmOld);
		hDCRes = CreateCompatibleDC(hDC);
		SelectObject(hDCRes, hRes);
		SelectObject(hDCRes, hBr);
		/* Get a subcopy of the screen */
		BitBlt(hDCRes, 0, 0, resBmWidth, resBmHeight, hDC, resBmXPos, resBmYPos, SRCCOPY);
		/* Erase the bits at the old position */
		BitBlt(hDCRes, oldResDivPosX, oldResDivPosY, resDivWidth,
			resDivHeight, hDCMem, 0, 0, SRCCOPY);
		/* Save the bits about to get modified */
		BitBlt(hDCMem, 0, 0, resDivWidth, resDivHeight, hDCRes, resDivPosX, resDivPosY, SRCCOPY);
		DeleteDC(hDCMem);
		/* Draw the divider drag marker at the new position */
		PatBlt(hDCRes, resDivPosX, resDivPosY, resDivWidth, resDivHeight, PATINVERT);
		BitBlt(hDC, resBmXPos, resBmYPos, resBmWidth, resBmHeight, hDCRes, 0, 0, SRCCOPY);
		DeleteDC(hDCRes);
		DeleteObject(hRes);

		DeleteObject(hBr);
		DeleteObject(hbm);
		ReleaseDC(frameWin, hDC);
	}
	oldDivPos = dragPanel->divPos;
}

/* You should have already set dragPanel->dragPos to its new value.  "frameWin" is
   the parent window that contains the entire frame set.  */
void EndDivDrag(HWND frameWin, BOOL cancel)
{
	HDC hDC;
	HDC hDCMem;
	long xOfs;
	long yOfs;
	long clearWidth;
	long clearHeight;

	/* Calculate horz./vert. independent measures.  */
	if (dragPanel->horzDiv == TRUE)
	{
		long cyFrame;
		cyFrame = GetSystemMetrics(SM_CYFRAME);
		xOfs = dragPanel->dims.left;
		yOfs = dragPanel->divPos - cyFrame / 2;
		clearWidth = CalcRtWidth(&dragPanel->dims);
		clearHeight = cyFrame;
	}
	else
	{
		long cxFrame;
		cxFrame = GetSystemMetrics(SM_CXFRAME);
		xOfs = dragPanel->divPos - cxFrame / 2;
		yOfs = dragPanel->dims.top;
		clearWidth = cxFrame;
		clearHeight = CalcRtHeight(&dragPanel->dims);
	}

	/* Erase the old hatched bar */
	hDC = GetDC(frameWin);
	hDCMem = CreateCompatibleDC(hDC);
	SelectObject(hDCMem, hbmOld);
	BitBlt(hDC, xOfs, yOfs, clearWidth, clearHeight,
		hDCMem, 0, 0, SRCCOPY);
	DeleteDC(hDCMem);
	ReleaseDC(frameWin, hDC);
	DeleteObject(hbmOld);
	/* End the operation */
	ReleaseCapture();
	//dragPanel->divPos = draggingDiv;
	if (cancel == TRUE)
	{
		if (dragPanel->horzDiv == TRUE)
		{
			dragPanel->divPos = dragPanel->sub1->dims.bottom;
			if (dragPanel->subProps == SUBPAN_NORMAL)
			{
				long cyFrame;
				cyFrame = GetSystemMetrics(SM_CYFRAME);
				dragPanel->divPos += cyFrame / 2;
			}
		}
		else
		{
			dragPanel->divPos = dragPanel->sub1->dims.right;
			if (dragPanel->subProps == SUBPAN_NORMAL)
			{
				long cxFrame;
				cxFrame = GetSystemMetrics(SM_CXFRAME);
				dragPanel->divPos += cxFrame / 2;
			}
		}
	}
	UpdatePanelSizes(dragPanel);
	SizePanelWindows(dragPanel);
}

/********************************************************************\
 * Windowing and messaging											*
\********************************************************************/

/* Variables that reduce function calling */
static POINT oldCursorPos;

BOOL RegisterDivWinClasses(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	/* Register the vertical divider window class.  */
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = DividerWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_SIZEWE);
	wcex.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "VdividerWindow";
	wcex.hIconSm = NULL;

	if (!RegisterClassEx(&wcex))
		return FALSE;

	/* Register the horizontal divider window class.  */
	wcex.hCursor = LoadCursor(NULL, IDC_SIZENS);
	wcex.lpszClassName = "HdividerWindow";

	if (!RegisterClassEx(&wcex))
		return FALSE;

	/* Set up window divider measures */
	//divDragPt = dividerWidth / 2;
	//if (dividerWidth % 2 != 1)
	//	divDragPt--;
	vScrollWidth = GetSystemMetrics(SM_CXVSCROLL);

	return TRUE;
}

LRESULT CALLBACK DividerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		SendMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), (LPARAM)hwnd);
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/* Dispatch a potential mouse divider drag event in WM_COMMAND.  */
BOOL DispatchDividerDrag(HWND frameWin, HWND divWin)
{
	Panel* clickedPanel;
	char* clsName;
	clsName = (LPSTR)malloc(sizeof(char) * 50); /* Never allocate "big data" on the stack.  */
	RealGetWindowClass(divWin, clsName, 50);
	if (strcmp(clsName, "VdividerWindow") != 0 &&
		strcmp(clsName, "HdividerWindow") != 0)
	{
		free(clsName);
		return FALSE;
	}
	free(clsName);
	clickedPanel = (Panel*)GetWindowLongPtr(divWin, GWLP_USERDATA);
	BeginDivDrag(frameWin, clickedPanel);
	return TRUE;
}

/* Update a divider drag mark of a divider being mouse dragged.  */
void ProcDivDrag(HWND frameWin, int xpos, int ypos)
{
	long cnFrame;
	RECT rt;
	if (dragPanel->horzDiv == TRUE)
		cnFrame = GetSystemMetrics(SM_CYFRAME);
	else
		cnFrame = GetSystemMetrics(SM_CXFRAME);
	CopyRect(&rt, &dragPanel->dims);
	rt.left += cnFrame + vScrollWidth * 2;
	rt.top += cnFrame + vScrollWidth * 2;
	rt.right -= cnFrame + vScrollWidth * 2;
	rt.bottom -= cnFrame + vScrollWidth * 2;
	if (CalcRtWidth(&dragPanel->dims) <= vScrollWidth * 2)
		xpos = (rt.left + rt.right) / 2;
	else if (xpos < rt.left)
		xpos = rt.left;
	else if (xpos >= rt.right)
		xpos = rt.right - 1;
	if (CalcRtHeight(&dragPanel->dims) <= vScrollWidth * 2)
		ypos = (rt.top + rt.bottom) / 2;
	else if (ypos < rt.top)
		ypos = rt.top;
	else if (ypos >= rt.bottom)
		ypos = rt.bottom - 1;

	if (dragPanel->horzDiv == TRUE)
		dragPanel->divPos = ypos;
	else
		dragPanel->divPos = xpos;
	DrawDivDrag(frameWin);
}

/* Initiate a keyboard-based divider adjusting session.  */
void KeyDividerDrag(HWND frameWin, HINSTANCE hInstance)
{
	keySplit = TRUE;
	//draggingDiv = true;
	GetCursorPos(&oldCursorPos);
	{
		POINT pt;
		WNDCLASSEX wcex;
		/* Set the cursor properly */
		GetCursorPos(&pt);
		ScreenToClient(frameWin, &pt);
		/* Start by highlighting the first draggable divider.  */
		pt.x = 2;
		pt.y = 2;
		ClientToScreen(frameWin, &pt);
		SetCursorPos(pt.x, pt.y);
		GetClassInfoEx(hInstance, "VdividerWindow", &wcex);
		SetCursor(wcex.hCursor);
		//BeginDivDrag(frameWin);
	}
}

/* Update a divider drag mark of a divider being keyboard edited.  */
void ProcKeyDivDrag()
{
	/* Prompt the user for the divider to change.  */
	/* Edit this divider.  */
		/*if (keySplit == true)
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
				keySplit = false;
				draggingDiv = false;
				EndDivDrag(hwnd);
				break;
			}
		}*/
			if (keySplit == TRUE)
			{
				keySplit = FALSE;
				SetCursorPos(oldCursorPos.x, oldCursorPos.y);
			}
}

void KeyLBtnDown()
{
		if (keySplit == TRUE)
		{
			SetCursorPos(oldCursorPos.x, oldCursorPos.y);
			keySplit = FALSE;
			//draggingDiv = false;
			//EndDivDrag(hwnd, FALSE);
		}
		//For WM_CAPTURECHANGED
				if (keySplit == TRUE)
					SetCursorPos(oldCursorPos.x, oldCursorPos.y);
}
