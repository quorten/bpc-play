/* TIGCC startup */
#define USE_TI89
#define RETURN_VALUE
#define OPTIMIZE_ROM_CALLS
#define MIN_AMS 200

#include <tigcclib.h>

void _main()
{
	int maxNum;
	ESI argptr = top_estack;

	/* Get your arguments using something like this: */
	if (RemainingArgCnt(argptr) < 1)
		fatal;
	if (GetArgType(argptr) == POSINT_TAG)
		maxNum = GetIntArg(argptr);

	/* Do your program stuff here.  */

	/* Note that you can also return types other than an integer.  */
	push_longint(0);
}
