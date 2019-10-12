#ifndef DIRENT_H
#define DIRENT_H

#include "../vfs.h"
#include "vfs_ctx.h"

#define DIR VFSDir
#define dirent vfs_dirent
#define opendir(name) vfs_opendir(vfs_root, vfs_cwd, name)
#define readdir vfs_readdir
#define closedir vfs_closedir

#endif /* not DIRENT_H */
