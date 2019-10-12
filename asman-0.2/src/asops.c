/* asops.c -- Process arbitrary file management operations and output
   commands that only operate on the archive root.

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

#include <stdio.h>
#include <string.h>

#include "bool.h"
#include "xmalloc.h"
#include "exparray.h"
#include "cmdline.h"
#include "shenv.h"

enum ScriptOp { CMD_DELSUFFIX, CMD_DELPREFIX, CMD_DELPART, CMD_PREFIX };
struct ScriptCmd_tag
{
	enum ScriptOp op;
	char *str;
};
typedef struct ScriptCmd_tag ScriptCmd;
EA_TYPE(ScriptCmd);

char *prog_name;
ScriptCmd_array script;

int proc_touch(int argc, char *argv[]);
int proc_cp(int argc, char *argv[]);
int proc_mv(int argc, char *argv[]);
int proc_rm(int argc, char *argv[]);

bool script_init(const char *script_file);
void script_shutdown();
bool is_inside(const char *path);

int main(int argc, char *argv[])
{
	int retval;
	char *script_file;
#define NUM_CMDS 7
	const char *commands[NUM_CMDS] =
		{ "touch", "cp", "mv", "rm", "ln", "mkdir", "rmdir" };
	const CmdFunc cmdfuncs[NUM_CMDS] = { proc_touch, proc_cp, proc_mv,
					 proc_touch, proc_cp, proc_touch, proc_touch };

	if (argc == 2 && (!strcmp(argv[1], "-h") ||
					  !strcmp(argv[1], "--help")))
	{
		printf("Usage: %s SCRIPT < CMDS > FILT-CMDS\n", argv[0]);
		puts(
"Filter commands to be specific to an archive root.\n"
"\n"
"SCRIPT is a filter script that is called to evaluate file names.  This file\n"
"takes the form of a shell script.  The following commands are recognized:\n"
"        delsuffix       Delete any path with the given suffix.\n"
"        delprefix       Delete any path with the given prefix.\n"
"        delpart         Delete any path with the given string.\n"
"        prefix          Only keep paths with the given prefix.");
		return 0;
	}

	if (argc == 3 && !strcmp(argv[1], "--"))
		script_file = argv[2];
	else if (argc != 2)
	{
		fprintf(stderr, "%s: invalid command line\n", argv[0]);
		fprintf(stderr, "Type `%s --help' for more information.\n", argv[0]);
		return 1;
	}
	else
		script_file = argv[1];
	prog_name = argv[0];

	if (!script_init(script_file))
		return 1;
	if (escape_newlines)
		fputs("NL='\n'\n", stdout);
	retval = proc_cmd_dispatch(stdin, NUM_CMDS, commands, cmdfuncs, true);
	script_shutdown();
	return retval;
}

/* Process a `touch' command.  "Touch" is the generic term for any
   kind of command that requires creating or updating a file or
   directory.  */
int proc_touch(int argc, char *argv[])
{
	char *src;

	if (argc < 2)
	{
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		return 1;
	}
	src = argv[argc-1];

	if (is_inside(src))
	{
		/* Pass the command through verbatim.  */
		write_cmdline(stdout, argc, argv, 0);
	}
	return 0;
}

/* Process a copy command.  All command-line options must come first.
   Command-line options are ignored, and if a file is a directory, the
   copy command is assumed to be recursive as if an `-R' switch were
   specified.  `-p' and `-T are implied, whether or not they are
   actually specified.  Only one source file may be specified.  */
int proc_cp(int argc, char *argv[])
{
	char *src, *dest;

	if (argc < 3)
	{
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		return 1;
	}
	if (!strcmp(argv[0], "ln") && !strcmp(argv[1], "-s"))
		return proc_touch(argc, argv);
	src = argv[argc-2];
	dest = argv[argc-1];

	if (is_inside(dest))
	{
		if (is_inside(src))
		{
			/* Pass the command through verbatim.  */
			write_cmdline(stdout, argc, argv, 0);
		}
		else
		{
			/* Write a touch command instead.  */
			char *new_argv[2];
			new_argv[0] = "touch";
			new_argv[1] = dest;
			write_cmdline(stdout, 2, new_argv, 0);
		}
	}
	/* else Ignore this unrelated command.  */
	return 0;
}

/* Process a move command.  All command-line options must come first.
   Command-line options are ignored.  `-T' is implied, whether or not
   it is actually specified.  Only one source file may be
   specified.  */
int proc_mv(int argc, char *argv[])
{
	char *src, *dest;

	if (argc < 3)
	{
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		return 1;
	}
	src = argv[argc-2];
	dest = argv[argc-1];

	if (is_inside(src))
	{
		if (is_inside(dest))
		{
			/* Pass the command through verbatim.  */
			write_cmdline(stdout, argc, argv, 0);
		}
		else
		{
			/* Write a remove command instead.  */
			char *new_argv[3];
			new_argv[0] = "rm";
			new_argv[1] = "-rf";
			new_argv[2] = src;
			write_cmdline(stdout, 3, new_argv, 0);
		}
	}
	else if (is_inside(dest))
	{
		/* Write a touch command instead.  */
		char *new_argv[2];
		new_argv[0] = "touch";
		new_argv[1] = dest;
		write_cmdline(stdout, 2, new_argv, 0);
	}
	/* else Ignore this unrelated command.  */
	return 0;
}

/* Evaluation script processing

   Basically, an evaluation script is treated just like an ordinary
   shell script, and various functions are called for various
   commands.  */

bool script_init(const char *script_file)
{
	int argc; char **argv;
	int read_status;
	FILE *fp_script = fopen(script_file, "r");
	if (fp_script == NULL)
	{
		fprintf(stderr, "%s: ", prog_name);
		perror(script_file);
		return false;
	}

	/* Read and parse the entire script ahead of time.  */
	EA_INIT(script, 16);
	while ((read_status = read_cmdline(fp_script, &argc, &argv)) != -1)
	{
		char *cmd_name;
		if (argc == 0)
		{
			xfree(argv);
			if (read_status == 0)
				break;
			last_line_num = line_num;
			continue;
		}
		cmd_name = argv[0];
		if (!strcmp(cmd_name, "delsuffix"))
			script.d[script.len].op = CMD_DELSUFFIX;
		else if (!strcmp(cmd_name, "delprefix"))
			script.d[script.len].op = CMD_DELPREFIX;
		else if (!strcmp(cmd_name, "delpart"))
			script.d[script.len].op = CMD_DELPART;
		else if (!strcmp(cmd_name, "prefix"))
			script.d[script.len].op = CMD_PREFIX;
		else
		{
			fprintf(stderr, "%s:%u: error: invalid command: %s\n",
					script_file, last_line_num, cmd_name);
			goto error;
		}
		if (argc != 2)
		{
			fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
					last_line_num, argv[0]);
			goto error;
		}
		script.d[script.len].str = argv[1];
		EA_ADD(script);
		xfree(argv[0]); xfree(argv);

		if (read_status == 0)
			break;
		last_line_num = line_num;
	}

	if (read_status == -1) /* fatal parse error */
		goto error;
	fclose(fp_script);
	return true;
error:
	{
		int i;
		for (i = 0; i < argc; i++)
			xfree(argv[i]);
		xfree(argv);
	}
	script_shutdown();
	fclose(fp_script);
	return false;
}

void script_shutdown()
{
	unsigned i;
	for (i = 0; i < script.len; i++)
		xfree(script.d[i].str);
	EA_DESTROY(script);
}

/* Delete a path name if it has the specified path suffix.  */
bool is_inside(const char *path)
{
	unsigned i;
	for (i = 0; i < script.len; i++)
	{
		switch (script.d[i].op)
		{
		case CMD_DELSUFFIX:
			if (strstr(path, script.d[i].str) ==
				path + strlen(path) - strlen(script.d[i].str))
				return false;
			break;
		case CMD_DELPREFIX:
			if (strstr(path, script.d[i].str) == path)
				return false;
			break;
		case CMD_DELPART:
			if (strstr(path, script.d[i].str) != NULL)
				return false;
			break;
		case CMD_PREFIX:
			if (strstr(path, script.d[i].str) != path)
				return false;
			break;
		}
	}
	return true;
}

#ifdef SED_SCRIPT

/* `sed' based script evaluation.  This kind of evaluation is way too
   slow.  */

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int child_exec_error = 0;

/* Generic function to check if a path should be passed through as an
   update.  */
int is_inside(const char *path)
{
	pid_t pid;
	int in_pipefd[2]; /* Input into child */
	int out_pipefd[2]; /* Output from child */
	if (pipe(in_pipefd) == -1 || pipe(out_pipefd) == -1)
	{
		perror(prog_name);
		goto error;
	}
	pid = fork();
	if (pid == -1)
	{
		perror(prog_name);
		close(in_pipefd[0]); close(in_pipefd[1]);
		close(out_pipefd[0]); close(out_pipefd[1]);
		goto error;
	}
	if (pid == 0) /* Child */
	{
		int retval;
		dup2(in_pipefd[0], 0);
		dup2(out_pipefd[1], 1);
		close(in_pipefd[0]); close(in_pipefd[1]);
		close(out_pipefd[0]); close(out_pipefd[1]);
		retval = execlp("sed", "sed", "-nf", script_file, NULL);
		/* error */
		perror("child");
		exit(retval);
	}
	else /* Parent */
	{
		FILE *fin_pipe, *fout_pipe;
		int ch;
		int status;
		close(in_pipefd[0]);
		close(out_pipefd[1]);
		fin_pipe = fdopen(in_pipefd[1], "w");
		fout_pipe = fdopen(out_pipefd[0], "r");
		if (fin_pipe == NULL || fout_pipe ==  NULL)
		{
			perror(prog_name);
			goto error;
		}
		fprintf(fin_pipe, "%s\n", path);
		fclose(fin_pipe);
		waitpid(pid, &status, 0);
		ch = getc(fout_pipe);
		fclose(fout_pipe);
		if (ch == EOF) /* Path was deleted */
			return 0;
		else
			return 1;
	}

error:
	fprintf(stderr, "%s: the results will be incorrect\n", prog_name);
	child_exec_error |= 1;
	return 0;
}

#endif

#if 0

/* Experimental code for spawning `sed' once and communicating through
   pipes.  This code currently doesn't work.  Even the older version
   that partially worked was too slow because of all the process
   spawning.  */
#include <fcntl.h>

/* `sed' child process variables */
pid_t sed_pid;
FILE *fin_pipe; /* Input into child */
FILE *fout_pipe; /* Output from child */

bool sed_init(const char *script_file);
bool is_inside(const char *path);
int sed_shutdown();

int more_main()
{
	int retval;
	...;

	if (!sed_init(argv[1]))
	{
		fprintf(stderr, "%s: could not spawn `sed' child process\n",
				prog_name);
		return 1;
	}
	retval = proc_cmd_dispatch(stdin, NUM_CMDS, commands, cmdfuncs, true);
	sed_shutdown();
	return retval;
}

/* Initialize the `sed' child process.  */
bool sed_init(const char *script_file)
{
	int in_pipefd[2]; /* Input into child */
	int out_pipefd[2]; /* Output from child */
	if (pipe(in_pipefd) == -1 || pipe(out_pipefd) == -1)
	{
		perror(prog_name);
		return false;
	}
	sed_pid = fork();
	if (sed_pid == -1)
	{
		perror(prog_name);
		close(in_pipefd[0]); close(in_pipefd[1]);
		close(out_pipefd[0]); close(out_pipefd[1]);
		return false;
	}
	if (sed_pid == 0) /* Child */
	{
		int retval;
		dup2(in_pipefd[0], 0);
		dup2(out_pipefd[1], 1);
		close(in_pipefd[0]); close(in_pipefd[1]);
		close(out_pipefd[0]); close(out_pipefd[1]);
		retval = execlp("sed", "sed", "-nf", script_file, NULL);
		/* error */
		perror("child");
		exit(retval);
	}
	else /* Parent */
	{
		close(in_pipefd[0]);
		close(out_pipefd[1]);
		fin_pipe = fdopen(in_pipefd[1], "w");
		fout_pipe = fdopen(out_pipefd[0], "r");
		if (fin_pipe == NULL || fout_pipe ==  NULL)
		{
			perror(prog_name);
			return false;
		}
		if (fcntl(fileno(fout_pipe), F_SETFL, O_NONBLOCK) == -1)
		{
			perror(prog_name);
			return false;
		}
		setvbuf(fout_pipe, NULL, _IONBF, 0);
		setvbuf(fin_pipe, NULL, _IONBF, 0);
		return true;
	}
	return false;
}

/* Generic function to check if a path should be passed through as an
   update.  */
bool is_inside(const char *path)
{
	int ch;
	fprintf(fin_pipe, "%s\n", path);
	fflush(fin_pipe);
	ch = getc(fout_pipe);
	if (ch == EOF) /* Path was deleted */
		return false;
	/* Read the rest of the line.  */
	while (getc(fout_pipe) != EOF)
		/* empty loop */;
	return true;
}

/* Shutdown the `sed' child process.  */
int sed_shutdown()
{
	int status;
	char buffer[1024];
	fclose(fin_pipe);
	waitpid(sed_pid, &status, 0);
	fputs("Was there pending output?\n", stderr);
	while (fgets(buffer, 1024, fout_pipe))
		fputs(buffer, stderr);
	fclose(fout_pipe);
	return WEXITSTATUS(status);
}

#endif
