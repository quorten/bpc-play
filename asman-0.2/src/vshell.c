/* vshell.c -- Simulate an interactive shell on an in-memory file
   system.  This is mainly useful for testing the internal code
   libraries to verify that they work correctly.

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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bool.h"
#include "xmalloc.h"
#include "cmdline.h"
#include "shenv.h"
#include "vfs.h"

FSNode *vfs_root;
FSNode *vfs_cwd;

/* This is used to control file creation semantics, see
   `proc_touch'.  */
unsigned int fast_import_mode = 0;

/* The following functions are compiled with VFS wrappers from other
   source files.  */
int proc_lsf(int argc, char *argv[]);
int proc_duhcs(int argc, char *argv[]);
int proc_treegen(int argc, char *argv[]);
int proc_delempty(int argc, char *argv[]);
int proc_szsave(int argc, char *argv[]);
int proc_timsave(int argc, char *argv[]);

/* The following functions are defined within this source file.  */
int proc_echo(int argc, char *argv[]);
int proc_set(int argc, char *argv[]);
int proc_stat(int argc, char *argv[]);
int proc_pwd(int argc, char *argv[]);
int proc_chdir(int argc, char *argv[]);
int proc_mkdir(int argc, char *argv[]);
int proc_mkdirp(int argc, char *argv[]);
int proc_rmdir(int argc, char *argv[]);
int proc_touch(int argc, char *argv[]);
int proc_mv(int argc, char *argv[]);
int proc_mvt(int argc, char *argv[]);
int proc_rm(int argc, char *argv[]);
int proc_rmr(int argc, char *argv[]);
int proc_ls(int argc, char *argv[]);
int proc_lsl(int argc, char *argv[]);
int proc_truncate(int argc, char *argv[]);
int proc_vfs_fastimport(int argc, char *argv[]);
int proc_vfs_endimport(int argc, char *argv[]);

int main(void)
{
	int retval = 0;
#define NUM_CMDS 27
	const char *commands[NUM_CMDS] =
		{ "echo", "set", "stat", "pwd", "cd", "chdir", "touch",
		  "cp", "mv", "mvt", "rm", "rmr", "ln", "mkdir", "mkdirp",
		  "rmdir", "ls", "lsl", "truncate", "lsf", "duhcs", "treegen",
		  "delempty", "szsave", "timsave",
		  "vfs_fastimport", "vfs_endimport" };
	const CmdFunc cmdfuncs[NUM_CMDS] = {
		proc_echo, proc_set, proc_stat, proc_pwd, proc_chdir, proc_chdir,
		proc_touch, proc_touch, proc_mv, proc_mvt, proc_rm, proc_rmr,
		proc_touch, proc_mkdir, proc_mkdirp, proc_rmdir, proc_ls, proc_lsl,
		proc_truncate, proc_lsf, proc_duhcs, proc_treegen, proc_delempty,
	    proc_szsave, proc_timsave, proc_vfs_fastimport, proc_vfs_endimport };

	vfs_cwd = vfs_root = vfs_create();

	/* Process the commands on standard input.  */
	retval = proc_cmd_dispatch(stdin, NUM_CMDS, commands, cmdfuncs, true);

	/* Cleanup */
	vfs_destroy(vfs_root);
	return retval;
}

int proc_echo(int argc, char *argv[])
{
	unsigned i;
	for (i = 1; i < argc; i++)
	{
		fputs(argv[i], stdout);
		if (i < argc - 1)
			putchar(' ');
		else
			putchar('\n');
	}
	return 0;
}

int proc_set(int argc, char *argv[])
{
	shenv_print_vars();
	return 0;
}

int proc_stat(int argc, char *argv[])
{
	int retval = 0;
	unsigned i;
	for (i = 1; i < argc; i++)
	{
		FSNode *file = vfs_stat(vfs_root, vfs_cwd, argv[i]);
		if (file == NULL)
		{ retval = 1; continue; }
		fputs(argv[i], stdout); putchar(' ');
		if ((file->delta_mode & DM_ISDIR))
			puts("directory");
		else
			puts("file");
	}
	return retval;
	return 0;
}

int proc_pwd(int argc, char *argv[])
{
	char *cwd_str = vfs_getcwd(vfs_root, vfs_cwd, NULL, 0);
	puts(cwd_str);
	xfree(cwd_str);
	return 0;
}

int proc_chdir(int argc, char *argv[])
{
	FSNode *new_cwd;
	if (argc < 2)
		return 1;
	new_cwd = vfs_chdir(vfs_root, vfs_cwd, argv[1]);
	if (new_cwd == NULL)
		return 1;
	vfs_cwd = new_cwd;
	return 0;
}

int proc_mkdir(int argc, char *argv[])
{
	int retval = 0;
	unsigned i;
	for (i = 1; i < argc; i++)
		retval |= (vfs_mkdir(vfs_root, vfs_cwd, argv[i], 0) ? 1 : 0);
	return retval;
}

/* `mkdir -p' */
int proc_mkdirp(int argc, char *argv[])
{
	int retval = 0;
	unsigned i;
	for (i = 1; i < argc; i++)
	{
		retval |= (vfs_mkdirp(vfs_root, vfs_cwd, argv[i], 0) ? 1 : 0);
	}
	return retval;
}

int proc_rmdir(int argc, char *argv[])
{
	int retval = 0;
	unsigned i;
	for (i = 1; i < argc; i++)
		retval |= (vfs_rmdir(vfs_root, vfs_cwd, argv[i]) ? 1 : 0);
	return retval;
}

int proc_touch(int argc, char *argv[])
{
	int retval = 0;
	unsigned i = 1;
	FSint64 mtime = 0;
	if (argc >= 2)
	{
		/* `tm_isdst' finicky... use negative for local time input
		   when you are not better informed on Daylight Savings
		   Time.  */
		if (!strcmp(argv[1], "-t"))
		{
			struct tm date;
			if (argc < 3)
				return 1;
			memset(&date, 0, sizeof(date));
			sscanf(argv[2], "%04d%02d%02d%02d%02d.%02d",
				   &date.tm_year, &date.tm_mon, &date.tm_mday,
				   &date.tm_hour, &date.tm_min, &date.tm_sec);
			date.tm_year -= 1900;
			date.tm_mon -= 1;
			date.tm_isdst = -1;
			mtime = mktime(&date);
			i += 2;
		}
		else if (!strcmp(argv[1], "-d"))
		{
			struct tm date;
			if (argc < 3)
				return 1;
			memset(&date, 0, sizeof(date));
			sscanf(argv[2], "%d-%02d-%02d %02d:%02d:%02d",
				   &date.tm_year, &date.tm_mon, &date.tm_mday,
				   &date.tm_hour, &date.tm_min, &date.tm_sec);
			date.tm_year -= 1900;
			date.tm_mon -= 1;
			date.tm_isdst = -1;
			mtime = mktime(&date);
			i += 2;
		}
		else if (!strcmp(argv[1], "--"))
			i++;
	}
	for (; i < argc; i++)
	{
		/* We have this weird conditional in here so that a `touch'
		   with no date is treated as a `creat', but with one behaves
		   more like a conventional touch.  */
		int ucreat_retval = 0;
		/* If `fast_import_mode' is set to 1, only attempt to create
		   files when there is not a timestamp provided.  Our touch
		   logic can be problematic in fast import mode since fast
		   import doesn't check for duplicate filenames during file
		   creation.  */
		if (fast_import_mode != 1 || mtime == 0)
			ucreat_retval =
				(vfs_ucreat(vfs_root, vfs_cwd, argv[i], 0) ? 1 : 0);
		if (mtime != 0)
			retval |= (vfs_utime(vfs_root, vfs_cwd,
								 argv[i], &mtime) ? 1 : 0);
		else
			retval |= ucreat_retval;
	}
	return retval;
}

int proc_mv(int argc, char *argv[])
{
	if (argc != 3)
		return 1;
	return (vfs_rename(vfs_root, vfs_cwd, argv[1], argv[2]) ? 1 : 0);
}

/* `mv' with `-t' option implied before the last argument.  This is
   similar to how the classic Unix `mv' behaves some of the time, but
   is more consistent.  */
int proc_mvt(int argc, char *argv[])
{
	int retval = 0;
	char *dest_dir;
	unsigned dest_dir_len;
	unsigned i;
	unsigned argc_1;
	if (argc < 3)
		return 1;
	argc_1 = argc - 1;
	dest_dir = argv[argc_1];
	dest_dir_len = strlen(dest_dir);
	{ /* Verify that the destination is in fact a directory.  */
		FSNode *dnode = vfs_stat(vfs_root, vfs_cwd, dest_dir);
		if (dnode == NULL || !(dnode->delta_mode & DM_ISDIR))
			return 1;
	}
	for (i = 1; i < argc_1; i++)
	{
		const char *src_bname;
		char *dest_file;
		strip_trail_slash(argv[i]);
		src_bname = vfs_basename(argv[i]);
		if (src_bname == NULL)
		{ retval = 1; continue; }
		dest_file = xmalloc(sizeof(char) *
							(dest_dir_len + 1 + strlen(src_bname) + 1));
		strcpy(dest_file, dest_dir);
		dest_file[dest_dir_len] = '/';
		strcpy(dest_file + dest_dir_len + 1, src_bname);
		retval |= (vfs_rename(vfs_root, vfs_cwd, argv[i], dest_file) ? 1 : 0);
		xfree(dest_file);
	}
	return retval;
}

int proc_rm(int argc, char *argv[])
{
	int retval = 0;
	unsigned i;
	for (i = 1; i < argc; i++)
		retval |= (vfs_unlink(vfs_root, vfs_cwd, argv[i]) ? 1 : 0);
	return retval;
}

/* Like `rm -r', except that if there are any errors while attempting
   to remove subtrees, the recursive removal of that larger tree is
   aborted.  (That should never happen in our simple VFS.)  */
int proc_rmr(int argc, char *argv[])
{
	int retval = 0;
	unsigned i;
	if (argc < 2)
		return 1;
	for (i = 1; i < argc; i++)
	{
		if (vfs_rmr(vfs_root, vfs_cwd, argv[i]) == -1)
			retval |= 1;
	}
	return retval;
}

int proc_ls(int argc, char *argv[])
{
	const char *path = ".";
	VFSDir *dir_handle;
	struct vfs_dirent *de;
	if (argc >= 2)
		path = argv[1];
	dir_handle = vfs_opendir(vfs_root, vfs_cwd, path);
	if (dir_handle == NULL)
		return 1;
	while (de = vfs_readdir(dir_handle))
		puts(de->d_name);
	vfs_closedir(dir_handle);
	return 0;
}

/* `ls -l' */
int proc_lsl(int argc, char *argv[])
{
	const char *path = ".";
	int retval = 0;
	char *old_cwd = NULL;
	VFSDir *dir_handle;
	struct vfs_dirent *de;
	if (argc >= 2)
	{
		old_cwd = vfs_getcwd(vfs_root, vfs_cwd, NULL, 0);
		if (proc_chdir(argc, argv) == 1)
		{ retval = 1; goto cleanup; }
	}
	dir_handle = vfs_opendir(vfs_root, vfs_cwd, path);
	if (dir_handle == NULL)
	{ retval = 1; goto cleanup; }
	while (de = vfs_readdir(dir_handle))
	{
		FSNode *file = vfs_stat (vfs_root, vfs_cwd, de->d_name);
		time_t fmt_time = file->st__mtime;
		struct tm *date = localtime(&fmt_time);
		date->tm_sec = (date->tm_sec >= 60) ? 59 : date->tm_sec;
		printf("%c %20lld %d-%02d-%02d %02d:%02d:%02d %s\n",
			   (file->delta_mode & DM_ISDIR) ? 'd' : '-',
			   (file->delta_mode & DM_ISDIR)
			   ?  file->data.files.len : file->data.data64.len,
			   1900 + date->tm_year, date->tm_mon + 1, date->tm_mday,
			   date->tm_hour, date->tm_min, date->tm_sec,
			   de->d_name);
	}
	vfs_closedir(dir_handle);
cleanup:
	if (old_cwd != NULL)
	{
		char *cwd_argv[] = { "", old_cwd };
		proc_chdir(2, cwd_argv);
	}
	xfree(old_cwd);
	return retval;
}

int proc_truncate(int argc, char *argv[])
{
	int retval = 0;
	unsigned i;
	FSint64 size;
	if (argc < 4 || strcmp(argv[1], "-s"))
		return 1;
	sscanf(argv[2], "%lld", &size);
	for (i = 3; i < argc; i++)
	{
		if (vfs_truncate(vfs_root, vfs_cwd, argv[i], size) == -1)
			retval |= 1;
	}
	return retval;
}

int proc_vfs_fastimport(int argc, char *argv[])
{
	g_vfs_sorted = false;
	g_vfs_fast_insert = true;
	if (argc == 2)
	{
		if (!strcmp(argv[1], "1"))
			fast_import_mode = 1;
	}
	return 0;
}

int proc_vfs_endimport(int argc, char *argv[])
{
	g_vfs_sorted = true;
	g_vfs_fast_insert = false;
	fast_import_mode = 0;
	fsnode_sort_names(vfs_root);
	return 0;
}
