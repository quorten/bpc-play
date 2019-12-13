/* Let's get it done!  */

#include <stdio.h>

/*

Test word.

Read the next word up until whitespace.
Check if this is a known word.
Get the type of the word.
If it is a built-in, execute the function.  The function
will include all the logic to read the arguments, and
also execute any recognized words.
Otherwise, execute the macro.
At the end of execution, print the output.

Otherwise, if it is not a known word, just echo to output.

So, a few more helper functions.

* Read N arguments
* Read arguments to mark.  That is, require a closing parenthesis at
  the end.
* Copy/execute body to named mark, i.e. ".end", ".endif", etc.  There
  can be more than one permitted named mark.

Okay, we can retool this a bit.

* '(' = arguments begin mark.
* ')' = arguments end mark.
* '{' = body begin mark.
* '}' = body end mark.

These can be command words that are always executed.  They check if a
context is set to expect them, and if not, parentheses act like
whitespace, braces are passed through.

*/

enum
{
  CW_NATIVE,
  CW_DEFINE,
  CW_MACRO,
} CodeWordType_tag;
typedef enum CodeWordType_tag CodeWordType;

struct CodeWord_tag
{
  unsigned char type; /* CW_NATIVE, CW_DEFINE, or CW_MACRO */
  char *name;
  union
  {
    void (*code)(void); /* native code */
    char *value; /* define value */
    void *macro; /* macro body */
  } d;
};

char *code_words[1024];
unsigned code_words_len;

void
test_word (void)
{
  char wordbuf[128];
  unsigned wordbuf_len = 0;
  int ch;
  char space_ch;
  unsigned i;

  while (wordbuf_len < 128 && (ch = getchar ()) != EOF &&
	 !isspace(ch)) {
    wordbuf[wordbuf_len++] = (char)ch;
  }
  if (wordbuf_len >= 128)
    abort ();
  wordbuf[wordbuf_len++] = '\0';
  if (isspace(ch))
    space_ch = (char)ch;

  for (i = 0; i < code_words.len; i++) {
    if (strcmp (code_words.d[i].name, wordbuf) == 0)
  }
}
