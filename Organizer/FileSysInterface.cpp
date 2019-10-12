/*File system organizer interface - Module for organizer tasks related to a general file system.
  This module is meant to enhance file system capabilities of creation dates,
  deletion of files, and file grouping by specifying data to reason why the file is created,
  how important specific data is to determine if it should be deleted and overwritten by more
  important data, and multiple grouping of files by keywords rather than just hierarchical
  directory grouping.

NOTE: This code does not check for errors in several places.
When loading a file, these are not checked for:
	Markup errors
	Attribute notation errors
	Illegal keywords
	Cyclicly-dependent groups
	Empty groups

NOTE: This code also is not algorithmically optimized in building the treeview.
This code does too much parameter passing.
*/

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0510
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>

enum EntryType {GROUP, FILE_T, H_GROUP};
enum FSSParseErr {PARSE_OK, ENCODINGFMT, WRONG_ROOT_TYPE, UNCLOSED_ELEMENT, UNCLOSED_TAG, UNKNOWN_ELEMENT};

struct FileStats_t
{
	char* filename;
	time_t dateCreated;
	time_t dateModified;
	char* keywords;
};

typedef struct FileStats_t FileStats;

typedef struct FSSEntry_t FSSEntry;

struct FSSEntry_t
{
	char* id;
	EntryType type;
	FileStats stats;
	char* history;
	char* importance;
	char* impPercent;
	FSSEntry** members; //If a group
	unsigned numMembers;
};

struct FSSData_t
{
	unsigned numEntries;
	FSSEntry* entries;
	FSSData* nextBlock;
};

typedef struct FSSData_t FSSData;

char* fssRawData;
char* nextRawData; //New data is added being segmented, the next pointer comes first
FSSData fssData;
bool isModified = false;
char* g_filename;
bool isCleanedUp = false; //Is the internal memory infrastructure clean?

//Forward private declarations
char* ReadWholeFile(const char* filename);
bool LoadCacheFile(const char* filename);
FSSParseErr ParseFSSummary();
time_t transDate(char* textDate);
void RecursTreeBuild(FSSEntry** recurs, unsigned numMembers, HWND treeWin, HTREEITEM hPrev, LPARAM lastlParam);

/* Load the file system summary file, which specifies the high level data of files for
   file system navigation.  If a cache file is present (which contains the data from
   after parsing and loading the other file), then load that instead of parsing. */
void LoadFSSummary(const char* filename)
{
	fssRawData = ReadWholeFile(filename);
	if (!fssRawData)
		return;

	if (!LoadCacheFile(filename))
		ParseFSSummary();
	g_filename = (char*)malloc(strlen(filename) + 1);
	strcpy(g_filename, filename);
}

/* WARNING: Returns data on the heap. You are responsible for freeing it. */
char* ReadWholeFile(const char* filename)
{
	FILE* fp;
	unsigned fileSize;
	char* buffer;

	/* Read the whole text file into memory */
	fp = fopen(filename, "rb");
	if (fp == NULL)
		return NULL;
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buffer = (char*)malloc(fileSize + 1);
	fread(buffer, fileSize, 1, fp);
	fclose(fp);
	buffer[fileSize] = '\0';

	/* Since we read the whole file this way, we will have to properly format newlines. */
	{
		unsigned src, dest;
		src = 0; dest = 0;
		while (src < fileSize)
		{
			if (buffer[src] != '\r')
				buffer[dest++] = buffer[src++];
			else
			{
				if (buffer[src+1] == '\n') /* CR+LF */
					src++;
				buffer[dest++] = '\n';
				if (src < fileSize) src++; /* ??? (You would think otherwise could not happen.) */
			}
		}
	}
	buffer[src] = '\0';
	{ /* Free extra bytes (if any) */
		unsigned newSize;
		newSize = fileSize + 1 - (src - dest);
		if (newSize != (fileSize + 1))
			buffer = (char*)realloc(buffer, newSize);
	}

	return buffer;
}

bool LoadCacheFile(const char* filename)
{
	//First construct the cache file name
	unsigned filNamLen = strlen(filename);
	char* cacheName = (char*)alloca(filNamLen + 1);
	strcpy(cacheName, filename);
	cacheName[filNamLen - 3] = 'b';
	cacheName[filNamLen - 2] = 'i';
	cacheName[filNamLen - 1] = 'n';

	//Load the file
	FILE* fp = fopen(cacheName, "rb");
	if (fp == NULL)
		return false;

	fread(&(fssData.numEntries), sizeof(unsigned), 1, fp);
	fssData.entries = (FSSEntry*)malloc(sizeof(FSSEntry) * fssData.numEntries);
	fread(fssData.entries, sizeof(FSSEntry), fssData.numEntries, fp);
	//Rebase pointers and allocate memory for member lists
	unsigned accMemberBytes = sizeof(FSSEntry) * fssData.numEntries;
	for (unsigned i = 0; i < fssData.numEntries; i++)
	{
		if (fssData.entries[i].type == GROUP)
		{
			if (fssData.entries[i].numMembers != 0)
			{
				FSSEntry** tMembers = (FSSEntry**)malloc(sizeof(FSSEntry*) * fssData.entries[i].numMembers);
				fread(tMembers, sizeof(FSSEntry*), fssData.entries[i].numMembers, fp);
				fssData.entries[i].members = tMembers;
				//Rebasing the member array pointer was unnecessary for writing the cache...
				//But the pointers in the array need to be rebased
				for (unsigned j = 0; j < fssData.entries[i].numMembers; i++)
					fssData.entries[i].members[j] = (FSSEntry*)(fssData.entries[i].members[j] + (unsigned)fssData.entries);
			}
		}
		else
		{
			fssData.entries[i].stats.filename = (char*)(fssData.entries[i].stats.filename + (unsigned)fssRawData);
			fssData.entries[i].stats.keywords = (char*)(fssData.entries[i].stats.keywords + (unsigned)fssRawData);
		}
		fssData.entries[i].id = (char*)(fssData.entries[i].id + (unsigned)fssRawData);
		fssData.entries[i].history = (char*)(fssData.entries[i].history + (unsigned)fssRawData);
		fssData.entries[i].impPercent = (char*)(fssData.entries[i].impPercent + (unsigned)fssRawData);
		fssData.entries[i].importance = (char*)(fssData.entries[i].importance + (unsigned)fssRawData);
	}

	fclose(fp);
	//free(cacheName);

	return true;
}

FSSParseErr ParseFSSummary()
{
	unsigned fileSize = strlen(fssRawData);
	fssData.numEntries = 0;
	fssData.entries = (FSSEntry*)malloc(sizeof(FSSEntry) * 100);
	fssData.nextBlock = NULL;
	unsigned i = 0;

	bool parseable;

	char* attribute;
	unsigned attribLen;
	char quoteType;
	char* value;
	unsigned valLen;

	/* Check first four bytes for disallowed characters.  Sorry, this is not a conformant
	   XML parser. */
	parseable = true;
	for (i = 0; i < 4; i++)
	{
		if (*(fssRawData + i) == 0xEF ||
			*(fssRawData + i) == 0xBB ||
			*(fssRawData + i) == 0xBF ||
			*(fssRawData + i) == 0xFE ||
			*(fssRawData + i) == 0xFF ||
			*(fssRawData + i) == 0x00)
			parseable = false;
	}
	if (parseable == false)
		return ENCODINGFMT;
	i = 0;

	/* Do the real parse */

#define CURPOS (fssRawData + i)

#define WHITESPACE(iter) \
	*CURPOS == ' ' || \
	*CURPOS == '\t' || \
	*CURPOS == '\n'

		//Read name
		//Skip whitespace and equal sign
		//Read quotes
		//Take value and replace last quote with null character
#define READATTRIB(iter) \
	attribute = CURPOS; \
	while (!(WHITESPACE(iter)) && *CURPOS != '=') \
		iter++; \
	attribLen = (unsigned)(CURPOS - attribute); \
\
	if (*CURPOS != '=') \
	{ \
		while (WHITESPACE(iter)) \
			iter++; \
	} \
	iter++; \
	while (WHITESPACE(iter)) \
		iter++; \
\
	quoteType = *CURPOS; \
	iter++; \
	value = CURPOS; \
	while (*CURPOS != quoteType) \
		iter++; \
	*CURPOS = '\0'; \
	valLen = (unsigned)(CURPOS - value); \
	iter++;

	if (strncmp(fssRawData, "<organizer version=\"1.1\" purpose=\"File system cataloging by metadata\">", 70) == 0)
	{
		bool rootClosed;
		rootClosed = false;
		i += 70;
		while (i < fileSize)
		{
			if (WHITESPACE(i))
				i++;
			else if (strncmp(fssRawData + i, "<entry", 6) == 0)
			{
				unsigned j;

				fssData.entries[fssData.numEntries].stats.keywords = NULL;

				while (WHITESPACE(i))
					i++;
				READATTRIB(i);
				if (strncmp(attribute, "id", 2) == 0)
					fssData.entries[fssData.numEntries].id = value;
				if (strncmp(attribute, "type", 4) == 0)
				{
					if (strncmp(value, "group", 5) == 0)
						fssData.entries[fssData.numEntries].type = GROUP;
					if (strncmp(value, "file", 4) == 0)
						fssData.entries[fssData.numEntries].type = FILE_T;
				}

				while (WHITESPACE(i))
					i++;
				READATTRIB(i);
				if (strncmp(attribute, "id", 2) == 0)
					fssData.entries[fssData.numEntries].id = value;
				if (strncmp(attribute, "type", 4) == 0)
				{
					if (strncmp(value, "group", 5) == 0)
						fssData.entries[fssData.numEntries].type = GROUP;
					if (strncmp(value, "file", 4) == 0)
						fssData.entries[fssData.numEntries].type = FILE_T;
				}

				while (WHITESPACE(i))
					i++;
				if (*(fssRawData + i) != '>')
					return UNCLOSED_TAG;
				i++;

				for (j = 0; j < 3; j++)
				{
					if (WHITESPACE(i))
						i++;
					else if (strncmp(fssRawData + i, "<statistics", 11) == 0)
					{
						unsigned k;

						i += 11;

						for (k = 0; k < 4; k++)
						{
							while (WHITESPACE(i))
								i++;
							READATTRIB(i);
							if (strncmp(attribute, "filename", 6) == 0)
								fssData.entries[fssData.numEntries].stats.filename = value;
							if (strncmp(attribute, "dateCreated", 9) == 0)
								fssData.entries[fssData.numEntries].stats.dateCreated = transDate(value);
							if (strncmp(attribute, "dateModified", 10) == 0)
								fssData.entries[fssData.numEntries].stats.dateModified = transDate(value);
							if (strncmp(attribute, "keywords", 8) == 0)
								fssData.entries[fssData.numEntries].stats.keywords = value;
						}

						while (WHITESPACE(i))
							i++;
						if (strncmp(fssRawData + i, "/>", 2) != 0)
							return UNCLOSED_TAG;
						i += 2;
					}
					else if (strncmp(fssRawData + i, "<history>", 9) == 0)
					{
						i += 9;

						while (WHITESPACE(i))
							i++;
						fssData.entries[fssData.numEntries].history = (fssRawData + i);
						while (strncmp(fssRawData + i, "</history>", 10) != 0)
							i++;
						*(fssRawData + i) = '\0';
						i += 10;
					}
					else if (strncmp(fssRawData + i, "<importance", 11) == 0)
					{
						i += 11;

						while (WHITESPACE(i))
							i++;
						READATTRIB(i);
						if (strncmp(attribute, "percent", 7) == 0)
							fssData.entries[fssData.numEntries].impPercent = value;

						while (WHITESPACE(i))
							i++;
						if (*(fssRawData + i) != '>')
							return UNCLOSED_TAG;
						i++;

						while (WHITESPACE(i))
							i++;
						fssData.entries[fssData.numEntries].importance = (fssRawData + i);
						while (strncmp(fssRawData + i, "</importance>", 13) != 0)
							i++;
						*(fssRawData + i) = '\0';
						i += 13;
					}
					else
						return UNKNOWN_ELEMENT;
				}

				while (WHITESPACE(i))
					i++;
				if (strncmp(fssRawData + i, "</entry>", 8) != 0)
					return UNCLOSED_ELEMENT;

				fssData.entries[fssData.numEntries].members = NULL;
				fssData.entries[fssData.numEntries].numMembers = 0;
				fssData.numEntries++;
				if (fssData.numEntries % 100 == 0)
					fssData.entries = (FSSEntry*)realloc(fssData.entries, sizeof(FSSEntry) * (fssData.numEntries + 100));
			}
			else if (strncmp(fssRawData + i, "</organizer>", 12) == 0)
			{
				i += 12;
				rootClosed = true; //We're done
			}
			else
				return UNKNOWN_ELEMENT;
		}
		if (rootClosed == false)
			return UNCLOSED_ELEMENT;
	}
	else
		return WRONG_ROOT_TYPE;
	//Deallocate extra entries (if any)
	if (fssData.numEntries % 100 != 0)
		fssData.entries = (FSSEntry*)realloc(fssData.entries, fssData.numEntries);

#undef CURPOS
#undef WHITESPACE
#undef READATTRIB

	/* Fill groups */
	for (i = 0; i < fssData.numEntries; i++)
	{
		if (fssData.entries[i].type == GROUP)
		{
			unsigned& curNumMembers = fssData.entries[i].numMembers;
			unsigned j;
			curNumMembers = 0; /* ??? (redundancy) */
			fssData.entries[i].members = (FSSEntry**)malloc(sizeof(FSSEntry*) * 20);
			for (unsigned j = 0; j < fssData.numEntries; j++)
			{
				char* testBegin; /* Keyword to test against */
				unsigned testSize;
				testBegin = fssData.entries[j].stats.keywords; /* Keyword to test against */
				testSize = 0;
				while (*(testBegin + testSize) != '\0')
				{
					if (*(testBegin + testSize) != ' ')
						testSize++;
					else
					{
						if (strncmp(fssData.entries[i].id, testBegin, testSize) == 0)
						{
							fssData.entries[i].members[curNumMembers++] = &(fssData.entries[j]);
							if (curNumMembers % 20 == 0)
								fssData.entries[i].members = (FSSEntry**)realloc(fssData.entries[i].members, sizeof(FSSEntry*) * (curNumMembers + 20));
							goto foundMatch;
						}
						else
						{
							testBegin += (testSize + 1);
							testSize = 0;
						}
					}
				}
				if (strncmp(fssData.entries[i].id, testBegin, testSize) == 0)
				{
					fssData.entries[i].members[curNumMembers++] = &(fssData.entries[j]);
					if (curNumMembers % 20 != 0)
						fssData.entries[i].members = (FSSEntry**)realloc(fssData.entries[i].members, sizeof(FSSEntry*) * curNumMembers);
					goto foundMatch;
				}
			}
			if (curNumMembers == 0)
			{
				free(fssData.entries[i].members);
				fssData.entries[i].members = NULL;
			}
foundMatch: ;
		}
	}

	return PARSE_OK;
}

time_t transDate(/*const */char* textDate)
{
	//Time format: "2007-05-14 21:09:32.582 UTC"
	char newTextDate[100];
	char* ymdhms[6]; //year month day hour minute second
	tm newTm;
	unsigned i, j;

	strcpy(newTextDate, textDate);

	//First split string up into ymdhms (see above)
	ymdhms[0] = &(newTextDate[0]);
	//Below, i is set to one in case there is a negative sign in front of the year. However,
	//years before the epoch are not handled by the conversion function below.
	for (i = 1, j = 1; textDate[i] != '\0'; i++)
	{
		if (textDate[i] == '-' || textDate[i] == ' ' || textDate[i] == ':')
		{
			textDate[i] = '\0';
			if (j == 6)
				break;
			ymdhms[j] = &(textDate[i+1]);
			j++;
		}
	}

	//Then do final conversion
	newTm.tm_year	= atoi(ymdhms[0]) - 1900;
	newTm.tm_mon	= atoi(ymdhms[1]);
	newTm.tm_mday	= atoi(ymdhms[2]);
	newTm.tm_hour	= atoi(ymdhms[3]);
	newTm.tm_min	= atoi(ymdhms[4]);
	newTm.tm_sec	= atoi(ymdhms[5]);
	newTm.tm_isdst	= -1;
	newTm.tm_wday	= -1;
	newTm.tm_yday	= -1;
	return mktime(&newTm);
}

bool SaveFSSData()
{
	unsigned numGroups = 0; //Used for the cache file
	FILE* fp;

	if (isModified == false)
		return false;
	numGroups = 0;
	//First write the XML file and
	//correct text pointers relative to the XML file
	FILE* fp = fopen(g_filename, "wt");
	if (fp == NULL)
		return false;
	fputs("<organizer version=\"1.1\" purpose=\"File system cataloging by metadata\">\n", fp);
	FSSData* curBlock = &fssData;
	while (curBlock != NULL)
	{
		unsigned i;
		unsigned lastPos;
		for (i = 0; i < curBlock->numEntries; i++)
		{
			if (curBlock->entries[i].type == GROUP)
			{
				numGroups++;

				lastPos = ftell(fp);
				fprintf(fp, "<entry id=\"%s\" type=\"%s\">\n",
					curBlock->entries[i].id, "group");
				curBlock->entries[i].id = (char*)(lastPos + 11);

				/*lastPos = ftell(fp);
				fprintf(fp, "<history>%s</history>\n", curBlock->entries[i].history);
				curBlock->entries[i].history = (char*)(lastPos + 9);

				lastPos = ftell(fp);
				fprintf(fp, "<importance percent=\"%s\">%s</importance>\n",
					curBlock->entries[i].impPercent,
					curBlock->entries[i].importance);
				curBlock->entries[i].impPercent = (char*)(lastPos + 21);
				curBlock->entries[i].importance = (char*)(lastPos + 23 + strlen(curBlock->entries[i].impPercent));*/
			}
			else
			{
				lastPos = ftell(fp);
				fprintf(fp, "<entry id=\"%s\" type=\"%s\">\n",
					curBlock->entries[i].id, "file");
				curBlock->entries[i].id = (char*)(lastPos + 11);
			}

			{
				tm* createdTm;
				tm* modifiedTm;
				char createdStr[100], modifiedStr[100];
				createdTm = gmtime(&(curBlock->entries[i].stats.dateCreated));
				modifiedTm = gmtime(&(curBlock->entries[i].stats.dateModified));

				sprintf(createdStr, "%i-%2i-%2i %2i:%2i:%2i UTC",
						createdTm->tm_year,
						createdTm->tm_mon,
						createdTm->tm_mday,
						createdTm->tm_hour,
						createdTm->tm_min,
						createdTm->tm_sec);

				sprintf(modifiedStr, "%i-%2i-%2i %2i:%2i:%2i UTC",
						modifiedTm->tm_year,
						modifiedTm->tm_mon,
						modifiedTm->tm_mday,
						modifiedTm->tm_hour,
						modifiedTm->tm_min,
						modifiedTm->tm_sec);

				lastPos = ftell(fp);
				fprintf(fp, "<statistics filename=\"%s\" dateCreated=\"%s\" dateModified=\"%s\" keywords=\"%s\">\n",
						curBlock->entries[i].stats.filename, createdStr, //22+54-39+74-58+90-78 for 65 below
						modifiedStr, curBlock->entries[i].stats.keywords);
				curBlock->entries[i].stats.filename = (char*)(lastPos + 22);
				curBlock->entries[i].stats.keywords = (char*)(lastPos + 65 + strlen(createdStr) + strlen(modifiedStr));

				lastPos = ftell(fp);
				fprintf(fp, "<history>%s</history>\n", curBlock->entries[i].history);
				curBlock->entries[i].history = (char*)(lastPos + 9);

				lastPos = ftell(fp);
				fprintf(fp, "<importance percent=\"%s\">%s</importance>\n",
						curBlock->entries[i].impPercent,
						curBlock->entries[i].importance);
				curBlock->entries[i].impPercent = (char*)(lastPos + 21);
				curBlock->entries[i].importance = (char*)(lastPos + 23 + strlen(curBlock->entries[i].impPercent));
				//}
				fputs("</entry>\n", fp);
			}
		}
		curBlock = curBlock->nextBlock;
	}
	fputs("</organizer>", fp);
	fclose(fp);

	//Then write the cache file
	{
		unsigned filNamLen;
		char* cacheName;
		filNamLen = strlen(g_filename);
		cacheName = (char*)alloca(filNamLen + 1);
		strcpy(cacheName, g_filename);
		cacheName[filNamLen - 3] = 'b';
		cacheName[filNamLen - 2] = 'i';
		cacheName[filNamLen - 1] = 'n';
		fp = fopen(cacheName, "wb");
		if (fp == NULL)
			return false;
	}

	//Find the total number of entries and blocks
	{
		unsigned totNumEntries;
		unsigned numBlocks;
		totNumEntries = 0;
		numBlocks = 0;
		curBlock = &fssData;
		while (curBlock != NULL)
		{
			totNumEntries += curBlock->numEntries;
			numBlocks++;
			curBlock = curBlock->nextBlock;
		}
		fwrite(&totNumEntries, sizeof(unsigned), 1, fp);

		//Write the headers
		curBlock = &fssData;
		while (curBlock != NULL)
		{
			fwrite(curBlock->entries, sizeof(FSSEntry), curBlock->numEntries, fp);
			curBlock = curBlock->nextBlock;
		}
	}

	//Find the entry begin and end addresses, entry array sizes, and accumulated previous entry data
	FSSEntry** entryAddrs = (FSSEntry**)malloc(sizeof(FSSEntry*) * numBlocks);
	FSSEntry** entryEndAddrs = (FSSEntry**)malloc(sizeof(FSSEntry*) * numBlocks); //stores pointer of the position after the end of the entry array
	unsigned* entryArrSizes = (unsigned*)malloc(sizeof(unsigned) * numBlocks);
	unsigned* accPrevSize = (unsigned*)malloc(sizeof(unsigned) * numBlocks); //starts after the first
	curBlock = &fssData;
	//Leave first iteration out of following loop
		entryAddrs[0] = curBlock->entries;
		entryArrSizes[0] = sizeof(FSSEntry) * curBlock->numEntries;
		entryEndAddrs[0] = entryAddrs[0] + entryArrSizes[0];
		accPrevSize[0] = 0;
		curBlock = curBlock->nextBlock;
	for (unsigned i = 1; i < numBlocks; i++)
	{
		entryAddrs[i] = curBlock->entries;
		entryArrSizes[i] = sizeof(FSSEntry) * curBlock->numEntries;
		entryEndAddrs[i] = entryAddrs[i] + entryArrSizes[i];
		accPrevSize[i] = accPrevSize[i-1] + entryArrSizes[i-1];
		curBlock = curBlock->nextBlock;
	}

	//Write the corrected member pointers relative to the file and correct bad member pointers
	//Corrections are relative to after the total number of entries indicator
	unsigned accMemberBytes = sizeof(FSSEntry) * totNumEntries;
	unsigned blockNum = 0;
	curBlock = &fssData;
	while (curBlock != NULL)
	{
		for (unsigned i = 0; i < curBlock->numEntries; i++)
		{
			if (curBlock->entries[i].members != NULL)
			{
				FSSEntry** correctPtrs = (FSSEntry**)malloc(sizeof(FSSEntry*) * curBlock->entries[i].numMembers);
				for (unsigned j = 0; j < curBlock->entries[i].numMembers; j++)
				{
					for (unsigned k = 0; k < numBlocks; k++)
					{
						if (curBlock->entries[i].members[j] >= entryAddrs[k] &&
							curBlock->entries[i].members[j] < entryEndAddrs[k])
						{
							unsigned localOffset = (unsigned)(curBlock->entries[i].members[j] - entryAddrs[k]);
							correctPtrs[j] = (FSSEntry*)(accPrevSize[k] + localOffset);
							break;
						}
					}
				}
				fwrite(correctPtrs, sizeof(FSSEntry*) * curBlock->entries[i].numMembers, 1, fp);
				free(correctPtrs);
				//Correct the old written pointer
				unsigned curPos = ftell(fp);
				fseek(fp, sizeof(unsigned) + accPrevSize[blockNum] + sizeof(FSSEntry) - sizeof(unsigned) - sizeof(FSSEntry**), SEEK_SET);
				fwrite((FSSEntry**)accMemberBytes, sizeof(FSSEntry**), 1, fp);
				fseek(fp, curPos, SEEK_SET);
				accMemberBytes += sizeof(FSSEntry*) * curBlock->entries[i].numMembers;
			}
		}
		blockNum++;
		curBlock = curBlock->nextBlock;
	}

	free(entryAddrs);
	free(entryEndAddrs);
	free(entryArrSizes);
	free(accPrevSize);

	fclose(fp);
	//free(cacheName);

	return true;
}

void FSSBuildTreeView(HWND treeWin)
{
	HTREEITEM hPrev;
	TVINSERTSTRUCT tv;
	//unsigned numRoot = 0;
	//FSSEntry** recurs = (FSSEntry**)malloc(sizeof(FSSEntry*) * 100);

	tv.hParent = NULL;
	tv.hInsertAfter = TVI_LAST;
	tv.item.mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_TEXT;
	tv.item.pszText = "Root";
	tv.item.cchTextMax = 5;
	tv.item.cChildren = 0;
	tv.item.lParam = 0;
	//Build the tree
	//Parse each entry
	//If the entry does not have any keywords
	//Then it is a root item
	//Do a second pass: (Maybe)
	//Parse each group recursively to build the tree
	for (unsigned i = 0; i < fssData.numEntries; i++)
	{
		if (fssData.entries[i].stats.keywords == NULL)
		{
			tv.item.pszText = fssData.entries[i].id;
			tv.item.cchTextMax = strlen(fssData.entries[i].id) + 1;
			if (fssData.entries[i].type == GROUP)
				tv.item.cChildren = 1;
			else
				tv.item.cChildren = 0;
			hPrev = TreeView_InsertItem(treeWin, &tv);
			tv.item.lParam++;
			if (fssData.entries[i].type == GROUP)
				RecursTreeBuild(fssData.entries[i].members, fssData.entries[i].numMembers, treeWin, hPrev, tv.item.lParam);
			//recurs[numRoot] = &fssData.entries[i];
			//numRoot++;
			//if (numRoot % 100 == 0)
			//	recurs = (FSSEntry**)realloc(recurs, sizeof(FSSEntry*) * (numRoot + 100));
		}
	}
	//if (numRoot % 100 != 0)
	//	recurs = (FSSEntry**)realloc(recurs, sizeof(FSSEntry*) * numRoot);
}

void RecursTreeBuild(FSSEntry** recurs, unsigned numMembers, HWND treeWin, HTREEITEM hPrev, LPARAM lastlParam)
{
	TVINSERTSTRUCT tv;

	tv.hParent = hPrev;
	tv.hInsertAfter = TVI_LAST;
	tv.item.mask = TVIF_CHILDREN | TVIF_PARAM | TVIF_TEXT;
	tv.item.pszText = "Root";
	tv.item.cchTextMax = 5;
	tv.item.cChildren = 0;
	tv.item.lParam = lastlParam;

	for (unsigned i = 0; i < numMembers; i++)
	{
		tv.item.pszText = recurs[i]->id;
		tv.item.cchTextMax = strlen(recurs[i]->id) + 1;
		if (recurs[i]->type == GROUP)
			tv.item.cChildren = 1;
		else
			tv.item.cChildren = 0;
		hPrev = TreeView_InsertItem(treeWin, &tv);
		tv.item.lParam++;
		if (recurs[i]->type == GROUP)
			RecursTreeBuild(recurs[i]->members, recurs[i]->numMembers, treeWin, hPrev, tv.item.lParam);
	}
}

void BuildTimeline()
{
}

void FSSOnChangeSelection(char* newItem, HWND dataWin)
{
	//Save old data
	//Relocate if old data is larger
	//Reparse and update groups and timeline
	for (unsigned i = 0; i < fssData.numEntries; i++)
	{
		if (strcmp(fssData.entries[i].id, newItem) == 0)
			break; //We found it!
	}
	//Load data from new item
}

void AddFSSEntry(EntryType type)
{
	//Get type
	//Add tree entry
	//Add text data when done editing
}

void ChangeFSSEntry()
{
	//Just do it explicitly
}

//Removing entries is done automatically on demand
//void RemoveFSSEntry()

void FSSscaleImportance()
{
	//Set what to scale the maximum to
	//-OR-
	//Do a curve style scaling
}

//We need to free dynamically allocated memory
bool FSSshutdown()
{
	free(fssRawData);
	{ //Free all the segmented raw data
		char* nextSegment = *((char**)nextRawData);
		while (nextSegment != NULL)
		{
			char* tNextSegment = *((char**)nextSegment);
			free(nextSegment);
			nextSegment = tNextSegment;
		}
	}
	for (unsigned i = 0; i < fssData.numEntries; i++)
		free(fssData.entries[i].members);
	free(fssData.entries);
	{ //Free all the segmented reference data
		FSSData* nextSegment = fssData.nextBlock;
		while (nextSegment != NULL)
		{
			FSSData* tNextSegment = nextSegment->nextBlock;
			for (unsigned i = 0; i < nextSegment->numEntries; i++)
				free(nextSegment->entries[i].members);
			free(nextSegment->entries);
			free(nextSegment);
			nextSegment = tNextSegment;
		}
	}
	free(g_filename);
	return true;
}

//Cleans up the internal memory infrastructure
void CleanUpMemory()
{
	if (isCleanedUp == false)
	{
		/*MessageBox(NULL, "Sorry, internal memory management capabilities are not fully\n\
						 implemented as of now. Because of that, this code must allocate\n\
						 more memory than is being used to clean up the memory.", "Information", MB_OK | MB_ICONINFORMATION);*/
		if (MessageBox(NULL, "Cleaning up the internal memory infrastructure involves\n\
						 saving your data to a file and reloading. Do you want to\n\
						 continue?", "Information", MB_YESNO | MB_ICONINFORMATION) == IDYES)
		{
			//Clean up
			char* t_filename = (char*)alloca(strlen(g_filename) + 1);
			strcpy(t_filename, g_filename);
			SaveFSSData();
			FSSshutdown();
			LoadFSSummary(t_filename);
		}
	}
}

LRESULT CALLBACK FSwindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		break;
	case WM_DESTROY:
		break;
	case WM_PAINT:
		break;
	case WM_LBUTTONDOWN:
		break;
	case WM_RBUTTONDOWN:
		break;
	case WM_CONTEXTMENU:
		break;
	case WM_LBUTTONUP:
		break;
	case WM_MOVE:
		break;
	case WM_SETCURSOR:
		break;
	case WM_MOUSEMOVE:
		break;
	case WM_TIMER:
		break;
	case WM_SETFONT:
		break;
	case WM_GETFONT:
		break;
	case WM_CUT:
		break;
	case WM_COPY:
		break;
	case WM_PASTE:
		break;
	case WM_CLEAR:
		break;
	case EM_UNDO:
		break;
	case EM_REDO:
		break;
	case WM_CAPTURECHANGED:
		break;
	case WM_SETFOCUS:
		break;
	case WM_KILLFOCUS:
		break;
	case WM_SIZE:
		break;
	case WM_VSCROLL:
	case WM_HSCROLL:
		break;
	case WM_KEYDOWN:
		break;
	case WM_KEYUP:
		break;
	case WM_CHAR:
		break;
	case WM_MOUSEWHEEL:
		break;
	case WM_MBUTTONDOWN:
		break;
	case WM_ADDENTRY:
		break;
	case WM_CHANGEENTRY:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
