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

struct FileStats
{
	char* filename;
	time_t dateCreated;
	time_t dateModified;
	char* keywords;
};

struct FSSEntry
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

struct FSSData
{
	unsigned numEntries;
	FSSEntry* entries;
	FSSData* nextBlock;
};

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

//Load the file system summary file, which specifies the high level data of files for
//file system navigation.  If a cache file is present (which contains the data from
//after parsing and loading the other file), then load that instead of parsing.
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

//WARNING: Returns data on the heap. You are responsible for freeing it.
char* ReadWholeFile(const char* filename)
{
	//Read the whole text file into memory
	FILE* fp = fopen(filename, "rb");
	if (fp == NULL)
		return NULL;
	fseek(fp, 0, SEEK_END);
	unsigned fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* buffer = (char*)malloc(fileSize + 1);
	fread(buffer, fileSize, 1, fp);
	fclose(fp);
	buffer[fileSize] = '\0';

	//Since we read the whole file this way, we will have to properly format newlines.
	unsigned src = 0, dest = 0;
	while (src < fileSize)
	{
		if (buffer[src] != '\r')
			buffer[dest++] = buffer[src++];
		else
		{
			if (buffer[src+1] == '\n') //CR+LF
				src++;
			buffer[dest++] = '\n';
			if (src < fileSize) src++; //??? (You would think otherwise could not happen.)
		}
	}
	buffer[src] = '\0';
	{ //Free extra bytes (if any)
		unsigned newSize = fileSize + 1 - (src - dest);
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

	//Check first four bytes for disallowed characters.  Sorry, this is not a conformant
	//XML parser.
	bool parseable = true;
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

	//Do the real parse
	char* attribute;
	unsigned attribLen;
	char quoteType;
	char* value;
	unsigned valLen;

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
		bool rootClosed = false;
		i += 70;
		while (i < fileSize)
		{
			if (WHITESPACE(i))
				i++;
			else if (strncmp(fssRawData + i, "<entry", 6) == 0)
			{
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

				for (unsigned j = 0; j < 3; j++)
				{
					if (WHITESPACE(i))
						i++;
					else if (strncmp(fssRawData + i, "<statistics", 11) == 0)
					{
						i += 11;

						for (unsigned k = 0; k < 4; k++)
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

	//Fill groups
	for (i = 0; i < fssData.numEntries; i++)
	{
		if (fssData.entries[i].type == GROUP)
		{
			unsigned& curNumMembers = fssData.entries[i].numMembers;
			curNumMembers = 0; //??? (redundancy)
			fssData.entries[i].members = (FSSEntry**)malloc(sizeof(FSSEntry*) * 20);
			for (unsigned j = 0; j < fssData.numEntries; j++)
			{
				char* testBegin = fssData.entries[j].stats.keywords; //Keyword to test against
				unsigned testSize = 0;
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
	strcpy(newTextDate, textDate);

	//First split string up into ymdhms (see above)
	ymdhms[0] = &(newTextDate[0]);
	//Below, i is set to one in case there is a negative sign in front of the year. However,
	//years before the epoch are not handled by the conversion function below.
	for (unsigned i = 1, j = 1; textDate[i] != '\0'; i++)
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
	if (isModified == false)
		return false;
	unsigned numGroups = 0; //Used for the cache file
	//First write the XML file and
	//correct text pointers relative to the XML file
	FILE* fp = fopen(g_filename, "wt");
	if (fp == NULL)
		return false;
	fputs("<organizer version=\"1.1\" purpose=\"File system cataloging by metadata\">\n", fp);
	FSSData* curBlock = &fssData;
	while (curBlock != NULL)
	{
		unsigned lastPos;
		for (unsigned i = 0; i < curBlock->numEntries; i++)
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
		curBlock = curBlock->nextBlock;
	}
	fputs("</organizer>", fp);
	fclose(fp);

	//Then write the cache file
	unsigned filNamLen = strlen(g_filename);
	char* cacheName = (char*)alloca(filNamLen + 1);
	strcpy(cacheName, g_filename);
	cacheName[filNamLen - 3] = 'b';
	cacheName[filNamLen - 2] = 'i';
	cacheName[filNamLen - 1] = 'n';
	fp = fopen(cacheName, "wb");
	if (fp == NULL)
		return false;

	//Find the total number of entries and blocks
	unsigned totNumEntries = 0;
	unsigned numBlocks = 0;
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

UINT g_wheelLines; //Number of text lines scrolled by one wheel notch
extern HINSTANCE g_hInstance;

//Variables for text display, wrapping, selecting, editing, and caret positioning
int fontHeight;
bool fixedPitch = false;
float cWidths[256]; //This is an ASCII only program for now
KERNINGPAIR* kernPairs;
int numkPairs;
unsigned textPos = 0;
unsigned textSize = 0;
float caretX = 0, caretY = 0;
unsigned caretLine = 0;
char* buffer;
unsigned buffSize = 1000;
bool atMidBrkPos = false; //If the caret was moved to a new line when in the middle of a word
bool spaceProcessed = false; //If a space caused the cursor to move to a newline <- delete this
unsigned numLines = 0;
unsigned* lineStarts;
unsigned numTabs = 0;
unsigned* tabsPos;
float* lineLens;
int avgCharWidth;
unsigned textAreaWidth, textAreaHeight;
bool spaceSubseqWrap = false;
bool spaceUnwrap = false;
bool spaceBetween = false;
unsigned caretWidth;
unsigned* tabStops;
unsigned numTabStops = 0;
bool caretAtWrongPos = false; //If the user clicked to set the caret or the window was resized
unsigned lastSizeState = (unsigned)-1; //Contains window width before window was minimized
bool wasMinimized = false; //Size state before current state
bool regionActive = false;
unsigned regionBegin = 0;

void WrapWords();
void RewrapWords();
void UnwrapWords();

LRESULT CALLBACK FSwindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFont = NULL;
	static POINT scrdiff; //Amount to subtract to convert screen to client coordinates
	static HCURSOR hIBeam;
	//Variables for middle mouse button variable speed scrolling
	static bool drawDrag = false;
	static POINT dragP;
	static int dragDist;
	static bool bad = false;
	bool lastLineModified = false;
	static unsigned leadOffset = 0;
	static unsigned trailOffset = 0;
	static bool lBtnDown = false;
	static int xBordWidth, yBordWidth;
	switch (uMsg)
	{
	case WM_CREATE:
	{
		//Get the number of mouse wheel scroll lines
		SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &g_wheelLines, 0);

		xBordWidth = GetSystemMetrics(SM_CXBORDER);
		yBordWidth = GetSystemMetrics(SM_CYBORDER);


		//Update font metrics for system font
		//Get character widths
		//Get kerning pairs
		TEXTMETRIC tm;
		ABCFLOAT abcWidths[256];
		HDC hDC = GetDC(hwnd);
		//int nHeight;
		//int logPelsY = GetDeviceCaps(hDC, LOGPIXELSY);
		//nHeight = -MulDiv(24, logPelsY, 72);
		//hFont = CreateFont(nHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Edwardian Script ITC");
		SelectObject(hDC, hFont);
		GetTextMetrics(hDC, &tm);
		avgCharWidth = tm.tmAveCharWidth;
		GetCharWidthFloat(hDC, 0x00, 0xFF, cWidths);
		for (unsigned i = 0; i < 256; i++)
			cWidths[i] *= 16;
		numkPairs = GetKerningPairs(hDC, -1, NULL);
		if (numkPairs < 0)
			numkPairs = 0;
		kernPairs = (KERNINGPAIR*)malloc(sizeof(KERNINGPAIR) * numkPairs);
		GetKerningPairs(hDC, numkPairs, kernPairs);
		GetCharABCWidthsFloat(hDC, 0x00, 0xFF, abcWidths);
		for (unsigned i = 0; i < 256; i++)
		{
			if (abcWidths[i].abcfA < -(float)leadOffset)
				leadOffset = (unsigned)(-abcWidths[i].abcfA);
			if (abcWidths[i].abcfC < -(float)trailOffset)
				trailOffset = (unsigned)(-abcWidths[i].abcfC);
		}
		ReleaseDC(hwnd, hDC);
		fontHeight = tm.tmHeight;
		RECT rt;
		GetClientRect(hwnd, &rt);
		if (fontHeight / 16 > 1)
			caretWidth = fontHeight / 16;
		else
			caretWidth = xBordWidth;
		textAreaWidth = rt.right - leadOffset - trailOffset;
		textAreaHeight = rt.bottom;
		scrdiff.x = 0; scrdiff.y = 0;
		ClientToScreen(hwnd, &scrdiff);
		hIBeam = LoadCursor(NULL, IDC_IBEAM);
		//Generate tab stops
		tabStops = (unsigned*)malloc(sizeof(unsigned) * 20);
		hDC = GetDC(hwnd);
		SelectObject(hDC, hFont);
		unsigned tsWidth = LOWORD(GetTabbedTextExtent(hDC, "\t", 1, 0, NULL));
		ReleaseDC(hwnd, hDC);
		for (unsigned dist = 0; ; )
		{
			dist += (unsigned)tsWidth;
			if (dist >= textAreaWidth)
			{
				tabStops[numTabStops] = textAreaWidth - 1;
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
		buffer = (char*)malloc(buffSize);
		strcpy(buffer, "This is sample text. It remains here only to give you an example of how real text \
may look. Really, you should get to doing the right thing, because this is getting \
kind of boring. aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa                        aaaa");
		textSize = strlen(buffer);
		//Parse the text to find the line beginnings and tabs
		lineStarts = (unsigned*)malloc(sizeof(unsigned) * 100);
		lineLens = (float*)malloc(sizeof(float) * 100);
		tabsPos = (unsigned*)malloc(sizeof(unsigned) * 20);
		WrapWords();
		break;
	}
	case WM_DESTROY:
		//DeleteObject(hFont);
		free(kernPairs);
		free(buffer);
		free(lineStarts);
		free(lineLens);
		free(tabsPos);
		free(tabStops);
		break;
	case WM_PAINT:
	{
		//We have to re-get these because Windows 95 does not send a proper notification
		COLORREF regBkCol = GetSysColor(COLOR_HIGHLIGHT);
		COLORREF regTxtCol = GetSysColor(COLOR_HIGHLIGHTTEXT);
		COLORREF norTxtCol = GetSysColor(COLOR_WINDOWTEXT);

		//Drawing font:
		//Draw visible lines
		//Stop at tab characters to do spacing
		//Draw region on top if active
		PAINTSTRUCT ps;
		HDC hDC;
		SCROLLINFO si;
		hDC = BeginPaint(hwnd, &ps);
		HideCaret(hwnd);
		SelectObject(hDC, hFont);
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS | SIF_RANGE;
		GetScrollInfo(hwnd, SB_VERT, &si);
		SetWindowOrgEx(hDC, -((int)leadOffset) - xBordWidth, si.nPos - yBordWidth, NULL);
		unsigned curHeight = 0;
		unsigned curTab = 0;
		SetTextColor(hDC, norTxtCol);
		SetBkMode(hDC, TRANSPARENT);
		/*if (regionActive == false)
		{*/
			for (unsigned i = 0; i < numLines - 1; i++)
			{
				TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[i], lineStarts[i+1] - lineStarts[i], numTabStops, (int*)tabStops, 0);
				curHeight += fontHeight;
			}
			TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[numLines - 1], textSize - lineStarts[numLines - 1], numTabStops, (int*)tabStops, 0);
		//}
		SetBkMode(hDC, OPAQUE);
		if (regionActive == true)
		{
			unsigned p1, p2;
			HRGN reg1;
			HRGN reg2;
			RECT rt;
			unsigned begLine, endLine, lineEndRef;
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
			for (begLine = 0; begLine < numLines - 1; begLine++)
			{
				if (begLine < numLines - 1 && lineStarts[begLine+1] > p1)
					break;
			}
			for (endLine = 0; endLine < numLines - 1; endLine++)
			{
				if (endLine < numLines - 1 && lineStarts[endLine+1] >= p2)
					break;
			}
			curHeight = begLine * fontHeight;
			if (begLine == numLines - 1)
				lineEndRef = textSize;
			else
				lineEndRef = lineStarts[begLine+1];
			if (begLine != endLine)
			{
				//Create our clip region
				rt.left = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[begLine], p1 - lineStarts[begLine], numTabStops, (int*)tabStops));
				rt.top = curHeight + yBordWidth;
				rt.right = textAreaWidth + leadOffset + trailOffset;
				rt.bottom = rt.top + fontHeight;
				rt.left += leadOffset + xBordWidth;
				reg1 = CreateRectRgnIndirect(&rt);
				rt.left = leadOffset + xBordWidth;
				rt.top += fontHeight;
				rt.bottom = endLine * fontHeight;
				if (rt.bottom != rt.top)
				{
					rt.bottom = rt.top + fontHeight * (endLine - begLine - 1);
					reg2 = CreateRectRgnIndirect(&rt);
					CombineRgn(reg1, reg1, reg2, RGN_OR);
					DeleteObject(reg2);
				}
				rt.right = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[endLine], p2 - lineStarts[endLine], numTabStops, (int*)tabStops));
				rt.right += leadOffset + xBordWidth;
				rt.top = rt.bottom;
				rt.bottom += fontHeight;
				reg2 = CreateRectRgnIndirect(&rt);
				CombineRgn(reg1, reg1, reg2, RGN_OR);
				DeleteObject(reg2);

				//Draw all the selected lines
				HRGN oldReg = NULL;
				GetClipRgn(hDC, oldReg);
				SelectClipRgn(hDC, reg1);
				SetTextColor(hDC, regTxtCol);
				SetBkColor(hDC, regBkCol);
				for (unsigned i = begLine; i < endLine; i++)
				{
					if (buffer[lineStarts[i]] != '\n')
						TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[i], lineStarts[i+1] - lineStarts[i], numTabStops, (int*)tabStops, 0);
					curHeight += fontHeight;
				}
				if (endLine == numLines - 1)
					lineEndRef = textSize;
				else
					lineEndRef = lineStarts[endLine+1];
				TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[endLine], lineEndRef - lineStarts[endLine], numTabStops, (int*)tabStops, 0);
				SetTextColor(hDC, norTxtCol);
				SelectClipRgn(hDC, oldReg);
				DeleteObject(reg1);
			}
			else
			{
				rt.left = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[begLine], p1 - lineStarts[begLine], numTabStops, (int*)tabStops));
				rt.top = curHeight + yBordWidth;
				rt.right = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[begLine], p2 - lineStarts[begLine], numTabStops, (int*)tabStops));
				rt.bottom = rt.top + fontHeight;
				rt.left += leadOffset + xBordWidth;
				rt.right += leadOffset + xBordWidth;
				reg1 = CreateRectRgnIndirect(&rt);
				HRGN oldReg = NULL;
				GetClipRgn(hDC, oldReg);
				SelectClipRgn(hDC, reg1);
				SetTextColor(hDC, regTxtCol);
				SetBkColor(hDC, regBkCol);
				TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[begLine], lineEndRef - lineStarts[begLine], numTabStops, (int*)tabStops, 0);
				SetTextColor(hDC, norTxtCol);
				SelectClipRgn(hDC, oldReg);
				DeleteObject(reg1);
			}
		}
		/*else
		{
			unsigned i = 0;
			bool regionDrawn = false;
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
			while (i < numLines - 1)
			{
				if (i < numLines - 1 && lineStarts[i+1] >= p1)
					break;
				TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[i], lineStarts[i+1] - lineStarts[i], numTabStops, (int*)tabStops, 0);
				curHeight += fontHeight;
				i++;
			}
			if (i < numLines - 2)
			{
				unsigned lastExtent = 0;
				//Draw the text before the region
				if (p1 - lineStarts[i] != 0)
				{
					TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[i], p1 - lineStarts[i], numTabStops, (int*)tabStops, 0);
					lastExtent = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[i], p1 - lineStarts[i], numTabStops, (int*)tabStops));
				}
				//Draw the region
				SetTextColor(hDC, regTxtCol);
				SetBkColor(hDC, regBkCol);
				if (p2 >= lineStarts[i+1])
				{
					TabbedTextOut(hDC, lastExtent, curHeight, buffer + p1, lineStarts[i+1] - p1, numTabStops, (int*)tabStops, 0);
					curHeight += fontHeight;
					i++;
					while (i < numLines - 1)
					{
						if (i < numLines - 1 && lineStarts[i+1] >= p2)
							break;
						TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[i], lineStarts[i+1] - lineStarts[i], numTabStops, (int*)tabStops, 0);
						curHeight += fontHeight;
						i++;
					}
					TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[i], p2 - lineStarts[i], numTabStops, (int*)tabStops, 0);
					lastExtent = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[i], p2 - lineStarts[i], numTabStops, (int*)tabStops));
					//Draw the text at the end of the region
					SetTextColor(hDC, norTxtCol);
					SetBkColor(hDC, norBkCol);
					if (i < numLines - 1)
					{
						TabbedTextOut(hDC, lastExtent, curHeight, buffer + p2, lineStarts[i+1] - p2, numTabStops, (int*)tabStops, 0);
						curHeight += fontHeight;
						i++;
						if (i == numLines - 1) //Draw the last line here
							TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[numLines - 1], textSize - lineStarts[numLines - 1], numTabStops, (int*)tabStops, 0);
					}
					else
						TabbedTextOut(hDC, lastExtent, curHeight, buffer + p2, textSize - p2, numTabStops, (int*)tabStops, 0);
				}
				else
				{
					//The region is only on one line
					TabbedTextOut(hDC, lastExtent, curHeight, buffer + p1, p2 - p1, numTabStops, (int*)tabStops, 0);
					lastExtent = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[i], p2 - lineStarts[i], numTabStops, (int*)tabStops));
					//Draw the text at the end of the region
					SetTextColor(hDC, norTxtCol);
					SetBkColor(hDC, norBkCol);
					TabbedTextOut(hDC, lastExtent, curHeight, buffer + p2, lineStarts[i+1] - p2, numTabStops, (int*)tabStops, 0);
					curHeight += fontHeight;
					i++;
				}
				regionDrawn = true;
			}
			if (regionActive == true && regionDrawn == false)
			{
				//The region is drawn on the second to last or last line
				unsigned lastExtent = 0;
				//Draw the text before the region
				if (p1 - lineStarts[i] != 0)
				{
					TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[i], p1 - lineStarts[i], numTabStops, (int*)tabStops, 0);
					lastExtent = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[i], p1 - lineStarts[i], numTabStops, (int*)tabStops));
				}
				//Draw the region
				SetTextColor(hDC, regTxtCol);
				SetBkColor(hDC, regBkCol);
				if (i < numLines - 1 && p2 > lineStarts[i+1])
				{
					TabbedTextOut(hDC, lastExtent, curHeight, buffer + p1, lineStarts[i+1] - p1, numTabStops, (int*)tabStops, 0);
					curHeight += fontHeight;
					i++;
					lastExtent = 0;
					if (p2 - lineStarts[i] != 0)
					{
						TabbedTextOut(hDC, lastExtent, curHeight, buffer + lineStarts[i], p2 - lineStarts[i], numTabStops, (int*)tabStops, 0);
						lastExtent = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[i], p2 - lineStarts[i], numTabStops, (int*)tabStops));
					}
				}
				else
				{
					TabbedTextOut(hDC, lastExtent, curHeight, buffer + p1, p2 - p1, numTabStops, (int*)tabStops, 0);
					lastExtent = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[i], p2 - lineStarts[i], numTabStops, (int*)tabStops));
				}
				//Draw the text at the end of the region
				SetTextColor(hDC, norTxtCol);
				SetBkColor(hDC, norBkCol);
				if (i < numLines - 1)
				{
					TabbedTextOut(hDC, lastExtent, curHeight, buffer + p2, lineStarts[i+1] - p2, numTabStops, (int*)tabStops, 0);
					curHeight += fontHeight;
					i++;
					if (i == numLines - 1) //Draw the last line here
						TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[numLines - 1], textSize - lineStarts[numLines - 1], numTabStops, (int*)tabStops, 0);
				}
				else
					TabbedTextOut(hDC, lastExtent, curHeight, buffer + p2, textSize - p2, numTabStops, (int*)tabStops, 0);
				regionDrawn = true;
			}
			if (i < numLines - 1)
			{
				while (i < numLines - 1)
				{
					TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[i], lineStarts[i+1] - lineStarts[i], numTabStops, (int*)tabStops, 0);
					curHeight += fontHeight;
					i++;
				}
				TabbedTextOut(hDC, 0, curHeight, buffer + lineStarts[numLines - 1], textSize - lineStarts[numLines - 1], numTabStops, (int*)tabStops, 0);
			}
		}*/
		SetCaretPos((int)(caretX + leadOffset) + xBordWidth, (int)caretY + yBordWidth);
		ShowCaret(hwnd);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		lBtnDown = true;
		if (regionActive == true)
		{
			regionActive = false;
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
			RECT rt;
			HDC hDC = GetDC(hwnd);
			SelectObject(hDC, hFont);
			unsigned i;
			for (i = 0; i < numLines - 1; i++)
			{
				if (lineStarts[i+1] >= p1)
					break;
			}
			rt.top = i * fontHeight + yBordWidth;
			for (; i < numLines - 1; i++)
			{
				if (lineStarts[i+1] >= p2)
					break;
			}
			rt.bottom = (i + 1) * (fontHeight) + yBordWidth;
			if (rt.bottom - fontHeight == rt.top)
			{
				rt.left = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[i], p1 - lineStarts[i], numTabStops, (int*)tabStops));
				rt.right = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[i], p2 - lineStarts[i], numTabStops, (int*)tabStops));
				rt.left += leadOffset + xBordWidth;
				rt.right += leadOffset + xBordWidth;
			}
			else
			{
				rt.left = 0;
				rt.right = textAreaWidth + leadOffset + trailOffset;
			}
			ReleaseDC(hwnd, hDC);
			InvalidateRect(hwnd, &rt, TRUE);
		}
		SetCapture(hwnd);
		float oldX = caretX, oldY = caretY;
		int xPos = (short)LOWORD(lParam);
		int yPos = (short)HIWORD(lParam);
		xPos -= (leadOffset + xBordWidth);
		yPos -= yBordWidth;
		caretLine = yPos / fontHeight;
		if (yPos < 0)
			caretLine = 0;
		if (caretLine > numLines - 1)
			caretLine = numLines - 1;
		caretY = (float)caretLine * fontHeight;
		caretX = 0;
		unsigned lineEndRef;
		if (caretLine == numLines - 1)
			lineEndRef = textSize;
		else
			lineEndRef = lineStarts[caretLine+1];
		for (textPos = lineStarts[caretLine]; textPos < lineEndRef && caretX < (float)xPos; textPos++)
		{
			if (buffer[textPos] == '\n')
				break;
			caretX += cWidths[(unsigned char)buffer[textPos]];
		}
		if (textPos < lineEndRef && caretX != 0 && buffer[textPos] != '\n')
		{
			textPos--;
			caretX -= cWidths[(unsigned char)buffer[textPos]];
			float midChar = (float)xPos - caretX;
			if (midChar >= cWidths[(unsigned char)buffer[textPos]] / 2)
			{
				caretX += cWidths[(unsigned char)buffer[textPos]];
				textPos++;
			}
		}
		if (caretX >= textAreaWidth)
		{
			caretX = 0;
			caretY += fontHeight;
			caretLine++;
		}
		if (caretX != oldX || caretY != oldY)
			SetCaretPos((int)(caretX + leadOffset) + xBordWidth, (int)caretY + yBordWidth);
		regionBegin = textPos;
	}
	case WM_RBUTTONDOWN:
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
		if (drawDrag == true)
		{
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
#endif
		SetFocus(hwnd);
		break;
	case WM_CONTEXTMENU:
		{
			HMENU hMaster;
			HMENU hMen;
			POINT pt;
			hMaster = LoadMenu(g_hInstance, (LPCTSTR)SHRTCMENUS);
			hMen = GetSubMenu(hMaster, 0);
			pt.x = (short)LOWORD(lParam);
			pt.y = (short)HIWORD(lParam);
			if (pt.x == -1)
			{
				pt.x = (long)caretX; pt.y = (long)caretY;
				ClientToScreen(hwnd, &pt);
			}
			TrackPopupMenu(hMen, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, NULL, GetParent(hwnd), NULL);
			DestroyMenu(hMaster);
			break;
		}
	case WM_LBUTTONUP:
		ReleaseCapture();
		lBtnDown = false;
		break;
	case WM_MOVE:
		scrdiff.x = 0; scrdiff.y = 0;
		ClientToScreen(hwnd, &scrdiff);
		return 0;
	case WM_SETCURSOR:
		{
			RECT rt;
			rt.left = 0;
			rt.top = 0;
			rt.right = textAreaWidth + leadOffset + trailOffset;
			rt.bottom = textAreaHeight;
			POINT pt;
			GetCursorPos(&pt);
			pt.x -= scrdiff.x;
			pt.y -= scrdiff.y;
			if (rt.left <= pt.x && pt.x < rt.right &&
				rt.top <= pt.y && pt.y < rt.bottom)
			{
				SetCursor(hIBeam);
				return TRUE;
			}
		}
		break;
	case WM_MOUSEMOVE:
		if (lBtnDown == true)
	{
		float oldX = caretX, oldY = caretY;
		int xPos = (short)LOWORD(lParam);
		int yPos = (short)HIWORD(lParam);
		xPos -= (leadOffset + xBordWidth);
		yPos -= yBordWidth;
		caretLine = yPos / fontHeight;
		if (yPos < 0)
			caretLine = 0;
		if (caretLine > numLines - 1)
			caretLine = numLines - 1;
		caretY = (float)caretLine * fontHeight;
		caretX = 0;
		unsigned lineEndRef;
		if (caretLine == numLines - 1)
			lineEndRef = textSize;
		else
			lineEndRef = lineStarts[caretLine+1];
		for (textPos = lineStarts[caretLine]; textPos < lineEndRef && caretX < (float)xPos; textPos++)
		{
			if (buffer[textPos] == '\n')
				break;
			caretX += cWidths[(unsigned char)buffer[textPos]];
		}
		if (textPos < lineEndRef && caretX != 0 && buffer[textPos] != '\n')
		{
			textPos--;
			caretX -= cWidths[(unsigned char)buffer[textPos]];
			float midChar = (float)xPos - caretX;
			if (midChar >= cWidths[(unsigned char)buffer[textPos]] / 2)
			{
				caretX += cWidths[(unsigned char)buffer[textPos]];
				textPos++;
			}
		}
		if (caretX >= textAreaWidth)
		{
			caretX = 0;
			caretY += fontHeight;
			caretLine++;
		}
		if (caretX != oldX || caretY != oldY)
		{
			SetCaretPos((int)(caretX + leadOffset) + xBordWidth, (int)caretY + yBordWidth);
			if (regionBegin != textPos)
				regionActive = true;
			else
				regionActive = false;
			RECT rt;
			rt.left = (oldX < caretX) ? (int)oldX : (int)caretX;
			rt.top = (oldY < caretY) ? (int)oldY : (int)caretY;
			rt.right = (oldX > caretX) ? (int)oldX : (int)caretX;
			rt.bottom = (oldY > caretY) ? (int)oldY : (int)caretY;
			if (rt.top != rt.bottom)
			{
				rt.bottom += fontHeight + yBordWidth;
				rt.left = 0;
				rt.right = textAreaWidth + leadOffset + trailOffset;
				rt.top += yBordWidth;
			}
			else
			{
				rt.bottom += fontHeight + yBordWidth;
				rt.left += leadOffset + xBordWidth;
				rt.right += leadOffset + xBordWidth;
				rt.top += yBordWidth;
			}
			InvalidateRect(hwnd, &rt, TRUE);
		}
	}
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
		if (drawDrag == true)
			dragDist = HIWORD(lParam) - dragP.y;
		break;
	case WM_TIMER:
		if (wParam == 1)
		{
			SCROLLINFO si;
			int yPos;
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_POS;
			GetScrollInfo(hwnd, SB_VERT, &si);
			yPos = si.nPos;
			si.nPos += dragDist / 10;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
			GetScrollInfo(hwnd, SB_VERT, &si);
			if (si.nPos != yPos)
			{
				RECT rt;
				rt.left = dragP.x - 16;
				rt.top = dragP.y - 16;
				rt.right = dragP.x + 16;
				rt.bottom = dragP.y + 16;
				InvalidateRect(hwnd, &rt, TRUE);
				caretY += yPos - si.nPos;
				//RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
				RECT rtScroll;
				GetClientRect(hwnd, &rtScroll);
				rtScroll.left = xBordWidth;
				rtScroll.top = yBordWidth;
				//rtScroll.bottom -= yBordWidth; //With horizontal scroll right needs to be clipped too
				ScrollWindowEx(hwnd, 0, yPos - si.nPos, &rtScroll, &rtScroll, NULL, NULL, SW_INVALIDATE | SW_ERASE);
				UpdateWindow(hwnd);
				HDC hdc = GetDC(hwnd);
				Ellipse(hdc, rt.left, rt.top, rt.right, rt.bottom);
				ReleaseDC(hwnd, hdc);
			}
		}
		break;
#endif
	case WM_SETFONT:
	{
		TEXTMETRIC tm;
		ABCFLOAT abcWidths[256];
		RECT rt;
		hFont = (HFONT)wParam;
		HDC hDC = GetDC(hwnd);
		SelectObject(hDC, hFont);
		GetTextMetrics(hDC, &tm);
		avgCharWidth = tm.tmAveCharWidth;
		GetCharWidthFloat(hDC, 0x00, 0xFF, cWidths);
		for (unsigned i = 0; i < 256; i++)
			cWidths[i] *= 16;
		numkPairs = GetKerningPairs(hDC, -1, NULL);
		if (numkPairs < 0)
			numkPairs = 0;
		kernPairs = (KERNINGPAIR*)realloc(kernPairs, sizeof(KERNINGPAIR) * numkPairs);
		GetKerningPairs(hDC, numkPairs, kernPairs);
		GetCharABCWidthsFloat(hDC, 0x00, 0xFF, abcWidths);
		leadOffset = 0;
		trailOffset = 0;
		for (unsigned i = 0; i < 256; i++)
		{
			if (abcWidths[i].abcfA < -(float)leadOffset)
				leadOffset = (unsigned)(-abcWidths[i].abcfA);
			if (abcWidths[i].abcfC < -(float)trailOffset)
				trailOffset = (unsigned)(-abcWidths[i].abcfC);
		}
		GetClientRect(hwnd, &rt);
		textAreaWidth = rt.right - leadOffset - trailOffset;
		ReleaseDC(hwnd, hDC);
		fontHeight = tm.tmHeight;
		if (fontHeight / 16 > 1)
			caretWidth = fontHeight / 16;
		else
			caretWidth = xBordWidth;
		numLines = 0;
		numTabs = 0;
		numTabStops = 0;
		lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * 100);
		lineLens = (float*)realloc(lineLens, sizeof(float) * 100);
		tabsPos = (unsigned*)realloc(tabsPos, sizeof(unsigned) * 20);
		//Generate tab stops
		tabStops = (unsigned*)realloc(tabStops, sizeof(unsigned) * 20);
		hDC = GetDC(hwnd);
		SelectObject(hDC, hFont);
		unsigned tsWidth = LOWORD(GetTabbedTextExtent(hDC, "\t", 1, 0, NULL));
		ReleaseDC(hwnd, hDC);
		for (unsigned dist = 0; ; )
		{
			dist += (unsigned)tsWidth;
			if (dist >= textAreaWidth)
			{
				tabStops[numTabStops] = textAreaWidth - 1;
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
		DestroyCaret();
		CreateCaret(hwnd, NULL, caretWidth, fontHeight);
		ShowCaret(hwnd);
		//Wrap all words
		WrapWords();
		//Reset all flags
		atMidBrkPos = false;
		spaceBetween = false;
		caretAtWrongPos = false;
		//Find the caret position
		unsigned newCaretLine;
		caretX = 0;
		caretY = 0;
		for (newCaretLine = 0; newCaretLine < numLines; newCaretLine++)
		{
			if (textPos < lineStarts[newCaretLine])
				break;
		}
		newCaretLine--;
		caretLine = newCaretLine;
		hDC = GetDC(hwnd);
		SelectObject(hDC, hFont);
		caretX = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[caretLine], textPos - lineStarts[caretLine], numTabStops, (int*)tabStops));
		ReleaseDC(hwnd, hDC);
		caretY = (float)fontHeight * caretLine;
		//Reconfigure the flags
		if (caretLine > 0 && buffer[lineStarts[caretLine] - 1] == ' ' &&
			lineLens[caretLine-1] + caretX + cWidths[0x20] < textAreaWidth && buffer[textPos] != ' ')
		{
			unsigned breakPos = lineStarts[caretLine];
			unsigned lineEndRef;
			if (caretLine == numLines - 1)
				lineEndRef = textSize;
			else
				lineEndRef = lineStarts[caretLine+1];
			while (breakPos < lineEndRef && buffer[breakPos] != ' ')
				breakPos++;
			if (textPos < breakPos || (textPos == breakPos && breakPos == textSize))
			{
				atMidBrkPos = true;
				if (caretX == 0)
				{
					if (textPos >= 2 && buffer[textPos-1] == ' ' && buffer[textPos-2] == ' ')
					{
						spaceBetween = true;
						caretLine--;
					}
					caretAtWrongPos = true;
				}
			}
		}
		SetCaretPos((int)(caretX + leadOffset) + xBordWidth, (int)caretY + yBordWidth);
		if (lParam == TRUE)
			InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	}
	/*case WM_GETFONT:
		return (int)hFont;*/
	case WM_CUT:
		break;
	case WM_COPY:
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
		if ((HWND)lParam != hwnd)
			lBtnDown = false;
		break;
	case WM_SETFOCUS:
		CreateCaret(hwnd, NULL, fontHeight / 16, fontHeight);
		ShowCaret(hwnd);
		SetCaretPos((int)(caretX + leadOffset) + xBordWidth, (int)caretY + yBordWidth);
		break;
	case WM_KILLFOCUS:
		DestroyCaret();
		if (drawDrag == true)
		{
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
		break;
	case WM_SIZE:
	{
		if (LOWORD(lParam) == lastSizeState && wasMinimized == true)
		{
			wasMinimized = false;
			break;
		}
		else if (LOWORD(lParam) != 0)
		{
			wasMinimized = false;
			lastSizeState = LOWORD(lParam);
		}
		else
		{
			wasMinimized = true;
			break;
		}
		SCROLLINFO si;
		unsigned xLen;
		unsigned yLen;
		//Retrieve the dimensions of the client area
		xLen = LOWORD(lParam);
		yLen = HIWORD(lParam);
		//Set the vertical scrolling range and page size
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_RANGE | SIF_PAGE;
		si.nMin = 0;
		si.nMax = 1024;
		si.nPage = yLen;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		numLines = 0;
		numTabs = 0;
		numTabStops = 0;
		textAreaWidth = xLen - leadOffset - trailOffset;
		textAreaHeight = yLen;
		lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * 100);
		lineLens = (float*)realloc(lineLens, sizeof(float) * 100);
		tabsPos = (unsigned*)realloc(tabsPos, sizeof(unsigned) * 20);
		//Generate tab stops
		tabStops = (unsigned*)realloc(tabStops, sizeof(unsigned) * 20);
		HDC hDC = GetDC(hwnd);
		SelectObject(hDC, hFont);
		unsigned tsWidth = LOWORD(GetTabbedTextExtent(hDC, "\t", 1, 0, NULL));
		ReleaseDC(hwnd, hDC);
		for (unsigned dist = 0; ; )
		{
			dist += (unsigned)tsWidth;
			if (dist >= textAreaWidth)
			{
				tabStops[numTabStops] = textAreaWidth - 1;
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
		//Wrap all words
		WrapWords();
		//Reset all flags
		atMidBrkPos = false;
		spaceBetween = false;
		caretAtWrongPos = false;
		//Find the caret position
		unsigned newCaretLine;
		caretX = 0;
		caretY = 0;
		for (newCaretLine = 0; newCaretLine < numLines; newCaretLine++)
		{
			if (textPos < lineStarts[newCaretLine])
				break;
		}
		newCaretLine--;
		caretLine = newCaretLine;
		hDC = GetDC(hwnd);
		SelectObject(hDC, hFont);
		caretX = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[caretLine], textPos - lineStarts[caretLine], numTabStops, (int*)tabStops));
		ReleaseDC(hwnd, hDC);
		caretY = (float)fontHeight * caretLine;
		//Reconfigure the flags
		if (caretLine > 0 && buffer[lineStarts[caretLine] - 1] == ' ' &&
			lineLens[caretLine-1] + caretX + cWidths[0x20] < textAreaWidth && buffer[textPos] != ' ')
		{
			unsigned breakPos = lineStarts[caretLine];
			unsigned lineEndRef;
			if (caretLine == numLines - 1)
				lineEndRef = textSize;
			else
				lineEndRef = lineStarts[caretLine+1];
			while (breakPos < lineEndRef && buffer[breakPos] != ' ')
				breakPos++;
			if (textPos < breakPos || (textPos == breakPos && breakPos == textSize))
			{
				atMidBrkPos = true;
				if (caretX == 0)
				{
					if (textPos >= 2 && buffer[textPos-1] == ' ' && buffer[textPos-2] == ' ')
					{
						spaceBetween = true;
						caretLine--;
					}
					caretAtWrongPos = true;
				}
			}
		}
		break;
	}
	case WM_VSCROLL:
	{
		SCROLLINFO si;
		//Get all the vertial scroll bar information
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		GetScrollInfo(hwnd, SB_VERT, &si);
		//Save the position for comparison later on
		int yPos;
		yPos = si.nPos;
		switch (LOWORD(wParam))
		{
		//user clicked the HOME keyboard key
		case SB_TOP:
			si.nPos = si.nMin;
			break;
		//user clicked the END keyboard key
		case SB_BOTTOM:
			si.nPos = si.nMax;
			break;
		//user clicked the top arrow
		case SB_LINEUP:
			si.nPos -= fontHeight;
			break;
		//user clicked the bottom arrow
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
		default:
			break;
		}
		//Set the position and then retrieve it. Due to adjustments
		//by Windows it may not be the same as the value set.
		si.fMask = SIF_POS;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		GetScrollInfo(hwnd, SB_VERT, &si);
		//If the position has changed, scroll window and update it
		if (si.nPos != yPos)
		{
			caretY += yPos - si.nPos;
			RECT rt;
			GetClientRect(hwnd, &rt);
			rt.left = xBordWidth;
			rt.top = yBordWidth;
			//rt.bottom -= yBordWidth; //With horizontal scroll right needs to be clipped too
			ScrollWindowEx(hwnd, 0, yPos - si.nPos, &rt, &rt, NULL, NULL, SW_INVALIDATE | SW_ERASE);
		}
		break;
	}
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_INSERT:
			break;
		case VK_DELETE:
			break;
		case VK_HOME:
			if (GetKeyState(VK_CONTROL) & 0x8000)
				SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_TOP, NULL), NULL);
			break;
		case VK_END:
			if (GetKeyState(VK_CONTROL) & 0x8000)
				SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, NULL), NULL);
			break;
		case VK_PRIOR:
			SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, NULL), NULL);
			break;
		case VK_NEXT:
			SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, NULL), NULL);
			break;
		case VK_UP:
			if (GetKeyState(VK_CONTROL) & 0x8000)
				SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, NULL), NULL);
			break;
		case VK_DOWN:
			if (GetKeyState(VK_CONTROL) & 0x8000)
				SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, NULL), NULL);
			break;
		case VK_LEFT:
			{
				if (textPos == 0)
					break;
				textPos--;
				caretX -= cWidths[(unsigned char)buffer[textPos]];
				if (caretX < 0)
				{
					caretLine--;
					caretY -= fontHeight;
					if (buffer[textPos] == ' ')
						caretX = lineLens[caretLine];
					else
						caretX += lineLens[caretLine];
				}
				SetCaretPos((int)(caretX + leadOffset) + xBordWidth, (int)caretY + yBordWidth);
				break;
			}
		case VK_RIGHT:
			{
				if (textPos == textSize)
					break;
				caretX += cWidths[(unsigned char)buffer[textPos]];
				textPos++;
				if (textPos == lineStarts[caretLine+1] || caretX >= textAreaWidth || buffer[textPos-1] == '\n')
				{
					caretX = 0;
					caretY += fontHeight;
					caretLine++;
				}
				else if (textPos - 1 == lineStarts[caretLine+1])
				{
					caretX = cWidths[(unsigned char)buffer[textPos-1]];
					caretY += fontHeight;
					caretLine++;
				}
				SetCaretPos((int)(caretX + leadOffset) + xBordWidth, (int)caretY + yBordWidth);
				break;
			}
			//CTRL+T
			//Shift selections
			//CTRL motion// broken by " !%()-?[]{}"
		}
		break;
	case WM_KEYUP:
		break;
	/*case WM_SETCURSOR:
		SetCursor(LoadCursor(NULL, IDC_IBEAM));
		return (LRESULT)TRUE;*/
	case WM_CHAR:
		if (wParam < 0x20 && wParam != 0x08 && wParam != 0x09 && wParam != 0x0D)
			break; //We don't insert control characters...
		if (caretAtWrongPos == true)
		{
			caretAtWrongPos = false;
			caretX = lineLens[caretLine-1] + cWidths[0x20];
			caretY -= fontHeight;
		}
		switch (wParam)
		{
		case 0x08: //backspace
		{
			if (textPos == 0)
				break;
			RECT rt;
			spaceUnwrap = true;
			if (textPos == textSize || buffer[textPos] == '\n')
				spaceUnwrap = false;
			bool brokenWord = false;
			//Update the flags
			float caretXTest = caretX - cWidths[(unsigned char)buffer[textPos-1]];
			if (caretXTest < 0.0f)
				caretXTest = 0;
			if (atMidBrkPos == false &&
				caretLine > 0 && buffer[lineStarts[caretLine] - 1] == ' ' &&
				lineLens[caretLine-1] + caretXTest + cWidths[0x20] < textAreaWidth && buffer[textPos] != ' ')
			{
				unsigned breakPos = lineStarts[caretLine];
				unsigned lineEndRef;
				if (caretLine == numLines - 1)
					lineEndRef = textSize;
				else
					lineEndRef = lineStarts[caretLine+1];
				while (breakPos < lineEndRef && buffer[breakPos] != ' ' && buffer[breakPos] != '\n')
					breakPos++;
				if (textPos < breakPos || (textPos == breakPos && breakPos == textSize))
				{
					atMidBrkPos = true;
					if (textPos >= 3 && lineStarts[caretLine] == textPos - 1 &&
						buffer[textPos-2] == ' ' && buffer[textPos-3] == ' ')
					{
						spaceBetween = true;
						lineLens[caretLine] -= cWidths[(unsigned char)buffer[textPos-1]];
						lineStarts[caretLine]++;
						//Check for line ending with a newline character
						if (buffer[lineEndRef-1] == '\n')
						{
							unsigned breakPos = textPos;
							while (breakPos < lineEndRef - 1 && buffer[breakPos] != ' ')
								breakPos++;
							if (buffer[breakPos] != ' ')
							{
								//Delete this line
								memmove(&(lineStarts[caretLine]), &(lineStarts[caretLine+1]), sizeof(unsigned) * (numLines - (caretLine + 1)));
								memmove(&(lineLens[caretLine]), &(lineLens[caretLine+1]), sizeof(float) * (numLines - (caretLine + 1)));
								numLines--;
							}
						}
						caretLine--;
						caretX = lineLens[caretLine] + cWidths[0x20];
						caretY -= fontHeight;
						brokenWord = true;
						goto backspRemvChar;
					}
				}
			}
			//Subtract last character width and update caret
			if (atMidBrkPos == true && textPos != lineStarts[caretLine])
			{
				//Find the length of the word at the caret (if there is one)
				unsigned breakPos = lineStarts[caretLine];
				float carryAmnt = 0;
				unsigned lineEndRef;
				if (caretLine == numLines - 1)
					lineEndRef = textSize;
				else
					lineEndRef = lineStarts[caretLine+1];
				while (breakPos != lineEndRef && buffer[breakPos] != ' ' && buffer[breakPos] != '\n')
				{
					if (breakPos == textPos - 1)
					{
						breakPos++;
						continue;
					}
					carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
					breakPos++;
				}
				if (buffer[lineStarts[caretLine]-1] == ' ' && breakPos != lineStarts[caretLine])
				{
					if (lineLens[caretLine-1] + carryAmnt + cWidths[0x20] < textAreaWidth)
					{
						//Move the word backward
						caretX = lineLens[caretLine-1];
						for (unsigned i = lineStarts[caretLine]-1; i < textPos - 1; i++)
							caretX += cWidths[(unsigned char)buffer[i]];
						lineLens[caretLine-1] += carryAmnt + cWidths[0x20];
						lineStarts[caretLine] = breakPos;
						lineLens[caretLine] -= (carryAmnt + cWidths[(unsigned char)buffer[textPos-1]]);
						if (breakPos != lineEndRef)
						{
							lineStarts[caretLine]++;
							lineLens[caretLine] -= cWidths[0x20];
						}
						if (caretLine == numLines - 1)
							numLines--;
						else
						{
							//Check for line ending with a newline character
							if (buffer[lineEndRef-1] == '\n')
							{
								unsigned breakPos = textPos;
								while (breakPos < lineEndRef - 1 && buffer[breakPos] != ' ')
									breakPos++;
								if (buffer[breakPos] != ' ')
								{
									//Delete this line
									memmove(&(lineStarts[caretLine]), &(lineStarts[caretLine+1]), sizeof(unsigned) * (numLines - (caretLine + 1)));
									memmove(&(lineLens[caretLine]), &(lineLens[caretLine+1]), sizeof(float) * (numLines - (caretLine + 1)));
									numLines--;
								}
							}
						}
						caretLine--;
						caretY -= fontHeight;
						atMidBrkPos = false;
						goto backspRemvChar;
					}
				}
			}
			if (caretLine != 0 && textPos - 1 == lineStarts[caretLine] &&
				buffer[lineStarts[caretLine]-1] != ' ' && buffer[lineStarts[caretLine]-1] != '\t' && buffer[lineStarts[caretLine]-1] != '\n')
			{
				if (buffer[textPos] == ' ')
				{
					//Make last line end with a space
					lineStarts[caretLine] += 2;
					lineLens[caretLine] -= cWidths[0x20];
				}
				else
					lineStarts[caretLine]++;
				//Subtract character on this line
				lineLens[caretLine] -= cWidths[(unsigned char)buffer[textPos-1]];
				//Check for line ending with a newline character
				unsigned lineEndRef;
				if (caretLine == numLines - 1)
					lineEndRef = textPos - 1;
				else
					lineEndRef = lineStarts[caretLine+1] - 1;
				if (buffer[lineEndRef] == '\n' && (buffer[textPos] != ' ' && lineEndRef == textPos))
				{
					//Delete this line
					memmove(&(lineStarts[caretLine]), &(lineStarts[caretLine+1]), sizeof(unsigned) * (numLines - (caretLine + 1)));
					memmove(&(lineLens[caretLine]), &(lineLens[caretLine+1]), sizeof(float) * (numLines - (caretLine + 1)));
					numLines--;
				}
				//Position caret
				caretLine--;
				caretY -= fontHeight;
				caretX = lineLens[caretLine];
			}
			else if (caretLine != 0 && buffer[lineStarts[caretLine]-1] != '\n' && textPos - 1 == lineStarts[caretLine] && buffer[textPos] == ' ')
			{
				//See if you can move this space back to the previous line
				if (lineLens[caretLine-1] + cWidths[0x20] < textAreaWidth)
				{
					lineLens[caretLine] -= cWidths[(unsigned char)buffer[textPos-1]];
					lineLens[caretLine-1] += cWidths[0x20];
					lineLens[caretLine] -= cWidths[0x20];
					lineStarts[caretLine]++;
					//Check for line ending with a newline character
					unsigned lineEndRef;
					if (caretLine == numLines - 1)
						lineEndRef = textPos - 1;
					else
						lineEndRef = lineStarts[caretLine+1] - 1;
					if (buffer[lineEndRef] == '\n' && lineEndRef - 1 == textPos)
					{
						//Delete this line
						memmove(&(lineStarts[caretLine]), &(lineStarts[caretLine+1]), sizeof(unsigned) * (numLines - (caretLine + 1)));
						memmove(&(lineLens[caretLine]), &(lineLens[caretLine+1]), sizeof(float) * (numLines - (caretLine + 1)));
						numLines--;
					}
					caretLine--;
					caretY -= fontHeight;
					caretX = lineLens[caretLine];
				}
			}
			else if (textPos == lineStarts[caretLine])
			{
				if (buffer[textPos-1] == '\t')
				{
					//Delete this tab
					for (unsigned i = 0; i < numTabs; i++)
					{
						if (tabsPos[i] == textPos - 1)
						{
							if (i != 0)
								memmove(&(tabsPos[i]), &(tabsPos[i+1]), sizeof(unsigned) * (numTabs - 1 - i));
							numTabs--;
							if (numTabs % 20 == 19)
								tabsPos = (unsigned*)realloc(tabsPos, (numTabs + 1) * sizeof(unsigned));
							break;
						}
					}
					//Check for line ending with a newline character
					unsigned lineEndRef;
					if (caretLine == numLines - 1)
						lineEndRef = textPos - 1;
					else
						lineEndRef = lineStarts[caretLine+1] - 1;
					if (buffer[lineEndRef] == '\n')
					{
						unsigned breakPos = textPos;
						while (breakPos < lineEndRef && buffer[breakPos] != ' ')
							breakPos++;
						if (buffer[breakPos] != ' ')
						{
							//Delete this line
							memmove(&(lineStarts[caretLine]), &(lineStarts[caretLine+1]), sizeof(unsigned) * (numLines - (caretLine + 1)));
							memmove(&(lineLens[caretLine]), &(lineLens[caretLine+1]), sizeof(float) * (numLines - (caretLine + 1)));
							numLines--;
						}
					}
					lineLens[caretLine-1] = (float)tabStops[numTabStops-2];
					caretX = lineLens[caretLine-1];
					caretY -= fontHeight;
					caretLine--;
					goto backspRemvChar;
				}
				if (lineLens[caretLine] == 0)
				{
					//Delete the empty line
					if (caretLine < numLines - 1)
						memmove(&(lineStarts[caretLine]), &(lineStarts[caretLine+1]), sizeof(unsigned) * (numLines - caretLine + 1));
						memmove(&(lineLens[caretLine]), &(lineLens[caretLine+1]), sizeof(float) * (numLines - caretLine + 1));
					numLines--;
					caretLine--;
					caretX = lineLens[caretLine];
					caretY -= fontHeight;
					goto backspRemvChar;
				}
				else if (lineLens[caretLine-1] == 0) //blank line
				{
					//Just go back, then the unwrapping code will delete the blank line
					caretLine--;
					caretY -= fontHeight;
					goto backspRemvChar;
				}
				if (caretLine == numLines - 1 && atMidBrkPos == false)
				{
					caretLine--;
					caretY -= fontHeight;
					if (buffer[textPos-1] != ' ')
						lineLens[caretLine] -= cWidths[(unsigned char)buffer[textPos-1]];
					caretX = lineLens[caretLine];
					numLines--;
					goto backspRemvChar;
				}
				//Find the length of the word at the caret (if there is one)
				unsigned breakPos = lineStarts[caretLine];
				float carryAmnt = 0;
				unsigned lineEndRef;
				if (caretLine == numLines - 1)
					lineEndRef = textSize;
				else
					lineEndRef = lineStarts[caretLine+1];
				while (breakPos != lineEndRef && buffer[breakPos] != ' ' && buffer[breakPos] != '\n')
				{
					carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
					breakPos++;
				}
				if (breakPos != lineEndRef && (buffer[breakPos] == ' ' || buffer[breakPos] == '\n') && buffer[lineStarts[caretLine]-1] == ' ' ||
					buffer[lineStarts[caretLine]-1] == '\n' && breakPos != lineStarts[caretLine])
				{
					//Find the length of the word before the caret (if there is one)
					//Don't count the character that will be deleted
					float carryAmnt2 = 0;
					unsigned breakPos2 = lineStarts[caretLine] - 1;
					while (breakPos2 != lineStarts[caretLine-1] && buffer[breakPos2-1] != ' ')
					{
						breakPos2--;
						carryAmnt2 += cWidths[(unsigned char)buffer[breakPos2]];
					}
					if (breakPos2 != lineStarts[caretLine-1] && buffer[breakPos2-1] == ' ' && breakPos != lineStarts[caretLine] &&
						breakPos2 != lineStarts[caretLine] - 1)
					{
						if (lineLens[caretLine-1] + carryAmnt < textAreaWidth)
						{
							//Move the word backward
							caretX = lineLens[caretLine-1];
							lineLens[caretLine-1] += carryAmnt;
							lineStarts[caretLine] = breakPos;
							lineLens[caretLine] -= carryAmnt;
							if (breakPos != lineEndRef)
							{
								lineStarts[caretLine]++;
								lineLens[caretLine] -= cWidths[0x20];
							}
							//Check for line ending with a newline character
							if (buffer[lineEndRef-1] == '\n')
							{
								unsigned breakPos = textPos;
								while (breakPos < lineEndRef - 1 && buffer[breakPos] != ' ')
									breakPos++;
								if (buffer[breakPos] != ' ')
								{
									//Delete this line
									memmove(&(lineStarts[caretLine]), &(lineStarts[caretLine+1]), sizeof(unsigned) * (numLines - (caretLine + 1)));
									memmove(&(lineLens[caretLine]), &(lineLens[caretLine+1]), sizeof(float) * (numLines - (caretLine + 1)));
									numLines--;
								}
							}
							if (caretLine * fontHeight == caretY)
								caretY -= fontHeight; //Undo the snapping position
							caretLine--;
							atMidBrkPos = false;
						}
						else if (carryAmnt2 != lineLens[caretLine-1])
						{
							//Move the word forward
							lineLens[caretLine-1] -= (carryAmnt2 + cWidths[0x20]);
							lineStarts[caretLine] = breakPos2;
							lineLens[caretLine] += carryAmnt2;
							if (caretLine * fontHeight > caretY)
								caretY += fontHeight; //Undo the snapping position
							caretX = carryAmnt2;
							spaceUnwrap = false;
							spaceSubseqWrap = true;
							lastLineModified = true;
						}
						else
						{
							//Break this word in between
							//Deleting a space at the end of a line does not change length
							caretY -= fontHeight;
							caretX = carryAmnt2;
							caretLine--;
							atMidBrkPos = true;
							brokenWord = true;
							lastLineModified = true;
						}
					}
					else //Check for newline problems
					{
						//We had two spaces in a row or a line that was a long word with spaces only at the end
						if (caretLine * fontHeight < caretY)
							caretY -= fontHeight; //Undo the snapping position
						caretX = lineLens[caretLine-1];
						if (buffer[textPos-1] == ' ' && buffer[textPos-2] == ' ')
						{
							lineLens[caretLine-1] -= cWidths[(unsigned char)buffer[textPos-1]];
							atMidBrkPos = true;
							if (textPos > 3 && buffer[textPos-2] == ' ' && buffer[textPos-3] == ' ')
							{
								spaceBetween = true;
								caretLine--;
							}
							else
								lineStarts[caretLine]--;
						}
						else
						{
							if (buffer[textPos-1] == '\n')
								caretY -= fontHeight;
							caretLine--;
						}
					}
				}
				else if (buffer[textPos] == ' ' && buffer[textPos-1] == ' ')
				{
					//Delete last space and move this space back in place
					lineLens[caretLine] -= cWidths[(unsigned char)buffer[textPos-1]];
					caretX = lineLens[caretLine-1];
					lineLens[caretLine] -= cWidths[0x20];
					lineStarts[caretLine]++;
					//Check for line ending with a newline character
					if (buffer[lineEndRef-1] == '\n' && lineEndRef - 2 == textPos)
					{
						//Delete this line
						memmove(&(lineStarts[caretLine]), &(lineStarts[caretLine+1]), sizeof(unsigned) * (numLines - (caretLine + 1)));
						memmove(&(lineLens[caretLine]), &(lineLens[caretLine+1]), sizeof(float) * (numLines - (caretLine + 1)));
						numLines--;
					}
					caretLine--;
					caretY -= fontHeight;
				}
				else
				{
					if (buffer[textPos-1] != ' ')
					{
						lineLens[caretLine-1] -= cWidths[(unsigned char)buffer[textPos-1]];
						caretX = lineLens[caretLine-1];
						lastLineModified = true;
						//Check for line ending with a newline character
						if (buffer[lineEndRef-1] == '\n')
						{
							if (lineEndRef - 1 == textPos)
							{
								//Delete this line
								memmove(&(lineStarts[caretLine]), &(lineStarts[caretLine+1]), sizeof(unsigned) * (numLines - (caretLine + 1)));
								memmove(&(lineLens[caretLine]), &(lineLens[caretLine+1]), sizeof(float) * (numLines - (caretLine + 1)));
								numLines--;
							}
						}
						caretLine--;
						if (caretLine * fontHeight < caretY)
							caretY -= fontHeight; //Undo the snapping position
					}
					else
					{
						//Find the length of the word before the caret
						float carryAmnt2 = 0;
						unsigned breakPos2 = lineStarts[caretLine] - 1;
						while (breakPos2 != lineStarts[caretLine-1] && buffer[breakPos2-1] != ' ')
						{
							breakPos2--;
							carryAmnt2 += cWidths[(unsigned char)buffer[breakPos2]];
						}
						//Move the word forward
						caretX = carryAmnt2;
						lineLens[caretLine-1] -= (carryAmnt2 + cWidths[0x20]);
						lineLens[caretLine] += carryAmnt2;
						lineStarts[caretLine] = breakPos2;
						if (caretLine * fontHeight > caretY)
							caretY += fontHeight; //Undo the snapping position
						lastLineModified = true;
						if (breakPos2 == lineStarts[caretLine-1])
						{
							//Delete the previous line
							memmove(&(lineStarts[caretLine-1]), &(lineStarts[caretLine]), sizeof(unsigned) * (numLines - caretLine));
							memmove(&(lineLens[caretLine-1]), &(lineLens[caretLine]), sizeof(float) * (numLines - caretLine));
							numLines--;
							caretLine--;
							caretY -= fontHeight;
						}
						spaceUnwrap = false;
						spaceSubseqWrap = true;
					}
				}
			}
			else if (buffer[textPos-1] == '\t')
			{
				//Delete this tab
				for (unsigned i = 0; i < numTabs; i++)
				{
					if (tabsPos[i] == textPos - 1)
					{
						if (i != 0)
							memmove(&(tabsPos[i]), &(tabsPos[i+1]), sizeof(unsigned) * (numTabs - 1 - i));
						numTabs--;
						if (numTabs % 20 == 19)
							tabsPos = (unsigned*)realloc(tabsPos, (numTabs + 1) * sizeof(unsigned));
						break;
					}
				}
				//Find which tab stop we are at
				unsigned curTabStop;
				for (curTabStop = numTabStops - 1; curTabStop != 0 && caretX < tabStops[curTabStop]; curTabStop--);
				//Find the text length before this tab stop
				unsigned lineLen;
				lineLen = textPos - 1 - lineStarts[caretLine];
				HDC hDC = GetDC(hwnd);
				SelectObject(hDC, hFont);
				unsigned leadTextLen = LOWORD(GetTabbedTextExtent(hDC, buffer + lineStarts[caretLine], lineLen, numTabStops, (int*)tabStops));
				ReleaseDC(hwnd, hDC);
				//Store the difference
				float tabSubAmnt = (float)tabStops[curTabStop] - (float)leadTextLen;
				lineLens[caretLine] -= tabSubAmnt;
				caretX -= tabSubAmnt;
			}
			else
			{
				lineLens[caretLine] -= cWidths[(unsigned char)buffer[textPos-1]];
				caretX -= cWidths[(unsigned char)buffer[textPos-1]];
			}
			//If in a kerning pair...
			//Forget kerning pairs for now because they don't affect my sreen DC

			//Delete the character
backspRemvChar:
			memmove(buffer + textPos - 1, buffer + textPos, textSize - textPos + 1);
			textSize--;
			textPos--;
			//Correct all line beginning indices
			for (unsigned i = caretLine + 1; i < numLines; i++)
				lineStarts[i]--;
			//Check flag states
			if (atMidBrkPos == true)
			{
				if (spaceBetween == true && textPos < 2)
					spaceBetween = false;
				else if (spaceBetween == true && textPos >= 2 && (buffer[textPos-1] != ' ' || buffer[textPos-2] != ' '))
				{
					spaceBetween = false;
					caretLine++;
				}
				//Snapping position correction is done earlier
			}
			//Perform proper wrapping correction
			if (spaceUnwrap == true)
			{
				spaceUnwrap = false;
				if (brokenWord == true)
					caretLine++;
				UnwrapWords();
				if (brokenWord == true)
					caretLine--;
			}
			if (spaceSubseqWrap == true)
			{
				spaceSubseqWrap = false;
				RewrapWords();
			}
			//Update the window
			GetClientRect(hwnd, &rt);
			rt.top = (int)caretY;
			if (lastLineModified == true)
			{
				lastLineModified = false;
				rt.top -= fontHeight;
			}
			InvalidateRect(hwnd, &rt, TRUE);
			break;
		}
		case 0x7F: //CTRL+backspace
			break;
		case 0x09: //tab
		{
			RECT rt;
			//Add the character
			textSize++;
			if (textSize == buffSize)
			{
				buffSize += 1000;
				buffer = (char*)realloc(buffer, buffSize);
			}
			if (textPos != (textSize - 1))
				memmove(buffer + textPos + 1, buffer + textPos, textSize - textPos);
			else
				buffer[textPos+1] = '\0';
			buffer[textPos] = '\t';
			textPos++;
			spaceSubseqWrap = true;
			if (caretX == (float)tabStops[numTabStops-1])
			{
				lineLens[caretLine] = caretX;
				if (textPos == textSize || buffer[textPos] == '\n')
				{
					spaceSubseqWrap = false;
					lineLens[caretLine] -= cWidths[(unsigned char)buffer[textPos-1]];
					//Add a line
					numLines++;
					if (numLines % 100 == 0)
					{
						lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 100));
						lineLens = (float*)realloc(lineLens, sizeof(float) * (numLines + 100));
					}
					//Set up this line
					caretLine++;
					if (buffer[textPos] == '\n')
					{
						memmove(&(lineStarts[caretLine+1]), &(lineStarts[caretLine]), sizeof(unsigned) * (numLines - caretLine));
						memmove(&(lineLens[caretLine+1]), &(lineLens[caretLine]), sizeof(float) * (numLines - caretLine));
					}
					lineStarts[caretLine] = textPos - 1;
					lineLens[caretLine] = 0;
					caretX = 0;
					caretY += fontHeight;
				}
				else
				{
					caretY += fontHeight;
					caretLine++;
					caretX = 0;
				}
			}
			//Correct all line beginning indices
			for (unsigned i = caretLine + 1; i < numLines; i++)
				lineStarts[i]++;
			//Mark the tab position
			numTabs++;
			if (numTabs % 20 == 0)
				tabsPos = (unsigned*)realloc(tabsPos, sizeof(unsigned) * (numTabs + 20));
			tabsPos[numTabs-1] = textPos - 1;
			//Add last character width
			unsigned curTabStop;
			for (curTabStop = 0; curTabStop < numTabStops && caretX >= tabStops[curTabStop]; curTabStop++);
			lineLens[caretLine] += (float)tabStops[curTabStop] - caretX;

			if (spaceSubseqWrap == true)
			{
				spaceSubseqWrap = false;
				RewrapWords();
			}

			//Update the caret
			for (curTabStop = 0; curTabStop < numTabStops && caretX >= tabStops[curTabStop]; curTabStop++);
			caretX = (float)tabStops[curTabStop];
			//Set the update rectangle
			GetClientRect(hwnd, &rt);
			rt.top = (int)caretY;
			//Update the window
			InvalidateRect(hwnd, &rt, TRUE);
			break;
		}
		case 0x0D: //carriage-return
		{
			RECT rt;
			//Flush any previously set flags
			bool prevFlags = false;
			if (atMidBrkPos == true)
			{
				atMidBrkPos = false;
				prevFlags = true;
				if (buffer[textPos-1] == ' ') //Only go on if a word was typed
				{
					if (spaceBetween == false)
					{
						spaceBetween = true;
						caretLine--;
					}
				}
				else if ((lineLens[caretLine-1] + caretX + cWidths[0x20]) < textAreaWidth)
				{
					//Unwrap the last word
					lineLens[caretLine] -= caretX;
					lineLens[caretLine-1] += caretX;
					lineStarts[caretLine] = textPos + 1;
					caretX = lineLens[caretLine-1];
					caretY -= fontHeight;
					//Perform subsequent unwrapping
					UnwrapWords();
					caretLine--;
				}
				else
					prevFlags = false;
			}
			if (spaceBetween == true)
			{
				spaceBetween = false;
				prevFlags = true;
			}
			//Add the character
			textSize++;
			if (textSize == buffSize)
			{
				buffSize += 1000;
				buffer = (char*)realloc(buffer, buffSize);
			}
			if (textPos != (textSize - 1))
				memmove(buffer + textPos + 1, buffer + textPos, textSize - textPos);
			else
				buffer[textPos+1] = '\0';
			buffer[textPos] = '\n';
			textPos++;
			//Update the line info
			if (prevFlags == false)
			{
				numLines++;
				if (numLines % 100 == 0)
				{
					lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 100));
					lineLens = (float*)realloc(lineLens, sizeof(float) * (numLines + 100));
				}
				memmove(&(lineStarts[caretLine+1]), &(lineStarts[caretLine]), sizeof(unsigned) * (numLines - (caretLine + 1)));
				memmove(&(lineLens[caretLine+1]), &(lineLens[caretLine]), sizeof(float) * (numLines - (caretLine + 1)));
				lineLens[caretLine] = caretX;
				caretLine++;
				lineLens[caretLine] -= caretX;
				lineStarts[caretLine] = textPos;
				if (caretX != 0)
					spaceUnwrap = true;
			}
			else
				caretLine++;
			//Correct all line beginning indices
			for (unsigned i = caretLine + 1; i < numLines; i++)
				lineStarts[i]++;
			//Unwrap words
			if (spaceUnwrap == true)
			{
				spaceUnwrap = false;
				UnwrapWords();
			}
			//Set the update rectangle
			GetClientRect(hwnd, &rt);
			rt.top = (int)caretY;
			//Update the caret
			caretX = 0;
			caretY += fontHeight;
			//Update the window
			InvalidateRect(hwnd, &rt, TRUE);
			break;
		}
		case 0x0A: //linefeed
		case 0x1B: //escape
			break;
		case ' ':
			if (atMidBrkPos == true)
			{
				atMidBrkPos = false;
				if (buffer[textPos-1] == ' ') //Only go on if a word was typed
				{
					if (spaceBetween == false)
					{
						spaceBetween = true;
						caretLine--;
					}
				}
				else if ((lineLens[caretLine-1] + caretX + cWidths[0x20]) < textAreaWidth)
				{
					if ((lineLens[caretLine-1] + caretX + cWidths[0x20] * 2) < textAreaWidth)
					{
						//Unwrap the last word
						lineLens[caretLine] -= caretX;
						lineLens[caretLine-1] += caretX + cWidths[0x20];
						lineStarts[caretLine] = textPos + 1;
						caretX = lineLens[caretLine-1] + cWidths[0x20];
						caretY -= fontHeight;
						//Perform subsequent unwrapping
						spaceUnwrap = true;
						//Get ready for another unwrap
						atMidBrkPos = true;
					}
					else
					{
						//Unwrap the last word
						lineLens[caretLine] -= caretX;
						lineLens[caretLine-1] += caretX + cWidths[0x20];
						lineStarts[caretLine] = textPos + 1;
						caretX = 0;
						//Notify later code for proper screen updating
						lastLineModified = true;
						//Perform subsequent unwrapping
						spaceUnwrap = true;
					}
				}
				else
				{
					unsigned lineEndRef;
					if (caretLine == numLines - 1)
						lineEndRef = textSize - 1;
					else
						lineEndRef = lineStarts[caretLine+1] - 1;
					if (buffer[lineEndRef] != ' ' &&
						buffer[lineEndRef] != '\t' &&
						buffer[lineEndRef] != '\n')
					{
						//Don't unwrap but add a new line
						numLines++;
						if (numLines % 100 == 0)
						{
							lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 100));
							lineLens = (float*)realloc(lineLens, sizeof(float) * (numLines + 100));
						}
						//Move the broken word down
						memmove(&(lineStarts[caretLine+1]), &(lineStarts[caretLine]), sizeof(unsigned) * (numLines - caretLine));
						memmove(&(lineLens[caretLine+1]), &(lineLens[caretLine]), sizeof(float) * (numLines - caretLine));
						//Update so that our word is on its own line
						lineLens[caretLine] = caretX;
						lineLens[caretLine+1] -= caretX;
						lineStarts[caretLine+1] = textPos + 1;
						caretLine++;
						caretX += cWidths[0x20];
						//Perform subsequent unwrapping
						spaceUnwrap = true;
						//Get ready for another unwrap
						atMidBrkPos = true;
					}
				}
			}
			else if (caretLine < numLines - 1 &&
				buffer[lineStarts[caretLine+1] - 1] != ' ' && buffer[lineStarts[caretLine+1] - 1] != '\n')
			{
				//Properly wrap the word which was broken in between
				if (caretX + cWidths[0x20] < textAreaWidth)
				{
					unsigned breakPos = lineStarts[caretLine+1] - 1;
					float carryAmnt = 0;
					while (breakPos != textPos)
					{
						carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
						breakPos--;
					}
					carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
					lineLens[caretLine] -= carryAmnt;
					lineStarts[caretLine+1] = breakPos;
					lineLens[caretLine+1] += carryAmnt;
					//Perform subsequent wrapping
					spaceSubseqWrap = true;
					//Get ready for another unwrap
					atMidBrkPos = true;
				}
				else
				{
					lineLens[caretLine] -= cWidths[0x20];
					//Let subsequent code set the caret
				}
			}
		default:
		{
			RECT rt;
			//Add the character
			textSize++;
			if (textSize == buffSize)
			{
				buffSize += 1000;
				buffer = (char*)realloc(buffer, buffSize);
			}
			if (textPos != (textSize - 1))
				memmove(buffer + textPos + 1, buffer + textPos, textSize - textPos);
			else
				buffer[textPos+1] = '\0';
			buffer[textPos] = (char)wParam;
			textPos++;
			//Correct all line beginning indices
			for (unsigned i = caretLine + 1; i < numLines; i++)
				lineStarts[i]++;
			//Set the update rectangle
			GetClientRect(hwnd, &rt);
			rt.top = (int)caretY;
			//Update the caret
			if (spaceProcessed == true)
			{
				//The caret and wrapping was already corrected
				spaceProcessed = false;
				rt.top -= fontHeight;
				goto updateText;
			}
			//Add last character width
			if (spaceUnwrap == false)
			{
				if (textPos != 0 && textPos != textSize && textPos - 1 == lineStarts[caretLine] && caretX != 0)
				{
					//Whip the cursor forward
					caretY += fontHeight;
					caretX = cWidths[(unsigned char)buffer[lineStarts[caretLine]]];
				}
				else if (fixedPitch == false)
					caretX += cWidths[(unsigned char)buffer[textPos-1]];
				else
					caretX += avgCharWidth;
			}
			//Check to see if this will cause subsequent word wrapping
			if (spaceSubseqWrap == false && spaceUnwrap == false)
				lineLens[caretLine] += cWidths[(unsigned char)buffer[textPos-1]];
			if (spaceUnwrap == true)
			{
				spaceUnwrap = false;
				UnwrapWords();
				goto updateText;
			}
			if ((textPos == textSize || buffer[textPos] == '\n') && lineLens[caretLine] >= textAreaWidth && buffer[textPos-1] == ' ')
			{
				lineLens[caretLine] -= cWidths[(unsigned char)buffer[textPos-1]];
				//Add a line
				numLines++;
				if (numLines % 100 == 0)
				{
					lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 100));
					lineLens = (float*)realloc(lineLens, sizeof(float) * (numLines + 100));
				}
				//Set up this line
				caretLine++;
				if (buffer[textPos] == '\n')
				{
					memmove(&(lineStarts[caretLine+1]), &(lineStarts[caretLine]), sizeof(unsigned) * (numLines - caretLine));
					memmove(&(lineLens[caretLine+1]), &(lineLens[caretLine]), sizeof(float) * (numLines - caretLine));
				}
				lineStarts[caretLine] = textPos;
				lineLens[caretLine] = 0;
				caretX = 0;
				caretY += fontHeight;
			}
			else
				RewrapWords();
			//If in a kerning pair...
			//Forget kerning pairs for now because they don't affect my sreen DC
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
			//Update the window
updateText:
			if (lastLineModified == true)
			{
				lastLineModified = false;
				rt.top -= fontHeight;
			}
			InvalidateRect(hwnd, &rt, TRUE);
			break;
		}
		}
		break;
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
	case WM_MOUSEWHEEL:
	{
		SCROLLINFO si;
		int yPos;
		if (drawDrag == true)
		{
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
		short wDist;
		wDist = HIWORD(wParam);
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS;
		GetScrollInfo(hwnd, SB_VERT, &si);
		yPos = si.nPos;
		si.nPos -= (int)fontHeight * wDist * (int)g_wheelLines / WHEEL_DELTA;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		GetScrollInfo(hwnd, SB_VERT, &si);
		if (si.nPos != yPos)
		{
			caretY += yPos - si.nPos;
			RECT rtScroll;
			GetClientRect(hwnd, &rtScroll);
			rtScroll.left = xBordWidth;
			rtScroll.top = yBordWidth;
			//rtScroll.bottom -= yBordWidth; //With horizontal scroll right needs to be clipped too
			ScrollWindowEx(hwnd, 0, yPos - si.nPos, &rtScroll, &rtScroll, NULL, NULL, SW_INVALIDATE | SW_ERASE);
		}
		break;
	}
	case WM_MBUTTONDOWN:
		SetFocus(hwnd);
		if (drawDrag == false)
		{
			dragP.x = LOWORD(lParam);
			dragP.y = HIWORD(lParam);
			drawDrag = true;
			SetTimer(hwnd, 1, 10, NULL);
			HDC hdc = GetDC(hwnd);
			Ellipse(hdc, dragP.x - 16, dragP.y - 16, dragP.x + 16, dragP.y + 16);
			ReleaseDC(hwnd, hdc);
		}
		else
		{
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
		/*RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
		if (drawDrag == true)
		{ MessageBeep
		}*/
		break;
#endif
	case WM_ADDENTRY:
		break;
	case WM_CHANGEENTRY:
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void WrapWords()
{
	float curLineLen = 0;
	unsigned lastSpacePos = 0;

	lineStarts[0] = 0;
	numLines++;
	for (unsigned i = 0; i < textSize; i++)
	{
		switch (buffer[i])
		{
		case '\n':
			lineStarts[numLines] = i + 1;
			lineLens[numLines] = curLineLen;
			curLineLen = 0;
			numLines++;
			if (numLines % 100 == 0)
			{
				lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 100));
				lineLens = (float*)realloc(lineLens, sizeof(float) * (numLines + 100));
			}
			break;
		case '\t':
			lastSpacePos = i;
			unsigned curTabStop;
			for (curTabStop = 0; curTabStop < numTabStops && curLineLen >= tabStops[curTabStop]; curTabStop++);
			curLineLen = (float)tabStops[curTabStop];
			tabsPos[numTabs] = i;
			numTabs++;
			if (numTabs % 20 == 0)
				tabsPos = (unsigned*)realloc(tabsPos, sizeof(unsigned) * (numTabs + 20));
			break;
		default:
			curLineLen += cWidths[(unsigned char)buffer[i]];
			if (buffer[i] == ' ')
				lastSpacePos = i;
			break;
		}
		if (curLineLen >= textAreaWidth)
		{
			if (buffer[i] != ' ')
			{
				//Go back to previous space
				if ((numLines == 1 && lastSpacePos == 0 && buffer[0] != ' ') || lineStarts[numLines-1] > lastSpacePos)
				{
					//We don't have a space at the beginning of this line so
					//we will break the line in between a word.
					lineStarts[numLines] = i;
					//We must also make sure that the character that got wrapped to the
					//next line gets counted in that line's length.
					i--;
				}
				else
				{
					//Subtract all characters including the space (see below)
					while (i > lastSpacePos)
					{
						curLineLen -= cWidths[(unsigned char)buffer[i]];
						i--;
					}
					lastSpacePos = 0;
					lineStarts[numLines] = i + 1;
				}
			}
			else
				lineStarts[numLines] = i + 1;
			curLineLen -= cWidths[(unsigned char)buffer[i]];
			lineLens[numLines-1] = curLineLen;
			curLineLen = 0;
			numLines++;
			if (numLines % 100 == 0)
			{
				lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 100));
				lineLens = (float*)realloc(lineLens, sizeof(float) * (numLines + 100));
			}
		}
	}
	if (curLineLen != 0)
		lineLens[numLines-1] = curLineLen;
}

void RewrapWords()
{
	if (lineLens[caretLine] >= textAreaWidth || spaceSubseqWrap == true)
	{
		//Add to current line length
		//If line is too long
		//Go back until it is short enough
		//Add extra to next line
		//Repeat check on next line
		unsigned curLine = caretLine;
		if (spaceSubseqWrap == true)
		{
			spaceSubseqWrap = false;
			curLine++;
		}
		float carryAmnt = 0;
		unsigned numMovBack = 0;
		unsigned oldLineStart = 0;
		while (lineLens[curLine] >= textAreaWidth)
		{
			unsigned lineEndRef;
			if (curLine == numLines - 1)
				lineEndRef = textSize - 1;
			else
				lineEndRef = lineStarts[curLine+1] - 1;
			unsigned breakPos = lineEndRef;
			//Skip the original breaking space (if it has one)
			if (buffer[lineEndRef] == ' ')
				breakPos--;
			else if (buffer[lineEndRef] == '\n')
			{
				unsigned lineEndRef;
				if (curLine == numLines - 1)
					lineEndRef = textSize - 1;
				else
					lineEndRef = lineStarts[curLine+1] - 1;
				//Skip the newline character
				breakPos--;
				//Move over or break one word
				if (lineLens[curLine] < textAreaWidth)
					//We don't need to do anything
					break;
				while (breakPos > lineStarts[curLine] && buffer[breakPos] != ' ')
				{
					carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
					breakPos--;
				}
				if (breakPos == lineStarts[curLine] && buffer[breakPos] != ' ')
				{
					//Break this word in between
					breakPos = lineEndRef - 1;
					carryAmnt = 0;
					while (lineLens[curLine] - carryAmnt >= textAreaWidth)
					{
						carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
						breakPos--;
					}
				}
				//Update the line length
				lineLens[curLine] -= carryAmnt;
				if (buffer[breakPos] == ' ')
					lineLens[curLine] -= cWidths[0x20];
				//Add a line
				numLines++;
				if (numLines % 100 == 0)
				{
					lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 100));
					lineLens = (float*)realloc(lineLens, sizeof(float) * (numLines + 100));
				}
				//Move in new line
				memmove(&(lineStarts[curLine+2]), &(lineStarts[curLine+1]), sizeof(unsigned) * (numLines - (curLine + 1)));
				memmove(&(lineLens[curLine+2]), &(lineLens[curLine+1]), sizeof(float) * (numLines - (curLine + 1)));
				lineStarts[curLine+1] = breakPos + 1;
				lineLens[curLine+1] = carryAmnt;
				//We're done!
				break;
			}
			else if (lineEndRef != textSize - 1)
			{
				breakPos = lineStarts[curLine];
				if (oldLineStart != 0)
				{
					//Skip past any good stuff move over
					while (breakPos < oldLineStart - 1)
					{
						carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
						breakPos++;
					}
					lineLens[curLine] -= cWidths[0x20];
					breakPos++;
					//Add a new line and move everything down manually
					numLines++;
					if (numLines % 100 == 0)
					{
						lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 100));
						lineLens = (float*)realloc(lineLens, sizeof(float) * (numLines + 100));
					}
					memmove(&(lineStarts[curLine+2]), &(lineStarts[curLine+1]), sizeof(unsigned) * (numLines - (curLine + 1)));
					memmove(&(lineLens[curLine+2]), &(lineLens[curLine+1]), sizeof(float) * (numLines - (curLine + 1)));
					//Fill the blank line with the bad stuff
					lineLens[curLine+1] = lineLens[curLine] - carryAmnt;
					lineLens[curLine] = carryAmnt;
					carryAmnt = 0;
					curLine++;
					lineStarts[curLine] = breakPos;
					curLine++;
					//We're done!
					break;
				}
				else
				{
					//Count backward
					breakPos = lineEndRef;
					while (lineLens[curLine] - carryAmnt >= textAreaWidth)
					{
						carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
						breakPos--;
					}
					lineLens[curLine] -= carryAmnt;
					numMovBack = lineEndRef - breakPos;
					curLine++;
					//Break and wrap to the next line
					goto newLineTest;
				}
			}
			//Do proper trailing space wrapping
			/*if (trailSpaces > 1)
			{
				//See if going back a few spaces will work
				carryAmnt = (trailSpaces - 1) * cWidths[0x20];
				while (lineLens[curLine] - carryAmnt < textAreaWidth)
					carryAmnt -= cWidths[0x20];
				carryAmnt += cWidths[0x20];
				numMovBack = (unsigned)(carryAmnt / cWidths[0x20]);
				curLine++;
				goto newLineTest;
			}*/
			//Go back some words
			while (lineLens[curLine] - carryAmnt >= textAreaWidth)
			{
				while (breakPos >= lineStarts[curLine] && breakPos != 0 && buffer[breakPos] != ' ' && buffer[breakPos] != '\n')
				{
					carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
					numMovBack++;
					breakPos--;
				}
				if (breakPos < lineStarts[curLine] || (breakPos == 0 && buffer[0] != ' '))
				{
					//Break this line in between a word
					breakPos = lineEndRef - 1;
					carryAmnt = 0;
					numMovBack = 0;
					while (lineLens[curLine] - carryAmnt >= textAreaWidth)
					{
						carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
						breakPos--;
					}
					numMovBack = lineEndRef - (breakPos + 1);
					lineLens[curLine] -= carryAmnt;
					if (buffer[lineEndRef] == ' ')
					{
						carryAmnt += cWidths[0x20]; //Add original breaking space
						numMovBack++;
					}
					curLine++;
					if (buffer[lineEndRef] == ' ' || lineEndRef == textSize - 1)
						goto newLineTest;
				}
				else
				{
					carryAmnt += cWidths[0x20];
					numMovBack++;
					breakPos--;
				}
			}
			lineLens[curLine] -= carryAmnt;
			carryAmnt -= cWidths[0x20]; //Don't move over the end space
			numMovBack--;
			if (buffer[lineEndRef] == ' ')
			{
				carryAmnt += cWidths[0x20]; //Add the original breaking space back
				numMovBack++;
			}
			curLine++;
newLineTest:
			if (curLine == numLines)
				break;
			lineLens[curLine] += carryAmnt;
			oldLineStart = lineStarts[curLine];
			lineStarts[curLine] -= numMovBack;
			if (buffer[lineStarts[curLine]-1] != ' ' &&
				buffer[lineStarts[curLine]-1] != '\t' &&
				buffer[lineStarts[curLine]-1] != '\n')
				oldLineStart = 0;
			carryAmnt = 0;
			numMovBack = 0;
		}
		if (curLine == numLines)
		{
			//Add a line
			numLines++;
			if (numLines % 100 == 0)
			{
				lineStarts = (unsigned*)realloc(lineStarts, sizeof(unsigned) * (numLines + 100));
				lineLens = (float*)realloc(lineLens, sizeof(float) * (numLines + 100));
			}
			//Put carried text on this line
			lineStarts[numLines-1] = textSize - numMovBack;
			lineLens[numLines-1] = carryAmnt;
		}
	}
}

void UnwrapWords()
{
	//Pull back as many words as will fit
	//If unable
	//Then stop
	unsigned curLine = caretLine + 1;
	float carryAmnt = 0;
	unsigned numMovFwd = 0;
	if (curLine < numLines && buffer[lineStarts[curLine]-1] == '\n')
	{
		if (lineStarts[curLine-1] == lineStarts[curLine])
		{
			//Delete the empty line
			memmove(&(lineStarts[curLine-1]), &(lineStarts[curLine]), sizeof(unsigned) * (numLines - curLine));
			memmove(&(lineLens[curLine-1]), &(lineLens[curLine]), sizeof(float) * (numLines - curLine));
			numLines--;
		}
		return;
	}
	while (curLine < numLines)
	{
		unsigned lineEndRef;
		if (curLine == numLines - 1)
			lineEndRef = textSize - 1;
		else
			lineEndRef = lineStarts[curLine+1] - 1;
		unsigned breakPos = lineStarts[curLine];
		if (lineStarts[curLine-1] == lineStarts[curLine] && buffer[lineEndRef] == '\n')
		{
			//Delete the empty line
			memmove(&(lineStarts[curLine-1]), &(lineStarts[curLine]), sizeof(unsigned) * (numLines - curLine));
			memmove(&(lineLens[curLine-1]), &(lineLens[curLine]), sizeof(float) * (numLines - curLine));
			numLines--;
			return;
		}
		if (buffer[lineEndRef] != ' ')
		{
			if (lineStarts[curLine-1] == lineStarts[curLine])
			{
				//Delete the last empty line
				memmove(&(lineStarts[curLine-1]), &(lineStarts[curLine]), sizeof(unsigned) * (numLines - curLine));
				memmove(&(lineLens[curLine-1]), &(lineLens[curLine]), sizeof(float) * (numLines - curLine));
				numLines--;
				return;
			}
		}
		if (curLine < numLines && buffer[lineStarts[curLine]-1] != ' ' &&
			buffer[lineStarts[curLine]-1] != '\n')
		{
			//Unwrap a mid-broken word
			while (breakPos != lineEndRef + 1 && lineLens[curLine-1] + carryAmnt < textAreaWidth &&
				buffer[breakPos] != ' ')
			{
				carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
				breakPos++;
			}
			if (buffer[breakPos] == ' ' && lineLens[curLine-1] + carryAmnt < textAreaWidth)
			{
				//Finalize and don't change curLine. The algorithm will loop to the normal
				//section to unwrap words.
				lineStarts[curLine] = breakPos + 1;
				lineLens[curLine-1] += carryAmnt;
				lineLens[curLine] -= (carryAmnt + cWidths[0x20]);
			}
			else if (breakPos == lineEndRef + 1)
			{
				breakPos--;
				carryAmnt -= cWidths[(unsigned char)buffer[breakPos]];
				lineLens[curLine-1] += carryAmnt;
				lineLens[curLine] -= carryAmnt;
				//Delete the empty line
				memmove(&(lineStarts[curLine]), &(lineStarts[curLine+1]), sizeof(unsigned) * (numLines - curLine + 1));
				memmove(&(lineLens[curLine]), &(lineLens[curLine+1]), sizeof(float) * (numLines - curLine + 1));
				numLines--;
			}
			else
			{
				//Finalize and possibly continue broken word unwrapping
				breakPos--;
				carryAmnt -= cWidths[(unsigned char)buffer[breakPos]];
				lineStarts[curLine] = breakPos;
				lineLens[curLine-1] += carryAmnt;
				lineLens[curLine] -= carryAmnt;
				curLine++;
			}
			carryAmnt = 0;
			continue;
		}
		if (buffer[lineStarts[curLine]-1] == '\n')
			break;
		//Go forward one word at a time
		while (lineLens[curLine-1] + carryAmnt < textAreaWidth && breakPos != textSize - 1)
		{
			while (breakPos != textSize - 1 && buffer[breakPos] != ' ')
			{
				if (buffer[breakPos] == '\n')
				{
					if (lineLens[curLine-1] + carryAmnt < textAreaWidth)
					{
						//Delete this line
						memmove(&(lineStarts[curLine-1]), &(lineStarts[curLine]), sizeof(unsigned) * (numLines - curLine));
						memmove(&(lineLens[curLine-1]), &(lineLens[curLine]), sizeof(float) * (numLines - curLine));
						numLines--;
					}
					else
					{
						//Try ungetting the last word
						if (buffer[breakPos-1] == ' ')
						{
							breakPos--;
							carryAmnt -= cWidths[0x20];
						}
						while (buffer[breakPos-1] != ' ')
						{
							breakPos--;
							carryAmnt -= cWidths[(unsigned char)buffer[breakPos]];
						}
						breakPos--;
						if (buffer[lineStarts[curLine]-1] != ' ')
						{
							carryAmnt -= cWidths[(unsigned char)buffer[breakPos]];
							//See how many characters can be moved over
							while (lineLens[curLine-1] + carryAmnt >= textAreaWidth)
							{
								breakPos--;
								carryAmnt -= cWidths[(unsigned char)buffer[breakPos]];
							}
						}
						if (lineLens[curLine-1] + carryAmnt < textAreaWidth)
						{
							//Delete this line
							memmove(&(lineStarts[curLine-1]), &(lineStarts[curLine]), sizeof(unsigned) * (numLines - curLine));
							memmove(&(lineLens[curLine-1]), &(lineLens[curLine]), sizeof(float) * (numLines - curLine));
							numLines--;
						}
						else
						{
							//Finalize unwrapping
							lineLens[curLine] -= carryAmnt;
							lineLens[curLine-1] += carryAmnt;
							lineStarts[curLine] = breakPos + 1;
						}
					}
					return;
				}
				carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
				breakPos++;
			}
			if (breakPos != textSize - 1 && buffer[breakPos] == ' ')
			{
				carryAmnt += cWidths[0x20];
				breakPos++;
			}
		}
		if (breakPos == textSize - 1)
		{
			if (lineLens[curLine-1] + carryAmnt + cWidths[(unsigned char)buffer[breakPos]] >= textAreaWidth)
			{
				carryAmnt += cWidths[(unsigned char)buffer[breakPos]];
				breakPos++;
			}
			else
			{
				//We got caught at the end of the data
				lineLens[curLine-1] += lineLens[curLine];
				//Delete the last line
				numLines--;
				return;
			}
		}
		//Unget the last word
		if (buffer[breakPos-1] == ' ')
		{
			breakPos--;
			carryAmnt -= cWidths[0x20];
		}
		while (buffer[breakPos-1] != ' ')
		{
			breakPos--;
			carryAmnt -= cWidths[(unsigned char)buffer[breakPos]];
		}
		carryAmnt -= cWidths[0x20];
		if (lineStarts[curLine] - breakPos <= 1)
			return;
		else
			numMovFwd = breakPos - lineStarts[curLine];
		lineLens[curLine] -= carryAmnt;
		lineLens[curLine] -= cWidths[0x20];
		if (buffer[lineStarts[curLine]-1] == ' ' && lineLens[curLine-1] != 0)
		{
			lineLens[curLine-1] += cWidths[0x20];
		}
		lineLens[curLine-1] += carryAmnt;
		lineStarts[curLine] += numMovFwd;
		curLine++;
		carryAmnt = 0;
		numMovFwd = 0;
		if (buffer[lineEndRef] == '\n')
			break;
	}
}