/* Parse a file system summary file. */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <crtdbg.h>

#include "core.h"
#include "fssparser.h"

FSSentry* entries = NULL;
FSSentry headEntry;
unsigned numEntries;

/* For information on the syntax of this format, see "fss-rules.txt" */
bool ParseFile(char* buffer, unsigned dataSize)
{
	unsigned curPos;
	unsigned lastPos;
	unsigned curLine;
	char* lastString;
	char* errorDesc;
	unsigned curEntry;
	unsigned curAttrib;
	bool sizeError;
	curPos = 0;
	lastPos = 0;
	curLine = 1;
	lastString = NULL;
	errorDesc = NULL;
	curEntry = 0;
	curAttrib = 0;
	sizeError = false;

	lastString = (char*)malloc(9);
	lastString[0] = '\0';

#define READ_LINE() \
	while (curPos < dataSize && buffer[curPos] != '\n') \
		curPos++;
#define SAVE_STRING() \
	lastString = (char*)realloc(lastString, curPos - lastPos + 1); \
	strncpy(lastString, &buffer[lastPos], curPos - lastPos); \
	lastString[curPos-lastPos] = '\0';
#define CHECK_SIZE() \
	if (curPos >= dataSize) \
		break;
#define CHECK_SIZE_CLEANUP(label) \
	if (curPos >= dataSize) \
		goto label;
#define CHECK_SIZE_ERROR() \
	if (curPos >= dataSize) \
	{ \
		sizeError = true; \
		break; \
	}
#define CHECK_CHAR(chCode) (buffer[curPos] == chCode)
#define SKIP_CHAR(chCode) \
	if (buffer[curPos] != chCode) \
		break; \
	curPos++;
#define READ_ENTDIV() \
	while (curPos < dataSize && buffer[curPos] == '-')	\
		curPos++;
#define READ_ATTRIB() \
	while (curPos < dataSize && buffer[curPos] != '\n' && \
		buffer[curPos] != ':') \
		curPos++;
#define CHECK_TRAILSPACE \
	(buffer[curPos-1] == ' ' || buffer[curPos-1] == '\t')

	/* First read the header */
	{
		char* orgVersion = "Organizer Version 1.1";
		char* orgPurpose =
			"Purpose: File system cataloging by metadata";

		lastPos = curPos;
		READ_LINE();
		SAVE_STRING();
		if (strcmp(orgVersion, lastString) != 0)
		{
			printf("Parse error: invalid file header.\n");
			return false;
		}
		curPos++; /* Skip the newline character */
		if (curPos >= dataSize)
		{
			printf("Parse error: missing purpose for organizer file.");
			return false;
		}
		curLine++;

		lastPos = curPos;
		READ_LINE();
		SAVE_STRING();
		if (strcmp(orgPurpose, lastString) != 0)
		{
			printf("Parse error: organizer purpose mismatch.\n");
			return false;
		}
		curPos++; /* Skip the newline character */
		if (curPos >= dataSize)
		{
			printf("Parse error: missing organizer data.");
			return false;
		}
		curLine++;

		headEntry.name = orgVersion;
		headEntry.attribs = NULL;
		headEntry.numAttribs = 0;
	}

	/* Parser loop */
	while (curPos < dataSize)
	{
		lastPos = curPos;
		READ_ATTRIB();
		errorDesc = "Incomplete data.";
		CHECK_SIZE_ERROR();
		SAVE_STRING();
		if (CHECK_CHAR(':'))
		{
			FSSentry* cePtr;
			cePtr = &entries[curEntry];
			/* Read the value */
			if (CHECK_TRAILSPACE)
			{
				errorDesc = "Extra space before colon.";
				break; /* This is an error */
			}
			curPos++; /* Skip the colon */
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
				(char*)malloc(strlen(lastString) + 1);
			strcpy(cePtr->attribs[curAttrib].name, lastString);
			cePtr->attribs[curAttrib].value = NULL;
			cePtr->attribs[curAttrib].type = FA_NORMAL;

			if (CHECK_CHAR('\n')) /* Read a verbatim attribute */
			{
				unsigned lineDisp; /* Used for forward token checking */
				unsigned blankLines;
				bool keepVbtmParse;
				lineDisp = 0;
				blankLines = 0;
				keepVbtmParse = false;

				/* Check for preceding blank line */
				errorDesc = "Missing blank line before verbatim attribute.";
				if (lastPos < 2 || buffer[lastPos-1] != '\n' ||
					buffer[lastPos-2] != '\n')
					break;

				curPos++; /* Skip the newline character */
				errorDesc = "Missing value for attribute.";
				CHECK_SIZE_ERROR();
				curLine++;
				/* Reset lastString */
				lastString[0] = '\0';
				/* Read until a blank newline */
readVbtmParagraph:
				while (true)
				{
					unsigned lastStrlen;

					lastPos = curPos;
					READ_LINE();
					CHECK_SIZE_CLEANUP(vbtmSaveString);
					if (lastPos == curPos)
					{
						curPos++; /* Skip the blank line */
						CHECK_SIZE_CLEANUP(vbtmSaveString);
						curLine++;
						blankLines++;
						/* The blank line might mark the end of the
						   attribute */
						break;
					}
					curPos++; /* Include the newline character in the
								 saved string */
					CHECK_SIZE_CLEANUP(vbtmSaveString);
					curLine++;
vbtmSaveString:		/* Append string */
					lastStrlen = strlen(lastString);
					lastString = (char*)realloc(lastString,
						lastStrlen + curPos - lastPos + 1);
					strncpy(&lastString[lastStrlen], &buffer[lastPos],
							curPos - lastPos);
					lastString[lastStrlen+curPos-lastPos] = '\0';
					CHECK_SIZE_CLEANUP(vbtmProcHook); /* Last regrets */
				}
				/* When a blank newline is reached, we may still be in
				   a verbatim attribute.  Therefore, we must check
				   that the next non-blank line is either an attribute
				   or the title of an entry header. */
				lastPos = curPos;
checkNextToken:
				READ_ATTRIB();
				CHECK_SIZE_CLEANUP(vbtmProcHook);
				if (CHECK_CHAR(':'))
				{
					READ_LINE();
					CHECK_SIZE_CLEANUP(vbtmProcHook);
					if (CHECK_TRAILSPACE)
						keepVbtmParse = true;
					else
						keepVbtmParse = false;
				}
				else if (CHECK_CHAR('\n') && lastPos == curPos)
				{
					curPos++; /* Skip the blank line */
					lineDisp++;
					if (curPos < dataSize)
						goto checkNextToken; /* Keep searching */
				}
				else if (CHECK_CHAR('\n') && buffer[lastPos] == '*' &&
						 buffer[curPos-1] == '*')
				{
					if (CHECK_TRAILSPACE)
						keepVbtmParse = true;
					else
						keepVbtmParse = false;
				}
				else if (CHECK_CHAR('\n'))
				{
					unsigned origLastPos;
					if (CHECK_TRAILSPACE)
					{
						keepVbtmParse = true;
						goto evalVbtmParse;
					}

					/* Check for preceding blank line */
					if (lastPos < 2 || buffer[lastPos-1] != '\n' ||
						buffer[lastPos-2] != '\n')
					{
						keepVbtmParse = true;
						goto evalVbtmParse;
					}

					curPos++; /* Skip the newline character */
					CHECK_SIZE_CLEANUP(vbtmProcHook);

					/* Check for entry header line */
					origLastPos = lastPos;
					lastPos = curPos;
					READ_ENTDIV();
					CHECK_SIZE_CLEANUP(vbtmProcHook);
					if (curPos - lastPos != 70)
						keepVbtmParse = true;
					else
						keepVbtmParse = false;
					lastPos = origLastPos;
				}

evalVbtmParse:
				if (keepVbtmParse == true)
				{
					/* Disregard the special meaning */
					curPos = lastPos;
					curLine += lineDisp;
					lineDisp = 0;
					/* Write blank lines */
					{
					unsigned lastStrlen;
					unsigned i;
					lastStrlen = strlen(lastString);
					lastString = (char*)realloc(lastString,
												lastStrlen + blankLines + 1);
					for (i = lastStrlen; i < lastStrlen + blankLines; i++)
						lastString[i] = '\n';
					lastString[lastStrlen+blankLines] = '\0';
					}
					goto readVbtmParagraph;
				}
				else
				{
					/* Verbatim parsing should end here */
					curPos = lastPos;
				}

vbtmProcHook:	/* Data processing hook */
				cePtr->attribs[curAttrib].type = FA_VERBATIM;
				cePtr->attribs[curAttrib].value =
					(char*)malloc(strlen(lastString) + 1);
				strcpy(cePtr->attribs[curAttrib].value, lastString);
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
				lastPos = curPos;
				READ_LINE();
				if (curPos >= dataSize)
					mustCleanup = true;
				if (mustCleanup == false && CHECK_TRAILSPACE)
				{
					errorDesc = "Extra space on end of line.";
					break; /* This is an error */
				}
			}
			SAVE_STRING();
			curPos++; /* Skip the newline character */
			CHECK_SIZE_CLEANUP(attribProcHook);
			curLine++;

			/* Pre-data processing hook */
			if (curPos < dataSize - 1 && buffer[curPos] == ' ')
				cePtr->attribs[curAttrib].type = FA_MULTILINE;

			/* Read a multiline attribute (if exists) */
			while (buffer[curPos] == ' ')
			{
				unsigned lastStrlen;

				/* The leading space is included (it is not skipped
				   like usual) */
				{
					bool mustCleanup;
					mustCleanup = false;
					lastPos = curPos;
					READ_LINE();
					if (curPos >= dataSize)
						mustCleanup = true;
					if (mustCleanup == false && CHECK_TRAILSPACE)
					{
						errorDesc = "Extra space on end of line.";
						break; /* This is an error. */
					}
				}
				/* Append string */
				lastStrlen = strlen(lastString);
				lastString = (char*)realloc(lastString,
											lastStrlen + curPos - lastPos + 1);
				strncpy(&lastString[lastStrlen], &buffer[lastPos],
						curPos - lastPos);
				lastString[lastStrlen+curPos-lastPos] = '\0';
				curPos++; /* Skip the newline character */
				CHECK_SIZE_CLEANUP(attribProcHook);
				curLine++;
			}

attribProcHook: /* Data processing hook */
			cePtr->attribs[curAttrib].value =
				(char*)malloc(strlen(lastString) + 1);
			strcpy(cePtr->attribs[curAttrib].value, lastString);
		}
		else if (CHECK_CHAR('\n') && lastPos == curPos)
		{
			curPos++; /* Skip the blank line */
			CHECK_SIZE();
			curLine++;
		}
		else if (buffer[lastPos] == '*' && buffer[curPos-1] == '*')
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
				(char*)malloc(strlen(lastString) + 1);
			strcpy(cePtr->attribs[curAttrib].name, lastString);
			cePtr->attribs[curAttrib].value = NULL;
			cePtr->attribs[curAttrib].type = FA_HEADER;

			if (CHECK_TRAILSPACE)
			{
				errorDesc = "Extra space on end of line.";
				break;
			}
			curPos++; /* Skip the newline character */
			errorDesc = "Empty subheaders are invalid.";
			CHECK_SIZE_ERROR();
			curLine++;
		}
		else if (CHECK_CHAR('\n')) /* Redundant check, last possible case */
		{
			if (CHECK_TRAILSPACE)
			{
				errorDesc = "Extra space on end of line.";
				break; /* This is an error */
			}

			/* Check for preceding blank line */
			errorDesc = "Missing blank line before entry header.";
			if (lastPos < 2 || buffer[lastPos-1] != '\n' ||
				buffer[lastPos-2] != '\n')
				break;

			curPos++; /* Skip the newline character */
			errorDesc = "Missing entry header line.";
			CHECK_SIZE_ERROR();
			curLine++;

			/* Check for entry header line */
			lastPos = curPos;
			READ_ENTDIV();
			errorDesc = "Missing attributes in entry.";
			CHECK_SIZE_ERROR();
			if (curPos - lastPos != 70)
			{
				/* Better error reporting */
				if (buffer[curPos] == '-')
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
			SKIP_CHAR('\n');
			errorDesc = "Missing attributes in entry.";
			CHECK_SIZE_ERROR();
			curLine++;

			errorDesc = "Missing blank line.";
			SKIP_CHAR('\n');
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
			entries[curEntry].name = (char*)malloc(strlen(lastString) + 1);
			strcpy(entries[curEntry].name, lastString);
			entries[curEntry].numAttribs = 0;
			entries[curEntry].attribs = NULL;
			curAttrib = 0;
		}
	}
	free(lastString);

	if (curPos < dataSize || sizeError == true)
	{
		printf("Parse error on line %u. %s\n", curLine, errorDesc);
		return false;
	}

#undef READ_LINE
#undef SAVE_STRING
#undef CHECK_SIZE
#undef CHECK_SIZE_CLEANUP
#undef CHECK_SIZE_ERROR
#undef CHECK_CHAR
#undef SKIP_CHAR
#undef READ_ENTDIV
#undef READ_ATTRIB
#undef CHECK_TRAILSPACE

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
