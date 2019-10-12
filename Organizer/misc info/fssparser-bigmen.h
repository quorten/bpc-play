/* Parse a file system summary file. */

#ifndef FSSPARSER_H
#define FSSPARSER_H

#define FA_NORMAL		0
#define FA_MULTILINE	1
#define FA_VERBATIM		2
/* Technically, a subheader is not an attribute, but it is stored as one. */
#define FA_HEADER	3

struct FSSattrib_t
{
	char* name;
	char* value;
	unsigned type;
};

typedef struct FSSattrib_t FSSattrib;

struct FSSentry_t
{
	char* name;
	FSSattrib* attribs;
	unsigned numAttribs;
};

typedef struct FSSentry_t FSSentry;

extern FSSentry* entries;
extern FSSentry headEntry;
extern unsigned numEntries;

bool ParseFile(char* buffer, unsigned dataSize);
void EchoFSSData(FILE* fp);
void FreeFSSData();

#endif
