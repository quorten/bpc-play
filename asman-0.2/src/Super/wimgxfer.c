/* Copy from serial COM to file, Windows API style.  */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

unsigned char g_buffer[532*4];

void main()
{
	unsigned long in_blksize, in_blkcount;
	unsigned long in_size;
	unsigned long buffer_size;
	unsigned long i = 0;
	char pchar = 'n';
	DCB dcb;
	HANDLE hCom, hFile;
	DWORD dwError;
	BOOL fSuccess;

	fputs("Block size: ", stdout);
	scanf(" %ld", &in_blksize);
	fputs("Number of blocks: ", stdout);
	scanf(" %ld", &in_blkcount);
	pchar = getchar(); /* Consume the newline character.  */
	if (in_blksize > 532)
	{
		puts("Error: Block size too big.");
		return;
	}
	in_size = in_blksize * in_blkcount;
	buffer_size = in_blksize * 4;

	hCom = CreateFile( "COM1",
		GENERIC_READ | GENERIC_WRITE,
		0,    // comm devices must be opened w/exclusive-access 
		NULL, // no security attributes 
		OPEN_EXISTING, // comm devices must use OPEN_EXISTING 
		0,    // not overlapped I/O 
		NULL  // hTemplate must be NULL for comm devices 
		);

	if (hCom == INVALID_HANDLE_VALUE) 
	{
		dwError = GetLastError();

		// handle error
		puts("Windows COM open error.");
		return;
	}

	// Omit the call to SetupComm to use the default queue sizes.
	// Get the current configuration.

	fSuccess = GetCommState(hCom, &dcb);

	if (!fSuccess) 
	{
		// Handle the error. 
		puts("Windows GetCommState error.");
		CloseHandle(hCom);
		return;
	}

	// Fill in the DCB: baud=9600, 8 data bits, no parity, 1 stop bit. 

	fputs("57.6 kbits/sec baud? (y/n) ", stdout);
	pchar = getchar(); putchar('\n');
	if (pchar == 'y')
		dcb.BaudRate = CBR_57600;
	else
		dcb.BaudRate = CBR_19200;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	fSuccess = SetCommState(hCom, &dcb);

	if (!fSuccess) 
	{
		// Handle the error. 
		puts("Windows SetCommState error.");
		CloseHandle(hCom);
		return;
	}

	hFile = CreateFile( "result.bin",
		GENERIC_READ | GENERIC_WRITE,
		0,    // comm devices must be opened w/exclusive-access 
		NULL, // no security attributes 
		CREATE_ALWAYS, 
		0,    // not overlapped I/O 
		NULL  // hTemplate must be NULL for comm devices 
		);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		dwError = GetLastError();

		// handle error
		puts("Windows file open error.");
		CloseHandle(hCom);
		return;
	}

	while (i < in_size)
	{
		DWORD bytesRead;
		unsigned long tg_read_bytes = buffer_size;
		/* Check if we have a non-divisible-by-4 block read at the
		   end.  */
		if (i + tg_read_bytes > in_size)
			tg_read_bytes = in_size - i;
		fSuccess = ReadFile(hCom, g_buffer, tg_read_bytes, &bytesRead, NULL);
		if (!fSuccess || bytesRead != tg_read_bytes)
		{
			puts("Windows read file error.");
		}
		fSuccess = WriteFile(hFile, g_buffer, tg_read_bytes, &bytesRead, NULL);
		if (!fSuccess || bytesRead != tg_read_bytes)
		{
			puts("Windows write file error.");
		}
		i += tg_read_bytes;
	}

	CloseHandle(hCom);
	CloseHandle(hFile);
}
