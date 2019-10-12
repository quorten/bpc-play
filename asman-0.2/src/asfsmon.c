/* asfsmon.c -- Monitor a filesystem for changes.

Copyright (C) 2013 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/inotify.h>

/* Recursively descend the directory(ies) to monitor.  */
/* Add watches to all directories.  */
/* Check the events received: */
/* If IN_MOVED_FROM and IN_MOVED_TO are found, link them by the cookie */
/* For IN_CREATE and IN_DELETE, touch or rm the respective entires.  */
/* For IN_ATTRIB and IN_CLOSE_WRITE, add a touch.  */

int main()
{
	int fd = inotify_init();
	int wd1, wd2;

	if (fd == -1)
	{
		perror("inotify_init");
		return 1;
	}

	wd1 = inotify_add_watch(
		fd, "/home/andrew/asman-0.2/tests",
		IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE |
		IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
	if (wd1 == -1)
	{
		perror("inotify_add_watch");
		return 1;
	}

	wd2 = inotify_add_watch(
		fd, "/home/andrew/asman-0.2/src",
		IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE |
		IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
	if (wd2 == -1)
	{
		perror("inotify_add_watch");
		return 1;
	}

	while (1)
	{
		struct inotify_event event[16];
		int num_read;
		int i;
		num_read = read(fd, &event, sizeof(struct inotify_event) * 16);
		if (num_read == -1)
		{
			perror("read");
			if (errno == EINVAL)
				puts("Try again with a larger buffer.");
			break;
		}
		if (num_read == 0)
			break;
		num_read /= sizeof(struct inotify_event);

		for (i = 0; i < num_read; i++)
		{
			if (event[i].wd == wd1)
				puts("Event read from ~/asman-0.2/tests");
			else if (event[i].wd == wd2)
				puts("Event read from ~/asman-0.2/src");
			else
			{
				/* printf("Event read from unknown descriptor: %d\n",
					   event[i].wd);
				puts("Ignored");
				puts(""); */
				continue;
			}
			puts("Events:");
			if (event[i].mask & IN_ATTRIB)
				puts("IN_ATTRIB");
			if (event[i].mask & IN_CLOSE_WRITE)
				puts("IN_CLOSE_WRITE");
			if (event[i].mask & IN_CREATE)
				puts("IN_CREATE");
			if (event[i].mask & IN_DELETE)
				puts("IN_DELETE");
			if (event[i].mask & IN_DELETE_SELF)
				puts("IN_DELETE_SELF");
			if (event[i].mask & IN_MOVE_SELF)
				puts("IN_MOVE_SELF");
			if (event[i].mask & IN_MOVED_FROM)
				puts("IN_MOVED_FROM");
			if (event[i].mask & IN_MOVED_TO)
				puts("IN_MOVED_TO");
			/* Should also check for IN_IGNORED, IN_Q_OVERFLOW, and
			   IN_UNMOUNT.  */
			/* To guarantee correct moves, compare the inode numbers
			   of the FROM and TO files.  */
			/* IN_DELETE_SELF and IN_MOVE_SELF do not use this field.  */
			printf("Name: %s\n", event[i].name);
			puts("");
		}
	}

	inotify_rm_watch(fd, wd1);
	inotify_rm_watch(fd, wd2);
	close(fd);
	return 0;
}

#ifdef FREEBSD

/* File alteration monitor for asman.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/event.h>

#define max(a, b) ((a > b) ? (a) : (b))

/*

Alright, things are starting to look much clearer from a development
perspective on how I am to make this file alteration monitor work on
FreeBSD.  Here are the basics of what I need to know:

* On FreeBSD, directories can be opened just like files.  When you
  read from a directory, you get its entries as a series of
  `struct dirent' entries, each of which indicate their actual size.
  Each entry also indicates the inode of each file that it lists.
  Sometimes, the type information can be used to determine if a file
  is a directory or not.

* When you add a new file to a directory, the `dirent' record will
  always be appended onto the end of the directory file.  Directory
  entries within a directory file are not stored in sorted order.

* You can use kqueue on a directory to watch for changes to the
  directory.

* You can only watch for changes in a directory one level at a time.
  Thus, to watch an entire tree, you must recursively descend it to
  individually watch each directory.

So here's how you implement a file alteration monitor for FreeBSD:

* Start by recursively descending a directory.  Read all directories
  directly to get their directory entries.

* Create a kqueue and add a kevent for every single directory file
  within the given directory.

* Listen for change notifications.  Whenever a directory changes,
  reread the directory file to get its new contents.

* Compare inode numbers of file creations and deletions.  Use this
  information to construct move and hardlink commands as necessary.

* Optionally, delete notifications can be listened for too so that
  once a directory is deleted, its watch is removed.

* Check when new files are created.  If they are directories, then
  a watch should be added.

*/

int scan_dir(int fd, const char *dir_name)
{
  struct stat sbuf;
  char *buffer;
  int bufsize;
  char *cur_pos;
  char *parse_pos;
  int num_read = 0;
  int reads = 0;

  if (fstat(fd, &sbuf) == -1)
  {
    perror("fstat");
    return 1;
  }

  if (!S_ISDIR(sbuf.st_mode))
  {
    fprintf(stderr, "Not a directory: %s\n", dir_name);
    return 1;
  }

  lseek(fd, 0, SEEK_SET);

  bufsize = max(sizeof(struct dirent) * 2, sbuf.st_blksize * 2);
  buffer = (char*)malloc(bufsize);
  if (buffer == NULL)
  {
    perror("malloc");
    return 1;
  }

  /* BSD implementation note: it's important that the directory read
     operation is atomic: specifically, a directory shouldn't have a
     new entry added or deleted to it while the code is still reading
     entries from the directory.  This is guaranteed by
     getdirentries() and getdents() only if all entries can be read
     from the directory with only a single call.
     opendir()/readdir()/closedir(), on the other hand, use DIRBLKSIZ
     (1024) or getpagesize() for the buffer size passed to
     getdirentries(), which may not guarantee atomicity.  At least I
     hope my interpretation from reading the source code is correct.
     Then again, there's only one way to find out how things
     work... test it!  NOTE: This is "important" mostly so that the
     directory read operation doesn't start reading from the middle of
     directory entries as if they were the beginning of an entry.  */

  /* Of course, the disadvantage to this kind of code is that it won't
     work as well in terms of portability.  Most notably, this code
     won't work on unionfs filesystems.  */

  /* NOTE: Some filesystems (e.g. NFS) will not support a read()
     operation on directories as if they were files.  */

  /* void rewinddir(DIR *dirp); */
  /* void dirfd(DIR *dirp); */

  puts("File name\tFile type\tFile number");
  cur_pos = buffer;
  parse_pos = cur_pos;
  while ((num_read = read(fd, cur_pos, bufsize / 2)) > 0)
  {
    int rewind;
    struct dirent *cur_entry = (struct dirent*)parse_pos;
    cur_pos += num_read;
    if (cur_pos - buffer >= bufsize / 2)
      rewind = 1;
    while (cur_pos - parse_pos >= 8 &&
           parse_pos + cur_entry->d_reclen <= cur_pos)
    {
      printf("%s\t%d\t%d\n", cur_entry->d_name, cur_entry->d_type,
             cur_entry->d_fileno);
      parse_pos += cur_entry->d_reclen;
      cur_entry = (struct dirent*)parse_pos;
    }
    if (rewind)
    {
      memmove(buffer, parse_pos, cur_pos - parse_pos);
      cur_pos -= parse_pos - buffer;
      parse_pos = buffer;
    }
  }

  free(buffer);
  return 0;
}

/* Summary of command-line options:

   --recurse-symlinks -- Traverse symlinks too when looking for directories
   to monitor.  NOT RECOMMENDED.

   --aggressive -- Aggressive mode -- Initialize kevents for every single
   file within the watched directories too.  Although this mode will
   result in a more accurate change log, it is not strictly required for
   correct operation.  Periodic stat()ing (default) is recommeded for
   most cases.

   --stat-interval -- The time interval to stat() every file under the
   watched directory tree.  A value of 0 means that recursive stat()ing
   will only be performed once on the program termination.  Default
   interval is 1 hour.

   --spread-stat -- Rather than recursively stat()ing all files at once
   when the stat interval expires, spread out the recursive stat()ing
   across the entire interval.  Not recommended unless you know what
   you are doing, because this can result in change logs that are less
   correct overall in the event of a crash.

   --adaptive-stat -- Optimistically assume that files that were found
   to be previously updated on the last stat() interval are likely to
   be updated more frequently than other files.

   --min-adaptive-interval -- Minimum stat() interval for files whose
   stat() intervals were downscaled based off of recent updates.

   --identify-copies -- Attempt to identify when a file was copied
   during the stat() interval.  Not recommended for large systems: this
   can be very slow.  =sha256: Check sha256 checksums and file sizes.
   =exact: Check files for byte-for-byte equality.

   --safe-dir-cmp -- Do not assume that the order of directory entries
   in directory files will be preserved under changes.  In other words,
   this option assumes that adding or deleting a single directory entry
   to a directory file could completely scramble the entire ordering
   of the directory entry records in a directory file.  This option is
   not recommended for standard use.

   --simulate -- Simulate mode.  Allow operation of files as if they
   were directories.

   By default, only directory files are monitored for changes, such as
   file creations, hardlinks, renames, and unlinks.  These are the only
   changes that need immediate monitoring for the output command list
   to still be functionally correct.  Changes made to non-directory
   files such as modifications and attribute changes are picked up
   by stat()ing every file periodically (default).

*/

int main(int argc, char *argv[])
{
  int fd;
  int kq;
  struct kevent init;
  struct kevent trigger_ev;
  int ret;

  if (argc != 2)
  {
    puts("Invalid command line.");
    return 1;
  }

  kq = kqueue();
  if (kq == -1)
  {
    perror("kqueue");
    return 1;
  }

  fd = open(argv[1], O_RDONLY);
  if (fd == -1)
  {
    perror("open");
    return 1;
  }

  /* NOTE_RENAME and NOTE_ATTRIB are also of interest.  NOTE_LINK will
     not be of use: hardlink handling is done by searching for matching
     inodes on newly created files.  */
  EV_SET(&init, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
         NOTE_WRITE | NOTE_DELETE, 0, NULL);
  if (kevent(kq, &init, 1, NULL, 0, NULL) == -1)
  {
    perror("kevent");
    return 1;
  }

  while (1)
  {
    if (kevent(kq, NULL, 0, &trigger_ev, 1, NULL) == -1)
    {
      perror("kevent");
      return 1;
    }
    if (trigger_ev.fflags & NOTE_DELETE)
    {
      puts("Oops!  I've been deleted.  Time to leave...");
      break;
    }
    if (trigger_ev.fflags & NOTE_WRITE && scan_dir(fd, argv[1]) != 0)
      return ret;
  }

  /* Note: It is not necessary to manually remove this kevent: When
     the file descriptor gets closed, it will be removed automatically.  */
  EV_SET(&init, fd, EVFILT_VNODE, EV_DELETE,
         NOTE_WRITE | NOTE_DELETE, 0, NULL);
  if (kevent(kq, &init, 1, NULL, 0, NULL) == -1)
  {
    perror("kevent");
    return 1;
  }
  close(fd);
  return 0;
}

#endif /* FREEBSD */
