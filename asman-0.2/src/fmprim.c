/* fmprim.c -- Translate all the nastiest and most sophisticated
   command line syntaxes of file management commands into something
   simple enough to be processed in the Archive System Management
   Tools pipeline.  Currently, this program only performs the
   following simplifications:

   * Moving all file name arguments to come after option switches.

   * Translating symlinked paths into canonical paths.

   * Translating commands with multiple file arguments into a series
     of commands with only one or two file arguments.

   * Converting the destination directory arguments for `cp', `mv',
     and `ln' when special cases occur.

   * Handling the `-T' option for `cp', `mv', and `ln'.

   * Copy commands that result in merging directories and broken down
     into a series of smaller commands.

   * Errors are issued if the "interactive" switch is set in any input
     commands.

   * Sequences of two or more slashes are reduced to one slash.

   * Trailing slashes are removed.

   * Excess "." and ".." entries in path names are simplified.

   Any other cases are bound to cause trouble further down the line.
   Unrecognized options that take an argument will be misinterpreted
   as another file name.  However, it would be a good idea to extend
   this program to perform more simplifications:

   * Eliminating invalid commands.

   * Processing GNU long options.

   * Processing all GNU coreutils options.

*/

/*

Check if the source argument is not a directory.  If the destination
argument is a directory, then change it to be a destination argument
on a file.  Make sure that there is not a directory of the same name
as the file.  If there is, then delete this erroneous command and
issue a diagnostic.

Check if the source argument is a directory.  If it is, then check if
the destination argument is a directory.  If no such file exists,
good.  If a directory exists, then change the destination argument to
be on the final destination name.  Now check the final destination
name.  If a non-directory exists, delete this erroneous command and
issue a diagnostic.  If a directory exists, issue a diagnostic error
for moves but not for copies.  For copies, make sure to add the `-T'
option so that the destination name will get striped off later.
Sorry... deadlock... simplification impossible.  Well, I guess I will
have to issue that message too when it happens.  This could happen if
there is just the wrong patterning of directories and directory
arguments.  Why in the world do the Unix tools have to have so much
special treatment of directory arguments by default?  And why can
moves silently unlink in their default behavior.  This is by far the
most annoying default behavior of Unix to me when I write the archive
system management tools.

mkdir -p droot/dnew
mkdir dcopy
cat <<EOF > droot/dnew/oldfile
This is the old file.  It will be in the original directory.
EOF
cat <<EOF > dcopy/newfile
This is the new file.  It will be in the copied directory.
EOF

# The idea is to copy the directory `dcopy' to `droot/dnew', placing
# `newfile' in the directory `droot/dnew', but this will not happen
# due to special treatment of directory arguments.  Thus, using GNU
# coreutils and specifiying the `-T' argument is essential.  Removing
# the `-T' argument later on will be impossible.

cp dcopy droot/dnew

Warning: Found `-T' option that cannot be removed under
simplification.  Actually this is not a warning, as the user who
generated it probably has GNU coreutils.

# Wait!  There is one possibility that I didn't think of: wildcards.
# But that will only work when the user does not care to copy their
# dotfiles that won't get matched.

Okay, the real solution has been found.  In the case of copy merges,
break up the copy command into a series of simpler, non-clobbering
commands.

Symlinks are treated as touches.  Hard links are treated as copies.

Check if there are more than two file arguments.  If there are, then
translate them to be commands that operate two at a time.  Use the
previous directory special case simplifications to handle each
translation.

Check for deletes.  If any are found, then just read the file
arguments and change the option flags to what is expected by the
archive system management tools.

In all cases, option flags must be homogenized into what the archive
system management tools expect.  Otherwise, `fmsimp' may not work well
on the input.

Gosh, if any one thread really trancends the archive system management
tools, it is simplifying the input so that the other tools that do the
real processing can work without problems.

In conclusion, the only times there are ever real problems with
simplifications is when files and directories already exist and the
Unix tools have to deal with a potentially clobbering operation.

But when the update script is generated, touch commands are always
safely translated.

 */

#include <stdio.h>

int main()
{
	return 0;
}
