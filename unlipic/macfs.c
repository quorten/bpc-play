/* Implement a simple data fork + resource fork hierarchical file
   system, Macintosh-style in a Unix database-style enchantment.

   What does this mean?  This means servers and sockets of course!  No
   way.  That's too complicated.  Well how else can you do it?  The
   only other way would be through OS locking or an exclusive
   cothreading implementation.

   This is the file server.

   Functions:

   * Open file

   * Create file

   * Read file

   * Write file

   * Truncate file

   * Rename file

   * Copy file

   * Delete file
*/

/* How does this file system work?  Basic allocation management.
   Files are packed one after another, contiguously.  Yes, I know,
   inefficient, but I'm going to implement the most efficient possible
   memory manager separately.  The file system maintains some field
   record headers throughout the file system to make it work.  */

/* At the start, the file system header.  */

struct FsHeader_tag
{
  unsigned fs_size; /* Storage size managed by file system.  */
  /* Offset to the offset table header, the table that maps file IDs
     (inodes) to the file's actual offset.  */
  unsigned offset_tbl_offset;
};
typedef struct FsHeader_tag FsHeader;

/* Immediately following, the first directory entry.  That is, the
   root directory.  */

struct DirHeader_tag
{
  unsigned magic; /* Must be equal to 0xa6fb46ba.  */
  unsigned parent_offset; /* Offset to header of parent directory.  */
  unsigned num_entries; /* Number of directory entries.  */
};
typedef struct DirHeader_tag DirHeader;

/* Immediately following, a list of `num_entries' directory
   contents.  */
struct DirEntry_tag
{
  /* Offset to header of entry.  This could be either a DirHeader or a
     FileHeader.  */
  unsigned header_offset;
};
typedef struct DirEntry_tag DirEntry;

struct FileHeader_tag
{
  unsigned magic; /* Must be equal to 0x26daa3ba.  */
  unsigned data_offset; /* Offset to data fork.  */
  unsigned rsrc_offset; /* Offset to resource fork.  */
};
typedef struct FileHeader_tag FileHeader;

/* The offset table header is always located at the end of the
   allocation.  The offset table grows downward rather than upward.
   Any unused entries have their offset set to zero.  */
struct OffsetTblHeader_tag
{
  unsigned num_entries;
};
typedef struct OffsetTblHeader_tag OffsetTblHeader;

/* Why does FAT work so well without an inode table?  Well, that's
   because it only points to the first block in the file, and
   additional blocks may be appended trivially when the file grows.
   Oh, that's why.  So I really do need that optimal memory manager
   for an optimal file system.  But doesn't my memory manager desgin
   so far use an indirection table?  Yes it does.

   Look, without an indirection table, your only other option is to
   generate a list of all pointers that need to be fixed up.  Walking
   forwards, of course, since you might not even have back-pointers.
   Then, you need to fix up all those pointers to point to the new
   location.  What if not all pointers are reachable from a forward
   walk?  Here, you mean that you've got some programs that have the
   only pointer to the data in question, and there are no system-level
   pointers.  Well, in that case, your only other option is to do
   nothing.

   Come on, that doesn't make sense in general.  In general, programs
   have activation records, and in theory, if a system understands a
   program, it can read out those activation records.  Yes, if a
   system understands a program.  If not, then it is not possible.
   Okay, that makes much more sense.  So really, the only breaking
   point is if a system does not understand the programs it is
   running.  Or, put another way, insufficiently understands the
   programs in relation to achieving a certain task.

   Do you really need to use handles?  No.  The main reason for using
   handles is that it speeds up short-term demands for allocation, at
   the expense of increasing system fragmentation in the long run.
   Though, with handles, one can choose between either of these two
   possible algorithms, or choose something in between.  Without
   handles, there is only one choice: the most efficient choice.  The
   conservative choice.

   It speeds up allocation, but it slows down access.  Here we go
   again.  Conservative versus eager.  Why not liberal?  Well, the
   word has come too frequently entangled with a meaning of government
   political associations.  Politics most frequently referring to
   government.

   Memory locking is a good compromise in this respect.  As opposed to
   always using handles.

   Moving memory around.  What if you have too many handles?  Then the
   system gets really slow to manage at the system level.  If you can
   group things together into modules, then that greatly speeds up
   high level system management.  But, again, it slows down local
   access.  But not by much.  Not if you use a binary tree of relative
   pointers.  Anyways, that being said, it is always a good idea to
   design for the smallest practical containment.  Defining limits is
   the greatest way to improve productivity.  But, at the same time,
   making it easy to remove limits also greatly enhances productivity.

   You can provide good average efficiency.  And, if the user really
   wants, you can design your algorithms to try to squeeze out a
   little bit extra efficiency too, if there is space remaining.
   Though the computational cost may make it impractical compared to
   other methods of lower cost.

   Another question.  Is rewriting memory cheap?  Yes.  No.  Choose
   one.  Depending on which one you choose, this will change your
   memory allocator's decision making process.  When rewriting memory
   is cheap, the tendency is to give programs scratch spaces that they
   can rewrite as many times as they want, but they cannot expand.
   But when rewriting is expensive, then the number of times the
   memory can be written to must also be allocated.  In other words,
   you assign programs with a maximum rate at which they can write
   memory at.  This makes sense, for example, when you have WORM
   media.  For most practical purposes, paper is an example of a WORM
   storage media.  Sometimes, the media can be rewritten a finite
   number of times, though not infinite number of times.  Flash memory
   is an example.  It's like a roll of paper.  Eventually, though, the
   roll will run out, and you'll have to buy a new one to keep
   working.

   Filesystem APIs.  Any thing beyond the bare minimum is at risk for
   loss.  The only thing that can really be guaranteed is a contiguous
   scratch space.

   The traditional method.  Each application has its own scratch
   space, whatever it may be.  For example, single floppy disk
   software for early microcomputers.  They can write saved user data
   to their disks, but that doesn't go to any particular central
   store.  Which is not the case for big centralized systems.
   Instead, applications must write their data to some user directory.
   But this means that when the application is uninstalled, the
   app-local data is not deleted, even though it should be.  That is,
   all data that belongs internal to an application, data that the
   application has not explicitly exported to a presumably
   shared-environment.

   * Ah ha!  This is the problem with modern Unix systems.  The
     concept of app-local data has not been very well standardized.
     Well, okay, it is, actually.  Dot files, for example.  The
     general consensus is that all dot-files are app-local files,
     generated by software unless otherwise noted.  Or, should a user
     want any of their dot-files to be preserved, they should write a
     script to preserve those particular dot-files.  Yes, a program
     for import, export, and backup, in an efficient manner.

*/

/* This is the space that stores the file system buffer.  */
unsigned char *storage = NULL;
unsigned storage_size;

/* Return zero on success, one on failure.  */
int storage_alloc()
{
  storage = (unsigned char*)malloc(storage_size);
  if (storage == NULL) return 1;
  return 0;
}

/* Return zero on success, one on failure.  */
int storage_save()
{
  FILE *fp = NULL;
  if ((  fp = fopen("storage.bin"))             == NULL) goto error;
  if (   fwrite(storage, storage_size, 1, fp)   != 1) goto error;
  if (   fclose(fp)                             == EOF) goto error;
         return 0;
 error:
  if (fp != NULL) fclose(fp);
  fputs("ERROR: Could not save storage!\n", stderr);
  return 1;
}

/* Return zero on success, one on failure.  Storage must have been
   already been allocated.  */
int storage_load()
{
  FILE *fp = NULL;
  if ((  fp = fopen("storage.bin"))            == NULL) goto error;
  if (   fread(storage, storage_size, 1, fp)   != 1) goto error;
  if (   fclose(fp)                            == EOF) goto error;
         return 0;
 error:
  if (fp != NULL) fclose(fp);
  return 1;
}

int main()
{
  return 0;
}
