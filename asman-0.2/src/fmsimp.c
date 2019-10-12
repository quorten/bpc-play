/* fmsimp2.c -- New and Improved File Management Simplifier.

Copyright (C) 2017, 2018 Andrew Makousky

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

/*

Okay, this is complicated, so let's try to plan this out.

Moves and deletes are easy to deal with and simplify if those are the
only operations.

Touches are easy to deal with and simplify if those are the only
operations.

Touch, move, and delete together?  Make sure you touch the correct
file location depending on what time you do the touch operations.

Hard links?  It is crucial to understand if touches preserve inode
numbers or not.  If they do, then hard links are easy too.  Just make
sure to create a "hard link staging" directory at the beginning, and
then move the files into place at the end.

Hard links on newly created files needs to be dealt with on a separate
step.

Copies?  That's a tough one.  A similar technique could be used here
too.  Create a staging directory, then move the files into place
later.  However, this is disadvantageous for disk fragmentation
optimization.

I'd say keep it simple for now.

Pre-process:
* Move existing files around.
* Create links to existing files.
* Delete existing files (optional, unless overwrites will happen).
** If delayed overwrites are desired, those can be setup to go
   to a staging directory for reversibility.
Process:
* Sync touched, i.e. created/updated, files to their final
  destinations.
Post-process:
* Create links to new files.
* Delete existing files (if delayed deletion is desired).

The problem is that full copy processing can require an indefinite
number of middle steps, depending on how much sharing we want to do at
the sync step.  If we assume no sharing at all, all copy operations
get transformed into touch operations.

 */

/*

No, no, no, this is how it is done.

* Any file is either an "existing" file or a "new" file.
* Only existing files can be moved.
* A deleted file can be moved before it gets deleted.
* Since files can be moved before deletion, links can refer to their
  final names before deletion.

Okay, I'm convinced.  We either can't support hard link optimization
or we can only support a limited form of it because we can't process
it down in a true single-state manner like we can touches, moves, and
deletes.

Single-state processing goes as follows:

* Move existing files into place.
* Delete files that will get overwritten, if necessary.
* Touch files.
* Delete files.

Easy as pie.  If a touched file gets moved?  Just rename the file and
only touch at the destination location.

What if you move files in a directory then you move the whole
directory?  You can't do that in a single-move manner, right?
Technically you can, but yes, you might not want to because it
wouldn't have Unix/POSIX semantics.

Don't worry.  For moves, just make sure to tag if the destination
directory already exists or needs to be created.

* Remember, the main reason why these simplifications are /mandatory/
  is because you don't want to try to touch-update a file that no
  longer exists when replaying a command-list.  Replaying
  command-lists not involving touch-synchronizations is perfectly fine
  even without simplification.

** Or, in other words, for the sake of correct syncing, you only need
   to do simplification on touch commands: don't touch a file that no
   longer exists.  Only touch the file in its final location, or
   alternatively, remove the file in its original location.

*** Copies are still more complicated, though, if you want to account
    for efficient delta-syncing.

* Okay, we have a solution for handling files that get moved around,
  copied or linked to, then deleted.  Any deleted file that is later
  referenced needs to keep a link to the /new reference/.  When the
  tree is walked to output the commands, the new reference is used in
  place of the old reference, and the chain is walked however long is
  necessary to get to an actual existing file.

** As a matter of fact, /candidate references/ can be added
   beforehand, since you generally don't know the fate of all such
   references.

** This gives us a reasonable method to deal with hard links, but it
   doesn't allow for copy-modify delta optimization.

About the whole moves problem.  Let's put it this way.

1. Moving /files/ is always harmless and easy to simplify.

2. Moving /directories/ is where things get complicated.

3. During command input, one should keep track of the true original
   location of a file.

4. When outputting the delta commands, directory moves that are chosen
   first need to cause the "original locations" tracked from
   files/directories to be renamed to refer to the current locations.
   This way, pending commands will refer to the current exiting name.

5. Because of these requirements, there may need to be more than one
   reference to a file at the same tree location, but at different
   points in time.  For example, there can be both a source and a
   destination reference in the same location.

   How do we want to handle this?  This is my idea.  We can have up to
   three different tree structures that we work with: original,
   latest, and current.

   Okay, let's try again.  Every file has an old and a new location.
   During command input, we create the old locations once and adjust
   the new locations throughout.  If we touch a file and don't know if
   it already existed, we may need to backtrack to find the "original"
   location.  (That is, the original location of the parent
   directory.)  During command output, we adjust the old location
   while walking the new tree structure.  When we are finished with
   command output, the old and the new location should be the same.

   So what does this mean?  We can in fact have two separate tree
   structures.

   What if when moving directories to their new target locations there
   would be an new collision?  We have to push a pending command and
   get things moved out of the way first.  (This may include deleting
   files that get overwritten.)  Yes, in some cases this might
   degenerate to being exactly the same as the input command list, no
   simpler, but at least we are not making it more complicated by
   inserting stop-gap commands to move to temporary names.

*/

/*

Okay, yes.  So, how this works.  We have two main steps.

1. Scan commands to generate implicit initial delta state and final
   delta state.

   * Generate reversed initial and final state if running in reverse.

   During read-in, the generated initial state is fairly static, and
   move commands mutate the final state.

2. Bring initial state to final state and output commands.

   * Walk the destination directories recursively.
   * Check back to each corresponding source directory and process
     deletes.  Push these onto a processing stack if delayed deletes
     are desired.
   * Process each entry in the new directory.  If there are
     dependencies, push them onto a stack and pop them off in
     dependency order.
   ** Dependencies are detected by checking if a source file exists
      and is in the way of a destination operand.

   This can actually be done in a source-first manner by crossing over
   to the destination directories to detect new files.  (And doing so
   recursively in the case of new directories.)  Okay, fine, now that
   you mention the solution, let's go source-first.

   * Walk the source directories recursively.
   * Process deletes.  Push these onto a processing stack if delayed
     deletes are desired.
   * Process moves in this directory.  If there are dependencies, push
     them onto a stack and pop them off in dependency order.
   ** Dependencies are detected by checking if a source file exists
      and is in the way of a destination operand.
   * Process touch updates.
   * Check the corresponding destination directory to touch new files.
   * Process delayed deletes, if applicable.

So, first of all, let's review.  Our file management commands:

* mkdir
* rmdir
* touch
* mv
* rm

Now, for their effects on implicit initial delta state and final delta
state.

You know what?  I've got to redefine my modes to be more elegant for
this.  <P1> and <P2> represent /paths/, not file nodes.

DM_DNE -> does not exist
DM_SRC -> existing file
DM_WHITEOUT -> existing file deleted
DM_MOVE -> renamed from source file

* <P1> DM_DNE
* mkdir <P1>
* <P1> DM_SRC

* <P1> DM_SRC
* rmdir <P1>
* <P1> DM_DNE

* <P1> DM_SRC || DM_DNE
* touch <P1>
* <P1> DM_SRC

* <P1> DM_SRC
* <P2> DM_DNE || (DM_SRC | DM_ISDIR) || DM_SRC
* mv <P1> <P2>
* <P1> DM_DNE
* <P2> DM_MOVE || (DM_WHITEOUT | DM_MOVE)

* <P1> DM_SRC
* rm <P1>
* <P1> DM_DNE

Moves, in particular, can encompass one of three cases:

1. Source file is moved to non-existing destination name.
2. Source file is moved to destination directory.
3. Source file overwrites existing destination name.

1.
* <P1> DM_SRC
* <P2> DM_DNE
* mv <P1> <P2>
* <P1> DM_DNE
* <P2> DM_MOVE

2.
* <P1> DM_SRC
* <P2> (DM_SRC | DM_ISDIR)
* mv <P1> <P2>
* <P1> DM_DNE
* <P2> DM_MOVE

3.
* <P1> DM_SRC
* <P2> DM_SRC
* mv <P1> <P2>
* <P1> DM_DNE
* <P2> DM_WHITEOUT | DM_MOVE

Important notes:

* If a file gets deleted, its name can get overwritten.  This is easy
  to flag by ORing DM_WHITEOUT with an operation that creates a file
  name.

** Oh, come on, that's too complicated.  Just tag the source as "to be
   deleted" so that you don't need to track around a destination
   marker that needs to be pinned to a single location rather than a
   single destination node.

* Cases 1 & 3 can be safely merged into implied case 3, but only on
  case-sensitive file systems.  Assuming case 3 on case-insensitive
  file systems could cause inadvertent loss of data, especially when
  files are renamed so as to only change their case.

** In other words, it would be safer, and probably preferable for
   non-CLI users, to always assume case 1.  When operating from a GUI,
   deletions can always be explicit commands.

* Rule for touches.
** If a file hasn't been deleted, it implicitly exists.
** If a file has been deleted, touching it creates a new one.
** When populating the final state, this requires checking back to the
   source state to check if an identical file name is in the scheduled
   deletion state.
*** Why do we use the initial state to store this?  Because, during
    load, it remains relatively static, so we don't have to deal with
    problems of files being moved into its place and multiple
    overwrites.

 */

/********************************************************************/
/* MOVE above notes to blog.

   So, what are we really doing?

   * Create a snapshot, possibly implicitly, of the "old" filesystem
     state.
   * Keep track of rename links between the two.
   * "Diff" the snapshots to generate the sync command list.

   So, really, it's just a matter of augmenting filesystem commands to
   be able to create an implicit old snapshot state, rather than
   relying on operating off of a real existing snapshot.  Therefore,
   we are able to simply take a journal list of change commands and
   simplify them, without any real knowledge of the whole filesystem
   state.  Provided that the input commands are sufficiently formal,
   that is.

   So, here's the main reason why our algorithm has a nifty twist
   compared to a regular directory diff.  Rather than only being able
   to assert file/directory creation/deletion, since we've kept track
   of old and new name information, we can properly deduce renames.
   Hence, we can rename in place of "creating" all new files.

   So, one problem to look out for when implementing a real
   filesystem.  Out of disk space?  You need to be able to reserve
   some disk space to delete files and write the deletes to the
   snapshot, then you can sync, then you can continue clearing up disk
   space.

   Union mounts?  Snapshotting can be used to ease the implementation.

*/

/* Okay, so the solution for supporting hard links: Just add another
   pointer to your current node structures.  There is no need to
   reorganize the file nodes so that they do not have an embedded
   name.  The embedded name is still useful for things like
   determining the exact paths that hard links are moved across, as
   opposed to creates and deletes.  */

/* TODO NOTE: The solution to the problem!  So to minimize dependency
   chains while doing the diff tree walk.  Rather than moving all
   paths into position right away, just focus on moving files into the
   correct parent directories, and the parent directories will be all
   straightened out at the end of the walk.  */

#include <stdio.h>
#include <stdlib.h> /* for rand() */
#include <string.h>

#include "bool.h"
#include "xmalloc.h"
#include "exparray.h"
#include "misc.h"
#include "cmdline.h"
#include "vfs.h"

typedef char* char_ptr;
EA_TYPE(char_ptr);
EA_TYPE(char_ptr_array);

/* The root node of the old and the new file tree structures.  */
FSNode *old_tree_root = NULL;
FSNode *new_tree_root = NULL;

/* Keep track of the current working directory in the trees.  */
FSNode *old_cwd = NULL;
FSNode *new_cwd = NULL;

/* Command line switches */
bool case_insens = false;
bool debug_trees = false;
bool lazy_mkdir = false;
bool lazy_rm = false;
/* Rather than delete immediately, rename to a temporary name before
   deleting.  */
bool mv_rm = false;
/* Do moves implicitly delete?  True by default to conform to Unix
   semantics, but can be disabled for other operating system
   semantics.  */
/* Move to central "trash" directory first before deleting?  */

int proc_mkdir(int argc, char *argv[]);
int proc_rmdir(int argc, char *argv[]);
int proc_touch(int argc, char *argv[]);
int proc_cp(int argc, char *argv[]);
int proc_mv(int argc, char *argv[]);
int proc_rm(int argc, char *argv[]);
int proc_chdir(int argc, char *argv[]);

unsigned gen_delete_cmds(FSNode *file, bool del_after);
bool gen_mkdir_cmds(FSNode *file);
bool gen_move_cmds(FSNode *file);
bool gen_touch_cmds(FSNode *file);
bool compare_final_trees(FSNode *file);

void debug_list_dirs(FSNode *file);

int main(int argc, char *argv[])
{
	int retval = 0;
#define NUM_CMDS 9
	const char *commands[NUM_CMDS] =
		{ "touch", "cp", "mv", "rm", "ln", "mkdir", "rmdir",
		  "cd", "chdir" };
	const CmdFunc cmdfuncs[NUM_CMDS] = { proc_touch, proc_cp,
		proc_mv, proc_rm, proc_cp, proc_mkdir, proc_rmdir,
		proc_chdir, proc_chdir };

	if (argc == 2 && !strcmp(argv[1], "-c"))
		case_insens = true;
	else if (argc == 2 && !strcmp(argv[1], "-d"))
		debug_trees = true;
	else if (argc != 1)
	{
		printf("Usage: %s [-c] < COMMANDS > SIMP-COMMANDS\n", argv[0]);
		puts(
"Read file management commands on standard input, and write a simplified\n"
"list of commands to standard output.  Note that the input commands must\n"
"be in a basic command syntax such as the syntax which `fmprim' emits.\n"
"\n"
"Options:\n"
"    -c    Case-insensitive: use simplifier semantics appropriate for a\n"
"          filesystem with case-insensitive file name matching.");
		return 0;
	}

	old_tree_root = vfs_create();
	new_tree_root = vfs_create();
	old_tree_root->other = new_tree_root;
	new_tree_root->other = old_tree_root;
	old_cwd = old_tree_root;
	new_cwd = new_tree_root;

	/* TODO FIXME ADD: There is an even simpler mode of operation:
	   pass through all file management commands except touches to
	   standard output, and build the new tree as usual.  Then as a
	   final stage, scan the tree for new/updated files and emit those
	   as touches.  This way we don't have to worry as much about bugs
	   in the simplification algorithm, and it straight to the point
	   solves the problem we were previously encountering with
	   journaling commands.  */

	/* Process the commands on standard input.  */
	retval = proc_cmd_dispatch(stdin, NUM_CMDS, commands, cmdfuncs, true);
	old_cwd = old_tree_root;
	new_cwd = new_tree_root;

	if (debug_trees)
	{
		/* DEBUG: Dump the tree structures.  */
		fputs("Old tree:\n", stderr);
		debug_list_dirs(old_tree_root);
		fputs("New tree:\n", stderr);
		debug_list_dirs(new_tree_root);
	}

	/* Generate the command lists by bringing the old tree up-to-date
	   with the new one.  */

	/* Write the net command lists specified by the user to standard
	   output.  */
	/* Three different lists of commands, written out in this order.

	   1. Delete before
	   2. Create directories
	   3. Moves
	   4. Delete after
	   5. Touches */
	/* Technically, delete before is optional and can be eliminated
	   completely: syncs on the same file name will naturally delete
	   the existing contents, and Unix-style moves will delete
	   existing files.  Likewise, disk space permitting, delete after
	   can be moved to the very end, after the touches.  */

	if (escape_newlines)
		fputs("NL='\n'\n", stdout);

	if (!lazy_rm)
	{
		/* First scan of the old tree, push delete before (overwrite)
		   commands.  */
		puts("# Delete before commands");
		if (gen_delete_cmds(old_tree_root, false) == 1)
		{
			fputs("error: Internal error generating "
				  "delete before commands.\n", stderr);
			retval = 1; goto cleanup;
		}
	}

	if (!lazy_mkdir)
	{
		/* First scan of the new tree, create new directories in old
		   tree.  */
		puts("# Create directory commands");
		if (!gen_mkdir_cmds(new_tree_root))
		{
			fputs("error: Internal error generating "
				  "create directory commands.\n", stderr);
			retval = 1; goto cleanup;
		}
	}

	/* Second scan of the new tree, process moves.  */
	puts("# Move commands");
	if (!gen_move_cmds(new_tree_root))
	{
		fputs("error: Internal error generating "
			  "move commands.\n", stderr);
		retval = 1; goto cleanup;
	}

	/* Scan the modified old tree, push delete after commands.  */
	puts("# Delete after commands");
	if (gen_delete_cmds(old_tree_root, true) == 1)
	{
		fputs("error: Internal error generating "
			  "delete after commands.\n", stderr);
		retval = 1; goto cleanup;
	}

	/* Third scan of the new tree, push touches (new and updated
	   files).  */
	puts("# Touch commands");
	if (!gen_touch_cmds(new_tree_root))
	{
		fputs("error: Internal error generating "
			  "touch commands.\n", stderr);
		retval = 1; goto cleanup;
	}

	/* TODO: Final delete commands.  */

	if (debug_trees)
	{
		/* DEBUG: Dump the tree structures.  */
		fputs("New tree:\n", stderr);
		debug_list_dirs(new_tree_root);
		fputs("Final tree:\n", stderr);
		debug_list_dirs(old_tree_root);
	}

	/* Verify the old and new tree structures are now identical.  If
	   they are not, error out.  */
	if (!compare_final_trees(old_tree_root))
	{
		fputs("error: Final tree does not match new tree.\n", stderr);
		retval = 1;
	}

cleanup:
	vfs_destroy(old_tree_root);
	vfs_destroy(new_tree_root);
	return retval;
}

/********************************************************************/

/*

Summary of available manipulation operations and state changes.

NIL = existence unknown
DNE = definitely does not exist
SRC = definitely exists
ICR = if nonexistent, create, otherwise ignore
IRM = if existent, delete, otherwise ignore
*   = all of above conditions
NEW = newly created
UPD = updated
SRC* = source file's destination is deleted

b_old = before operation, old tree
b_new = before operation, new tree
a_old = after operation, old tree
a_new = after operation, new tree

vfs_ucreat:
vfs_mkdir:

b_old b_new  a_old a_new  result
NIL   DNE    DNE   NEW    success  <-- TODO FIXME NOTE
DNE   DNE    DNE   NEW    success
SRC   DNE    SRC   NEW    success
*     SRC    *     SRC    failure

vfs_rm:
vfs_rmdir:

b_old b_new  a_old a_new  result
NIL   DNE    IRM   DNE    success
DNE   DNE    DNE   DNE    failure
SRC   DNE    SRC   DNE    failure
*     SRC    *     DNE    success

fmsimp_touch:

b_old b_new  a_old a_new  result
NIL   DNE    ICR   SRC    success
DNE   DNE    DNE   NEW    success
SRC   DNE    SRC   NEW    success
*     SRC    *     UPD    success

vfs_rename:

src                       dest
b_old b_new  a_old a_new  b_old b_new  a_old a_new  result
NIL   DNE    SRC   DNE    NIL   DNE    IRM   MV     success
DNE   DNE    DNE   DNE    NIL   DNE    NIL   DNE    failure
SRC   DNE    SRC*  DNE    NIL   DNE    IRM   MV     success
NIL   DNE    SRC   DNE    DNE   DNE    DNE   MV     success
DNE   DNE    DNE   DNE    DNE   DNE    DNE   DNE    failure
SRC   DNE    SRC*  DNE    DNE   DNE    DNE   MV     success
NIL   DNE    SRC   DNE    SRC   DNE    SRC*  MV     success
DNE   DNE    DNE   DNE    SRC   DNE    SRC   DNE    failure
SRC   DNE    SRC*  DNE    SRC   DNE    SRC*  MV     success
*     DNE    *     DNE    *     DNE    *     DNE    failure
*     SRC    *     DNE    *     DNE    *     MV     success
*     DNE    *     DNE    *     SRC    *     DNE    failure
*     SRC    *     DNE    *     SRC    *     MV     success

implicit:

"if it might exist, make it definitely exist"
"if it might not exist, make it definitely not exist"

assume:

"if it is unkown to exist, make it exist"
"if it is definitely known not to exist, fail"

TODO FIXME NOTE: The key case that we don't yet handle.  When we
delete a file, storing a DNE by means of a DM_WHITEOUT.

*/

/********************************************************************/

/* DUPLICATE CODE */
/* Like `mkdir -p': Take a Unix-style path string where the directory
   segments are separated with slashes, traverse the directory tree
   accordingly, and create directories when they do not already exist,
   in both the old and the new trees.  (Directories are not created in
   the old tree if they have been deleted.)  If there is an error such
   as trying to traverse a non-directory or a directory name is not
   found, NULL is returned.  Note that this function modifies the
   `path' argument to temporarily replace slash characters with null
   characters to ease string processing.

   `parent' is a reference from the new tree.

   `assume_mode' switches behavior from "implicit creates" to "assume
   creates": if the directory has been definitely deleted from the old
   tree, fail as we cannot "assume" it to already exist.

   On failure, the newly created directories for the earlier segments
   are not removed.

   Interestingly, this is designed to create intermediate directories
   referenced in a relative path such as "one/two/../three".

   Also, note that this function doesn't check that the final
   mentioned path segment is actually a directory.  It could just as
   well be an existing file and that would be okay.

   Why can't it be the simpler approach of simply walking backwards
   from the last path segment up by parents to the last segment with a
   link to the old directory?  Because, we need to support that
   strange behavior where you can do "one/two/../three", which breaks
   such a simple approach.  */
FSNode *implicit_low_mkdirp(
	FSNode *parent, char *path, DeltaMode delta_mode,
	bool assume_mode)
{
	FSNode *file = parent;
	char *end;
	if (parent == NULL)
		return NULL;
	if (path == NULL)
		return file;
	if (!(file->delta_mode & DM_ISDIR))
		return NULL;
	delta_mode |= DM_ISDIR;
	/* Skip leading slashes.  */
	while (*path == '/')
		path++;
	end = path;
	while (/* file != NULL && */ *path)
	{
		while (*end != '\0' && *end != '/')
			end++;
		STR_BEGIN_SPLICE(end);
		if (*path == '\0')
			; /* Skip zero-length path segment, i.e. consecutive
			     leading or trailing slash.  */
		else if (!strcmp(path, "."))
			; /* skip */
		else if (!strcmp(path, ".."))
		{
			FSNode *new_file = file->parent;
			if (new_file != NULL)
				file = new_file;
		}
		else
		{
			unsigned index = fsnode_find_name(file, path);
			if (index == (unsigned)-1)
			{
				/* Attempt to create this missing directory segment,
				   checkin/creating the old segment first before the
				   new segment.  */
				FSNode *other = file->other;
				FSNode *old_tree_file = NULL;
				unsigned old_index = (unsigned)-1;
				if (other != NULL)
					old_index = fsnode_find_name(other, path);
				if (old_index == (unsigned)-1)
				{
					old_tree_file = fsnode_ucreat(
						other, xstrdup(path), delta_mode);
					if (old_tree_file == NULL)
						file = NULL;
					else
						old_tree_file->delta_mode = DM_ICR | DM_ISDIR;
				}
				else
				{
					/* An old tree file exists that either got deleted
					   or renamed.

					   * For assume creates, fail.

					   * For implicit creates, do not create and link
					     a segment in the old tree.  */
					old_tree_file = other->data.files.d[old_index];
					if (assume_mode && old_tree_file->other == NULL)
						file = NULL;
					else
					{
						/* By definition, we know we are creating a
						   new directory.  */
						old_tree_file = NULL;
					}
				}
				if (file != NULL)
				{
					FSNode *new_file = fsnode_ucreat(
						file, xstrdup(path), delta_mode);
					if (new_file != NULL)
					{
						new_file->other = old_tree_file;
						if (old_tree_file != NULL)
							old_tree_file->other = new_file;
						else
							new_file->delta_mode = DM_NEW | DM_ISDIR;
					}
					file = new_file;
				}
			}
			else
				file = file->data.files.d[index];
		}
		STR_END_SPLICE();
		if (file == NULL)
			break;
		if (*end != '\0')
		{
			/* (*end == '/') */
			end++;
			/* Paths with trailing slashes must always correspond to
			   directories.  */
			if (!(file->delta_mode & DM_ISDIR))
				return NULL;
		}
		path = end;
	}
	return file;
}

/* DUPLICATE CODE */
/* `mkdir -p' and return the final dnode or NULL on error.  */
FSNode *implicit_high_mkdirp(FSNode *vfs_root, FSNode *vfs_cwd,
							 char *pathname, DeltaMode delta_mode)
{
	/* DUPLICATE CODE */
	FSNode *parent, *result;
	char *strip_path;
	if (pathname == NULL)
		return NULL;
	strip_path = xstrdup(pathname);
	strip_trail_slash(strip_path);
	delta_mode |= DM_ISDIR;
	/* DUPLICATE CODE */
	if (strip_path[0] == '/')
		parent = vfs_root;
	else
		parent = vfs_cwd;
	result = implicit_low_mkdirp(parent, strip_path, delta_mode, false);
	xfree(strip_path);
	return result;
}

/* DUPLICATE CODE */
/* `mkdir -p' and return the final dnode or NULL on error.  */
FSNode *assume_high_mkdirp(FSNode *vfs_root, FSNode *vfs_cwd,
							 char *pathname, DeltaMode delta_mode)
{
	/* DUPLICATE CODE */
	FSNode *parent, *result;
	char *strip_path;
	if (pathname == NULL)
		return NULL;
	strip_path = xstrdup(pathname);
	strip_trail_slash(strip_path);
	delta_mode |= DM_ISDIR;
	/* DUPLICATE CODE */
	if (strip_path[0] == '/')
		parent = vfs_root;
	else
		parent = vfs_cwd;
	result = implicit_low_mkdirp(parent, strip_path, delta_mode, true);
	xfree(strip_path);
	return result;
}

/* DUPLICATE CODE */
/* This assumes a "touch" operation: implicitly create an entry in the
   old tree if one such entry has not been deleted, and also create an
   entry in the new tree if it does not already exist.  */
FSNode *implicit_high_ucreat(FSNode *vfs_root, FSNode *vfs_cwd,
							 const char *pathname, DeltaMode delta_mode)
{
	VFSDnodeName parts;
	unsigned index = (unsigned)-1;
	FSNode *result, *other_parent, *old_path_result = NULL;

	{ /* Do not allow trailing slashes since (1) these must not appear
		 in paths for regular files and (2) these must have been
		 removed in advance for directories.  */
		unsigned path_len = strlen(pathname);
		if (path_len > 0 && pathname[path_len-1] == '/')
			return NULL;
	}

	parts = vfs_dnode_basename(vfs_root, vfs_cwd, pathname);
	if (parts.parent == NULL)
		return NULL;
	/* Do not allow usage of the Unix reserved names "." and "..".  */
	if (!strcmp(parts.name, ".") || !strcmp(parts.name, ".."))
		return NULL;
	/* If the node already exists in the new tree, return that.  */
	index = fsnode_find_name(parts.parent, parts.name);
	if (index != (unsigned)-1)
		return parts.parent->data.files.d[index];
	result = fsnode_ucreat(parts.parent, xstrdup(parts.name), delta_mode);
	/* If there exists and old file of the same name that has been
	   definitely moved/deleted, do not create and link a segment in
	   the old tree.  */
	other_parent = parts.parent->other;
	if (other_parent != NULL)
	{
		index = fsnode_find_name(other_parent, parts.name);
		if (index == (unsigned)-1)
		{
			old_path_result = fsnode_ucreat(
				other_parent, xstrdup(parts.name), delta_mode);
			if (old_path_result == NULL)
				result = NULL;
			else
				old_path_result->delta_mode = DM_ICR;
		}
	}
	else
	{
		/* An old tree file exists that either got deleted or renamed.
		   By definition, we know we are creating a new file.  */
		old_path_result = NULL;
		if (result != NULL)
			result->delta_mode = DM_NEW;
	}
	if (result != NULL)
	{
		result->other = old_path_result;
		if (old_path_result != NULL)
			old_path_result->other = result;
	}
	return result;
}

/* DUPLICATE CODE */
int fmsimp_unlink(FSNode *vfs_root, FSNode *vfs_cwd, const char *pathname)
{
	VFSDnodeName parts;
	unsigned index, other_index;
	FSNode *file, *other;
	if (pathname == NULL)
		return -1;

	{ /* Do not allow trailing slashes since you must use `rmdir' to
		 remove a directory.  */
		unsigned path_len = strlen(pathname);
		if (path_len > 0 && pathname[path_len-1] == '/')
			return -1;
	}

	parts = vfs_dnode_basename(vfs_root, vfs_cwd, pathname);
	if (parts.parent == NULL)
		return -1;
	/* Do not allow usage of the Unix reserved names "." and "..".  */
	if (!strcmp(parts.name, ".") || !strcmp(parts.name, ".."))
		return -1;
	index = fsnode_find_name(parts.parent, parts.name);
	if (index == (unsigned)-1)
		return -1;
	file = parts.parent->data.files.d[index];
	if (file == NULL || (file->delta_mode & DM_ISDIR))
		return -1;
	other = file->other;
	if (other) other->other = NULL;
	if (!fsnode_unlink_index(parts.parent, index))
		return -1;
	fsnode_destroy(file);

	/* Only run this code when we want to delete both the old and the
	   new nodes.  Currently, we only want to simulate commands on the
	   new nodes, so the old nodes can serve as a snapshot.  */
	/* FSNode *other, *other_parent; */
	/* other_parent = other->parent;
	other_index = fsnode_find_name(other_parent, other->name);
	if (other_index == (unsigned)-1)
		return -1;
	if (!fsnode_unlink_index(other_parent, other_index))
		return -1;
	fsnode_destroy(other); */

	return 0;
}

/* DUPLICATE CODE */
/* Remember, for directories, we always remove trailing slashes from
   the pathname.  */
int fmsimp_rmdir(FSNode *vfs_root, FSNode *vfs_cwd, const char *pathname)
{
	char *strip_path;
	if (pathname == NULL)
		return -1;
	strip_path = xstrdup(pathname);
	VFSDnodeName parts;
	unsigned index, other_index;
	FSNode *dnode, *other;
	strip_trail_slash(strip_path);
	parts = vfs_dnode_basename(vfs_root, vfs_cwd, strip_path);
	if (parts.parent == NULL)
		goto fail;
	/* Do not allow usage of the Unix reserved names "." and "..".  */
	if (!strcmp(parts.name, ".") || !strcmp(parts.name, ".."))
		goto fail;
	index = fsnode_find_name(parts.parent, parts.name);
	if (index == (unsigned)-1)
		goto fail;
	dnode = parts.parent->data.files.d[index];
	if (dnode == NULL || !(dnode->delta_mode & DM_ISDIR))
		goto fail;
	if (dnode->data.files.len != 0)
		goto fail;
	/* Unix allows you to delete a directory that is still being used
	   as the working directory, but we don't.  */
	if (vfs_cwd == dnode)
		goto fail;
	other = dnode->other;
	if (other) other->other = NULL;
	if (!fsnode_unlink_index(parts.parent, index))
		goto fail;
	fsnode_destroy(dnode);

	/* Only run this code when we want to delete both the old and the
	   new nodes.  Currently, we only want to simulate commands on the
	   new nodes, so the old nodes can serve as a snapshot.  */
	/* FSNode *other, *other_parent; */
	/* other_parent = other->parent;
	other_index = fsnode_find_name(other_parent, other->name);
	if (other_index == (unsigned)-1)
		goto fail;
	if (!fsnode_unlink_index(other_parent, other_index))
		goto fail;
	fsnode_destroy(other); */

	xfree(strip_path);
	return 0;
fail:
	xfree(strip_path);
	return -1;
}

/********************************************************************/

/* Assume that a directory exists, so create nodes recursively in the
   old and new trees if necessary.  Effectively, this is an `mkdir
   -p'.  The corresponding dnode in the new tree is returned, or NULL
   on failure.  */
FSNode *assume_exist_dir(char *filename)
{
	FSNode *result = assume_high_mkdirp(
		new_tree_root, new_cwd, filename, 0);
	if (result == NULL || (result->delta_mode & DM_ISDIR) == 0)
		return NULL;
	return result;
}

/* DUPLICATE CODE */
/* Take a path string, compute the base name of the file, and assume
   that the parent directory exists, using the same method as
   `vfs_basename()' and `vfs_dnode_basename()'.  The returned name is
   a pointer into the last segment of `pathname'.  On failure, both
   `parent' and `name' will be NULL.  */
VFSDnodeName assume_dnode_basename(char *pathname)
{
	VFSDnodeName result = { NULL, NULL };
	char *dirname_end;

	if (pathname == NULL)
		return result;
	dirname_end = strrchr(pathname, '/');
	if (dirname_end == NULL)
	{
		/* Easy, the dnode is the current directory, and the entire
		   string is the base name.  */
		result.parent = new_cwd;
		result.name = pathname;
		return result;
	}
	if (dirname_end == pathname)
	{
		/* The dnode is the root directory and the string excluding
		   the single leading slash is the base name.  */
		result.parent = new_tree_root;
		result.name = pathname + 1;
		return result;
	}

	result.name = dirname_end + 1;

	/* Temporarily splice our string into only the dirname.  */
	*dirname_end = '\0';
	/* We must assume the parent directory exists.  */
	result.parent = assume_exist_dir(pathname);
	*dirname_end = '/';
	if (!result.parent)
	{
		/* result.parent = NULL; */
		result.name = NULL;
	}

	return result;
}

/* Assume that a file exists, so create a node in the old and new
   trees if necessary.  Also, this must necessarily assume that the
   parent directory of the file exists too.

   Note that the filename argument gets temporarily modified by this
   function.  */
FSNode *assume_exist_file(char *filename)
{
	VFSDnodeName parts = assume_dnode_basename(filename);
	FSNode *parent_other;
	unsigned index = (unsigned)-1;
	FSNode *result;
	if (parts.parent == NULL)
		return NULL;
	parent_other = parts.parent->other;
	/* If a move/delete node already exists in the old tree, and the
	   file also doesn't exist in the new tree, we must return failure
	   as we know the file definitely does not exist.  */
	if (parent_other)
		index = fsnode_find_name(parent_other, parts.name);
	if (index != (unsigned)-1)
	{
		index = fsnode_find_name(parts.parent, parts.name);
		if (index == (unsigned)-1)
			return NULL;
		result = parts.parent->data.files.d[index];
		if ((result->delta_mode & DM_ISDIR))
			return NULL;
		return result;
	}
	result = implicit_high_ucreat(new_tree_root, new_cwd, filename, 0);
	if (result == NULL)
		return NULL;
	return result;
}

/* Assume that a file/directory exists.  If it doesn't exist, create a
   directory recursively in the old and new trees.

   Note that the filename argument gets temporarily modified by this
   function.  */
FSNode *assume_exist_dir_or_file(char *filename)
{
	FSNode *result = assume_high_mkdirp(
		new_tree_root, new_cwd, filename, 0);
	/* if (result == NULL)
		return result; */
	return result;
}

/* Assume that a file/directory does not exist, so return failure if
   something actually does exist.  Note, however, that due to the
   intended use cases of this function, we must assume that the parent
   directory exists: the purpose is to verify that a name is free to
   use within a directory node.  */
bool assume_not_exist(char *filename)
{
	VFSDnodeName parts = assume_dnode_basename(filename);
	FSNode *parent_other;
	unsigned index = (unsigned)-1;
	FSNode *result;
	if (parts.parent == NULL)
		return false;
	parent_other = parts.parent->other;

	/* Verify that the name does not exist in the current tree.  */
	index = fsnode_find_name(parts.parent, parts.name);
	if (index != (unsigned)-1)
		return false;

	/* TODO FIXME: VERIFY that all code holds this condition true.  If
	   a file definitely exists in the old tree and still does exist
	   in the new tree, it must definitely link to an entry somewhere
	   in the new tree.  Therefore, we only need to check for entries
	   in the new tree.  */
	if (parent_other)
	{
		/* Create a DM_DNE node in the old tree to indicate that this
		   filename definitely does not exist.  */
		/* TODO FIXME: We must disable this as the rest of our code
		   doesn't know how to properly process DM_DNE?  */
		result = fsnode_ucreat(parent_other, xstrdup(parts.name), DM_DNE);
		if (result == NULL)
			return false;
	}
	return true;

	/* OLD VERSION: */
	/* FSNode *file = vfs_traverse(new_tree_root, new_cwd, filename, 0);
	if (file != NULL)
		return false; */
}

/* Update a file if it exists, assuming it already exists in the old
   tree.  For the purposes of our simplifier, lacking better
   knowledge, we should always assume the file might already exist so
   that we don't simplify in a way that creates a duplicate file.  */
FSNode *implicit_update_file(char *filename)
{
	DeltaMode delta_mode = DM_TOUCH;
	VFSDnodeName parts = assume_dnode_basename(filename);
	FSNode *parent_other;
	unsigned index = (unsigned)-1;
	FSNode *result;
	if (parts.parent == NULL)
		return NULL;
	/* If a move/delete node exists in the old tree, then we are
	   definitely creating a new file.  */
	parent_other = parts.parent->other;
	if (parent_other != NULL)
		index = fsnode_find_name(parent_other, parts.name);
	if (index != (unsigned)-1)
		delta_mode = DM_NEW;
	result = implicit_high_ucreat(new_tree_root, new_cwd,
								  filename, delta_mode);
	/* if (result == NULL)
		return false; */
	return result;
}

/* Delete a file if it exists, otherwise do nothing.  In practical
   terms, we must assume the file to exist so the implicit deletes do
   not get optimized out of the final command list.  */
bool implicit_delete_file(char *filename)
{
	FSNode *file, *other;
	if (!assume_exist_file(filename))
	{
		/* If, however, we know that this file definitely does not
		   exist (DM_DNE). then we return success right away.  */
		VFSDnodeName parts = assume_dnode_basename(filename);
		FSNode *parent_other;
		unsigned index = (unsigned)-1;
		FSNode *result;
		if (parts.parent == NULL)
			return false;
		parent_other = parts.parent->other;
		if (parent_other == NULL)
			return false;
		index = fsnode_find_name(parent_other, parts.name);
		if (index == (unsigned)-1)
			return false;
		result = parent_other->data.files.d[index];
		if (result->delta_mode == DM_DNE)
			return true;

		return false;
	}
	/* Find the old node so we can record this delete as implicit.  */
	file = vfs_traverse(new_tree_root, new_cwd, filename, 0);
	if (file == NULL)
		return false;
	other = file->other;
	/* If the file definitely exists in the old tree, then we
	   definitely delete it.  No need to mark the delete as
	   implicit.  */
	if (other != NULL)
	{
		if (other->delta_mode == DM_SRC)
			other->delta_mode = DM_SRC | DM_WHITEOUT;
		else
			other->delta_mode = DM_IRM;
	}
	if (fmsimp_unlink(new_tree_root, new_cwd, filename) == -1)
		return false;
	return true;
}

/* Delete a directory if it exists, otherwise do nothing.  In
   practical terms, we must assume the directory to exist so the
   implicit deletes do not get optimized out of the final command
   list.  */
bool implicit_delete_dir(char *filename)
{
	FSNode *file, *other;
	if (!assume_exist_dir(filename))
		return false;
	/* Find the old node so we can record this delete as implicit.  */
	file = vfs_traverse(new_tree_root, new_cwd, filename, 0);
	if (file == NULL)
		return false;
	other = file->other;
	/* If the file definitely exists in the old tree, then we
	   definitely delete it.  No need to mark the delete as
	   implicit.  */
	if (other != NULL)
	{
		if (other->delta_mode == DM_SRC)
			other->delta_mode = DM_SRC | DM_WHITEOUT | DM_ISDIR;
		else
			other->delta_mode = DM_IRM | DM_ISDIR;
	}
	if (fmsimp_rmdir(new_tree_root, new_cwd, filename) == -1)
		return false;
	return true;
}

/* Delete a directory recursively if it exists, otherwise do
   nothing.  */
void implicit_clobber_dir(char *filename)
{
}

/********************************************************************/

/* Create a temporary name to move a file to when intermediary file
   management steps are required.  Flag to allow moving within the
   existing parent directory or to a temporary directory.

   The common case: a file gets moved to a subdirectory of the same
   name.  */
char *fmsimp_mktemp(FSNode *vfs_root, FSNode *vfs_cwd, char *template)
{
	unsigned template_len = strlen(template);
	char *template_end = template + template_len;
	char *suffix_start = template_end;
	do
		suffix_start--;
	while (suffix_start > template && *suffix_start == 'X');
	if (suffix_start == template && *suffix_start == 'X')
		return NULL;
	suffix_start++;
	do
	{
		char *cur_pos = suffix_start;
		while (cur_pos != template_end)
			*cur_pos++ = (char)(rand() & 0xff);
	} while (vfs_traverse(vfs_root, vfs_cwd, template, 0) != NULL);
	return template;
}

/********************************************************************/

/* Process an `mkdir' command.  The command is assumed to take no
   options and only one directory name.  */
int proc_mkdir(int argc, char *argv[])
{
	strip_trail_slash(argv[1]);
	if (!assume_not_exist(argv[1]))
		return 1;
	return (vfs_mkdir(new_tree_root, new_cwd, argv[1], DM_NEW) ? 1 : 0);
}

/* Process an `rmdir' command.  The command is assumed to take no
   options and only one directory name.  */
int proc_rmdir(int argc, char *argv[])
{
	if (!assume_exist_dir(argv[1]))
		return 1;
	return (fmsimp_rmdir(new_tree_root, new_cwd, argv[1]) ? 1 : 0);
}

/* Process a `touch' command.  "Touch" is the generic term for any
   kind of command that requires creating or updating a file or
   directory.  Only one file argument is allowed, and it must be the
   last argument on the command line with any option arguments coming
   before it.  */
int proc_touch(int argc, char *argv[])
{
	if (!implicit_update_file(argv[1]))
		return 1;
	return 0;
}

/* Process a copy command.  All command-line options must come first.
   All command-line options except `-s' are ignored, and if a file is
   a directory, the copy command is assumed to be recursive as if an
   `-R' switch were specified.  `-p' and `-T' are implied, whether or
   not they are actually specified.  Only one source file may be
   specified.  Copies may not be used for merging directory contents.
   If they are, then this simplifier will not operate correctly.  */
int proc_cp(int argc, char *argv[])
{
	char *new_argv[] = { argv[0], argv[2], NULL };
	return proc_touch(2, new_argv);
}

/* TODO VERIFY: Why are we specifying "implicit create" on the source
   files?  These must be mandated to exist, we better fix the code so
   that this is the case.  */

/* Process a move command.  All command-line options must come first.
   Command-line options are ignored.  `-T' is implied, whether or not
   it is actually specified.  Only one source file may be specified.
   Unlinking the destination file or directory before moving is
   implied, independent of the command line options.  Directory
   destinations will not be unlinked.  */
int proc_mv(int argc, char *argv[])
{
	/* Judging from only a path name, we don't know if our source is a
	   file or a directory, so assume it to be a directory to be on
	   the safe side.  */
	if (!assume_exist_dir_or_file(argv[1]))
		return 1;
	/* TODO FIXME: Canonicalize the paths before doing the
	   case-insensitive string comparison.  */
	if (!case_insens || strcasecmp(argv[1], argv[2]))
	{
		/* Note: Rename-deleting a directory works only when both the
		   source and the destination are directories and the
		   destination directory is empty.  Because of these
		   constraints, I believe this is a rare encounter in the real
		   world, hence we have no support for it here.  */
		if (!implicit_delete_file(argv[2]))
			return 1;
	}
	if (vfs_rename(new_tree_root, new_cwd, argv[1], argv[2]) == -1)
		return 1;
	return 0;
}

/* Process a remove command.  Command-line switches must come first.
   Only one path name may be specified on the command-line.  `-rf' is
   implied, whether or not it is actually specified.  The path name
   must not contain a trailing slash.  */
int proc_rm(int argc, char *argv[])
{
	if (!assume_exist_file(argv[1]))
		return 1;
	if (fmsimp_unlink(new_tree_root, new_cwd, argv[1]) == -1)
		return 1;
	return 0;
}

int proc_chdir(int argc, char *argv[])
{
	FSNode *new2_cwd;
	if (argc < 2)
		return 1;
	if (!assume_exist_dir(argv[1]))
		return 1;
	new2_cwd = vfs_chdir(new_tree_root, new_cwd, argv[1]);
	if (new2_cwd == NULL)
		return 1;
	new_cwd = new2_cwd;
	old_cwd = new_cwd->other;
	return 0;
}

/********************************************************************/

/* DUPLICATE CODE */
/* If `del_after' is `false', run in "delete before" mode, otherwise
   run in "delete after" mode.

   Returns 0 on success, 1 on failure.  Return value 2 is for internal
   use only, to indicate if a file was deleted.  */
unsigned gen_delete_cmds(FSNode *file, bool del_after)
{
	if ((file->delta_mode & DM_ISDIR))
	{
		unsigned i;
		FSNode **files_d = file->data.files.d;
		unsigned files_len = file->data.files.len;
		for (i = 0; i < files_len; i++)
		{
			FSNode *child = files_d[i];
			unsigned result = gen_delete_cmds(child, del_after);
			if (result == 2)
			{
				bool del_result = fsnode_unlink_index(file, i);
				/* This will never happen...  */
				/* if (del_result == false)
					return 1; */ /* fail */
				i--;
				fsnode_destroy(child);
			}
			/* This will never happen...  */
			/* else if (result == 1)
				return 1; */
		}
	}

	if (file->parent && !file->other)
	{
		/* This file is deleted, check if the name is overwritten in
		   the new parent.  */
		FSNode *parent_other = file->parent->other;
		unsigned index;
		bool condition;
		if (parent_other == NULL)
		{
			/* The parent directory was deleted too, so we know for
			   sure this name can't be overwritten in the new
			   tree.  */
			index = (unsigned)-1;
		}
		else
			index = fsnode_find_name(parent_other, file->name);
		if (del_after)
			condition = (index == (unsigned)-1);
		else
			condition = (index != (unsigned)-1);
		if (condition)
		{
			/* Write out a delete before command for this file.  */
			/* This is a hack, but it works.  */
			char *pathname = vfs_getcwd(NULL, file, NULL, 0);
			char *new_argv[3];
			unsigned len = 0;
			char *delete_cmd =
				((file->delta_mode & DM_ISDIR)) ? "rmdir" : "rm";
			new_argv[len++] = delete_cmd;
			if ((file->delta_mode == DM_IRM) ||
				(file->delta_mode == DM_ISDIR | DM_IRM))
				new_argv[len++] = "-f";
			new_argv[len++] = pathname;
			write_cmdline(stdout, len, new_argv, 0);
			xfree(pathname);

			/* Delete this from the old tree.  */
			return 2;
		}
	}

	return 0;
}

/* Move a conflict name, a name already found in the old tree, out of
   the way.  */
/* TODO FIXME: Vulnerable to stack overflow as we use the call stack
   for pushing state here, which could get quite big under some
   workloads.  */
/* TODO FIXME: Likewise, we do not detect circular dependencies.
   Instead, we overflow the stack.  */
bool move_conflict_name(FSNode *file)
{
	FSNode *other = file->other;
	char *name = file->name;
	unsigned name_len = strlen(name);

	/* If the conflicting name is destined to be deleted, move it to a
	   temporary name.  If lazy deletes mode is enabled, delete it
	   immediately.  */
	if (!other)
	{
		const char *tmpl_suffix = ".XXXXXX";
		unsigned tmpl_suffix_len = 7;
		char *temp_name;
		char *pathname1, *pathname2;
		char *new_argv[3];
		unsigned len = 0;

		temp_name = (char*)xmalloc(name_len + tmpl_suffix_len + 1);
		strcpy(temp_name, name);
		strcpy(temp_name + name_len, tmpl_suffix);
		fmsimp_mktemp(old_tree_root, old_cwd, temp_name);

		new_argv[len++] = "mv";
		/* This is a hack, but it works.  */
		pathname1 = vfs_getcwd(NULL, file, NULL, 0);
		new_argv[len++] = pathname1;

		xfree(name);
		file->name = temp_name;

		pathname2 = vfs_getcwd(NULL, file, NULL, 0);
		new_argv[len++] = pathname2;
		write_cmdline(stdout, len, new_argv, 0);
		xfree(pathname1);
		xfree(pathname2);
	}
	/* Otherwise, check if the conflicting name is destined to be
	   moved out of the way.  */
	else if (other &&
			 (file->parent != other->parent ||
			  strcmp(file->name, other->name)))
	{
		if (!gen_move_cmds(other))
			return false; /* fail */
	}
	/* Otherwise fail.  */
	else
		return false;
	return true;
}

/* DUPLICATE CODE */
bool gen_mkdir_cmd(FSNode *file, bool lazy)
{
	FSNode *other;
	if (!(file->delta_mode & DM_ISDIR))
		return true;

	/* If the current directory was created in the new tree and does
	   not exist in the old tree, create it in the old tree.  */
	other = file->other;
	if (!other)
	{
		FSNode *parent = file->parent;
		FSNode *parent_other = parent->other;

		if (parent_other == NULL)
		{
			if (lazy)
			{
				/* Lazy execute the pending create directory
				   command.  */
				/* DEBUG */
				/* fputs("lazy create 2\n", stderr); */
				gen_mkdir_cmd(parent, lazy);
				parent_other = parent->other;
			}
			else
				return false; /* fail */
		}

		/* Check if the destination file name is available.  If
		   not, move the conflict name out of the way.  */
		unsigned index = fsnode_find_name(parent_other, file->name);
		if (index != (unsigned)-1)
		{
			/* DEBUG */
			/* fputs("Conflict\n", stderr); */
			if (!move_conflict_name(parent_other->data.files.d[index]))
				return false; /* fail */
		}

		/* Execute the mkdir.  */
		other = fsnode_ucreat(parent_other,
							   xstrdup(file->name), DM_ISDIR | DM_SRC);
		if (other == NULL)
			return false; /* fail */
		file->other = other;
		other->other = file;

		{ /* Write out an mkdir command for this file.  */
			/* This is a hack, but it works.  */
			char *pathname = vfs_getcwd(NULL, other, NULL, 0);
			char *new_argv[2];
			unsigned len = 0;
			new_argv[len++] = "mkdir";
			new_argv[len++] = pathname;
			write_cmdline(stdout, len, new_argv, 0);
			xfree(pathname);
		}
	}

	return true;
}

/* DUPLICATE CODE */
bool gen_mkdir_cmds(FSNode *file)
{
	if (!(file->delta_mode & DM_ISDIR))
		return true;
	if (!gen_mkdir_cmd(file, false))
		return false;

	{ /* Now process all files contained.  */
		unsigned i;
		FSNode **files_d = file->data.files.d;
		unsigned files_len = file->data.files.len;
		for (i = 0; i < files_len; i++)
		{
			if (!gen_mkdir_cmds(files_d[i]))
				return false;
		}
	}

	return true;
}

/* DUPLICATE CODE */
bool gen_move_cmds(FSNode *file)
{
	/* Depth-first search the new tree, bringing the old tree
	   up-to-date.  Maintain a stack of dependent commands to run
	   first if necessary, pushing and popping, and detecting circular
	   dependencies.  */
	/* If we find we are about to overwrite a file, we need to push
	   the current command as pending and process the conflicting file
	   first.  */
	/* NOTE: At the final walk, some moves may be over-simplified.
	   Introduce a move to temporary file name if necessary for
	   this.  */
	FSNode *parent = file->parent;
	FSNode *other = file->other;
	if (parent && other)
	{
		FSNode *parent_other = parent->other;
		FSNode *other_parent = other->parent;
		/* This is a move command across directories as the old and
		   new parents are different.  */
		bool move_parents = parent_other != other_parent;
		/* This is a rename within the same directory.  */
		bool move_names = strcmp(file->name, other->name);
		if (move_parents || move_names)
		{
			bool fail = false;
			char *pathname1 = NULL, *pathname2 = NULL;
			char *new_argv[3];
			unsigned len = 0;

			if (parent_other == NULL)
			{
				/* Lazy execute the pending create directory
				   command.  */
				/* DEBUG */
				/* fputs("lazy mkdir\n", stderr); */
				if (!gen_mkdir_cmd(parent, true))
				{ fail = true; goto cleanup; } /* fail */
				parent_other = parent->other;
			}

			/* Check if the destination file name is available.  If
			   not, move the conflict name out of the way.  */
			unsigned index = fsnode_find_name(parent_other, file->name);
			if (index != (unsigned)-1)
			{
				/* DEBUG */
				/* fputs("Conflict\n", stderr); */
				if (!move_conflict_name(parent_other->data.files.d[index]))
				{ fail = true; goto cleanup; } /* fail */
			}

			/* Generate our move command, source part.  */
			new_argv[len++] = "mv";
			/* This is a hack, but it works.  */
			pathname1 = vfs_getcwd(NULL, other, NULL, 0);
			new_argv[len++] = pathname1;

			/* DEBUG */
			/* pathname = vfs_getcwd(NULL, parent_other, NULL, 0);
			fputs(pathname, stderr);
			putc('\n', stderr);
			xfree(pathname);
			pathname = vfs_getcwd(NULL, other_parent, NULL, 0);
			fputs(pathname, stderr);
			putc('\n', stderr);
			xfree(pathname); */

			/* Execute the move.  */
			if (move_parents)
			{
				index = fsnode_find_name(other_parent, other->name);
				if (index == (unsigned)-1)
				{ fail = true; goto cleanup; } /* fail */
				fsnode_unlink_index(other_parent, index);
				fsnode_link(parent_other, other);
			}
			if (move_names)
			{
				xfree(other->name);
				other->name = xstrdup(file->name);
			}

			/* Generate our move command, destination part.  */
			/* This is a hack, but it works.  */
			pathname2 = vfs_getcwd(NULL, other, NULL, 0);
			new_argv[len++] = pathname2;
			write_cmdline(stdout, len, new_argv, 0);
		cleanup:
			xfree(pathname1);
			xfree(pathname2);
			if (fail)
				return false;
		}
	}

	if ((file->delta_mode & DM_ISDIR))
	{
		unsigned i;
		FSNode **files_d = file->data.files.d;
		unsigned files_len = file->data.files.len;
		for (i = 0; i < files_len; i++)
		{
			if (!gen_move_cmds(files_d[i]))
				return false;
		}
	}

	return true;
}

/* DUPLICATE CODE */
bool gen_touch_cmds(FSNode *file)
{
	/* Note: We don't process touch commands on directories.
	   Currently we don't process touch commands on special files
	   either.  */
	if (file->delta_mode == DM_TOUCH)
	{
		/* Execute the touch.  */
		FSNode *other = fsnode_ucreat(file->parent->other,
									  xstrdup(file->name), DM_SRC);
		if (other == NULL)
			return false; /* fail */
		file->other = other;
		other->other = file;

		{ /* Write out a touch command for this file.  */
			/* This is a hack, but it works.  */
			char *pathname = vfs_getcwd(NULL, file, NULL, 0);
			char *new_argv[2];
			unsigned len = 0;
			new_argv[len++] = "touch";
			new_argv[len++] = pathname;
			write_cmdline(stdout, len, new_argv, 0);
			xfree(pathname);
		}
	}

	if ((file->delta_mode & DM_ISDIR))
	{
		unsigned i;
		FSNode **files_d = file->data.files.d;
		unsigned files_len = file->data.files.len;
		for (i = 0; i < files_len; i++)
		{
			if (!gen_touch_cmds(files_d[i]))
				return false; /* fail */
		}
	}

	return true;
}

/* DUPLICATE CODE */
bool compare_final_trees(FSNode *file)
{
	FSNode *other = file->other;
	if (other == NULL)
		return false;
	if (strcmp(file->name, other->name))
		return false;

	if ((file->delta_mode & DM_ISDIR))
	{
		unsigned i;
		FSNode **files_d;
		FSNode **other_files_d;
		unsigned files_len;
		if (!(other->delta_mode & DM_ISDIR))
			return false;
		files_len = file->data.files.len;
		if (other->data.files.len != files_len)
			return false;
		files_d = file->data.files.d;
		other_files_d = other->data.files.d;
		for (i = 0; i < files_len; i++)
		{
			bool result;
			/* NOTE: Since both directory lists are sorted by name, we
			   can walk down comparing identical names in order,
			   rather than needing to search the other directory for
			   the name.  */
			if (strcmp(other_files_d[i]->name, files_d[i]->name))
				return false;
			result = compare_final_trees(files_d[i]);
			if (!result)
				return false;
		}
	}

	return true;
}

/********************************************************************/

void debug_list_dirs(FSNode *file)
{
	/* This is a hack, but it works.  */
	char *pathname = vfs_getcwd(NULL, file, NULL, 0);
	char dir_char = (file->delta_mode & DM_ISDIR) ? 'd' : '-';
	fprintf(stderr,
			"%c %1d 0x%04hx 0x%04hx %s\n",
			dir_char,
			file->delta_mode & ~DM_ISDIR,
			(unsigned short)(unsigned long)file,
			(unsigned short)(unsigned long)file->other,
			pathname);
	xfree(pathname);

	if ((file->delta_mode & DM_ISDIR))
	{
		unsigned i;
		FSNode **files_d = file->data.files.d;
		unsigned files_len = file->data.files.len;
		for (i = 0; i < files_len; i++)
			debug_list_dirs(files_d[i]);
	}
}
