//Startup code

#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

int main()
{
	unsigned char* asmCode;
	char str[200];
	int fd;
	unsigned length;
	void (*MyFunc)(int, char*);

	fd = open("function.bin", O_RDWR);
	if (fd == -1)
		return 1;
	length = (unsigned)lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	asmCode = (unsigned char*)mmap(NULL, length, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, 0);
	if (asmCode == (unsigned char*)-1)
	{
		close(fd);
		return 2;
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
	munmap(asmCode, length);
	close(fd);
	return 0;
}
