/* fmsimp.c -- File Management Simplifier.

Copyright (C) 2013, 2017 Andrew Makousky

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

/* The file management simplifier processes a list of "touches",
   copies, moves and deletes on standard input and writes out a
   corresponding list to standard output, with redundant move chains
   and touches removed from the list, emitting the minimum number of
   commands to achieve the same net effect.  "Touch" is an abstract
   term for updating a file.  Note that only the simplest input syntax
   is supported.  See the comments above each processing function for
   details.  */

#include <stdio.h>
#include <string.h>

#include "bool.h"
#include "xmalloc.h"
#include "exparray.h"
#include "cmdline.h"
#include "strheap.h"

EA_TYPE(StrNode_ptr_array);

/* `net_cmds' is an array of argument vector arrays.  Each argument is
   a reference-counted string.  It is easier to check previously
   processed commands when the command lines are stored as argument
   vectors rather than a string.  */
StrNode_ptr_array_array net_cmds;

/* Common reference-counted strings.  */
StrNode_ptr touch_ptr;
StrNode_ptr cp_ptr;
StrNode_ptr ln_ptr;
StrNode_ptr mv_ptr;
StrNode_ptr rm_ptr;
StrNode_ptr mkdir_ptr;
StrNode_ptr rmdir_ptr;

/* Command line switches */
bool case_insens = false;

void remove_cmd(unsigned index);
void replace_cmd(unsigned index);
void insert_cmd(unsigned index);
int proc_null(int argc, char *argv[]);
int proc_dedup(int argc, char *argv[]);
int proc_dircmd(int argc, char *argv[]);
int proc_touch(int argc, char *argv[]);
int proc_cp(int argc, char *argv[]);
int proc_mv(int argc, char *argv[]);
int proc_rm(int argc, char *argv[]);
void cvt_mv_to_rm(unsigned cmd_idx);
StrNode_ptr rebase_cmd(StrNode_ptr src, StrNode_ptr dest,
					   StrNode_ptr old_name);

int main(int argc, char *argv[])
{
	int retval = 0;
#define NUM_CMDS 7
	const char *commands[NUM_CMDS] =
		{ "touch", "cp", "mv", "rm", "ln", "mkdir", "rmdir" };
	const CmdFunc cmdfuncs[NUM_CMDS] = { proc_touch, proc_cp,
		proc_mv, proc_rm, proc_cp, proc_dircmd, proc_dircmd };

	if (argc == 2 && !strcmp(argv[1], "-c"))
		case_insens = true;
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

	str_init();
	touch_ptr = str_add(xstrdup("touch"));
	cp_ptr = str_add(xstrdup("cp"));
	ln_ptr = str_add(xstrdup("ln"));
	mv_ptr = str_add(xstrdup("mv"));
	rm_ptr = str_add(xstrdup("rm"));
	mkdir_ptr = str_add(xstrdup("mkdir"));
	rmdir_ptr = str_add(xstrdup("rmdir"));
	EA_INIT(net_cmds, 16);

	/* Process the commands on standard input.  */
	retval = proc_cmd_dispatch(stdin, NUM_CMDS, commands, cmdfuncs, false);

	{ /* Write the net commands to standard output.  */
		unsigned i;
		if (escape_newlines)
			fputs("NL='\n'\n", stdout);
		for (i = 0; i < net_cmds.len; i++)
		{
			/* Build `new_argv' to pass to `write_cmdline()'.  */
			StrNode_ptr_array *cmd = &net_cmds.d[i];
			char **new_argv;
			unsigned j;
			if (cmd->len == 0)
			{
				EA_DESTROY(net_cmds.d[i]);
				continue;
			}
			new_argv = (char**)xmalloc(sizeof(char*) * cmd->len);
			for (j = 0; j < cmd->len; j++)
				new_argv[j] = str_cstr(cmd->d[j]);

			write_cmdline(stdout, cmd->len, new_argv, 0);
			for (j = 0; j < cmd->len; j++)
				xfree(new_argv[j]);
			xfree(new_argv);
			EA_DESTROY(net_cmds.d[i]);
		}
	}

	/* Cleanup */
	EA_DESTROY(net_cmds);
	str_destroy();
	return retval;
}

/* Remove a command from the net command list.  The command list and
   the index to the command to remove should be given.  */
void remove_cmd(unsigned index)
{
	unsigned j;
	for (j = 0; j < net_cmds.d[index].len; j++)
		str_unref(net_cmds.d[index].d[j]);
	EA_REMOVE(net_cmds, index);
}

/* Replace a command in the net command list by moving the last
   command on the list into the given position.  */
void replace_cmd(unsigned index)
{
	unsigned j;
	for (j = 0; j < net_cmds.d[index].len; j++)
		str_unref(net_cmds.d[index].d[j]);
	EA_REMOVE_FAST(net_cmds, index);
}

/* Move a command at the end of the net command list into the position
   of the given index.  The element at that position is moved
   down.  */
void insert_cmd(unsigned index)
{
	EA_INSERT_MULT(net_cmds, index,
				   &net_cmds.d[net_cmds.len-1], 1);
	net_cmds.len--;
}

/* Pass a command through without any special processing.  */
int proc_null(int argc, char *argv[])
{
	int i;
	EA_INIT(net_cmds.d[net_cmds.len], 16);
	for (i = 0; i < argc; i++)
	{
		StrNode *segment = str_add(argv[i]);
		EA_APPEND(net_cmds.d[net_cmds.len], segment);
	}
	EA_ADD(net_cmds);
	xfree(argv);
	return 0;
}

/* Process a command by not adding any redundant occurances of the
   command.  */
int proc_dedup(int argc, char *argv[])
{
	StrNode_ptr_array *new_cmd;
	bool found_cmd = false;
	unsigned i;

	{ /* First add the strings to the string heap.  */
		int j;
		EA_INIT(net_cmds.d[net_cmds.len], 16);
		for (j = 0; j < argc; j++)
		{
			StrNode *segment = str_add(argv[j]);
			EA_APPEND(net_cmds.d[net_cmds.len], segment);
		}
		EA_ADD(net_cmds);
		new_cmd = &net_cmds.d[net_cmds.len-1];
	}

	/* Test for an equivalent command line.  */
	for (i = 0; i < net_cmds.len - 1; i++)
	{
		StrNode_ptr_array *cmd_line = &net_cmds.d[i];
		bool cmd_line_eq = true;
		unsigned j;
		for (j = 0; j < cmd_line->len; j++)
		{
			if (new_cmd->d[j] != cmd_line->d[j])
			{
				cmd_line_eq = false;
				break;
			}
		}
		if (cmd_line_eq)
		{
			found_cmd = true;
			break;
		}
	}

	if (found_cmd)
	{
		/* Remove the command added in anticipation.  */
		int j;
		for (j = 0; j < argc; j++)
			str_unref(new_cmd->d[j]);
		EA_DESTROY(*new_cmd);
		net_cmds.len--;
	}

	xfree(argv);
	return 0;
}

/* Process an `mkdir' or `rmdir' command.  The commands are assumed to
   take no options and only one directory name.  */
int proc_dircmd(int argc, char *argv[])
{
	StrNode_ptr_array *new_cmd;
	bool found_cmd = false;
	unsigned i;

	{ /* First add the strings to the string heap.  */
		int j;
		EA_INIT(net_cmds.d[net_cmds.len], 16);
		for (j = 0; j < argc; j++)
		{
			StrNode *segment = str_add(argv[j]);
			EA_APPEND(net_cmds.d[net_cmds.len], segment);
		}
		EA_ADD(net_cmds);
		new_cmd = &net_cmds.d[net_cmds.len-1];
	}

	/* Test for an `mkdir' or `rmdir' with equivalent arguments after
	   the command name.  */
	for (i = 0; i < net_cmds.len - 1; i++)
	{
		StrNode_ptr_array *cmd_line = &net_cmds.d[i];
		StrNode_ptr cmd_name = cmd_line->d[0];
		bool cmd_line_eq = true;
		unsigned j;
		if (cmd_name == mkdir_ptr || cmd_name == rmdir_ptr)
		{
			for (j = 1; j < cmd_line->len; j++)
			{
				if (new_cmd->d[j] != cmd_line->d[j])
				{
					cmd_line_eq = false;
					break;
				}
			}
			if (cmd_line_eq)
			{
				found_cmd = true;
				break;
			}
		}
	}

	if (found_cmd && net_cmds.d[i].d[0] == new_cmd->d[0])
	{
		/* Remove the command added in anticipation.  */
		int j;
		for (j = 0; j < argc; j++)
			str_unref(new_cmd->d[j]);
		EA_DESTROY(*new_cmd);
		net_cmds.len--;
	}
	else if (found_cmd)
	{
		/* Remove the opposite command that was found.  */
		remove_cmd(i);
	}

	xfree(argv);
	return 0;
}

/* Process a `touch' command.  "Touch" is the generic term for any
   kind of command that requires creating or updating a file or
   directory.  Only one file argument is allowed, and it must be the
   last argument on the command line with any option arguments coming
   before it.  */
int proc_touch(int argc, char *argv[])
{
	StrNode_ptr src;
	unsigned i;

	/* Process the command line.  */
	if (argc < 2)
	{
		int j;
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		for (j = 0; j < argc; j++)
			xfree(argv[j]);
		xfree(argv);
		return 1;
	}
	src = str_add(argv[argc-1]);

	/* Check existing commands in the net command list.  */
	for (i = 0; i < net_cmds.len; i++)
	{
		StrNode_ptr_array *cmd_line = &net_cmds.d[i];
		StrNode_ptr cmd_name = cmd_line->d[0];
		StrNode_ptr cmd_dest = cmd_line->d[cmd_line->len-1];
		char end_char = cmd_dest->d[strlen(src->d)];
		if (cmd_name == touch_ptr ||
			cmd_name == cp_ptr ||
			cmd_name == ln_ptr ||
			cmd_name == mkdir_ptr ||
			cmd_name == rmdir_ptr ||
			cmd_name == rm_ptr)
		{
			/* Check if the destination pathname is the same file or
			   under the directory we are touching.  */
			if (str_prefix(cmd_dest, src) &&
				(end_char == '/' || end_char == '\0'))
			{
				remove_cmd(i);
				i--; /* Don't skip the next command.  */
			}
		}
		else if (cmd_name == mv_ptr)
		{
			/* Check if the destination pathname is the same file or
			   under the directory we are touching.  */
			if (str_prefix(cmd_dest, src) &&
				(end_char == '/' || end_char == '\0'))
				cvt_mv_to_rm(i);
		}
	}

	{ /* Add the touch command to the net command list.  */
		int j;
		EA_INIT(net_cmds.d[net_cmds.len], 16);
		for (j = 0; j < argc - 1; j++)
		{
			StrNode *segment = str_add(argv[j]);
			EA_APPEND(net_cmds.d[net_cmds.len], segment);
		}
		EA_APPEND(net_cmds.d[net_cmds.len], src);
		EA_ADD(net_cmds);
	}
	xfree(argv);
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
	StrNode_ptr src, dest;
	unsigned i;

	/* Process the command line.  */
	if (argc < 3)
	{
		int j;
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		for (j = 0; j < argc; j++)
			xfree(argv[j]);
		xfree(argv);
		return 1;
	}
	{
		bool make_symlink = false;
		int j;
		for (j = 1; j < argc - 2 && !make_symlink; j++)
		{
			if (argv[j][0] == '-' && argv[j][1] != '-')
			{
				char *argvwalk = argv[j];
				while (*(++argvwalk))
					if (*argvwalk == 's')
					{
						make_symlink = true;
						break;
					}
			}
		}
		if (make_symlink)
			return proc_touch(argc, argv);
	}
	src = str_add(argv[argc-2]);
	dest = str_add(argv[argc-1]);

	/* Check for existing commands in the net command list.  */
	for (i = 0; i < net_cmds.len; i++)
	{
		StrNode_ptr_array *cmd_line = &net_cmds.d[i];
		StrNode_ptr cmd_name = cmd_line->d[0];
		if (cmd_name == cp_ptr ||
			cmd_name == ln_ptr ||
			cmd_name == mv_ptr)
		{
			/* Check the source name of this copy command and the
			   destination name of the other command to see if there
			   is a link.  */
			if (src == cmd_line->d[cmd_line->len-1])
			{
				/* If a file is moved to a new name then copied back
				   to the original name, just use one copy command
				   instead of a move followed by a copy.  */
				if (dest == cmd_line->d[cmd_line->len-2])
				{
					/* Delete the move.  */
					remove_cmd(i);
					i--; /* Don't skip the next command.  */
					{ /* Swap the source and destination of the
					     copy.  */
						StrNode_ptr temp;
						temp = src;
						src = dest;
						dest = temp;
					}
				}
			}
			/* Remove any commands that get stomped out by this copy
			   command.  Note that copy commands do not automatically
			   recursively delete an existing destination
			   directory.  */
			else if (dest == cmd_line->d[cmd_line->len-1])
			{
				if (cmd_name == mv_ptr)
					cvt_mv_to_rm(i);
				else
				{
					remove_cmd(i);
					i--; /* Don't skip the next command.  */
				}
			}
		}
		/* Remove any touch or remove commands that get stomped out by
		   this copy command.  */
		else if ((cmd_name == rm_ptr || cmd_name == touch_ptr) &&
				 cmd_line->d[cmd_line->len-1] == dest)
		{
			remove_cmd(i);
			i--; /* Don't skip the next command.  */
		}
	}

	{ /* Add the copy command to the net command list.  */
		int j;
		EA_INIT(net_cmds.d[net_cmds.len], 16);
		for (j = 0; j < argc - 2; j++)
		{
			StrNode *segment = str_add(argv[j]);
			EA_APPEND(net_cmds.d[net_cmds.len], segment);
		}
		EA_APPEND(net_cmds.d[net_cmds.len], src);
		EA_APPEND(net_cmds.d[net_cmds.len], dest);
		EA_ADD(net_cmds);
	}
	xfree(argv);
	return 0;
}

/* Process a move command.  All command-line options must come first.
   Command-line options are ignored.  `-T' is implied, whether or not
   it is actually specified.  Only one source file may be specified.
   Unlinking the destination file or directory before moving is
   implied, independent of the command line options.  Directory
   destinations will not be unlinked.  */
int proc_mv(int argc, char *argv[])
{
	StrNode_ptr src, dest;
	unsigned i;
	bool move_coalesced = false;
	unsigned move_idx;
	bool case_insens_trigger = false;
	bool move_to_self = false;

	/* Process the command line.  */
	if (argc < 3)
	{
		int j;
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		for (j = 0; j < argc; j++)
			xfree(argv[j]);
		xfree(argv);
		return 1;
	}
	src = str_add(argv[argc-2]);
	dest = str_add(argv[argc-1]);

	/* Check for existing commands in the net command list.  */
	for (i = 0; i < net_cmds.len; i++)
	{
		StrNode_ptr_array *cmd_line = &net_cmds.d[i];
		StrNode_ptr cmd_name = cmd_line->d[0];
		if (cmd_name == mv_ptr &&
			src == cmd_line->d[cmd_line->len-1])
		{
			move_coalesced = true;
			move_idx = i;
			if (cmd_line->d[cmd_line->len-2] != dest)
			{
				/* Change the existing move command to be the net move
				   command of the move command chain.  */
				str_unref(cmd_line->d[cmd_line->len-1]);
				cmd_line->d[cmd_line->len-1] = dest;
			}
			else
			{
				/* Delete this move command rather than moving a file
				   to itself.  `move_coalesced' is still set to `true'
				   so that any intermediate commands will be properly
				   rebased.  */
				move_to_self = true;
				remove_cmd(i);
				i--; /* Don't skip the next command.  */
			}
			/* Note: A special case of move-to-self occurs on non-Unix
			   filesystems that match file names case-insensitively:
			   if the source or destination file names differ only by
			   case, then moving from one file name to another should
			   result in the move command being rebased, but the
			   remove command should not be added.  */
			if (case_insens &&
				/* TODO FIXME nonstandard */
				!strcasecmp(src->d, dest->d))
				case_insens_trigger = true;
		}
		else if (cmd_name == touch_ptr ||
				 cmd_name == rm_ptr ||
				 cmd_name == cp_ptr ||
				 cmd_name == ln_ptr ||
				 cmd_name == mkdir_ptr ||
				 cmd_name == rmdir_ptr ||
				 cmd_name == mv_ptr)
		{
			/* Check if the move command stomps out the file of this
			   command.  Note that move commands do not automatically
			   recursively delete an existing destination
			   directory.  */
			if (dest == cmd_line->d[cmd_line->len-1])
			{
				/* Delete the relevant command.  */
				remove_cmd(i);
				i--; /* Don't skip the next command.  */
			}
			else if (move_coalesced)
			{
				/* Check if this command needs to be rebased because
				   of this move simplification.  */
				StrNode_ptr cmd_src = cmd_line->d[cmd_line->len-2];
				StrNode_ptr cmd_dest = cmd_line->d[cmd_line->len-1];

				char end_char = cmd_src->d[strlen(src->d)];
				if ((cmd_name == cp_ptr || cmd_name == ln_ptr ||
					 cmd_name == mv_ptr) && str_prefix(cmd_src, src) &&
					(end_char == '/' || end_char == '\0'))
					cmd_line->d[cmd_line->len-2] =
						rebase_cmd(src, dest, cmd_src);

				end_char = cmd_dest->d[strlen(src->d)];
				if (str_prefix(cmd_dest, src) &&
					(end_char == '/' || end_char == '\0'))
					cmd_line->d[cmd_line->len-1] =
						rebase_cmd(src, dest, cmd_dest);
			}
		}
	}
	if (move_coalesced)
	{
		if (!case_insens_trigger)
		{
			/* Add a remove command for the source of this move.  It
			   is not necessary to do full simplification processing
			   on the new remove command because any commands that
			   would have been clobbered were already deleted from
			   move processing.  */
			EA_INIT(net_cmds.d[net_cmds.len], 16);
			EA_APPEND(net_cmds.d[net_cmds.len], rm_ptr);
			str_ref(rm_ptr);
			EA_APPEND(net_cmds.d[net_cmds.len], src);
			/* `src' memory ownership is passed on.  */
			EA_ADD(net_cmds);
			/* Make sure to add the remove before the existing move
			   command.  */
			insert_cmd(move_idx);
		}

		{ /* Cleanup */
			unsigned j;
			for (j = 0; j < argc - 2; j++)
				xfree(argv[j]);
			xfree(argv);
			if (case_insens_trigger)
				str_unref(src);
			if (move_to_self)
				str_unref(dest);
		}
		return 0;
	}

	{ /* Add the move command to the net command list.  */
		int j;
		EA_INIT(net_cmds.d[net_cmds.len], 16);
		for (j = 0; j < argc - 2; j++)
		{
			StrNode *segment = str_add(argv[j]);
			EA_APPEND(net_cmds.d[net_cmds.len], segment);
		}
		EA_APPEND(net_cmds.d[net_cmds.len], src);
		EA_APPEND(net_cmds.d[net_cmds.len], dest);
		EA_ADD(net_cmds);
	}
	xfree(argv);
	return 0;
}

/* Process a remove command.  Command-line switches must come first.
   Only one path name may be specified on the command-line.  `-rf' is
   implied, whether or not it is actually specified.  The path name
   must not contain a trailing slash.  */
int proc_rm(int argc, char *argv[])
{
	StrNode_ptr src;
	unsigned i;

	/* Process the command line.  */
	if (argc < 2)
	{
		int j;
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		for (j = 0; j < argc; j++)
			xfree(argv[j]);
		xfree(argv);
		return 1;
	}
	src = str_add(argv[argc-1]);

	/* Check existing commands in the net command list.  */
	for (i = 0; i < net_cmds.len; i++)
	{
		StrNode_ptr_array *cmd_line = &net_cmds.d[i];
		StrNode_ptr cmd_name = cmd_line->d[0];
		StrNode_ptr cmd_dest = cmd_line->d[cmd_line->len-1];
		char end_char = cmd_dest->d[strlen(src->d)];
		if (cmd_name == touch_ptr ||
			cmd_name == rm_ptr ||
			cmd_name == mkdir_ptr ||
			cmd_name == rmdir_ptr ||
			cmd_name == cp_ptr ||
			cmd_name == ln_ptr)
		{
			/* Check if the destination name is under the deleted
			   directory.  */
			if (str_prefix(cmd_dest, src) &&
				(end_char == '/' || end_char == '\0'))
			{
				remove_cmd(i);
				i--; /* Don't skip the next command.  */
			}
		}
		else if (cmd_name == mv_ptr)
		{
			/* Check if the destination name is under the deleted
			   directory.  */
			if (str_prefix(cmd_dest, src) &&
				(end_char == '/' || end_char == '\0'))
				cvt_mv_to_rm(i);
		}
	}

	{ /* Add the remove command to the net command list.  */
		int j;
		EA_INIT(net_cmds.d[net_cmds.len], 16);
		for (j = 0; j < argc - 1; j++)
		{
			StrNode *segment = str_add(argv[j]);
			EA_APPEND(net_cmds.d[net_cmds.len], segment);
		}
		EA_APPEND(net_cmds.d[net_cmds.len], src);
		EA_ADD(net_cmds);
	}
	xfree(argv);
	return 0;
}

/* Replace a move command with a remove command for the source
   name.  */
void cvt_mv_to_rm(unsigned cmd_idx)
{
	StrNode_ptr_array *cmd_line = &net_cmds.d[cmd_idx];
	/* Copy strings to prepare for function calls.  */
	char **new_argv = (char**)xmalloc(sizeof(char*) * 2);
	new_argv[0] = xstrdup("rm");
	new_argv[1] = str_cstr(cmd_line->d[cmd_line->len-2]);
	/* Replace the move comand with a remove for the source name.
	   Luckily, we don't need to recursively do remove checks on other
	   move command related to this one thanks to move
	   simplification.  */
	/* rm_scan(cmd_line->d[cmd_line->len-2], cmd_idx); */
	proc_null(2, new_argv);
	replace_cmd(cmd_idx);
}

/* Take a string `old_name' and use the prefixes `src' and `dest' to
   rebase it and return the new string heap node.  The old one is
   automatically dereferenced.  */
StrNode_ptr rebase_cmd(StrNode_ptr src, StrNode_ptr dest,
					   StrNode_ptr old_name)
{
	unsigned pfx_src_len = strlen(src->d);
	unsigned pfx_dest_len = strlen(dest->d);
	char *new_name =
		(char*)xmalloc(sizeof(char) *
					   (strlen(old_name->d) - pfx_src_len +
						pfx_dest_len + 1));
	strcpy(new_name, dest->d);
	strcpy(new_name + pfx_dest_len, old_name->d + pfx_src_len);
	str_unref(old_name);
	return str_add(new_name);
}
