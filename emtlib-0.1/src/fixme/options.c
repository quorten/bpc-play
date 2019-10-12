/* Centralized customization and option retrieval interface.
   This is an inferior module, meaning that it must be manually
   configured to work with other modules.

   All you have to do is specify your variables which can be
   customized in InitializeOptions().  It is preferred that you use
   the inline hook function notation.

   NOTE: Data formats for customization are of utmost importance.
   They and by default handled through the user interface standards.
   Supported data formats:

     boolean
     enumeration
     integer
     string  */

#include "core.h"

struct OptnsVar
{
	const char* name; /* Variable name */
	unsigned num; /* Unique identifier */
	void* value;
};

struct OptnsModule
{
	const char* name; /* For display */
	CustVar* vars;
};

/*

Here's how the new options module works:

* Options are defined by pointers to variables.

* Each variable has an associated binary data with it.

* Before a variable gets saved to a file, it needs to be translated
  proper.

* There are two ways to save a file: save in binary and save in text.

* Save in binary needs translation in order to ensure platform
  independence.  There are two important translations that need to be
  performed: ensure that the data is translated between host and
  network byte order, and ensure that the data is read and written
  with the right number of bytes.  This should be relatively easy.

* Save in text may require special reading and writing functions for
  certain data types.  Save in text also needs to make sure there is a
  reasonable amount of structure in the target file.

* The easiest way to save and load variables is to use printf() and
  scanf().  Maybe the only way to do it right is to write your own
  code.  Yes, I guess that is the only way to do it right.  Until
  then, I can only provide recommendations on how to do it.  Maybe I
  have to actually try doing it once myself and convince myself that
  it doesn't have to be perfect, it just has to work.

*/
