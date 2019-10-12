/* Convert a source file that uses the simple expandable
   implementation to use the fast expandable array implementation.

   This program should be given a list of *.c.in files.  Those files
   will be processed to become *.c files that use fast expandable
   arrays.  */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

/* ctlib includes */
#include <bool.h>
#include <fexparray.h>

#define CHECK_SIZE_ERROR(errLabel)				\
	if (curpos >= textsize)						\
		goto errLabel;

int main(int argc, char* argv[])
{
	FILE* fp;
	char* buffer;
	unsigned textsize;
	char* errorDesc;
	regex_t array_def;

	/* Read the whole file into memory.  The file must use Unix line
	   endings.  */
	fp = fopen("file.c.in", "rb");
	fseek(fp, 0, SEEK_END);
	textsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buffer = (char*)malloc(textsize + 1);
	fread(buffer, textsize, 1, fp);
	buffer[textsize] = '\0';
	fclose(fp);

	/* Save this regular expression.  */
	regcomp(&array_def, "[[:alnum:]]+_array", REG_EXTENDED);

	while (curpos < textsize)
	{
		/* Ignore comments.  */
		if (buffer[curpos] == '/' && buffer[curpos+1] == '*')
		{
			errorDesc = "Unclosed comment.";
			curpos += 2;
			CHECK_SIZE_ERROR();
			do
				curpos++;
			while (curpos < textsize && buffer[curpos] != '*' && buffer[curpos+1] != '/');
			CHECK_SIZE_ERROR();
			curpos++; continue;
		}

		/* Ignore single quotes.  */
		if (buffer[curpos] == '\'' && in_single_quote == false)
		{
			errorDesc = "Unclosed character quote.";
			curpos++;
			CHECK_SIZE_ERROR();
			while (curpos < textsize && !(buffer[curpos-1] != '\\' && buffer[curpos] == '\''))
				curpos++;
			CHECK_SIZE_ERROR();
			curpos++; continue;
		}

		/* Ignore strings.  */
		if (buffer[curpos] == '"')
		{
			errorDesc = "Unclosed string.";
			curpos++;
			CHECK_SIZE_ERROR();
			while (curpos < textsize && !(buffer[curpos-1] != '\\' && buffer[curpos] == '"'))
				curpos++;
			CHECK_SIZE_ERROR();
			curpos++; continue;
		}

		/* Check for scope changes.  Note that macros are not
		   expanded.  */
		if (buffer[curpos] == '{')
		{
			EA_PUSH(typename, stride, array);
			curpos++; continue;
		}
		if (buffer[curpos] == '}')
		{
			EA_POP(typename, stride, array);
			curpos++; continue;
		}

		/* Look for *_array VAR,... definitions.  */
		if (regexec(&array_def, &buffer[curpos], , , 0) == 0)
		{
			curpos++; continue;
		}

		/* Look for EA_INIT() so that the stride metainformation can
		   be filled.  */
		if (strncmp(&buffer[curpos], "EA_INIT", 7) == 0)
		{
			curpos++; continue;
		}
		/* Keep track of each array's element type.  */
		/* Keep track of scoping.  */
		/* Note that arrays passed to functions have undefined
		   behavior with this implementation.  */

		curpos++;
	}

	/* Look for all occurances of exparray functions.  */
	/* Substitute them with the proper template definitions for
	   type.  */
	/* If any exparray's are passed as function arguments, issue a
	   warning of non-determinate exparray stride and emit a version
	   of exparray.h that does not expect strides to be passed in
	   through the macro definition.  */
	return 0;
}
