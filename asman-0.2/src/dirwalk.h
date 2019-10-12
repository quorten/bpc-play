/* A simple directory walking interface.

Copyright (C) 2013, 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#ifndef DIRWALK_H
#define DIRWALK_H

enum DIR_STATUSCODE_tag
{
	DIR_EMPTY, DIR_OK,
	OPEN_ERROR, CHDIR_ERROR, LSTAT_ERROR, UNKNOWN_ERROR
};
typedef enum DIR_STATUSCODE_tag DIR_STATUSCODE;
#define DIR_HAS_CONTENTS DIR_OK
#define DIR_ERRORS OPEN_ERROR

typedef struct DirNode_tag DirNode;
struct DirNode_tag
{
	const char *name;
	const unsigned name_len;
	const DirNode *parent;
};

extern DIR_STATUSCODE (*pre_search_hook)(const DirNode *node,
										 unsigned *num_files,
										 const char *filename,
										 const struct stat *sbuf);

extern DIR_STATUSCODE (*post_search_hook)(const DirNode *node,
										  unsigned *num_files,
										  const char *filename,
										  const struct stat *sbuf,
										  DIR_STATUSCODE status);
DIR_STATUSCODE chdir_hook(const DirNode *node, unsigned *num_files,
						  const char *filename, const struct stat *sbuf,
						  DIR_STATUSCODE status);

DIR_STATUSCODE search_dir(const DirNode *node);

char *dirnode_construct_path(const DirNode *node, const char *filename);

#endif /* not DIRWALK_H */
