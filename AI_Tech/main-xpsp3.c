//Startup code

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

int main()
{
	unsigned char* asmCode;
	char str[200];
	HANDLE hFile;
	HANDLE hMap;
	void (*MyFunc)(int, char*);

	hFile = CreateFile("function.bin", GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL)
		return 1;
	hMap = CreateFileMapping(hFile, NULL, PAGE_EXECUTE_READWRITE, 0, 0, NULL);
	if (hMap == NULL)
	{
		CloseHandle(hFile);
		return 2;
	}
	asmCode = (unsigned char*)MapViewOfFile(hMap, FILE_MAP_WRITE | FILE_MAP_EXECUTE, 0, 0, 0);
	if (asmCode == NULL)
	{
		CloseHandle(hMap);
		CloseHandle(hFile);
		return 3;
	}
	//Initialize I/O
	//	Search for all DLLs/EXEs
	//	Get their exports
	//	Call AINotify
	//Initialize database
	//	Load entire file(?) into memory
	//Initialize AI
	//	Start new thread from AImain
	//I/O loop
	//	Send any captured information to AINotify
	//Shutdown
	//	Call AINotify
	//	Flush database
	//	Shutdown I/O
	MyFunc = (void (*)(int, char*))asmCode;
	MyFunc((int)asmCode, str);
	printf("The written string is: %s\n", str);
	UnmapViewOfFile(asmCode);
	CloseHandle(hMap);
	CloseHandle(hFile);
	return 0;
}
