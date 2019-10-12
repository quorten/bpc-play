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
	curPos = 0;
	lastPos = 0;
	numNewlines = 0;
	lineBegSet = false;
	tryBLToken = false;
	tryEntLine = false;
	foundEntLine = false;
	inEntryHeader = false;
	inVerbatim = false;

	/* First read the header */
	{
		const char orgHeader[] =
"Organizer Version 1.1\n
Purpose: File system cataloging by metadata\n\n";
		unsigned headLen;

		headLen = strlen(orgHeader);
		if (dataSize < headLen)
			return false;
		if (strncmp(data, orgHeader, headLen) != 0)
			return false;
		curPos += headLen;
	}

	/* Parser loop */
	while (curPos < dataSize)
	{
		/* Constraint testers */
		if (curPos >= dataSize)
			break;

		/* Switch setters */
		if (buffer[curPos] == '\n' && tryBLToken == false)
		{
			numNewlines++;
			if (lineBegSet == true)
			{
			}
		}
		else
			numNewlines = 0;
		if (numNewlines == 0 && lineBegSet == false)
		{
			lastPos = curPos;
			lineBegSet = true;
		}
		if (buffer[curPos] == ':')
		{
			/* We found an attribute */
		}
		if (tryBLToken == true)
		{
			lastPos = curPos;
			tryEntLine = true;
			tryBLToken = false;
		}
		if (tryEntLine == true)
		{
			if (numNewlines != 0)
				tryEntLine = false;
			else if (buffer[curPos] != '-')
				tryEntLine = false;
		}
		if (curPos - lastPos == 70)
			foundEntLine = true;

		/* Switch handlers */
		if (numNewlines == 2)
		{
			tryBLToken = true;
			curPos++;
			continue;
		}

		/* First try to read a line */
		/* If there is a colon in the line */
		/* Mark the position and read the value */
		/* If there is no colon */
		/* Look for the next line being an entry marker */
		/* If this fails, stop */

		/* Standard motion */
		curPos++;
	}
}
