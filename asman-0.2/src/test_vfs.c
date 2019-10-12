/* test_vfs.c -- API-level tests for the VFS.

Copyright (C) 2018 Andrew Makousky

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

/* At the moment, these API-level tests focus on code coverage for
   failure cases from incorrect API use, which will otherwise never
   happen at the lowest levels when only the high-level APIs are
   used.  */

#include "bool.h"
#include "vfs.h"

void test_fail_fsnode_destroy(void)
{
	fsnode_destroy(NULL);
}

void test_fail_fsnode_find_inode(void)
{
	FSNode file;
	file.delta_mode = DM_SRC;
	fsnode_find_inode(NULL, NULL);
	fsnode_find_inode(&file, NULL);
	fsnode_find_inode(NULL, &file);
}

void test_fail_fsnode_find_name(void)
{
	FSNode file;
	char *name = "test";
	file.delta_mode = DM_SRC;
	fsnode_find_name(NULL, NULL);
	fsnode_find_name(&file, NULL);
	fsnode_find_name(NULL, name);
	fsnode_find_name(&file, name);
}

void test_fail_fsnode_sort_names(void)
{
	FSNode file;
	file.delta_mode = DM_SRC;
	fsnode_sort_names(NULL);
	fsnode_sort_names(&file);
}

void test_fail_fsnode_link(void)
{
	FSNode file;
	file.delta_mode = DM_SRC;
	fsnode_link(NULL, NULL);
	fsnode_link(&file, NULL);
	fsnode_link(NULL, &file);
	fsnode_link(&file, &file);
}

void test_fail_fsnode_unlink_index(void)
{
	FSNode file;
	file.delta_mode = DM_SRC;
	fsnode_unlink_index(NULL, (unsigned)-1);
	fsnode_unlink_index(&file, (unsigned)-1);
	fsnode_unlink_index(NULL, 0);
	fsnode_unlink_index(&file, 0);
}

void test_fail_fsnode_ucreat(void)
{
	FSNode file;
	char *name = "test";
	file.delta_mode = DM_SRC;
	fsnode_ucreat(NULL, NULL, DM_SRC);
	fsnode_ucreat(&file, NULL, DM_SRC);
	fsnode_ucreat(NULL, xstrdup(name), DM_SRC);
	fsnode_ucreat(&file, xstrdup(name), DM_SRC);
}

void test_fail_fsnode_rename_index(void)
{
	FSNode file;
	file.delta_mode = DM_SRC;
	fsnode_rename_index(NULL, (unsigned)-1, NULL, NULL);
	fsnode_rename_index(&file, (unsigned)-1, &file, NULL);
}

/********************************************************************/

void test_fail_fsnode_traverse(void)
{
	FSNode file;
	char *name = "test";
	file.delta_mode = DM_SRC;
	fsnode_traverse(NULL, NULL);
	fsnode_traverse(&file, NULL);
	fsnode_traverse(NULL, name);
	fsnode_traverse(&file, name);
}

void test_fail_vfs_traverse(void)
{
	vfs_traverse(NULL, NULL, NULL, 0);
}

void test_fail_vfs_basename(void)
{
	vfs_basename(NULL);
}

void test_fail_vfs_dnode_basename(void)
{
	vfs_dnode_basename(NULL, NULL, NULL);
}

/* NOTE: This function DOES NOT check for invalid arguments!  */
/* void test_fail_strip_trail_slash(void)
{
	strip_trail_slash(NULL);
} */

/********************************************************************/

void test_fail_vfs_fchdir(void)
{
	FSNode file;
	file.delta_mode = DM_SRC;
	vfs_fchdir(NULL);
	vfs_fchdir(&file);
}

void test_fail_vfs_ftruncate(void)
{
	FSNode file;
	file.delta_mode = DM_ISDIR;
	vfs_ftruncate(NULL, 0);
	vfs_ftruncate(&file, 0);
}

void test_fail_vfs_futime(void)
{
	FSNode file;
	file.delta_mode = DM_SRC;
	FSint64 times[2];
	vfs_futime(NULL, NULL);
	vfs_futime(&file, NULL);
	vfs_futime(NULL, times);
}

void test_fail_vfs_rename(void)
{
	char *name = "test";
	vfs_rename(NULL, NULL, NULL, NULL);
	vfs_rename(NULL, NULL, name, NULL);
	vfs_rename(NULL, NULL, NULL, name);
}

void test_fail_vfs_unlink(void)
{
	vfs_unlink(NULL, NULL, NULL);
}

void test_fail_vfs_mkdir(void)
{
	vfs_mkdir(NULL, NULL, NULL, DM_SRC);
}

void test_fail_vfs_rmdir(void)
{
	vfs_rmdir(NULL, NULL, NULL);
}

void test_fail_vfs_rmr(void)
{
	vfs_rmr(NULL, NULL, NULL);
}

/********************************************************************/

void test_fail_vfs_fopendir(void)
{
	FSNode file;
	file.delta_mode = DM_SRC;
	vfs_fopendir(NULL);
	vfs_fopendir(&file);
}

void test_fail_vfs_readdir(void)
{
	vfs_readdir(NULL);
}

/********************************************************************/

int main(void)
{
	test_fail_fsnode_destroy();
	test_fail_fsnode_find_inode();
	test_fail_fsnode_find_name();
	test_fail_fsnode_sort_names();
	test_fail_fsnode_link();
	test_fail_fsnode_unlink_index();
	test_fail_fsnode_ucreat();
	test_fail_fsnode_rename_index();
	test_fail_fsnode_traverse();
	test_fail_vfs_traverse();
	test_fail_vfs_basename();
	test_fail_vfs_dnode_basename();
	test_fail_vfs_fchdir();
	test_fail_vfs_ftruncate();
	test_fail_vfs_futime();
	test_fail_vfs_rename();
	test_fail_vfs_unlink();
	test_fail_vfs_mkdir();
	test_fail_vfs_rmdir();
	test_fail_vfs_rmr();
	test_fail_vfs_fopendir();
	test_fail_vfs_readdir();
	return 0;
}
