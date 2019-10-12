//File system organizer interface - Module for organizer tasks related to a general file system.
//See FileSysInterface.cpp for more details

#ifndef FSINTERF_H
#define FSINTERF_H

//Public declarations
enum EntryType {GROUP, FILE_T, H_GROUP};

void LoadFSSummary(const char* filename);
bool SaveFSSData();
void FSSBuildTreeView(HWND treeWin);
void BuildTimeline();
void FSSOnChangeSelection(char* newItem, HWND dataWin);
void AddFSSEntry(EntryType type);
void ChangeFSSEntry();
bool FSSshutdown();
void CleanUpMemory();

LRESULT CALLBACK FSwindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif