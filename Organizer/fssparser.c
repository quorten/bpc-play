/* Parse a file system summary file.
   For information on the syntax of this format, see "fss-rules.txt" */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <crtdbg.h>

#include "core.h"
#include "fssparser.h"
#include "exparray.h"

CREATE_EXP_ARRAY_TYPE(char);

FSSentry* entries = NULL;
FSSentry headEntry;
unsigned numEntries;

/* Helper functions and macros */

#define IS_NLCHAR(c) (c == '\r' || c == '\n')
#define CHECK_SPACE (c == ' ' || c == '\t')

char CheckChar(FILE* fp)
{
	char c;
	c = fgetc(fp);
	if (c == '\r')
		c = '\n';
	fseek(fp, -1, SEEK_CUR);
	return c;
}

bool CheckTrailspace(FILE* fp)
{
	char c;
	fseek(fp, -1, SEEK_CUR);
	c = fgetc(fp);
	if (c == '\r')
	{
		fseek(fp, -2, SEEK_CUR);
		c = fgetc(fp);
		fseek(fp, 1, SEEK_CUR);
	}
	if (CHECK_SPACE)
		return true;
	return false;
}

bool CheckSubheader(FILE* fp, long lastPos)
{
	char c1, c2;
	long curPos;
	curPos = ftell(fp);
	fseek(fp, lastPos, SEEK_SET);
	c1 = fgetc(fp);
	fseek(fp, curPos - 1, SEEK_SET);
	c2 = fgetc(fp);
	if (c1 == '*' && c2 == '*')
		return true;
	return false;
}

bool CheckPrecBlankLine(FILE* fp, long lastPos)
{
	unsigned curPos;
	char c1, c2;
	bool foundTwoNl;
	foundTwoNl = false;
	curPos = ftell(fp);
	fseek(fp, lastPos, SEEK_SET);
	fseek(fp, -2, SEEK_CUR);
	c1 = fgetc(fp);
	c2 = fgetc(fp);
	if (c1 == '\r' && c2 == '\n')
	{
		fseek(fp, -4, SEEK_CUR);
		c1 = fgetc(fp);
		c2 = fgetc(fp);
		/* In case more than one line ending type is in the same
		   file */
		if (IS_NLCHAR(c2)) /* Only c2 needs to be checked */
			foundTwoNl = true;
		else
			foundTwoNl = false;
	}
	else
	{
		if (IS_NLCHAR(c1) || IS_NLCHAR(c2))
			foundTwoNl = true;
		else
			foundTwoNl = false;
	}
	fseek(fp, curPos, SEEK_SET);
	if (lastPos < 2 || foundTwoNl == false)
		return false;
	return true;
}

/* Generic line reader.  We always leave room for the null
   character. */
void SkelRead(FILE* fp, unsigned type, char_array *lastString, bool skip)
{
	char c;
	c = fgetc(fp);
	while (!IS_NLCHAR(c))
	{
		if (type == 1 && c == ':') /* Read attribute */
			break;
		if (skip == false)
		{
			lastString->d[lastString->num-1] = c;
			ADD_ARRAY_ELEMENT(char, (*lastString), 100);
		}
		if (feof(fp))
			break;
		c = fgetc(fp);
	}
	if (skip == false)
	{
		if (feof(fp))
			lastString->num--; /* Don't include the EOF character */
		lastString->d[lastString->num-1] = '\0';
	}
	if (!feof(fp))
		fseek(fp, -1, SEEK_CUR);
}

void InitRead(FILE* fp, unsigned type, char_array *lastString)
{
	lastString->num = 1;
	lastString->d[0] = '\0';
	SkelRead(fp, type, lastString, false);
}

/* The important function */
bool ParseFile(const char* filename)
{
	FILE* fp;
	long lastPos;
	char c;
	unsigned curLine;
	char_array lastString;
	char* errorDesc;
	unsigned curEntry;
	unsigned curAttrib;
	bool sizeError;
	lastPos = 0;
	curLine = 1;
	errorDesc = NULL;
	curEntry = 0;
	curAttrib = 0;
	sizeError = false;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		return false;

	INIT_ARRAY(char, lastString, 100);

#define READ_LINE()		InitRead(fp, 0, &lastString);
#define READ_ATTRIB()	InitRead(fp, 1, &lastString);
#define APPEND_LINE()	SkelRead(fp, 2, &lastString, false);
#define SKIP_LINE()		SkelRead(fp, 0, NULL, true);
#define SKIP_ATTRIB()	SkelRead(fp, 1, NULL, true);
#define CHECK_SIZE() \
	if (feof(fp)) \
		break;
#define CHECK_SIZE_CLEANUP(label) \
	if (feof(fp)) \
		goto label;
#define CHECK_SIZE_ERROR() \
	if (feof(fp)) \
	{ \
		sizeError = true; \
		break; \
	}
#define SKIP_CHAR(chCode) \
	c = fgetc(fp); \
	if (c != chCode) \
		break;
#define COMMON_NLSKIP() \
	if (c == '\r' && !feof(fp)) \
	{ \
		c = fgetc(fp); \
		if (c != '\n' && !feof(fp)) \
			fseek(fp, -1, SEEK_CUR); \
		if (feof(fp)) \
			fseek(fp, 0, SEEK_END); \
		c = '\n'; \
	}
#define SKIP_NLCHAR() \
	c = fgetc(fp); \
	if (!IS_NLCHAR(c)) \
		break; \
	COMMON_NLSKIP();
#define SKIP_NLCHAR_NOCHECK() \
	c = fgetc(fp); \
	COMMON_NLSKIP();
#define SKIP_ENT_H_LN() \
	do \
	{ \
		if (feof(fp)) \
			break; \
		c = fgetc(fp); \
	} while (c == '-'); \
	fseek(fp, -1, SEEK_CUR);

	/* First read the header */
	{
		char* orgVersion = "Organizer Version 1.1";
		char* orgPurpose =
			"Purpose: File system cataloging by metadata";

		lastPos = ftell(fp);
		READ_LINE();
		if (strcmp(orgVersion, lastString.d) != 0)
		{
			printf("Parse error: invalid file header.\n");
			fclose(fp);
			return false;
		}
		SKIP_NLCHAR_NOCHECK();
		if (feof(fp))
		{
			printf("Parse error: missing purpose for organizer file.\n");
			fclose(fp);
			return false;
		}
		curLine++;

		lastPos = ftell(fp);
		READ_LINE();
		if (strcmp(orgPurpose, lastString.d) != 0)
		{
			printf("Parse error: organizer purpose mismatch.\n");
			fclose(fp);
			return false;
		}
		SKIP_NLCHAR_NOCHECK();
		if (feof(fp))
		{
			printf("Parse error: missing organizer data.\n");
			fclose(fp);
			return false;
		}
		curLine++;

		headEntry.name = orgVersion;
		headEntry.attribs = NULL;
		headEntry.numAttribs = 0;
	}

	/* Parser loop */
	while (!feof(fp))
	{
		lastPos = ftell(fp);
		READ_ATTRIB();
		errorDesc = "Incomplete data.";
		CHECK_SIZE_ERROR();
		c = CheckChar(fp);
		if (c == ':')
		{
			FSSentry* cePtr;
			cePtr = &entries[curEntry];
			/* Read the value */
			if (CheckTrailspace(fp))
			{
				errorDesc = "Extra space before colon.";
				break; /* This is an error */
			}
			c = fgetc(fp); /* Skip the colon */
			errorDesc = "Missing value for attribute.";
			CHECK_SIZE_ERROR();

			/* Data processing hook */
			if (numEntries == 0)
			{
				/* Right now this is an error, even though there could later
				   be support for custom attributes in the header. */
				errorDesc = "Attributes must be inside entries.";
				break;
			}
			if (cePtr->numAttribs == 0)
			{
				cePtr->attribs = (FSSattrib*)malloc(sizeof(FSSattrib) * 20);
				cePtr->numAttribs++;
			}
			else
			{
				cePtr->numAttribs++;
				if (cePtr->numAttribs % 20 == 0)
				{
					cePtr->attribs = (FSSattrib*)realloc(cePtr->attribs,
						sizeof(FSSattrib) * (cePtr->numAttribs + 20));
				}
				curAttrib++;
			}
			cePtr->attribs[curAttrib].name =
				(char*)malloc(strlen(lastString.d) + 1);
			strcpy(cePtr->attribs[curAttrib].name, lastString.d);
			cePtr->attribs[curAttrib].value = NULL;
			cePtr->attribs[curAttrib].type = FA_NORMAL;

			c = CheckChar(fp);
			if (c == '\n') /* Read a verbatim attribute */
			{
				unsigned lineDisp; /* Used for forward token checking */
				unsigned blankLines;
				bool keepVbtmParse;
				lineDisp = 0;
				blankLines = 0;
				keepVbtmParse = false;

				/* Check for preceding blank line */
				errorDesc = "Missing blank line before verbatim attribute.";
				if (!CheckPrecBlankLine(fp, lastPos))
					break;

				SKIP_NLCHAR(); /* Skip the newline character */
				errorDesc = "Missing value for attribute.";
				CHECK_SIZE_ERROR();
				curLine++;
				/* Reset lastString */
				lastString.num = 1;
				lastString.d[0] = '\0';
				/* Read until a blank newline */
readVbtmParagraph:
				while (true)
				{
					lastPos = ftell(fp);
					APPEND_LINE();
					CHECK_SIZE_CLEANUP(vbtmSaveString);
					if (lastPos == ftell(fp))
					{
						SKIP_NLCHAR();
						CHECK_SIZE_CLEANUP(vbtmSaveString);
						curLine++;
						blankLines++;
						/* The blank line might mark the end of the
						   attribute */
						break;
					}
					/* Include the newline character in the saved string */
					SKIP_NLCHAR();
					CHECK_SIZE_CLEANUP(vbtmSaveString);
					curLine++;
vbtmSaveString:
					/* Don't append a newline character when the end
					   of file is reached. */
					CHECK_SIZE_CLEANUP(vbtmProcHook);
					/* Append the newline character */
					lastString.d[lastString.num-1] = c;
					ADD_ARRAY_ELEMENT(char, lastString, 100);
					lastString.d[lastString.num-1] = '\0';
				}
				/* When a blank newline is reached, we may still be in
				   a verbatim attribute.  Therefore, we must check
				   that the next non-blank line is either an attribute
				   or the title of an entry header. */
				lastPos = ftell(fp);
checkNextToken:
				SKIP_ATTRIB();
				CHECK_SIZE_CLEANUP(vbtmProcHook);
				c = CheckChar(fp);
				if (c == ':')
				{
					SKIP_LINE();
					CHECK_SIZE_CLEANUP(vbtmProcHook);
					if (CheckTrailspace(fp))
						keepVbtmParse = true;
					else
						keepVbtmParse = false;
				}
				else if (c == '\n' && lastPos == ftell(fp))
				{
					SKIP_NLCHAR(); /* Skip the blank line */
					lineDisp++;
					if (!feof(fp))
						goto checkNextToken; /* Keep searching */
				}
				else if (c == '\n' && CheckSubheader(fp, lastPos))
				{
					if (CheckTrailspace(fp))
						keepVbtmParse = true;
					else
						keepVbtmParse = false;
				}
				else if (c == '\n')
				{
					unsigned origLastPos;
					if (CheckTrailspace(fp))
					{
						keepVbtmParse = true;
						goto evalVbtmParse;
					}

					/* Check for preceding blank line */
					if (!CheckPrecBlankLine(fp, lastPos))
					{
						keepVbtmParse = true;
						goto evalVbtmParse;
					}

					SKIP_NLCHAR(); /* Skip the newline character */
					CHECK_SIZE_CLEANUP(vbtmProcHook);

					/* Check for entry header line */
					origLastPos = lastPos;
					lastPos = ftell(fp);
					SKIP_ENT_H_LN();
					CHECK_SIZE_CLEANUP(vbtmProcHook);
					if (ftell(fp) - lastPos != 70)
						keepVbtmParse = true;
					else
						keepVbtmParse = false;
					lastPos = origLastPos;
				}

evalVbtmParse:
				if (keepVbtmParse == true)
				{
					/* Disregard the special meaning */
					fseek(fp, lastPos, SEEK_SET);
					curLine += lineDisp;
					lineDisp = 0;
					/* Write blank lines */
					{
						unsigned i;
						for (i = 0; i < blankLines; i++)
						{
							lastString.d[lastString.num-1] = '\n';
							ADD_ARRAY_ELEMENT(char, lastString, 100);
						}
						lastString.d[lastString.num-1] = '\0';
					}
					goto readVbtmParagraph;
				}
				else
				{
					/* Verbatim parsing should end here */
					fseek(fp, lastPos, SEEK_SET);
				}

vbtmProcHook:	/* Data processing hook */
				cePtr->attribs[curAttrib].type = FA_VERBATIM;
				cePtr->attribs[curAttrib].value =
					(char*)malloc(strlen(lastString.d) + 1);
				strcpy(cePtr->attribs[curAttrib].value, lastString.d);
				CHECK_SIZE(); /* Last regrets */
				continue;
			}

			errorDesc = "Missing space after colon.";
			SKIP_CHAR(' ');
			errorDesc = "Missing value for attribute.";
			CHECK_SIZE_ERROR();

			/* Read until the end of the line */
			{
				bool mustCleanup;
				mustCleanup = false;
				lastPos = ftell(fp);
				READ_LINE();
				if (feof(fp))
					mustCleanup = true;
				if (mustCleanup == false && CheckTrailspace(fp))
				{
					errorDesc = "Extra space on end of line.";
					break; /* This is an error */
				}
			}
			SKIP_NLCHAR(); /* Skip the newline character */
			CHECK_SIZE_CLEANUP(attribProcHook);
			curLine++;

			/* Pre-data processing hook */
			c = CheckChar(fp);
			if (!feof(fp) && c == ' ')
				cePtr->attribs[curAttrib].type = FA_MULTILINE;

			/* Read a multiline attribute (if exists) */
			while (c == ' ')
			{
				/* The leading space is included (it is not skipped
				   like usual) */
				{
					bool mustCleanup;
					mustCleanup = false;
					lastPos = ftell(fp);
					APPEND_LINE();
					if (feof(fp))
						mustCleanup = true;
					if (mustCleanup == false && CheckTrailspace(fp))
					{
						errorDesc = "Extra space on end of line.";
						break; /* This is an error. */
					}
				}
				SKIP_NLCHAR(); /* Skip the newline character */
				CHECK_SIZE_CLEANUP(attribProcHook);
				curLine++;

				/* Check next character */
				c = CheckChar(fp);
			}

attribProcHook: /* Data processing hook */
			cePtr->attribs[curAttrib].value =
				(char*)malloc(strlen(lastString.d) + 1);
			strcpy(cePtr->attribs[curAttrib].value, lastString.d);
		}
		else if (c == '\n' && lastPos == ftell(fp))
		{
			SKIP_NLCHAR(); /* Skip the blank line */
			CHECK_SIZE();
			curLine++;
		}
		else if (CheckSubheader(fp, lastPos))
		{
			/* Read a subheader */
			FSSentry* cePtr;
			cePtr = &entries[curEntry];
			/* Data processing hook */
			if (cePtr->numAttribs == 0)
			{
				cePtr->attribs = (FSSattrib*)malloc(sizeof(FSSattrib) * 20);
				cePtr->numAttribs++;
			}
			else
			{
				cePtr->numAttribs++;
				if (cePtr->numAttribs % 20 == 0)
				{
					cePtr->attribs = (FSSattrib*)realloc(cePtr->attribs,
						sizeof(FSSattrib) * (cePtr->numAttribs + 20));
				}
				curAttrib++;
			}
			cePtr->attribs[curAttrib].name =
				(char*)malloc(strlen(lastString.d) + 1);
			strcpy(cePtr->attribs[curAttrib].name, lastString.d);
			cePtr->attribs[curAttrib].value = NULL;
			cePtr->attribs[curAttrib].type = FA_HEADER;

			if (CheckTrailspace(fp))
			{
				errorDesc = "Extra space on end of line.";
				break;
			}
			SKIP_NLCHAR(); /* Skip the newline character */
			errorDesc = "Empty subheaders are invalid.";
			CHECK_SIZE_ERROR();
			curLine++;
		}
		else if (c == '\n') /* Redundant check, last possible case */
		{
			if (CheckTrailspace(fp))
			{
				errorDesc = "Extra space on end of line.";
				break; /* This is an error */
			}

			/* Check for preceding blank line */
			errorDesc = "Missing blank line before entry header.";
			if (!CheckPrecBlankLine(fp, lastPos))
				break;

			SKIP_NLCHAR(); /* Skip the newline character */
			errorDesc = "Missing entry header line.";
			CHECK_SIZE_ERROR();
			curLine++;

			/* Check for entry header line */
			lastPos = ftell(fp);
			SKIP_ENT_H_LN();
			errorDesc = "Missing attributes in entry.";
			CHECK_SIZE_ERROR();
			if (ftell(fp) - lastPos != 70)
			{
				lastPos = ftell(fp);
				/* Better error reporting */
				if (CheckChar(fp) == '-')
					errorDesc = "Invalid entry header line.";
				else
				{
					curLine--;
					errorDesc =
						"Invalid attribute name or missing entry header line.";
				}
				break;
			}

			errorDesc = "Extra information on end of line.";
			SKIP_NLCHAR();
			errorDesc = "Missing attributes in entry.";
			CHECK_SIZE_ERROR();
			curLine++;

			errorDesc = "Missing blank line.";
			SKIP_NLCHAR();
			errorDesc = "Missing attributes in entry.";
			CHECK_SIZE_ERROR();
			curLine++;

			/* Data processing hook */
			if (numEntries == 0)
			{
				entries = (FSSentry*)malloc(sizeof(FSSentry) * 20);
				numEntries++;
			}
			else
			{
				numEntries++;
				if (numEntries % 20 == 0)
				{
					entries = (FSSentry*)realloc(entries,
						sizeof(FSSentry) * (numEntries + 20));
				}
				curEntry++;
			}
			entries[curEntry].name = (char*)malloc(strlen(lastString.d) + 1);
			strcpy(entries[curEntry].name, lastString.d);
			entries[curEntry].numAttribs = 0;
			entries[curEntry].attribs = NULL;
			curAttrib = 0;
		}
	}
	free(lastString.d);

	if (!feof(fp) || sizeError == true)
	{
		printf("Parse error on line %u. %s\n", curLine, errorDesc);
		fclose(fp);
		return false;
	}
	fclose(fp);

#undef READ_LINE
#undef READ_ATTRIB
#undef CHECK_SIZE
#undef CHECK_SIZE_CLEANUP
#undef CHECK_SIZE_ERROR
#undef SKIP_CHAR
#undef SKIP_ENT_H_LN

	return true;
}

void EchoFSSData(FILE* fp)
{
	unsigned i, j;
	fputs("Organizer Version 1.1\n", fp);
	fputs("Purpose: File system cataloging by metadata\n\n", fp);
	for (i = 0; i < numEntries; i++)
	{
		fprintf(fp, "%s\n", entries[i].name);
		fputs(
"----------------------------------------------------------------------\n\n",
			fp);
		for (j = 0; j < entries[i].numAttribs; j++)
		{
			unsigned attType;
			attType = entries[i].attribs[j].type;
			switch (attType)
			{
			case FA_NORMAL:
				fprintf(fp, "%s: %s\n", entries[i].attribs[j].name,
						entries[i].attribs[j].value);
				break;
			case FA_MULTILINE:
			{
				/* Format the contents for output */
				char* fmtOutput;
				unsigned outputPos;
				bool firstTime;
				outputPos = 0;
				firstTime = true;
				fprintf(fp, "%s:", entries[i].attribs[j].name);
				while (outputPos < strlen(entries[i].attribs[j].value))
				{
					char* attValue;
					unsigned nextOpPos;
					attValue = entries[i].attribs[j].value;
					nextOpPos = outputPos;
					if (firstTime == true)
					{
						firstTime = false;
						nextOpPos +=
							70 - (strlen(entries[i].attribs[j].name) + 1);
					}
					else
						nextOpPos += 69;
					if (nextOpPos >= strlen(attValue))
					{
						fmtOutput = (char*)malloc(nextOpPos - outputPos + 1);
						strncpy(fmtOutput, &attValue[outputPos],
								nextOpPos - outputPos);
						fmtOutput[nextOpPos-outputPos] = '\0';
						fprintf(fp, " %s\n", fmtOutput);
						free(fmtOutput);
						break;
					}
					while (attValue[nextOpPos] != ' ')
						nextOpPos--;
					fmtOutput = (char*)malloc(nextOpPos - outputPos + 1);
					strncpy(fmtOutput, &attValue[outputPos],
							nextOpPos - outputPos);
					fmtOutput[nextOpPos-outputPos] = '\0';
					fprintf(fp, " %s\n", fmtOutput);
					free(fmtOutput);
					nextOpPos++; /* Skip the space */
					outputPos = nextOpPos;
				}
				break;
			}
			case FA_VERBATIM:
				fprintf(fp, "\n%s:\n%s\n", entries[i].attribs[j].name,
						entries[i].attribs[j].value);
				break;
			case FA_HEADER:
				fprintf(fp, "\n%s\n", entries[i].attribs[j].name);
				break;
			}
		}

		{
			unsigned attType;
			attType = entries[i].attribs[entries[i].numAttribs-1].type;
			if (attType != FA_MULTILINE && attType != FA_VERBATIM)
				fprintf(fp, "\n");
		}
	}
}

void FreeFSSData()
{
	unsigned i, j;
	for (i = 0; i < numEntries; i++)
	{
		free(entries[i].name);
		for (j = 0; j < entries[i].numAttribs; j++)
		{
			free(entries[i].attribs[j].name);
			free(entries[i].attribs[j].value);
		}
		free(entries[i].attribs);
	}
	free(entries);
}
