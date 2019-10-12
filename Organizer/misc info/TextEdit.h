//Defines general layout for reworked text editor

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

//Private global variable declarations
/*char* buffer;
unsigned textSize;
unsigned textPos; //Byte-wise caret position
unsigned caretLine;
unsigned caretX; //X position of caret (without margins)
bool wasInsert; //Was the last operation an insert or a delete?

//Variables that reduce function calling
unsigned wheelLines; //This needs updating at WM_SETTINGCHANGE
unsigned dblClickTime; //Same with this
unsigned visLine; //First visible line
unsigned visLnOffset; //Negative offset to top of first visible line
POINT lastMousePos;

//Private function declarations
void ScrollContents(bool offset, unsigned otherType, unsigned amount, int sBar);
void MButtonScroll(); //Function called for processing mouse movements after pressing middle mouse button
void InsertText(char c, const char* str);
void DeleteText(unsigned p1, unsigned p2);
void CopyRegion();
void MoveCaret(bool ctrlHeld);
void CheckUndoAdd(); //Add an operation to the undo queue
void WrapWords();
void RewrapWords();*/

//Public function delcarations
LRESULT CALLBACK TextEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
/*
Window management: WM_CREATE, WM_SETTINGCHANGE, WM_DESTROY, WM_PAINT, WM_SIZE,
		WM_MOVE, WM_CAPTURECHANGED
Special input: WM_HSCROLL, WM_VSCROLL, WM_MOUSEACTIVATE, WM_CONTEXTMENU, WM_TIMER,
		EM_SETHANDLE
Edit commands: WM_CUT, WM_COPY, WM_PASTE, WM_CLEAR, WM_UNDO, EM_REDO, WM_SETFONT
Mouse input: WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP,
		WM_MBUTTONDOWN, WM_MBUTTONUP, WM_MOUSEMOVE, WM_SETCURSOR, WM_MOUSEWHEEL,
		MSH_MOUSEWHEEL
Keyboard input: WM_SETFOCUS, WM_KILLFOCUS, WM_KEYDOWN, WM_CHAR
Custom messages: WM_ADDENTRY, WM_CHANGEENTRY
*/

#endif
