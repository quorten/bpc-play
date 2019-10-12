/* Text manipulation functions.  */

/* Macros to assist selecting a subset of a C string by temporarily
   splicing it in place.  */
#define STR_BEGIN_SPLICE(string, position) \
	{ \
		char *splice_str; \
		unsigned char_pos; \
		char last_char; \
		splice_str = string; \
		char_pos = position; \
		last_char = splice_str[char_pos]; \
		splice_str[char_pos] = '\0';
#define STR_END_SPLICE() \
		splice_str[char_pos] = last_char; \
	}

void splice_test()
{
	char *string = "Hello, there.  How are you doing?";
	puts(string);
	STR_BEGIN_SPLICE(string, 13);
	puts(string);
	STR_END_SPLICE();
	puts(string);
}

/* Calculate the amount a string would expand if its newlines were
   converted to DOS newlines.  `size' is the length of the string, not
   including the null character.  */
unsigned calc_dos_exp(const char *buffer, unsigned size)
{
	unsigned expansion = 0;
	unsigned i;
	for (i = 0; i < size; i++)
	{
		if (buffer[i] == '\n')
			expansion++;
		else if (buffer[i] == '\r' && buffer[i+1] != '\n')
			expansion++, i++;
	}
	return expansion;
}

/* Count linefeed ('\n') characters within a string.  `size' is the
   length of the string, not including the null character.  */
unsigned count_newlines(const char *buffer, unsigned size)
{
	unsigned num_newlines;
	unsigned i;
	for (i = 0; i < size; i++)
	{
		if (buffer[i] == '\n')
			num_newlines++;
	}
	return num_newlines;
}

/* Convert newlines to DOS newlines by expanding the string in place.
   The conversion is done by counting backwards from the end of the
   string to the beginning and moving each chunk of memory only once.
   The buffer must be big enough to hold the resulting string.  `size'
   is the length of the string, not including the null character, and
   `new_size' is the length of the converted string.  `new_size' is
   used to determine where to start counting backwards.  The null
   character is appended automatically.  */
void nl_to_dos(char *buffer, unsigned size, unsigned new_size)
{
	unsigned i, j;
	i = size; j = new_size;
	buffer[new_size] = '\0';
	do
	{
		if (i > 1 && buffer[i-1] == '\n' && buffer[i-2] == '\r')
			buffer[--j] = buffer[--i];
		else if (buffer[i-1] == '\r' || buffer[i-1] == '\n')
		{
			buffer[--j] = '\n';
			buffer[--j] = '\r';
			--i;
			continue;
		}
		buffer[--j] = buffer[--i];
	} while (i > 0 && j > 0);
}

/* Convert a string to Mac newlines by modifying the string in place.
   `size' is the length of the string, not including the null
   character.  The new size of the string is returned, which may have
   shrunken.  The null character is appended automatically.  */
unsigned nl_to_mac(char *buffer, unsigned size)
{
	unsigned i, j;
	for (i = 0, j = 0; j < size; i++, j++)
	{
		if (buffer[i] == '\r' && i + 1 < size && buffer[i+1] == '\n')
		{
			j++;
			buffer[i] = buffer[j];
		}
		else if (buffer[i] == '\n')
			buffer[i] = '\r';
		else
			buffer[i] = buffer[j];
	}
	buffer[i] = '\0';
	return i;
}

/* Translates a text buffer to Unix line endings in place.  Returns
   the new size of the data, which may have shrunken.  `size' is the
   length of the string, not including the null character.  */
unsigned nl_to_unix(char *buffer, unsigned size)
{
	unsigned i, j;
	for (i = 0, j = 0; i < size; i++, j++)
	{
		if (buffer[i] != '\r')
			buffer[j] = buffer[i];
		else
		{
			if (i + 1 < size && buffer[i+1] == '\n') /* CR+LF */
				i++;
			buffer[j] = '\n';
		}
	}
	buffer[j] = '\0';
	return j;
}

/* Copy a string and convert newlines to DOS newlines during the copy.
   The destination memory address must be big enough to hold the
   resulting string.  `size' refers to the length of the source
   string, not including the null character.  The null character is
   automatically appended.  */
void cp_to_dos(char *src, char *dest, unsigned size)
{
	unsigned i, j;
	for (i = 0, j = 0; i < size; i++, j++)
	{
		if (src[i] == '\n' ||
			(src[i] == '\r' && i + 1 < size && src[i+1] != '\n'))
		{
			dest[j++] = '\r';
			dest[j] = '\n';
		}
		else
			dest[j] = src[i];
	}
	dest[j] = '\0';
}

/* Copy a string and convert newlines to Mac newlines during the copy.
   The new size of the string is returned.  The null character is
   automatically appended.  */
unsigned cp_to_mac(char *src, char *dest, unsigned size)
{
	unsigned i, j;
	for (i = 0, j = 0; i < size; i++, j++)
	{
		if (src[i] == '\r' && i + 1 < size && src[i+1] == '\n')
		{
			dest[j] = '\r';
			i++;
		}
		else if (src[i] == '\n')
			dest[j] = '\r';
		else
			dest[j] = src[i];
	}
	dest[j] = '\0';
	return j;
}

/* Copy a string and convert newlines to Unix newlines during the
   copy.  The new size of the string is returned.  The null character
   is automatically appended.  */
unsigned cp_to_unix(char *, char *dest, unsigned size)
{
	unsigned i, j;
	for (i = 0, j = 0; i < size; i++, j++)
	{
		if (src[i] == '\r')
		{
			dest[j] = '\n';
			if (i + 1 < size && src[i+1] == '\n')
				i++;
		}
		else
			dest[j] = src[i];
	}
	dest[j] = '\0';
	return j;
}

/* Output a string through a stream and convert newlines to DOS
   newlines during the transaction.  */
void putdoss(char *buffer, unsigned size, FILE *fp)
{
	unsigned i;
	for (i = 0; i < size; i++)
	{
		if (buffer[i] == '\n')
		{
			fputc('\r', fp);
			fputs('\n', fp);
		}
		else if (buffer[i] == '\r')
		{
			fputc('\r', fp);
			fputc('\n', fp);
			if (i + 1 < size && buffer[i+1] == '\n')
				i++;
		}
		else
			fputc(buffer[i], fp);
	}
}

/* Output a string through a stream and convert newlines to Mac
   newlines during the transaction.  */
void putmacs(char *buffer, unsigned size, FILE *fp)
{
	unsigned i;
	for (i = 0; i < size; i++)
	{
		if (buffer[i] == '\n')
			fputc('\r', fp);
		else if (buffer[i] == '\r')
		{
			fputc('\r', fp);
			if (i + 1 < size && buffer[i+1] == '\n')
				i++;
		}
		else
			fputc(buffer[i], fp);
	}
}

/* Output a string through a stream and convert newlines to Unix
   newlines during the transaction.  `putps' is for Put Proper
   String.  */
void putps(char *buffer, unsigned size, FILE *fp)
{
	unsigned i;
	for (i = 0; i < size; i++)
	{
		if (buffer[i] == '\n')
			fputc('\n', fp);
		else if (buffer[i] == '\r')
		{
			fputc('\n', fp);
			if (i + 1 < size && buffer[i+1] == '\n')
				i++;
		}
		else
			fputc(buffer[i], fp);
	}
}

/* Count the number of lines within a text buffer.  */
unsigned count_lines(char *buffer, unsigned size)
{
	unsigned num_lines = 1;
	unsigned i;
	for (i = 0; i < size; i++)
	{
		if (buffer[i] == '\n')
			num_lines++;
	}
	return num_lines;
}

/* Add a prefix string to the beginning of every line by modifying the
   buffer in place.  The buffer must be large enough to hold the
   resulting string.  If `do_empty_lines' is `true', then empty lines
   will have the prefix added to them too; otherwise, only lines any
   other characters than the newline character will be prefixed.  */
void prefix_lines(char *buffer, unsigned size,
				  char *prefix, bool do_empty_lines)
{
	unsigned i;
	for (i = 0; i < size; i++)
	{
	}
}

/* Copy a text buffer and add a prefix to the beginning of every
   line.  */

/* Remove whitespace from lines that contain no non-whitespace
   characters on them.  Returns the new size of the text buffer, which
   may have shrunken.  The null character is appended
   automatically.  */
unsigned clean_blank_lines(char *buffer, unsigned size)
{
	unsigned i, j;
	for (i = 0, j = 0; i < size; i++, j++)
	{
		if (buffer[i] == '\n')
		{
			unsigned line_start;
			buffer[j++] = buffer[i++];
			line_start = i;
			while (i < size && (buffer[i] == ' ' || buffer[i] == '\t'))
				i++;
			if (i >= size)
				break;
			if (buffer[i] == '\n')
			{
				/* We have a line with only whitespace on it.  Just
				   skip the whitespace (already done).  */
				/* If necessary, back up so that the newline character
				   will get picked up for the next cycle.  */
				if (i > line_start)
					i--;
				continue;
			}
		}
		buffer[j] = buffer[i];
	}
	buffer[j] = '\0';
	return j;
}

/* Format a text buffer in-place into a virtual paper image.  A
   virtual paper image has a fixed number of characters per line.
   Thus, lines are not terminated by newline characters, but are
   instead delimited by memory offsets.  The text buffer must be large
   enough to hold the resulting image.  */
void vpage_create(char *buffer, unsigned size, unsigned line_len)
{
	unsigned i, j;
	for (i = 0, j = 0; i < size; i++)
	{
	}
}

/* Find the end of a given line within a virtual paper image.  */
void vpage_line_end(char *buffer, unsigned line_len)
{
}

/* Convert a virtual paper image to a text buffer.  Returns the size
   of the text buffer.  The null character is appended
   automatically.  */
unsigned vpage_to_textbuf(char *buffer, unsigned size)
{
}

/* Wrap words by modifying whitespace in place.  Data can optionally
   be saved to allow the word wrapping to be undone.  Words are
   wrapped at the given delimiter characters.  Backslashes can also be
   appended to wrapped lines.  Wrapped lines can also be indented.  */
void wrap_words(char *buffer, unsigned size, unsigned row_width,
				unsigned tab_width, unsigned *wrap_places)
{
	unsigned col_pos = 0;
	unsigned last_space = (unsigned)-1;
	unsigned num_breaks = 0;
	unsigned i;
	for (i = 0; i < size; i++)
	{
		switch (buffer[i])
		{
		case '\t':
			last_space = i;
			col_pos += tab_width;
			break;
		case ' ':
			last_space = i;
		default:
			col_pos++;
			break;
		}
		if (col_pos >= row_width && last_space != (unsigned)-1)
		{
			buffer[last_space] = '\n';
			col_pos = 0;
			wrap_places[num_breaks++] = last_space;
		}
	}
}

/* Change newlines added by word wrap back to spaces. */
void unwrap_words(char *buffer, unsigned *wrap_places, unsigned num_breaks)
{
	unsigned i;
	for (i = 0; i < num_breaks; i++)
		buffer[wrap_places[i]] = ' ';
}

/* Convert all possible spaces for which there are at least two spaces
   in a row to tabs.  The new size of the text is returned, which may
   have shrunken.  The null character is appended automatically.  */
unsigned tabify(char *buffer, unsigned size, unsigned tab_width)
{
	unsigned i = 0, j = 0;
	unsigned tab_modulo = 0;
	unsigned spaces_in_row = 0;
	while (i < size)
	{
		if (buffer[i] == ' ')
			spaces_in_row++;
		else
			spaces_in_row = 0;
		buffer[j] = buffer[i];
		if (buffer[i] == '\t')
		{
			tab_modulo = 0;
			if (spaces_in_row > 0)
			{
				/* Delete any leading spaces.  */
				j -= spaces_in_row;
				buffer[j++] = '\t';
				spaces_in_row = 0;
			}
		}
		else
			tab_modulo++;
		i++, j++;
		if (tab_modulo >= tab_width)
		{
			tab_modulo = 0;
			if (spaces_in_row > 1)
			{
				j -= spaces_in_row;
				buffer[j++] = '\t';
				spaces_in_row = 0;
			}
		}
	}
	buffer[j] = '\0';
	return j;
}

/* Convert spaces at the left margin to tabs when there are at least
   two spaces in a row.  The new size of the text is returned, which
   may have shrunken.  The null character is appended
   automatically.  */
unsigned tabify_margin(char *buffer, unsigned size, unsigned tab_width)
{
	unsigned i = 0, j = 0;
	unsigned spaces_in_row = 0;
	bool at_margin = true;
	while (i < size)
	{
		if (buffer[i] == '\n')
			at_margin = true;
		else if (buffer[i] != ' ' && buffer[i] != '\t')
			at_margin = false;
		else if (at_margin == true)
		{
			if (buffer[i] == ' ')
				spaces_in_row++;
			if (buffer[i] == '\t' && spaces_in_row > 0)
			{
				/* Delete any leading spaces.  */
				j -= spaces_in_row;
				buffer[j++] = '\t';
				spaces_in_row = 0;
			}
		}
		i++, j++;
		if (spaces_in_row >= tab_width)
		{
			j -= spaces_in_row;
			buffer[j++] = '\t';
			spaces_in_row = 0;
		}
	}
	buffer[j] = '\0';
	return j;
}

/* Convert tabs to spaces in-place.  */

/* Copy a text buffer and convert tabs to spaces.  */

/* Convert a table that contains spaces for alignment into a
   delimiter-separated table.  */

/* Convert comma-separated values into a Unix-style
   delimiter-separated table.  */

/* Compute the maximum column length for a table columns.  */

/* Convert a delimiter-separated table into a space separated table
   with the given column widths.  */

unsigned count_pipes(char *buffer, unsigned size)
{
	unsigned num_pipes = 0;
	unsigned i;
	for (i = 0; i < size; i++)
	{
		if (buffer[i] == '|')
			num_pipes++;
	}
	return num_pipes;
}

/* Read column widths by parsing a string with '|' characters.  */
void read_col_widths(char *buffer, unsigned size, unsigned *col_widths)
{
	unsigned cur_col = 0;
	unsigned i;
	for (i = 0; i < size; i++)
	{
		if (buffer[i] == '|')
			col_widths[cur_col++] = i;
	}
}
