//Startup code

#include <stdio.h>
#include <malloc.h>

int main()
{
	unsigned char* asmCode;
	char str[200];
	FILE* fp;
	int length;
	void (*MyFunc)(int, char*);

	fp = fopen("function.bin", "rb");
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	asmCode = malloc(length);
	fread(asmCode, length, 1, fp);
	fclose(fp);
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
	printf("The written string is: %s", str);
	free(asmCode);
	return 0;
}
