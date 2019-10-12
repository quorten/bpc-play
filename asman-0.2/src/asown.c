/* Change the user of each file to `asman' and the group to `htrust'.
   This command is designed to only change the user and group of files
   within asman's home directory.

Copyright (C) 2013 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>

int main (int argc, char *argv[])
{
  int i;
  char workdir[PATH_MAX];
  int inhome;
  struct passwd *pw = getpwnam ("asman");
  struct group *gr = getgrnam ("htrust");
  if (pw == NULL || gr == NULL ||
      getcwd(workdir, PATH_MAX) == NULL)
    return 1;
  inhome = !strncmp(workdir, "/home/asman", 11);

/*

Parse through the entire string
Count the number of directory components
Subtract one for every ..
Verify the number of directories never drops below two.

If there is no prefix
Verify that the number of directories never drops below zero.

*/

  for (i = 1; i < argc; i++)
    {
      int absdir = (argv[i][0] == '/') ? 1 : 0;
      int absdir_inhome = 0;
      int dir_level = 0;
      char *dir_part = &argv[i][absdir];
      int j;
      inhome = 1;

      for (j = absdir; argv[i][j] != '\0'; j++)
	{
	  if (argv[i][j] == '/')
	    {
	      argv[i][j] = '\0';
	      dir_part = &argv[i][j+1];
	      dir_level++;

	      if (absdir &&
		  dir_level == 1 &&
		  !strcmp(dir_part, "home"))
		;

	      if (!strcmp(dir_part, ".."))
		dir_level -= 2;
	      if (dir_level < 0)
		{ inhome = 0; break; }
	      if (absdir && !absdir_inhome)
		{ inhome = 0; break; }
	      if (absdir && absdir_inhome && dir_level <= 2)
		{ inhome = 0; break; }
	    }
	}

      if (inhome)
	{
	  if (chown (argv[i], pw->pw_uid, gr->gr_gid))
	    {
	      fprintf(stderr, "%s: ", argv[0]);
	      perror ("error changing ownership");
	    }
	}
      else
	{
	  fprintf (stderr, "%s: file not within asman's home: %s\n",
		   argv[0], argv[i]);
	  continue;
	}
    }

  return 0;
}
