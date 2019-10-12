/* userfd.c -- Implementation of Unix-style file descriptors
   associated with a user-mode process.

2019  */

/* What makes a PTY special???  As far as I understand, the main thing
   that makes it special is that it states that it is a tty, only one
   user can open it, and there is fancy terminal input processing
   configuration.  Otherwise, it behaves just like a pipe.  Okay, so
   it is mainly the fancy input processing and other
   configuration.  */

enum UserFDType_tag
{
	UFD_NULL,
	UFD_BPIPE,
	UFD_DUMFD,
	UFD_MEMFD,
	UFD_TTY,
	UFD_PTY,
	UFD_SLIP,
	UFD_PPP,
	UFD_SOCKET,
};

#define MAX_USERFDS 128

struct UserFDShare_tag
{
	unsigned short ref_count;
	unsigned char type;
	void *data;
};
typedef struct UserFDShare_tag UserFDShare;

struct UserFD_tag
{
	unsigned offset; /* if applicable */
	UserFDShare *d; /* NULL if not mapped */
};
typedef struct UserFD_tag UserFD;

struct UserFDCtx_tag
{
	UserFD d[MAX_USERFDS];
	unsigned short len;
};
typedef struct UserFDCtx_tag UserFDCtx;

/* What about setting blocking or non-blocking I/O?  By default, all
   low-level implementations are non-blocking I/O.  Blocking I/O is
   implemented as a higher-level convenience layer... so I guess the
   configuration and implementation of that comes down to this
   abstraction layer.  */

/* That's all there is to it!  Then we simply have an abstraction
   layer to map the generic file operations to the specific routines
   based off of the type.

   The operation of this code is fairly simple.  With Coproc, the data
   structure can be bundled with the process context.  Context
   switches will therefore switch out the active mapping data
   structures.  For convenience, map to and from global variables may
   also be used.

*/
