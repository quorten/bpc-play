/* Parse a file system summary file.
   There are also hierarchal text files. */

	/* Open the file */
	/* Read the contents */
	/* Transform end-of-lines */
	/* Parse the DATA! */
	/* Store results in array */
	/* Save as a cache */
	/* Load only a cache */

bool ParseFile(char* data, unsigned dataSize)
{
	unsigned curPos;
	unsigned lastPos;
	unsigned numNewlines;
	bool lineBegSet = false; /* Did we save the beginning of a line? */
	bool tryBLToken; /* Look for a token that is preceded with a blank line */
	bool tryEntLine; /* Look for dashed line */
	bool foundEntLine;
	bool inEntryHeader;
	bool inVerbatim;
	bool foundAttrib;
	bool spaceAtLnBeg;
	curPos = 0;
	lastPos = 0;
	numNewlines = 0;
	lineBegSet = false;
	tryBLToken = false;
	tryEntLine = false;
	foundEntLine = false;
	inEntryHeader = false;
	inVerbatim = false;
	foundAttrib = false;
	spaceAtLnBeg = false;

	/* First read the header */
	{
		const char orgHeader[] =
"Organizer Version 1.1\n\
Purpose: File system cataloging by metadata\n\n";
		unsigned headLen;

		headLen = strlen(orgHeader);
		if (dataSize < headLen)
			return false;
		if (strncmp(data, orgHeader, headLen) != 0)
			return false;
		curPos += headLen;
	}

#define READ_LINE() \
	while (curPos < dataSize && buffer[curPos] != '\n') \
		curPos++;
#define CHECK_SIZE() \
	if (curPos >= dataSize) \
		break;
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
#define CHECK_TRAILSPACE() \
	if (buffer[curPos-1] == ' ') \
		break;

	/* Parser loop */
	while (curPos < dataSize)
	{
		lastPos = curPos;
		READ_ATTRIB();
		CHECK_SIZE();
		if (CHECK_CHAR(':'))
		{
			/* Read the value */
			curPos++; /* Skip the colon */
			CHECK_SIZE();

			if (buffer[curPos] == '\n') /* Read a verbatim attribute */
			{
				continue;
			}

			SKIP_CHAR(' ');
			CHECK_SIZE();

			/* Read until the end of the line */
			READ_LINE();
			CHECK_SIZE();
			curPos++; /* Skip the newline character */
			CHECK_SIZE();

			if (buffer[curPos] == ' ') /* Read a multiline attribute */
			{
				continue;
			}

			/* Make sure to always check for no trailing spaces */
		}
		else if (CHECK_CHAR('\n')) /* Redundant check, last possible case */
		{
			/* Check for preceding blank line */
			if (lastPos >= 2 && buffer[lastPos-1] == '\n' &&
				buffer[lastPos-2] == '\n')
			{
				/* Check for entry divider */
				lastPos = curPos;
				READ_ENTDIV();
				CHECK_SIZE();
				if (curPos - lastPos != 70)
					break;

				SKIP_CHAR('\n');
				CHECK_SIZE();

				SKIP_CHAR('\n');
				CHECK_SIZE();
			}
			else
				break;
		}

		/* First read the line */
		lastPos = curPos;
		READ_LINE();
		CHECK_SIZE();
		SKIP_CHAR('\n');
		CHECK_SIZE();

		/* Read the entry header marker */
		lastPos = curPos;
		READ_ENTDIV();
		CHECK_SIZE();
		if (curPos - lastPos != 70)
			break;

		SKIP_CHAR('\n');
		CHECK_SIZE();

		SKIP_CHAR('\n');
		CHECK_SIZE();

		/* Read the entries */
		lastPos = curPos;
		READ_ATTRIB();
		CHECK_SIZE();

		curPos++; /* Skip the colon */
		CHECK_SIZE();

		if (buffer[curPos] == '\n') /* Read a verbatim attribute */
		{
			continue;
		}

		SKIP_CHAR(' ');
		CHECK_SIZE();

		/* Read until the end of the line */
		READ_LINE();
		CHECK_SIZE();
		curPos++; /* Skip the newline character */
		CHECK_SIZE();

		if (buffer[curPos] == ' ') /* Read a multiline attribute */
		{
			continue;
		}

		/* Make sure to always check for no trailing spaces */

		SKIP_CHAR('\n');
		CHECK_SIZE();

		/* Read until the end of the line */
		READ_LINE();
		if (buffer[curPos-1] == ':')
			break;
		curPos++; /* Skip the newline character */
		CHECK_SIZE();

		/* Check the next line for entry indicator */
		/* Read the entry header marker */
		lastPos = curPos;
		READ_ENTDIV();
		CHECK_SIZE();
		if (curPos - lastPos != 70)
			break;

		SKIP_CHAR('\n');
		CHECK_SIZE();

		SKIP_CHAR('\n');
		CHECK_SIZE();

		/* If the mark is present, set a new entry */
		/* Otherwise, don't count on it */


		curPos++;
	}
}
