/*Core Windows text editor code base - Foundation module for custom coded text editor.
  This module is meant to kind of like the code base that is behind the functioning
  of the standard multiline edit control available in Windows. However, some minor
  flaws have been discovered in this simple but convenient window control:
		1. Word wrapping does not always stay up to date.
		2. In order draw custom multiple highlights behind the text, it seems that
		   you would process the WM_CTRLCOLOREDIT message and draw to the DC based
		   off of what text is on what line.
  It is really only the last flaw that makes things kind of unreliable. You could
  experiment and read on more about Windows edit controls, but I chose to do a more
  bottom-up implementation of the text editor, thinking that it would be better in
  the long run.

  NOTE: Mouse vanish is not supported on Windows 95/98
  NOTE: Mouse wheel requires special support on Windows 95
  General code revision
  Revise the wrapping code
  Check the border code
  Check mouse position code/various 16/32 bit parts
  Get rid of HIWORDs and LOWORDs
  Check for numeric overflows
*/


#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501 //or _WIN32_WINDOWS 0x410
#include <windows.h>
#include <commctl.h>
#include "resource.h"

#include <stdlib.h>
#include <string.h>

const unsigned truncLen = 8192; //Maximum length of lines before line breaking is forced
#define TRUNC_LEN 8192

struct UndoEntry
{
	unsigned type; //Was the operation a (0) insert, (1) delete, or (2) replace?
	bool caretFirst; //Did the caret come before the region beginning?
	unsigned opPos; //Position to change text
	char* data; //If replace, this is the new data
	char* oldData; //Only set on replace operations
};

//Text data and caret variables
char* buffer;
unsigned buffSize;
bool ownBuffer;
unsigned textSize;
unsigned textPos; //Byte-wise caret position
unsigned caretLine;
int caretLnOffst; //Difference from text line to displayed line (used to emulate weird caret snapping behavior)
unsigned caretX; //X position of caret (without borders or margins)
unsigned caretY; //Y position of caret (without borders)
bool overwrite; //Is overwrite mode enabled?
bool wasInsert; //Was the last operation an insert or a delete?
bool sameUndoOp; //Should the next operation be added as part of the current undo entry?
unsigned regionBegin;
bool regionActive;
UndoEntry* undoHistory;
unsigned numUndo;
unsigned curUndo;

//Text metric and display varaibles
HFONT hFont;
unsigned fontHeight;
bool fixedPitch;
unsigned avgCharWidth;
unsigned maxCharWidth;
unsigned cWidths[256];
int numkPairs;
KERNINGPAIR* kernPairs;

unsigned leftMargin, rightMargin;
unsigned textAreaWidth;
unsigned textAreaHeight;
unsigned numLines;
unsigned* lineStarts; //This array always maintains an extra entry that is equal to the text size.
POLYTEXT* norTextRI;
POLYTEXT* regTextRI;
unsigned numTabStops;
unsigned* tabStops;
bool wrapWords;

//Mouse and mouse wheel scroll variables
bool cursorVis; //Is the cursor visible now?
bool drawDrag; //Should the mouse pan mark be drawn?
POINT dragP; //Point to draw the mouse pan mark at
bool mBtnTimeUp; //Will releasing the middle mouse button deactivate the mouse pan mark?
HICON panMark;
HCURSOR panCurs[11];
unsigned panCursMap[] = {10, 1, 0, 8,  //This is used to reduce the size of the cursor setting code
						 3, 7, 5, 0,   //(see cursMap.txt for picture)
						 2, 6, 4, 0,
						 9, 0, 0, 0};

//Render parameters
unsigned beginUpdLine, endUpdLine; //Line range for recalculating render info (display relative)
//End update line is one line beyond the lines getting updated
unsigned norUpLine, numNorUpLines; //Offset from and number in visible text lines to redraw
//unsigned regUpLine, numRegUpLines; //Offset from and number in visible region lines to redraw
unsigned lastWrappedLine; //Which line was last corrected in rewrapping or retruncating lines
unsigned lastTextPos; //Used to find which parts of the region need to be drawn
unsigned lastCaretLine; //Same as lastTextPos
RECT rendRect; //What the clip rectagle should be for previous parameters to apply

//Variables that reduce function calling
unsigned wheelLines; //This needs updating at WM_SETTINGCHANGE
unsigned dblClickTime; //Same with this
BOOL mouseVanish; //And this
unsigned bordWidthX, bordWidthY; //Thin-line border widths
unsigned numVisLines; //Number of visible lines (including partials)
unsigned regBeginLine; //Which line the region begins on
unsigned numRegVisLines;
unsigned visLine; //First partial visible line
unsigned visLnOffset; //Offset from top of first partial visible line to window top
unsigned xScrollPos; //Position of the horizontal scroll bar
unsigned xMaxScroll; //Length of longest line in the text
unsigned* numVisChars; //Number of visible characters on each line
unsigned* firstVisChars; //First characters on each line to be partially visible
unsigned* firstVisOffset; //Offset from beginning of partially visible characters to left window edge
POINT lastMousePos;
POINT scrdiff; //Difference from client area origin to screen origin
bool lBtnDown; //The mouse was clicked in our window
unsigned winWidth, winHeight;

//Variables that reduce parameter passing
HWND lastHwnd;

//Private function declarations
void UpdateFont(HFONT newFont);
void CalcTextAreaDims();
void CalcNumVisLines();
void GenTabStops();
void WrapWords();
void ProcessTabs(unsigned lineNum, int* charWidths, unsigned endIndex);
void TruncateLines();
void UpdateRenderInfo(bool onlyRegion = false);
void UpdateScrollInfo();
void ScrollContents(bool offset, unsigned otherType, int amount, int sBar);
bool ScrollRenderStructs(bool vert, int offset);
void MButtonScroll(bool setCursor); //Function called for processing mouse movements after pressing middle mouse button
void MScrollEnd();
void MWheelScroll(int amount);
void InsertText(char c, const char* str, bool silent);
void DeleteText(unsigned p1, unsigned p2, bool silent);
void ProcessMiscKey(WPARAM wParam);
void AddUndo(); //Add an operation to the undo queue
void RewrapWords(bool setUpdLines = false);
void RetruncateLines(bool setUpdLines = false);
void InsertLineStart(unsigned prevLine);
void CopyRegion();
void SkipWord(bool forward);
void CalcCaretPos();
void UpdateCaretPos();
void TextPosFromPoint();
void UpdateRegion(bool setRegActive);
void TryCursorUnhide();
void SortAscending(unsigned* p1, unsigned* p2);
void FreeUndoHist();
void FreeRenderInfo(bool freeNorRI = true);
void FreeLineInfo();

LRESULT CALLBACK TextEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HCURSOR hIBeam;

	lastHwnd = hwnd;
	switch (uMsg)
	{
	//Window management messages
	case WM_CREATE:
		//Initialize caret
		textPos = 0;
		caretLine = 0;
		caretX = 0;
		caretY = 0;
		caretLnOffst = 0;

		//Nullify font display pointers
		kernPairs = NULL;
		lineStarts = NULL;
		norTextRI = NULL;
		regTextRI = NULL;
		tabStops = NULL;
		//Set default word wrapping state
		wrapWords = true;
		//Set client to screen coordinate corrector
		scrdiff.x = 0; scrdiff.y = 0;
		ClientToScreen(hwnd, &scrdiff);
		//Initialize icon handle
		hIBeam = LoadCursor(NULL, IDC_IBEAM);

		//Initialize font
		UpdateFont(NULL);

		//Initialize buffer
		buffer = (char*)malloc(1000);
		ownBuffer = true;
		buffSize = 1000;
		textSize = 0;

		//Initialize basic text edit
		overwrite = false;
		wasInsert = false;
		sameUndoOp = false;
		regionActive = false;
		undoHistory = NULL;
		numUndo = 0;
		curUndo = 0;

		//Initialize mouse
		lBtnDown = false;
		cursorVis = true;

		//Initialize mouse wheel
		drawDrag = false;
		mBtnTimeUp = false;
		if (wrapWords == true)
			panMark = LoadIcon(g_hInstance, (LPCTSTR)VERT_MARK);
		else
			panMark = LoadIcon(g_hInstance, (LPCTSTR)CENT_MARK);
		for (unsigned i = 0; i < 11; i++)
			panCurs[i] = LoadCursor(g_hInstance, (LPCTSTR)(U_ARROW+i));

		//Initialize text scroll cache
		numVisLines = 0;
		numRegVisLines = 0;
		visLine = 0;
		visLnOffset = 0;
		regBeginLine = 0;
		xScrollPos = 0;
		xMaxScroll = 0;
		numVisChars = NULL;
		firstVisChars = NULL;
		firstVisOffset = NULL;

		//Initialize redraw cache
		beginUpdLine = 0;
		endUpdLine = 0;
		norUpLine = 0;
		numNorUpLines = 0;
		lastWrappedLine = 0;
		lastTextPos = 0;
		lastCaretLine = 0;
		newLineAtBottom = false;
		newLineAtTop = false;

		//Initialize the system variables too
	case WM_SETTINGCHANGE:
		SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &wheelLines, 0);
		SystemParametersInfo(SPI_GETMOUSEVANISH, 0, &mouseVanish, 0);
		dblClickTime = GetDoubleClickTime();
		bordWidthX = GetSystemMetrics(SM_CXBORDER);
		bordWidthY = GetSystemMetrics(SM_CYBORDER);
		break;
	case WM_DESTROY:
		//Is WM_KILLFOCUS called before WM_DESTROY automatically?
		TryCursorUnhide();
		if (ownBuffer == true)
		{
			free(buffer);
			buffer = NULL;
		}
		FreeUndoHist();
		free(kernPairs);
		kernPairs = NULL;
		free(lineStarts);
		lineStarts = NULL;
		FreeRenderInfo();
		free(tabStops);
		tabStops = NULL;
		FreeLineInfo();
		break;
	case WM_PAINT:
	{
		//We have to re-get these because Windows 95 does not send a proper notification
		COLORREF regBkCol = GetSysColor(COLOR_HIGHLIGHT);
		COLORREF regTxtCol = GetSysColor(COLOR_HIGHLIGHTTEXT);
		COLORREF norTxtCol = GetSysColor(COLOR_WINDOWTEXT);

		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwnd, &ps);
		IntersectClipRect(hDC, bordWidthX, bordWidthY,
			winWidth - bordWidthX, winHeight - bordWidthY);
		PolyTextOut(hDC, norTextRI, numVisLines);
		if (regionActive == true)
			PolyTextOut(hDC, regTextRI, numRegLines);
		if (drawDrag == true)
			DrawIcon(hDC, dragP.x, dragP.y, panMark);
		EndPaint(hwnd, &ps);
		endUpdLine = 0;
		break;
	}
	case WM_SIZE:
		winWidth = LOWORD(lParam);
		winHeight = HIWORD(lParam);
		CalcTextAreaDims();
		GenTabStops();
		if (wrapWords == true)
			WrapWords();
		else
			TruncateLines();
		UpdateScrollInfo();
		CalcCaretPos();
		UpdateCaretPos();
		break;
	case WM_MOVE:
		scrdiff.x = 0; scrdiff.y = 0;
		ClientToScreen(hwnd, &scrdiff);
		break;
	case WM_CAPTURECHANGED:
		lBtnDown = false;
		break;
	//Special input messages
	case WM_HSCROLL:
		ScrollContents(false, LOWORD(wParam), NULL, SB_HORZ);
		break;
	case WM_VSCROLL:
		ScrollContents(false, LOWORD(wParam), NULL, SB_VERT);
		break;
	case WM_MOUSEACTIVATE:
		return MA_ACTIVATE;
	case WM_CONTEXTMENU:
	{
		HMENU hMaster;
		HMENU hMen;
		POINT pt;
		hMaster = LoadMenu(g_hInstance, (LPCTSTR)SHRTCMENUS);
		hMen = GetSubMenu(hMaster, 0);
		pt.x = GET_X_PARAM(lParam);
		pt.y = GET_Y_PARAM(lParam);
		if (pt.x == -1)
		{
			pt.x = (long)caretX; pt.y = (long)caretY;
			ClientToScreen(hwnd, &pt);
		}
		if (!IsClipboardFormatAvailable(CF_TEXT))
			EnableMenuItem(hMen, M_PASTE, MF_BYCOMMAND | MF_GRAYED);
		if (regionActive == false)
		{
			EnableMenuItem(hMen, M_CUT, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMen, M_COPY, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMen, M_DELETE, MF_BYCOMMAND | MF_GRAYED);
		}
		TrackPopupMenu(hMen, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, NULL, hwnd, NULL);
		DestroyMenu(hMaster);
		break;
	}
	case WM_TIMER:
		if (wParam == 1)
			MButtonScroll(false);
		if (wParam == 2)
		{
			mBtnTimeUp = true;
			KillTimer(hwnd, 2);
		}
		break;
	case EM_SETHANDLE:
		if (ownBuffer == true)
		{
			ownBuffer = false;
			free(buffer);
		}
		buffer = (char*)wParam;
		buffSize = (unsigned)lParam;
		textSize = strlen(buffer);
		textPos = 0;
		if (regionActive == true)
		{
			UpdateRegion(false);
		} //more?
		WrapWords();
		CalcCaretPos();
		UpdateCaretPos();
		break;
	//Edit commands
	case WM_CUT:
		if (regionActive == true)
		{
			CopyRegion();
			DeleteText(regionBegin, textPos, false);
			UpdateRegion(false);
		}
		break;
	case WM_COPY:
		if (regionActive == true)
			CopyRegion();
		break;
	case WM_PASTE:
		if (OpenClipboard(hwnd))
		{
			HANDLE clipMem = GetClipboardData(CF_TEXT);
			char* clipCont = (char*)GlobalLock(clipMem);
			if (regionActive == true)
			{
				DeleteText(regionBegin, textPos, false);
				UpdateRegion(false);
			}
			if (clipCont != NULL)
			{
				//Convert \r\n pairs to \n
				unsigned dataSize = strlen(clipCont);
				char* newData = (char*)malloc(dataSize + 1);
				unsigned newDataSize = 0;
				for (unsigned i = 0; i < dataSize; i++)
				{
					if (clipCont[i] != '\r')
						newData[newDataSize++] = clipCont[i];
					else
					{
						if (clipCont[i+1] == '\n') //CR+LF
							i++;
						newData[newDataSize++] = '\n';
					}
				}
				newData[newDataSize++] = '\0';
				InsertText(NULL, newData, false);
				free(newData);
			}
			GlobalUnlock(clipMem);
			CloseClipboard();
		}
		break;
	case WM_CLEAR:
		if (regionActive == true)
		{
			DeleteText(regionBegin, textPos, false);
			UpdateRegion(false);
		}
		break;
	case WM_UNDO:
		if (curUndo > 0)
		{
			if (undoHistory[curUndo].type == 0)
			{
				regionBegin = undoHistory[curUndo].opPos;
				textPos = regionBegin;
				InsertText(NULL, undoHistory[curUndo].data, true);
				if (undoHistory[curUndo].caretFirst == true)
				{
					unsigned t;
					t = regionBegin;
					regionBegin = textPos;
					textPos = t;
				}
				UpdateRegion(true);
			}
			else if (undoHistory[curUndo].type == 1)
			{
				DeleteText(undoHistory[curUndo].opPos,
					undoHistory[curUndo].opPos + strlen(undoHistory[curUndo].data), true);
				textPos = undoHistory[curUndo].opPos;
			}
			else
			{
				//Replace text
				DeleteText(undoHistory[curUndo].opPos,
					undoHistory[curUndo].opPos + strlen(undoHistory[curUndo].data), true);
				textPos = undoHistory[curUndo].opPos;
				InsertText(NULL, undoHistory[curUndo].oldData, true);
			}
			curUndo--;
		}
		break;
	case EM_REDO:
		if (curUndo < numUndo - 1)
		{
			if (undoHistory[curUndo].type == 1)
			{
				textPos = undoHistory[curUndo].opPos;
				InsertText(NULL, undoHistory[curUndo].data, true);
			}
			else if (undoHistory[curUndo].type == 0)
			{
				DeleteText(undoHistory[curUndo].opPos,
					undoHistory[curUndo].opPos + strlen(undoHistory[curUndo].data), true);
				textPos = undoHistory[curUndo].opPos;
			}
			else
			{
				//Replace text
				DeleteText(undoHistory[curUndo].opPos,
					undoHistory[curUndo].opPos + strlen(undoHistory[curUndo].oldData), true);
				textPos = undoHistory[curUndo].opPos;
				InsertText(NULL, undoHistory[curUndo].data, true);
			}
			curUndo++;
		}
		break;
	case WM_SETFONT:
		UpdateFont((HFONT)wParam);
		CalcTextAreaDims();
		GenTabStops();
		if (wrapWords == true)
			WrapWords();
		else
			TruncateLines();
		UpdateScrollInfo();
		CalcCaretPos();
		UpdateCaretPos();
		if (lParam == TRUE)
			InvalidateRect(hwnd, NULL, TRUE);
		break;
	//Mouse input messages
	case WM_LBUTTONDOWN:
		sameUndoOp = false;
		TryCursorUnhide();
		lastMousePos.x = GET_X_PARAM(lParam);
		lastMousePos.y = GET_Y_PARAM(lParam);
		TextPosFromPoint();
		if (GetKeyState(VK_SHIFT) & 0x8000)
			UpdateRegion(true);
		else
		{
			regionBegin = textPos;
			if (regionActive == true)
				UpdateRegion(false);
		}
		lBtnDown = true;
		break;
	case WM_LBUTTONUP:
		lBtnDown = false;
		break;
	case WM_RBUTTONDOWN:
		TryCursorUnhide();
		break;
	case WM_RBUTTONUP:
		break;
	case WM_MBUTTONDOWN:
		TryCursorUnhide();
		if (drawDrag == false)
		{
			dragP.x = GET_X_PARAM(lParam);
			dragP.y = GET_Y_PARAM(lParam);
			drawDrag = true;
			SetTimer(hwnd, 1, 10, NULL);
			SetTimer(hwnd, 2, dblClickTime, NULL);
			RECT rt;
			rt.left = dragP.x - 16;
			rt.top = dragP.y - 16;
			rt.right = dragP.x + 16;
			rt.bottom = dragP.y + 16;
			InvalidateRect(hwnd, &rt, TRUE);
		}
		else
			MScrollEnd();
		break;
	case WM_MBUTTONUP:
		if (mBtnTimeUp == true)
			MScrollEnd();
		break;
	case WM_MOUSEMOVE:
		TryCursorUnhide();
		lastMousePos.x = GET_X_PARAM(lParam);
		lastMousePos.y = GET_Y_PARAM(lParam);
		if (lBtnDown == true)
		{
			TextPosFromPoint();
			UpdateRegion(true);
		}
		break;
	case WM_SETCURSOR:
	{
		POINT pt;
		GetCursorPos(&pt);
		pt.x -= scrdiff.x;
		pt.y -= scrdiff.y;
		if (drawDrag == true)
		{
			lastMousePos.x = pt.x;
			lastMousePos.y = pt.y;
			MButtonScroll(true);
			return TRUE;
		}
		if (0 <= pt.x && pt.x < winWidth &&
			0 <= pt.y && pt.y < winHeight)
		{
			SetCursor(hIBeam);
			return TRUE;
		}
	}
	case WM_MOUSEWHEEL:
	//case MSH_MOUSEWHEEL:
		MWheelScroll(HIWORD(wParam));
		break;
	//Keyboard input messages
	case WM_SETFOCUS:
		CreateCaret(hwnd, NULL, fontHeight / 16, fontHeight);
		ShowCaret(hwnd);
		UpdateCaretPos();
		break;
	case WM_KILLFOCUS:
		TryCursorUnhide();
		DestroyCaret();
		MScrollEnd();
		break;
	case WM_KEYDOWN:
		ProcessMiscKey(wParam);
		break;
	case WM_CHAR:
		if (wParam < 0x20 && wParam != 0x08 && wParam != 0x09 && wParam != 0x0D)
			break; //We don't insert control characters...
		if (cursorVis == false && mouseVanish == TRUE)
		{
			cursorVis = true;
			ShowCursor(FALSE);
		}
		if (regionActive == true)
		{
			DeleteText(regionBegin, textPos, false);
			UpdateRegion(false);
		}
		else if (wParam == 0x08) //backspace
		{
			DeleteText(textPos - 1, textPos, false);
			textPos--;
		}
		else if (wParam == 0x7F) //CTRL+backspace
		{
			regionBegin = textPos;
			SkipWord(false);
			DeleteText(textPos, regionBegin, false);
		}
		if (wParam != 0x08 && wParam != 0x7F)
			InsertText((char)wParam, NULL, false);
		break;
	//Custom messages
	default:
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void UpdateFont(HFONT newFont)
{
	LOGFONT fontInfo;
	TEXTMETRIC tm;
	unsigned oldFontHeight;

	if (hFont == NULL)
		oldFontHeight = 0;
	else
		oldFontHeight = fontHeight;

	hFont = newFont;

	HDC hDC = GetDC(lastHwnd);
	SelectObject(hFont);
	GetTextMetrics(hDC, &tm);
	fontHeight = tm.tmHeight;
	avgCharWidth = tm.tmAveCharWidth;
	maxCharWidth = tm.tmMaxCharWidth;

	//Reset visLine and visLnOffset
	unsigned oldYPos = oldFontHeight * visLine + visLnOffset;
	visLine = oldYPos / fontHeight;
	visLnOffset = oldYPos % fontHeight;
	//Reset numVisLines
	CalcNumVisLines();

	GetObject(hFont, sizeof(LOGFONT), &fontInfo);
	if (fontInfo.lfPitchAndFamily & FIXED_PITCH)
		fixedPitch = true;
	else
	{
		float tempcWidths[256];
		ABCFLOAT abcWidths[256];

		fixedPitch = false;
		//Get the character widths (optional)
		GetCharWidthFloat(hDC, 0x00, 0xFF, tempcWidths);
		for (unsigned i = 0; i < 256; i++)
			cWidths[i] = (unsigned)(tempcWidths * 16);

		numkPairs = GetKerningPairs(hDC, -1, NULL);
		if (numkPairs <= 0)
		{
			if (kernPairs != NULL)
			{
				free(kernPairs);
				kernPairs = NULL;
			}
			numkPairs = 0;
		}
		else
		{
			if (kernPairs != NULL)
			{
				free(kernPairs);
				kernPairs = NULL;
			}
			kernPairs = (KERNINGPAIR*)malloc(sizeof(KERNINGPAIR) * numkPairs);
			GetKerningPairs(hDC, numkPairs, kernPairs);
		}

		leftMargin = 0;
		rightMargin = 0;
		GetCharABCWidthsFloat(hDC, 0x00, 0xFF, abcWidths);
		for (unsigned i = 0; i < 256; i++)
		{
			if (abcWidths[i].abcfA < -(float)leftMargin)
				leftMargin = (unsigned)(-abcWidths[i].abcfA);
			if (abcWidths[i].abcfC < -(float)rightMargin)
				rightMargin = (unsigned)(-abcWidths[i].abcfC);
		}
	}
	ReleaseDC();
}

void CalcTextAreaDims()
{
	if (winWidth > leftMargin + rightMargin + bordWidthX * 2)
		textAreaWidth = winWidth - (leftMargin + rightMargin + bordWidthX * 2);
	else
		textAreaWidth = 0;

	if (winHeight > bordWidthY * 2)
		textAreaHeight = winHeight - bordWidthY * 2;
	else
		textAreaHeight = 0;

	//Calculate the number of visible lines when the scroll bar is at the top
	numVisLines = textAreaHeight / fontHeight;
	if (textAreaHeight % fontHeight > 0)
		numVisLines++;

	//Calculate the total number of visible lines
	totVisLines = numVisLines + 1;

	//Check for a valid first visible line
	if (visLine + numVisLines > numLines)
	{
		visLine = numLines - numVisLines;
		visLnOffset = fontHeight - (textAreaHeight % fontHeight);
	}
	else
	{
		//Do the normal visible line calculation
		CalcNumVisLines();
	}

	//Perform line information allocations
	if (numVisChars == NULL)
	{
		numVisChars = (unsigned*)malloc(sizeof(unsigned) * totVisLines);
		firstVisChars = (unsigned*)malloc(sizeof(unsigned) * totVisLines);
		firstVisOffset = (unsigned*)malloc(sizeof(unsigned) * totVisLines);
		memset(numVisChars, 0, sizeof(unsigned) * totVisLines);
	}
	else
	{
		numVisChars = (unsigned*)realloc(numBisChars, sizeof(unsigned) * totVisLines);
		firstVisChars = (unsigned*)realloc(firstVisChars, sizeof(unsigned) * totVisLines);
		firstVisOffset = (unsigned*)realloc(firstVisOffset, sizeof(unsigned) * totVisLines);
		memset(numVisChars, 0, sizeof(unsigned) * totVisLines);
	}
}

void CalcNumVisLines()
{
	numVisLines = (textAreaHeight + visLnOffset) / fontHeight;
	if ((textAreaHeight + visLnOffset) % fontHeight > 0)
		numVisLines++;
}

void GenTabStops()
{
	HDC hDC = GetDC(lastHwnd);
	SelectObject(hDC, hFont);
	unsigned tsWidth = LOWORD(GetTabbedTextExtent(hDC, "\t", 1, 0, NULL));
	ReleaseDC(lastHwnd, hDC);
	if (tabStops != NULL)
	{
		free(tabStops);
		tabStops = NULL;
	}
	numTabStops = 0;
	tabStops = (unsigned*)malloc(sizeof(unsigned) * 20);
	for (unsigned dist = 0; ; )
	{
		dist += (unsigned)tsWidth;
		if (dist >= textAreaWidth)
		{
			if (textAreaWidth != 0)
				tabStops[numTabStops] = textAreaWidth - 1;
			else
				tabStops[numTabStops] = textAreaWidth;
			numTabStops++;
			if (numTabStops % 20 == 0)
				tabStops = (unsigned*)realloc(tabStops, sizeof(unsigned) * (numTabStops + 20));			
			break;
		}
		tabStops[numTabStops] = dist;
		numTabStops++;
		if (numTabStops % 20 == 0)
			tabStops = (unsigned*)realloc(tabStops, sizeof(unsigned) * (numTabStops + 20));
	}
	tabStops = (unsigned*)realloc(tabStops, sizeof(unsigned) * (numTabStops));
}

void WrapWords()
{
	if (textAreaWidth <= maxCharWidth)
		return;
	//Reset the necessary variables
	if (lineStarts != NULL)
	{
		free(lineStarts);
		lineStarts = NULL;
	}
	numLines = 0;
	lineStarts = (unsigned*)malloc(sizeof(unsigned) * 20);

	HDC hDC = GetDC(lastHwnd);
	unsigned wrapPos = 0;
	lineStarts[0] = 0;
	numLines++;
	while (wrapPos < textSize)
	{
		GCP_RESULTS wrapRes;
		unsigned wrapStride = truncLen;
		int charWidths[TRUNC_LEN];
		unsigned newLength = 0;
		unsigned lastSpace = 0;
		ZeroMemory(&wrapRes, sizeof(GCP_RESULTS));
		wrapRes.lStructSize = sizeof(GCP_RESULTS);
		wrapRes.lpDx = charWidths;
		if (wrapPos + wrapStride > textSize)
			wrapStride = textSize - wrapPos;
		GetCharacterPlacement(hDC, &(buffer[wrapPos]), wrapStride, textAreaWidth, &wrapRes, GCP_MAXEXTENT);
		wrapPos += wrapRes.nMaxFit;
		//Process for tabs
		ProcessTabs(numLines - 1, charWidths, wrapPos);
		//Process for newlines and make sure that the line ends with whitespace, if possible
		for (unsigned i = lineStarts[numLines-1], j = 0; newLength < textAreaWidth && i < wrapPos; i++)
		{
			if (buffer[i] == '\t' ||
				buffer[i] == ' ')
				lastSpace = i;
			if (buffer[i] == '\n')
			{
				lastSpace = i;
				break;
			}
			newLength += charWidths[j];
			j++;
		}

		if (lastSpace != 0 && buffer[lastSpace] == ' ' ||
			buffer[lastSpace] == '\t' || buffer[lastSpace] == '\n')
			wrapPos = lastSpace + 1;
		else if (numLines == 1 && buffer[lastSpace] == ' ' ||
			buffer[lastSpace] == '\t' || buffer[lastSpace] == '\n')
			wrapPos = lastSpace + 1;

		lineStarts[numLines] = wrapPos;
		numLines++;
		if (numLines % 20 == 0)
			lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 20));
	}
	lineStarts[numLines] = textSize;
	if (numLines + 1 % 20 == 0)
		lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 21));
	ReleaseDC(lastHwnd, hDC);
}

void ProcessTabs(unsigned lineNum, int* charWidths, unsigned endIndex)
{
	unsigned newLength = 0;

	for (unsigned i = lineStarts[lineNum], j = 0; i < endIndex; i++)
	{
		if (buffer[i] == '\t')
		{
			//Change the proper lpDx
			unsigned curTabStop;
			for (curTabStop = 0; curTabStop < numTabStops && newLength >= tabStops[curTabStop]; curTabStop++);
			charWidths[j] = tabStops[curTabStop] - newLength;
		}
		newLength += charWidths[j];
		j++;
	}
}

void TruncateLines()
{
	//Reset the necessary variables
	if (lineStarts != NULL)
	{
		free(lineStarts);
		lineStarts = NULL;
	}
	numLines = 0;
	lineStarts = (unsigned*)malloc(sizeof(unsigned) * 20);

	lineStarts[0] = 0;
	numLines++;

	//Count through string, looking for newlines and lines in need of truncating
	for (unsigned i = 0; i < textSize; i++)
	{
		if (buffer[i] == '\n' ||
			i - lineStarts[numLines-1] == truncLen)
		{
			//Start a new line
			lineStarts[numLines] = i;
			numLines++;
			if (numLines % 20 == 0)
				lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 20));
		}
	}
	lineStarts[numLines] = textSize;
	if (numLines + 1 % 20 == 0)
		lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 20));

	//Find the length of the longest line
	HDC hDC = GetDC(lastHwnd);
	unsigned wrapPos = 0;
	xScrollMax = 0;

	for (unsigned i = 0; i < numLines; i++)
	{
		GCP_RESULTS wrapRes;
		unsigned wrapStride = truncLen;
		int charWidths[truncLen];
		unsigned newLength = 0;

		ZeroMemory(&wrapRes, sizeof(GCP_RESULTS));
		wrapRes.lStructSize = sizeof(GCP_RESULTS);
		wrapRes.lpDx = charWidths;
		if (wrapPos + wrapStride > textSize)
			wrapStride = textSize - wrapPos;
		GetCharacterPlacement(hDC, &(buffer[wrapPos]), wrapStride, 0, &wrapRes, NULL);

		for (unsigned j = 0; j < wrapStride; j++)
			newLength += wrapRes.lpDx[j];
		if (newLength > xScrollMax)
			xScrollMax = newLength;
	}

	ReleaseDC(lastHwnd, hDC);
}

void UpdateRenderInfo(bool onlyRegion)
{
	char* curLinePtr;
	unsigned lineSize;
	GCP_RESULTS charPlac;
	int* charWidths;
	HDC hDC;
	unsigned p1, p2;

	if (endUpdLine == 0 || onlyRegion == true)
	{
		FreeRenderInfo();
		beginUpdLine = 0;
		endUpdLine = numVisLines;
		if (onlyRegion != true)
			norTextRI = (POLYTEXT*)malloc(sizeof(POLYTEXT) * numVisLines);
	}
	else
	{
		//Reallocation already happened if necessary
		//Line render info structures were scrolled
		//Region render info is always fully recalculated
		FreeRenderInfo(false);
	}

	if (onlyRegion == true)
		goto regRIUpdate;

	hDC = GetDC(lastHwnd);

	//Fill in normal text render information
	for (unsigned i = beginUpdLine; i < endUpdLine; i++)
	{
		curLinePtr = &(buffer[lineStarts[visLine+i]]);
		if (wrapWords == true)
			lineSize = lineStarts[visLine+i+1] - lineStarts[visLine+i];
		else
		{
			lineSize = numVisChars[i];
			curLinePtr += firstVisChars[i];
		}

		charWidths = (int*)malloc(sizeof(int) * lineSize);
		ZeroMemory(&charPlac, sizeof(GCP_RESULTS));
		charPlac.lStructSize = sizeof(GCP_RESULTS);
		charPlac.lpDx = charWidths;
		GetCharacterPlacement(hDC, curLinePtr, lineSize, 0, &charPlac, NULL);
		ProcessTabs(visLine + i, charWidths, lineStarts[visLine+i] + lineSize);

		norTextRI[i].x = bordWidthX + leftMargin;
		if (wrapWords == false)
			norTextRI[i].x -= firstVisOffset[i];
		norTextRI[i].y = (int)bordWidthY - visLnOffset + fontHeight * i;
		norTextRI[i].n = lineSize;
		norTextRI[i].lpstr = curLinePtr;
		norTextRI[i].uiFlags = NULL;
		norTextRI[i].rcl = NULL;
		norTextRI[i].pdx = charWidths;
	}
	ReleaseDC(lastHwnd, hDC);

	if (regionActive == false)
		return;

regRIUpdate:

	//Fill in region render information

	//Sort the beginning and the end
	p1 = regionBegin;
	p2 = textPos;
	SortAscending(&p1, &p2);

	//Find the number of visible region lines
	unsigned beginLine;
	unsigned endLine;
	for (unsigned i = 0; i < numLines; i++)
	{
		if (p1 >= lineStarts[i])
			beginLine = i;
		if (p2 >= lineStarts[i])
			endLine = i;
	}
	if (beginLine < visLine)
	{
		//There is no region to render
		numRegVisLines = 0;
		return;
	}
	else
		numRegVisLines = endLine - beginLine + 1;
	beginLine -= visLine;
	endLine -= visLine;

	for (unsigned i = beginLine; i < endLine + 1; i++)
	{
		unsigned curLineStart, curLineEnd;
		unsigned numLeadChars;
		unsigned leadCharWidth;
		unsigned curLineWidth;

		curLineStart = lineStarts[visLine+i];
		curLineEnd = lineStarts[visLine+i+1];
		if (i == beginLine)
			curLineStart = p1;
		if (i == endLine)
			curLineEnd = p2;
		curLinePtr = &(buffer[curLineStart]);

		if (wrapWords == true)
			lineSize = curLineEnd - curLineStart;
		else
		{
			lineSize = numVisChars[i];
			if (firstVisChars[i] > curLineStart - lineStarts[visLine+i])
				curLinePtr = lineStarts[visLine+i] + firstVisChars[i];
			else
				lineSize -= ((curLineStart - lineStarts[visLine+i]) - firstVisChars[i]);
		}

		numLeadChars = curLineStart - lineStarts[visLine+i];
		leadCharWidth = 0;
		for (unsigned j = 0; j < numLeadChars; j++)
			leadCharWidth += norTextRI[i].pdx[j];

		curLineWidth = 0;
		for (unsigned j = numLeadChars; j < numLeadChars + lineSize; j++)
			curLineWidth += norTextRI[i].pdx[j];

		regTextRI[i].x = bordWidthX + leftMargin + leadCharWidth;
		if (wrapWords == false)
			regTextRI[i].x -= firstVisOffset[i];
		regTextRI[i].y = (int)bordWidthY - visLnOffset + fontHeight * i;
		regTextRI[i].n = lineSize;
		regTextRI[i].lpstr = curLinePtr;
		regTextRI[i].uiFlags = ETO_OPAQUE;
			regTextRI[i].rcl.left = regTextRI[i].x;
			regTextRI[i].rcl.top = regTextRI[i].y;
			regTextRI[i].rcl.right = regTextRI[i].x + curLineWidth;
			regTextRI[i].rcl.bottom = regTextRI[i].y + fontHeight;
		regTextRI[i].pdx = norTextRI[i].pdx + numLeadChars;
	}
}

void UpdateScrollInfo()
{
	SCROLLINFO si;

	//Set common parameters
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
	si.nMin = 0;

	//Set the vertical scroll bar
	si.nMax = numLines * fontHeight;
	if (textAreaHeight == 0)
		si.nPage = 1;
	else
		si.nPage = textAreaHeight;
	si.nPos = visLine * fontHeight + visLnOffset;
	SetScrollInfo(lastHwnd, SB_VERT, &si, TRUE);

	if (wrapWords == false)
	{
		//Set the horizontal scroll bar
		si.nMax = xMaxScroll;
		si.nPage = textAreaWidth;
		si.nPos = xScrollPos;
		SetScrollInfo(lastHwnd, SB_HORZ, &si, TRUE);
	}
	else
	{
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMax = 0;
		si.nPage = 0;
		si.nPos = 0;
		SetScrollInfo(lastHwnd, SB_HORZ, &si, TRUE);
	}
}

void ScrollContents(bool offset, unsigned otherType, int amount, int sBar)
{
	//These variables are used for scrolling both horizontally and vertically at once
	static int lastScrollAmnt;
	static bool secondRound = false;

	SCROLLINFO si;
	//Get all the scroll bar information
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	GetScrollInfo(lastHwnd, sBar, &si);
	//Save the position for comparison later on
	int sbPos;
	sbPos = si.nPos;

	if (offset == false)
	{
		switch (otherType)
		{
		//user pressed the HOME keyboard key
		case SB_TOP:
			si.nPos = si.nMin;
			break;
		//user pressed the END keyboard key
		case SB_BOTTOM:
			si.nPos = si.nMax;
			break;
		//user pressed the top arrow
		case SB_LINEUP:
			si.nPos -= fontHeight;
			break;
		//user pressed the bottom arrow
		case SB_LINEDOWN:
			si.nPos += fontHeight;
			break;
		//user clicked the scroll bar shaft above the scroll box
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;
		//user clicked the scroll bar shaft below the scroll box
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			break;
		//user dragged the scroll box
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		}
	}
	else
		si.nPos += amount;

	/*Set the position and then retrieve it. Due to adjustments
	  by Windows it may not be the same as the value set.*/
	si.fMask = SIF_POS;
	SetScrollInfo(lastHwnd, sBar, &si, TRUE);
	GetScrollInfo(lastHwnd, sBar, &si);
	//If the position has changed, scroll window and update it
	if (si.nPos != sbPos)
	{
		//Scroll the render info structures (if necessary)
		if (sBar == SB_VERT || secondRound == true)
		{
			if (ScrollRenderStructs(true, si.nPos - sbPos))
				UpdateRenderInfo();
		}
		if (sBar == SB_HORZ || sBar == SB_BOTH && secondRound == false)
		{
			if (ScrollRenderStructs(false, si.nPos - sbPos))
				UpdateRenderInfo();
		}

		//Scroll the window bits
		RECT rt;
		rt.left = bordWidthX;
		rt.top = bordWidthY;
		rt.right = winWidth - bordWidthX;
		rt.bottom = winHeight - bordWidthY;
		if (sBar == SB_VERT)
			ScrollWindowEx(lastHwnd, 0, sbPos - si.nPos, &rt, &rt, NULL, NULL, SW_INVALIDATE | SW_ERASE);
		if (sBar == SB_HORZ)
			ScrollWindowEx(lastHwnd, sbPos - si.nPos, 0, &rt, &rt, NULL, NULL, SW_INVALIDATE | SW_ERASE);
		if (sBar == SB_BOTH)
		{
			if (secondRound == false)
			{
				secondRound = true;
				lastScrollAmnt = sbPos - si.nPos;
			}
			else
			{
				secondRound = false;
				ScrollWindowEx(lastHwnd, lastScrollAmnt, sbPos - si.nPos, &rt, &rt, NULL, NULL, SW_INVALIDATE | SW_ERASE);
			}
		}
	}
}

bool ScrollRenderStructs(bool vert, int offset)
{
	if (vert == true)
	{
		int offSize = visLnOffset + offset;
		int offLines;
		if (offSize > 0)
		{
			offLines = offSize / fontHeight
			visLine += offLines;
			visLnOffset = offSize % fontHeight;
		}
		else if (offSize < 0)
		{
			offLines = offSize / fontHeight
			visLnOffset = fontHeight + offSize % fontHeight;
			if (visLnOffset != fontHeight)
				offLines--;
			else
				visLnOffset = 0;
			visLine += offLines;
		}
		else
			return false;

		//Calculate the number of visible lines
		oldNumVisLines = numVisLines;
		CalcNumVisLines();

		//Scroll render info structures and mark update regions
		if (offLines >= 0)
		{
			for (unsigned i = oldNumVisLines - offLines; i < oldNumVisLines; i++)
				free(norTextRI[i].pdx);
			if (numVisLines > oldNumVisLines)
				norTextRI = (POLYTEXT*)realloc(norTextRI, sizeof(POLYTEXT) * numVisLines);
			memmove(norTextRI, &norTextRI[offLines], sizeof(POLYTEXT) * (oldNumVisLines - offLines));
			beginUpdLine = oldNumVisLines - offLines;
			endUpdLine = numVisLines;
		}
		else
		{
			for (unsigned i = 0; i < offLines; i++)
				free(norTextRI[i].pdx);
			if (numVisLines > oldNumVisLines)
			{
				norTextRI = (POLYTEXT*)realloc(norTextRI, sizeof(POLYTEXT) * numVisLines);
				memmove(&norTextRI[-offLines+1], norTextRI, sizeof(POLYTEXT) * (oldNumVisLines + offLines));
				endUpdLine = -offLines + 1;
			}
			else
			{
				memmove(&norTextRI[-offLines], norTextRI, sizeof(POLYTEXT) * (oldNumVisLines + offLines));
				endUpdLine = -offLines;
			}
			beginUpdLine = 0;
		}

		return true;
	}
	else
	{
		//We only scroll vertical render information
		return true;
		/*bool retVal = false;
		xScrollPos += offset;

		for (unsigned i = 0; i < numVisLines; i++)
		{
			//Count up character widths until xScrollPos is exceeded
			//Go back one
			//Find the difference between xScrollPos and the current length
		}
		return retVal;*/
	}
	return false;
}

void MButtonScroll(bool setCursor)
{
	if (setCursor == true)
	{
		bool down = false, up = false, right = false, left = false;
		if (lastMousePos.y > dragP.y + 16)
			down = true;
		else if (lastMousePos.y < dragP.y - 16)
			up = true;
		if (lastMousePos.x > dragP.x + 16)
			right = true;
		else if (lastMousePos.x < dragP.x - 16)
			left = true;
		if (wrapWords == true && (down || up) == false)
		{
			right = false;
			left = false;
		}
		unsigned pick = (int)down + ((int)up << 1) + ((int)right << 2) + ((int)left << 3);
		SetCursor(panCurs[panCursMap[pick]]);
	}
	else
	{
		static int modX = 0, modY = 0; //Holds accumulated division remainders

		int dragDistX = lastMousePos.x - drayP.x;
		int dragDistY = lastMousePos.y - dragP.y;
		if ((dragDistX < -16 || 16 < dragDistX) &&
			(dragDistY < -16 || 16 < dragDistY))
		{
			modX += dragDistX % 10;
			modY += dragDistY % 10;
			dragDistX /= 10;
			dragDistY /= 10;
			if (modY >= 10)
			{
				modY -= 10;
				dragDistY++;
			}
			if (modY <= -10)
			{
				modY += 10;
				dragDistY--;
			}
			if (wrapWords == true)
				ScrollContents(true, NULL, dragDistY, SB_VERT);
			else
			{
				if (modX >= 10)
				{
					modX -= 10;
					dragDistX++;
				}
				if (modX <= -10)
				{
					modX += 10;
					dragDistX--;
				}
				ScrollContents(true, NULL, dragDistX, SB_BOTH);
				ScrollContents(true, NULL, dragDistY, SB_BOTH);
			}
		}
	}
}

void MScrollEnd()
{
	if (drawDrag == false)
		return;

	mBtnTimeUp = false;
	drawDrag = false;
	dragDist = 0;
	KillTimer(hwnd, 1);
	RECT rt;
	rt.left = dragP.x - 16;
	rt.top = dragP.y - 16;
	rt.right = dragP.x + 16;
	rt.bottom = dragP.y + 16;
	InvalidateRect(hwnd, &rt, TRUE);
}

void MWheelScroll(int amount)
{
	static int pixelMod = 0; //Accumulated partial pixel scrolls
	int wheelScroll = -amount;
	int offset;

	if (wheelLines == WHEEL_PAGESCROLL)
		offset = textAreaHeight;
	else
		offset = fontHeight * wheelLines;
	offset *= wheelScroll;
	offset += pixelMod;
	//Store the fractional scroll pixels not accounted for
	pixelMod = offset % WHEEL_DELTA;
	//Finalize offset calculation
	offset /= WHEEL_DELTA;

	//According to the Platform SDK documentation, if the number of mouse wheel
	//scroll lines is greater than the window area, it should be interpreted as
	//a page up or page down.
	if (offset > textAreaHeight)
		offset = textAreaHeight;
	if (offset < -textAreaHeight)
		offset = -textAreaHeight;

	ScrollContents(true, NULL, offset, SB_VERT);
}

/*NOTE: For InsertText() and DeleteText(), the parameter "silent" suppresses
		updating the undo queue if set to true. This parameter should probably
		only be used for the undo and redo functions.*/
/*Incomplete*/
void InsertText(char c, const char* str, bool silent)
{
	if (c != NULL)
	{
		//Add the character
		if (overwrite == false || buffer[textPos] == '\n' || buffer[textPos] == '\0')
		{
			textSize++;
			if (textSize == buffSize)
			{
				buffSize += 1000;
				buffer = (char*)realloc(buffer, buffSize);
			}
			if (textPos != (textSize - 1))
				memmove(&(buffer[textPos+1]), &(buffer[textPos]), textSize - textPos);
			else
				buffer[textPos+1] = '\0';

			//Correct all line beginning indices
			for (unsigned i = caretLine + 1; i < numLines; i++)
				lineStarts[i]++;
		}
		buffer[textPos] = c;
		textPos++;

		//Check for undo addition
		if (silent == false && sameUndoOp == true)
		{
			char* curUndoData;
			unsigned dataLen;
			curUndoData = undoHistory[curUndo].data;
			dataLen = strlen(curUndoData) + 2;
			curUndoData = (char*)realloc(curUndoData, dataLen);
			curUndoData[dataLen-2] = c;
			curUndoData[dataLen-1] = '\0';
		}
		else if (silent == false)
		{
			AddUndo();
			undoHistory[curUndo].type = 1;
			undoHistory[curUndo].data = (char*)malloc(2);
			undoHistory[curUndo].data[0] = c;
			undoHistory[curUndo].data[1] = '\0';
			sameUndoOp = true;
		}
	}
	else
	{
		//Allocate proper space
		unsigned strLen = strlen(str);
		textSize += strLen;
		if (textSize == buffSize)
		{
			buffSize += 1000;
			buffer = (char*)realloc(buffer, buffSize);
		}
		if (textPos != (textSize - 1))
			memmove(&(buffer[textPos+strLen]), &(buffer[textPos]), textSize - textPos);
		else
			buffer[textPos+strLen] = '\0';

		//Paste the string in
		strncpy(&(buffer[textPos]), str, strLen);

		//Correct all line beginning indices
		for (unsigned i = caretLine + 1; i < numLines; i++)
			lineStarts[i] += strLen;

		//Add an undo entry
		if (silent == false)
		{
			AddUndo();
			undoHistory[curUndo].type = 1;
			undoHistory[curUndo].data = (char*)malloc(strLen + 1);
			strcpy(undoHistory[curUndo].data, str);
			sameUndoOp = false;
		}
	}

	if (wrapWords == true)
		RewrapWords();
	else
		RetruncateLines();

	beginUpdLine = caretLine;
	CalcCaretPos();
	UpdateCaretPos();

	//Update the window
	if (beginUpdLine < visLine)
		beginUpdLine = visLine;
	beginUpdLine -= visLine;
	endUpdLine = lastWrappedLine - visLine;
	if (endUpdLine <= beginUpdLine)
		endUpdLine = beginUpdLine + 1;
	if (endUpdLine > numVisLines)
		endUpdLine = numVisLines;
	RECT rt;
	rt.left = 0;
	rt.right = winWidth;
	rt.top = beginUpdLine * fontHeight + visLnOffset;
	rt.bottom = endUpdLine * fontHeight + visLnOffset;
	UpdateRenderInfo();
	InvalidateRect(lastHwnd, &rt, TRUE);

	//Configure miscellaneous flags
	wasInsert = true;
}

/*See InsertText() for information regarding the parameter "silent".*/
/*Incomplete*/
void DeleteText(unsigned p1, unsigned p2, bool silent)
{
	SortAscending(&p1, &p2);

	//Check for undo addition
	if (silent == false)
	{
		if (p2 - p1 == 1)
		{
			if (sameUndoOp == true)
			{
				char* curUndoData;
				unsigned dataLen;
				curUndoData = undoHistory[curUndo].data;
				dataLen = strlen(curUndoData) + 2;
				curUndoData = (char*)realloc(curUndoData, dataLen);
				curUndoData[dataLen-2] = buffer[p1];
				curUndoData[dataLen-1] = '\0';
			}
			else
			{
				AddUndo();
				undoHistory[curUndo].type = 0;
				undoHistory[curUndo].data = (char*)malloc(2);
				undoHistory[curUndo].data[0] = buffer[p1];
				undoHistory[curUndo].data[1] = '\0';
				sameUndoOp = true;
			}
		}
		else
		{
			unsigned strLen = p2 - p1;
			AddUndo();
			undoHistory[curUndo].type = 0;
			undoHistory[curUndo].data = (char*)malloc(strLen + 1);
			strncpy(undoHistory[curUndo].data, &(buffer[p1]), strLen);
			undoHistory[curUndo].data[strLen] = '\0';
			sameUndoOp = false;
		}
	}

	//Erase the region from p1 to p2
	memmove(&(buffer[p1]), &(buffer[p2]), textSize - p2);
	textSize -= (p2 - p1);
	if (textPos >= p2)
		textPos -= (p2 - p1);
	else
		textPos = p1;

	if (wrapWords == true)
	{
		//Collapse the next line with this line so this line is too long
		if (caretLine + 1 != numLines - 1)
			lineStarts[caretLine+1] = lineStarts[caretLine+2];
	}

	if (wrapWords == true)
		RewrapWords();
	else
		RetruncateLines();

	beginUpdLine = caretLine;
	CalcCaretPos();
	UpdateCaretPos();

	//Update the window
	if (beginUpdLine < visLine)
		beginUpdLine = visLine;
	beginUpdLine -= visLine;
	endUpdLine = lastWrappedLine - visLine;
	if (endUpdLine <= beginUpdLine)
		endUpdLine = beginUpdLine + 1;
	if (endUpdLine > numVisLines)
		endUpdLine = numVisLines;
	RECT rt;
	rt.left = 0;
	rt.right = winWidth;
	rt.top = beginUpdLine * fontHeight + visLnOffset;
	rt.bottom = endUpdLine * fontHeight + visLnOffset;
	UpdateRenderInfo();
	InvalidateRect(lastHwnd, &rt, TRUE);

	//Configure miscellaneous flags
	wasInsert = false;
}

/*Incomplete*/
void ProcessMiscKey(WPARAM wParam)
{
	//Any of these keys pressed will interrupt undo accumulation into a single command
	sameUndoOp = false;

	unsigned oldTextPos = textPos;
	if (wParam == VK_HOME || wParam == VK_END ||
		wParam == VK_PRIOR || wParam == VK_NEXT ||
		wParam == VK_UP || wParam == VK_DOWN ||
		wParam == VK_LEFT || wParam == VK_RIGHT && !(GetKeyState(VK_SHIFT) & 0x8000))
		UpdateRegion(false);

	if (GetKeyState(VK_CONTROL) & 0x8000 && wParam != VK_INSERT && wParam != VK_DELETE)
	{
		switch (wParam)
		{
		case VK_HOME: //Beginning of file
			textPos = 0;
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_END: //End of file
			textPos = textSize;
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_PRIOR: //Move caret to top of screen
			caretLine = visLine;
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_NEXT: //Move caret to bottom of screen
			caretLine = visLine + textAreaHeight  / fontHeight;
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_UP: //Scroll view one line up and move caret up
			ScrollContents(true, SB_LINEUP, NULL, SB_VERT);
			if (caretLine <= visLine + numVisLines - 1)
			{
				unsigned oldExtent = 0;
				for (unsigned i = 0; i < textPos - lineStarts[caretLine]; i++)
					oldExtent += norTextRI[caretLine-visLine].pdx[i];
				caretLine--;
				caretX = 0;
				unsigned i;
				for (i = 0; caretX < oldExtent; i++)
					caretX += norTextRI[caretLine-visLine].pdx[i];
				caretX -= norTextRI[caretLine-visLine].pdx[i];
				i--;
				textPos = lineStarts[caretLine] + i;
			}
			UpdateCaretPos();
			//TextPosFromPoint();
			break;
		case VK_DOWN: //Scroll view one line down and move caret down
			ScrollContents(true, SB_LINEDOWN, NULL, SB_VERT);
			if (caretLine <= visLine)
			{
				unsigned oldExtent = 0;
				for (unsigned i = 0; i < textPos - lineStarts[caretLine]; i++)
					oldExtent += norTextRI[caretLine-visLine].pdx[i];
				caretLine++;
				caretX = 0;
				unsigned i;
				for (i = 0; caretX < oldExtent; i++)
					caretX += norTextRI[caretLine-visLine].pdx[i];
				caretX -= norTextRI[caretLine-visLine].pdx[i];
				i--;
				textPos = lineStarts[caretLine] + i;
			}
			UpdateCaretPos();
			//TextPosFromPoint();
			break;
		case VK_LEFT: //Move one word left
			SkipWord(false);
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_RIGHT: //Move one word right
			SkipWord(true);
			CalcCaretPos();
			UpdateCaretPos();
			break;
		}
	}
	else
	{
		switch (wParam)
		{
		case VK_INSERT: //Insert/overwrite mode
			overwrite = !overwrite;
			break;
		case VK_DELETE: //Delete character
			if (regionActive == false)
			{
				if (textPos == textSize)
					break;
				regionBegin = textPos;
				SkipWord(true);
			}
			else
				UpdateRegion(false);
			DeleteText(regionBegin, textPos, false);
			break;
		case VK_HOME: //Beginning of line
			textPos = lineStarts[caretLine];
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_END: //End of line
			if (caretLine == numLines - 1)
				textPos = textSize;
			else
				textPos = lineStarts[caretLine+1] - 1;
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_PRIOR: //Page up
			caretLine -= textAreaHeight / fontHeight;
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_NEXT: //Page down
			caretLine += textAreaHeight / fontHeight;
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_UP: //Move caret up
		{
			unsigned oldExtent = 0;
			for (unsigned i = 0; i < textPos - lineStarts[caretLine]; i++)
				oldExtent += norTextRI[caretLine-visLine].pdx[i];
			caretLine--;
			if (caretLine <= visLine)
				ScrollContents(true, NULL, -visLnOffset, SB_VERT);
			caretX = 0;
			unsigned i;
			for (i = 0; caretX < oldExtent; i++)
				caretX += norTextRI[caretLine-visLine].pdx[i];
			caretX -= norTextRI[caretLine-visLine].pdx[i];
			i--;
			textPos = lineStarts[caretLine] + i;
			UpdateCaretPos();
			//TextPosFromPoint();
			break;
		}
		case VK_DOWN: //Move caret down
		{
			unsigned oldExtent = 0;
			for (unsigned i = 0; i < textPos - lineStarts[caretLine]; i++)
				oldExtent += norTextRI[caretLine-visLine].pdx[i];
			caretLine++;
			if (caretLine <= visLine + numVisLines - 1)
				ScrollContents(true, NULL, -visLnOffset, SB_VERT);
			caretX = 0;
			unsigned i;
			for (i = 0; caretX < oldExtent; i++)
				caretX += norTextRI[caretLine-visLine].pdx[i];
			caretX -= norTextRI[caretLine-visLine].pdx[i];
			i--;
			textPos = lineStarts[caretLine] + i;
			UpdateCaretPos();
			//TextPosFromPoint();
			break;
		}
		case VK_LEFT: //Move caret left
			if (textPos == 0)
				break;
			textPos--;
			CalcCaretPos();
			UpdateCaretPos();
			break;
		case VK_RIGHT: //Move caret right
			if (textPos == textSize)
				break;
			textPos++;
			CalcCaretPos();
			UpdateCaretPos();
			break;
		}
	}
}

void AddUndo()
{
	if (numUndo == 0)
	{
		undoHistory = (UndoEntry*)malloc(sizeof(UndoEntry) * 20);
		numUndo++;
		return;
	}
	if (curUndo != numUndo - 1)
	{
		//Delete all the subsequent undos (one is left over)
		for (unsigned lastUndo = numUndo - 1; lastUndo > curUndo; lastUndo--)
		{
			free(undoHistory[lastUndo].data);
		}
		//We want the allocation to be divisible by 20
		numUndo = curUndo + 2;
		undoHistory = (UndoEntry*)realloc(undoHistory, sizeof(UndoEntry) *
										  (numUndo + (20 - numUndo % 20)));
	}
	else if (++numUndo % 20 == 0)
		undoHistory = (UndoEntry*)realloc(undoHistory, sizeof(UndoEntry) * (numUndo + 20));
	curUndo++;
}

/*Incomplete*/
void RewrapWords(bool setUpdLines)
{
	/*Check to see if the caret is at a word near the beginning
	  of a line.*/
	unsigned curLine = caretLine;
	unsigned wrapPos = lineStarts[curLine];
	unsigned oldTextPos = textPos;
	SkipWord(false);
	if (textPos <= lineStarts[caretLine] && textPos != 0 && buffer[oldTextPos-1] != ' ')
	{
		//Start wrapping words one before the caret line
		curLine--;
		wrapPos = lineStarts[curLine];
	}
	//Keep wrapping words like normal until it is unnecessary
	HDC hDC = GetDC(lastHwnd);
	while (wrapPos < textSize)
	{
		GCP_RESULTS wrapRes;
		unsigned wrapStride = truncLen;
		int charWidths[TRUNC_LEN];
		unsigned newLength = 0;
		unsigned lastSpace = 0;
		ZeroMemory(&wrapRes, sizeof(GCP_RESULTS));
		wrapRes.lStructSize = sizeof(GCP_RESULTS);
		wrapRes.lpDx = charWidths;
		if (wrapPos + wrapStride > textSize)
			wrapStride = textSize - wrapPos;
		GetCharacterPlacement(hDC, &(buffer[wrapPos]), wrapStride, textAreaWidth, &wrapRes, GCP_MAXEXTENT);
		wrapPos += wrapRes.nMaxFit;
		//Process for tabs
		ProcessTabs(curLine, charWidths, wrapPos);
		//Process for newlines and make sure that the line ends with whitespace, if possible
		for (unsigned i = lineStarts[curLine], j = 0; newLength < textAreaWidth && i < wrapPos; i++)
		{
			if (buffer[i] == '\t' ||
				buffer[i] == ' ')
				lastSpace = i;
			if (buffer[i] == '\n')
			{
				//Insert another line start reference
				InsertLineStart(curLine);
				lineStarts[curLine+1] = i + 1;
				lastSpace = i;
				break;
			}
			newLength += charWidths[j];
			j++;
		}

		if (lastSpace != 0 && buffer[lastSpace] == ' ' ||
			buffer[lastSpace] == '\t' || buffer[lastSpace] == '\n')
			wrapPos = lastSpace + 1;
		else if (curLine == 0 && buffer[lastSpace] == ' ' ||
			buffer[lastSpace] == '\t' || buffer[lastSpace] == '\n')
			wrapPos = lastSpace + 1;

		if (wrapPos >= textSize)
			break;
		if (curLine + 1 == numLines)
		{
			//Add another line
			numLines++;
			if (numLines + 1 % 20 == 0)
				lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 21));
			lineStarts[numLines] = textSize;
		}
		if (lineStarts[curLine+1] == wrapPos)
			break;
		lineStarts[curLine+1] = wrapPos;
		curLine++;
	}
	ReleaseDC(lastHwnd, hDC);
	if (setUpdLines == true)
		lastWrappedLine = curLine + 1;
}

/*Incomplete*/
void RetruncateLines(bool setUpdLines)
{
	//This function is simple, mostly because it usually doesn't happen
	unsigned testPos = lineStarts[caretLine];
	while (testPos < lineStarts[caretLine] + truncLen)
	{
		if (buffer[i] == '\n')
		{
			unsigned curLine = caretLine;
			while (lineStarts[caretLine+1] != i + 1)
			{
				InsertLineStart(curLine);
				lineStarts[curLine+1] = i + 1;
				while (i < insertEnd && i != '\n')
					i++;
			}
			break;
		}
		testPos++;
	}
	if (testPos == lineStarts[caretLine] + truncLen)
	{
		InsertLineStart(curLine);
		lineStarts[curLine+1] = i + 1;
	}
	if (setUpdLines == true)
		lastWrappedLine = caretLine + 1;
}

void InsertLineStart(unsigned prevLine)
{
	numLines++;
	if (numLines + 1 % 20 == 0)
		lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 21));
	memmove(&lineStarts[prevLine+2], &lineStarts[prevLine+1], sizeof(unsigned) * (numLines - (prevLine + 1)));
	lineStarts[numLines] = textSize;
}

void CopyRegion()
{
	if (OpenClipboard(hwnd))
	{
		unsigned p1, p2;
		if (regionBegin < textPos)
		{
			p1 = regionBegin;
			p2 = textPos;
		}
		else
		{
			p1 = textPos;
			p2 = regionBegin;
		}
		//We will need to convert any newlines to carriage-return linefeed pairs
		unsigned numNewlines = 0;
		for (unsigned i = p1; i < p2; i++)
		{
			if (buffer[i] == '\n')
				numNewlines++;
		}
		HANDLE clipData = GlobalAlloc(GMEM_MOVEABLE, p2 - p1 + 1 + numNewlines);
		char* clipLock = (char*)GlobalLock(clipData);
		memcpy(clipLock, buffer + p1, p2 - p1);
		for (unsigned i = p1, j = 0; i < p2; i++)
		{
			if (buffer[i] == '\n')
			{
				clipLock[j] = '\r';
				j++;
				clipLock[j] = '\n';
			}
			else
				clipLock[j] = buffer[i];
			j++;
		}
		clipLock[p2-p1+numNewlines] = '\0';
		GlobalUnlock(clipData);
		EmptyClipboard();
		SetClipboardData(CF_TEXT, clipData);
		CloseClipboard();
	}
	else
		MessageBeep(MB_OK);
}

void SkipWord(bool forward)
{
	if (forward == true)
	{
		while (buffer[textPos] != ' ' && textPos != textSize)
			textPos++;
	}
	else
	{
		while (buffer[textPos] != ' ' && textPos != 0)
			textPos--;
		if (buffer[textPos] == ' ')
			textPos++;
	}
}

/*Incomplete*/
void CalcCaretPos()
{
	//Find which line the caret is on
	unsigned newCaretLine;
	for (newCaretLine = caretLine; newCaretLine < numLines; newCaretLine++)
	{
		if (textPos < lineStarts[newCaretLine])
			break;
	}
	newCaretLine--;

	//Scroll the window vertically if necessary
	if (caretLine >= visLine + numVisLines)
		ScrollContents(true, NULL, (visLine + numVisLines - 2 - caretLine) * fontHeight, SB_VERT);
	if (caretLine < visLine)
		ScrollContents(false, NULL, -((int)(visLine - caretLine)) * fontHeight, SB_VERT);

	//Find the x position of the caret
	caretX = 0;
	if (wrapWords == true)
	{
		for (unsigned i = lineStarts[caretLine]; i < textPos; i++)
			caretX += norTextRI[caretLine-visLine].pdx[i-lineStarts[caretLine]];
	}
	else
	{
		while (firstVisChars[caretLine-visLine] > textPos)
			ScrollContents(true, NULL, -avgCharWidth, SB_HORZ);
		while (firstVisChars[caretLine-visLine] +
			   numVisChars[caretLine-visLine] >= textPos)
			ScrollContents(true, NULL, avgCharWidth, SB_HORZ);
		for (unsigned i = firstVisChars[caretLine-visLine]; i < textPos; i++)
			caretX += norTextRI[caretLine-visLine].pdx[i-firstVisChars[caretLine-visLine]];
		caretX -= firstVisOffset[caretLine];
	}
	return;

	//The previous function follows as reference information

	//UpdateCaretX()
	HDC hDC = GetDC(lastHwnd);
	SelectObject(hFont);
	//BAD
	caretX = LOWORD(GetTabbedTextExtent(hDC, &(buffer[lineStarts[caretLine]+firstVisChars[caretLine]]),
		textPos - (lineStarts[caretLine] + firstVisChars[caretLine]), numTabStops, (int*)tabStops));
	ReleaseDC(lastHwnd, hDC);
	caretX -= firstVisOffset[caretLine];

	//UpdateCaretLine()
	//Find which line the caret is on
	unsigned newCaretLine;
	for (newCaretLine = caretLine; newCaretLine < numLines; newCaretLine++)
	{
		if (textPos < lineStarts[newCaretLine])
			break;
	}
	newCaretLine--;
	//Keep the caret on the current line until there is no more room
	if (newCaretLine != caretLine)
	{
		//Recalculate extent
		unsigned lineEndRef;
		if (caretLine == numLines - 1)
			lineEndRef = textSize - 1;
		else
			lineEndRef = lineStarts[caretLine+1] - 1;
		if (spaceBetween == true && textPos > 0 && buffer[textPos-1] != ' ')
		{
			spaceBetween = false;
			caretLine++;
			lineStarts[caretLine]--;
			caretX = cWidths[(unsigned char)buffer[textPos-1]];
			lineLens[caretLine-1] -= caretX;
			lineLens[caretLine] += caretX;
			caretY += fontHeight;
		}
		if (textPos != textSize && buffer[textPos] != ' ' && (buffer[lineEndRef] == ' ' || lineEndRef == textSize - 1))
			atMidBrkPos = true; //Save information for going back
		if (caretX >= textAreaWidth || ((buffer[lineEndRef] == ' ' || lineEndRef == textSize - 1) && spaceBetween == false))
		{
			//If a space is typed to break a word and cause rewrapping, keep the caret on the current line
			if (textPos > 0 && textPos != textSize && buffer[textPos-1] == ' ' && buffer[textPos] != ' ' && spaceBetween == false)
				caretLine++;
			else
			{
				if (caretX >= textAreaWidth)
					atMidBrkPos = false;
				spaceBetween = false;
				caretX = 0;
				caretY += fontHeight;
				caretLine++;
				for (unsigned i = lineStarts[newCaretLine]; i < textPos; i++)
					caretX += cWidths[(unsigned char)buffer[i]];
			}
		}
	}

	//Scroll the window if necessary
}

void UpdateCaretPos()
{
	caretY = (caretLine + caretLnOffst - visLine) * fontHeight - visLnOffset;
	SetCaretPos(caretX + leftMargin + bordWidthX, caretY + bordWidthY);
}

/*Incomplete*/
void TextPosFromPoint()
{
	unsigned oldX = caretX, oldY = caretY;
	int xPos = lastMousePos.x;
	int yPos = lastMousePos.y;
	xPos -= (leftMargin + bordWidthX);
	yPos -= bordWidthY;
	int lineTest = (yPos + visLnOffset) / fontHeight;

	//Scroll vertically, if necessary
	if (lineTest < 0)
		ScrollContents(true, NULL, lineTest * fontHeight, SB_VERT);
	else if (lineTest >= numVisLines)
		ScrollContents(true, NULL, (numVisLines - lineTest + 1) * fontHeight, SB_VERT);

	//Make sure the caret is on a valid line
	if (lineTest < 0 && (unsigned)(-lineTest) > visLine)
		caretLine = 0;
	else
		caretLine = (unsigned)(visLine + lineTest);
	if (caretLine > numLines - 1)
		caretLine = numLines - 1;

	unsigned lineEndRef;
	if (caretLine == numLines - 1)
		lineEndRef = textSize;
	else
		lineEndRef = lineStarts[caretLine+1];

	//Scroll horizontally, if necessary
	/*if (numVisChars[caretLine-visLine] == 0)
	{
		//Find the end of this line
		//Scroll so that it is in view
		ScrollContents(false, NULL, 0, SB_HORZ);
	}
	if (xPos < 0 || xPos >= textAreaWidth)
	{
		//Check that there is a valid character in that direction
		//If there is, scroll so that it is in view
	}*/

	//Find all the characters behind the caret
	for (textPos = lineStarts[caretLine]; textPos < lineEndRef && caretX < xPos; textPos++)
	{
		if (buffer[textPos] == '\n')
			break;
		caretX += norTextRI[caretLine].pdx[textPos-lineStarts[caretLine]];
	}

	//Perform proper rounding
	if (textPos < lineEndRef && caretX != 0 && buffer[textPos] != '\n')
	{
		textPos--;
		caretX -= norTextRI[caretLine].pdx[textPos-lineStarts[caretLine]];
		int midChar = xPos - caretX;
		if (midChar >= norTextRI[caretLine].pdx[textPos-lineStarts[caretLine]] / 2)
		{
			caretX += norTextRI[caretLine].pdx[textPos-lineStarts[caretLine]];
			textPos++;
		}
	}
}

void BadFunc()
{
	//The rest of this function is actually not of our concern

	if (caretX == oldX && caretY == oldY)
		return;

	UpdateCaretPos();

	if (regionBegin != textPos)
		regionActive = true;
	else
		regionActive = false;

	//Invalidate the proper areas
	RECT rt;
	rt.left = (oldX < caretX) ? (int)oldX : (int)caretX;
	rt.top = (oldY < caretY) ? (int)oldY : (int)caretY;
	rt.right = (oldX > caretX) ? (int)oldX : (int)caretX;
	rt.bottom = (oldY > caretY) ? (int)oldY : (int)caretY;
	if (rt.top != rt.bottom)
	{
		rt.bottom += fontHeight + bordWidthY;
		rt.left = 0;
		rt.right = textAreaWidth + leftMargin + rightMargin;
		rt.top += bordWidthY;
	}
	else
	{
		rt.bottom += fontHeight + bordWidthY;
		rt.left += leftMargin + bordWidthX;
		rt.right += leftMargin + bordWidthX;
		rt.top += bordWidthY;
	}
	InvalidateRect(hwnd, &rt, TRUE);
}

void UpdateRegion(bool setRegActive)
{
	if (regionActive == false && setRegActive == true)
	{
		lastTextPos = regionBegin;
		regBeginLine = caretLine;
	}
	regionActive = setRegActive;

	if (regionBegin == textPos) //Dummy check
		regionActive = false;

	if (regionActive == true)
	{
		/*{
			unsigned p1 = lastCaretLine, p2 = caretLine;
			SortAscending(&p1, &p2);
			numRegLines = p2 - p1 + 1;
		}*/
		UpdateRenderInfo(true);
		/*RECT rt;
		rt.left = 0;
		rt.top = 0;
		rt.right = textExtent;
		rt.bottom = fontHeight;*/
		//Add as many rectangles as necessary
		InvalidateRect(lastHwnd, NULL, TRUE);
		lastTextPos = textPos;
		lastCaretLine = caretLine;
	}
	else
	{
		//Delete the region render info
		free(regTextRI);
		regTextRI = NULL;
	}
}

void TryCursorUnhide()
{
	if (cursorVis == false)
	{
		cursorVis = true;
		ShowCursor(TRUE);
	}
}

void SortAscending(unsigned* p1, unsigned* p2)
{
	if (*p2 < *p1)
	{
		unsigned temp = *p2;
		*p2 = *p1;
		*p1 = temp;
	}
}

void FreeUndoHist()
{
	//Free all the undo data
	curUndo = 0;
	while (curUndo < numUndo)
		free(undoHistory[curUndo].data);

	//Do the usual
	free(undoHistory);
	undoHistory = NULL;
	numUndo = 0;
	curUndo = 0;
}

void FreeRenderInfo(bool freeNorRI)
{
	if (freeNorRI == true)
	{
		for (unsigned i = 0; i < numVisLines; i++)
			free(norTextRI[i].pdx);
		free(norTextRI);
		norTextRI = NULL;
	}
	if (regionActive == false)
		return;
	free(regTextRI);
	regTextRI = NULL;
}

void FreeLineInfo()
{
	free(numVisChars);
	numVisChars = NULL;
	free(firstVisChars);
	firstVisChars = NULL;
	free(firstVisOffset);
	firstVisOffset = NULL;
}
