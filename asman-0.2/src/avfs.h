/* avfs.h -- Model an attribute-based in-memory file system.

Copyright (C) 2017 Andrew Makousky

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/* Modern file systems have gone somewhat crazy over extended
   attributes (xattr) and forks.  Suffice it to say that such file
   systems can be readily modeled as nested dictionaries containing
   attribute-value pairs.  Although this modeling is interesting, it
   is not of very much practical use for most applications that work
   with filesystem data.  Attribute-value data is better worked with
   in higher level programming languages, not to mention that it is a
   lot less efficient when the same can be modeled without the use of
   attribute-value pairs.

   File system forks were always inherently non-user-friendly from the
   perspective of GUI operating system design and in practice, most
   communications software does not have support for alternate data
   streams.  In practice, directories and container files are more
   widely supported by mainstream software; therefore, they are also
   more user-friendly.  */

#ifndef AVFS_H
#define AVFS_H

typedef unsigned char uchar;
typedef void* void_ptr;
EA_TYPE(char);
EA_TYPE(uchar);
EA_TYPE(void_ptr);

typedef struct Mapping_tag Mapping;
EA_TYPE(Mapping);

struct Mapping_tag
{
	char_array name;
	void *value;
};

typedef struct SzMapping_tag SzMapping;
EA_TYPE(SzMapping);

struct SzMapping_tag
{
	char_array name;
	uchar_array value;
};

/* What is the main difference between a Mapping and an SzMapping?  An
   SzMapping, by containing the size of the data object, is supposed
   to "own" it, whereas a Mapping, by containing only a pointer to the
   data object but not its size, can "share" the data object.  An
   SzMapping could just as well have the name and data value allocated
   contiguously with the header, whereas a Mapping generally would
   not.  */

/* So how do you implement a conventional file system using these
   building blocks?  Here's an example.

   SzMapping_array root {
     "xattrs":
     SzMapping_array {
       "unix_flags": IS_DIR,
       "inode": 7293,
       "parent": 7293
     },
     "forks":
     SzMapping_array {
       "dirents":
       Mapping_array {
         "mydir": 1234,
         "myfile": 3827,
       }
     }
   };

   SzMapping_array myfile {
     "xattrs":
     SzMapping_array {
       "unix_flags": IS_FILE,
       "inode": 3827,
     },
     "forks":
     SzMapping_array {
       "data": 389a24789e23c4...
     }
   };

   As you can see here, the names of several of the mappings are
   direct indications of the type of the data.  What if you want to
   have several named objects that can be of different types?  Have
   the name mapping at one level, and a sub-level mapping to indicate
   the types.  Ultimately, however, this is really a discretionary
   decision of the programmer as to how the fields are to be
   interpreted.  Some fields can be a name, some fields can be a type,
   some fields can be a namespace and a name, some fields can be a
   namespace, name, and type all rolled into one and parsing is
   required to separate them out... it's up to the programmer as how
   to do the interpretation.

   Unfortunately, such great amounts of flexibility tend to lend
   themselves to lack of agreement, resulting in uninterpretable and
   lost data when diverse software is used.

   But, my opinion?  The above structure is a good start to creating
   compatibility with many different kinds of historic and popular
   operating systems.

   * Unix-style "open" expects to read from a "data" fork.
   * Macintosh software can read from either a "data" fork (the only
     fork on traditional Unix) or a "resource" (RSRC) fork.
   * Except for historic Unix compatibility, most modern operating
     systems do not allow Unix-style "open" on a directory.  Hence,
     directory data goes in a fork of a name other than "data".  In
     practice, directories can only be read with "opendir".
   * Because different operating system vintages support different
     file attributes, it makes the most sense to lump even these as
     extended attributes.  Hence, "xattrs" encompasses all attributes.

*/

#endif /* not AVFS_H */
