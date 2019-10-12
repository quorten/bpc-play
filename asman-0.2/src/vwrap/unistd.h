#ifndef UNISTD_H
#define UNISTD_H

#include "../shenv.h"

#include "../vfs.h"
#include "vfs_ctx.h"

/* Note: We must be careful to never override the link names of the
   standard C library functions, otherwise we loose our ability to use
   the real ones.  */
#define stat vwrap_stat
#define stat64 vwrap_stat

struct stat
{
	FSint32 st_mode;
	FSint64 st_size;
	time_t st_mtime;
	FSint32 st_blksize;
};

#define S_ISREG(mode) (!((mode) & DM_ISDIR))
#define S_ISDIR(mode) ((mode) & DM_ISDIR)
#define S_ISLNK(mode) ((mode) & DM_ISLNK)

#define getcwd(buf, size) vfs_getcwd(vfs_root, vfs_cwd, buf, size)			
#define lstat vwrap_lstat
#define chdir vwrap_chdir

/* Including `getenv()' etc. in `unistd.h' is a hack, but it works for
   our limited set of programs.  */
#define getenv shenv_getenv
#define setenv shenv_setenv
#define unsetenv shenv_unsetenv

int stat(const char *path, struct stat *sbuf);
int lstat(const char *path, struct stat *sbuf);
int chdir(const char *path);

#endif /* not UNISTD_H */
