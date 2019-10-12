#ifndef ALLSYS_H
#define ALLSYS_H

/* TODO FIXME: end_result used to avoid conflicting with result name.  */
/* TODO: Check for correct sizing and padding on data structure
   members such as pid_t, gid_t, ino_t, dev_t, clock_t, key_t,
   etc.  */
/* TODO: I should probably write a program to test the recorded data
   structure sizes and alignments, so that you can compare them with a
   system compile.  Fast, easy, and doesn't require any system calls
   that need data structures.  */

#define __WORDSIZE 32
#define BITS_PER_LONG 32
// #define BITS_PER_LONG 64
#define _I386
// __x86_64__
// #define _ARM
// #define _MIPS
#define __LITTLE_ENDIAN
// #define __BIG_ENDIAN
#define _MIPS_SIM_ABI32 10
#define _MIPS_SIM_ABI64 20
#define _MIPS_SIM_NABI32 30
#define _MIPS_SIM 0
#include "x86/unistd_32.h"
// #include "arm/unistd.h"
// #include "mips/unistd.h"
// #include "x86/unistd_64.h"
#include "../i386/syscalls.h"
// #include "../arm_oabi/syscalls.h"
// #include "../mips/syscalls.h"
// #include "../x86_64/syscalls.h"
#include "../syscalls_gen.h"

/* TYPES: */

typedef long size_t;
typedef unsigned long ssize_t;
typedef long off_t; // ???



#ifdef _I386
struct stat {
	unsigned long  st_dev;
	unsigned long  st_ino;
	unsigned short st_mode;
	unsigned short st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
	unsigned long  st_rdev;
	unsigned long  st_size;
	unsigned long  st_blksize;
	unsigned long  st_blocks;
	unsigned long  st_atime;
	unsigned long  st_atime_nsec;
	unsigned long  st_mtime;
	unsigned long  st_mtime_nsec;
	unsigned long  st_ctime;
	unsigned long  st_ctime_nsec;
	unsigned long  __unused4;
	unsigned long  __unused5;
};

#define STAT64_HAS_BROKEN_ST_INO	1

/* TODO FIXME: Properly emulate quirks in stat64 based off of
   architecture.  */

/* This matches struct stat64 in glibc2.1, hence the absolutely
 * insane amounts of padding around dev_t's.
 */
struct stat64 {
	unsigned long long	st_dev;
	unsigned char	__pad0[4];

	unsigned long	__st_ino;

	unsigned int	st_mode;
	unsigned int	st_nlink;

	unsigned long	st_uid;
	unsigned long	st_gid;

	unsigned long long	st_rdev;
	unsigned char	__pad3[4];

	long long	st_size;
	unsigned long	st_blksize;

	/* Number 512-byte blocks allocated. */
	unsigned long long	st_blocks;

	unsigned long	st_atime;
	unsigned long	st_atime_nsec;

	unsigned long	st_mtime;
	unsigned int	st_mtime_nsec;

	unsigned long	st_ctime;
	unsigned long	st_ctime_nsec;

	unsigned long long	st_ino;
};

#else /* _I386 */

struct stat {
	unsigned long	st_dev;
	unsigned long	st_ino;
	unsigned long	st_nlink;

	unsigned int	st_mode;
	unsigned int	st_uid;
	unsigned int	st_gid;
	unsigned int	__pad0;
	unsigned long	st_rdev;
	long		st_size;
	long		st_blksize;
	long		st_blocks;	/* Number 512-byte blocks allocated. */

	unsigned long	st_atime;
	unsigned long	st_atime_nsec;
	unsigned long	st_mtime;
	unsigned long	st_mtime_nsec;
	unsigned long	st_ctime;
	unsigned long   st_ctime_nsec;
	long		__unused[3];
};
#endif



struct pollfd {
	int fd;
	short events;
	short revents;
};
typedef unsigned long nfds_t;
typedef long time_t;
struct timespec {
	time_t		tv_sec;			/* seconds */
	long		tv_nsec;		/* nanoseconds */
};
typedef long __suseconds_t;
struct timeval
  {
    time_t tv_sec;		/* Seconds.  */
    __suseconds_t tv_usec;	/* Microseconds.  */
  };
struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};
typedef unsigned long sigset_t;
struct sigaction;
/* Structure for scatter/gather I/O.  */
struct iovec
  {
    void *iov_base;	/* Pointer to data.  */
    size_t iov_len;	/* Length of data.  */
  };
typedef long fd_set; // ???
typedef long key_t; // ???
struct itimerval {
	struct timeval it_interval;	/* timer interval */
	struct timeval it_value;	/* current value */
};
/* POSIX.1g specifies this type name for the `sa_family' member.  */
typedef unsigned short int sa_family_t;
/* This macro is used to declare the initial common members
   of the data types used for socket addresses, `struct sockaddr',
   `struct sockaddr_in', `struct sockaddr_un', etc.  */

#define	__SOCKADDR_COMMON(sa_prefix) \
  sa_family_t sa_prefix##family
/* Structure describing a generic socket address.  */
struct sockaddr
  {
    __SOCKADDR_COMMON (sa_);	/* Common data: address family and length.  */
    char sa_data[14];		/* Address data.  */
  };
typedef unsigned int socklen_t;
typedef int pid_t;
struct linux_dirent {
	unsigned long  d_ino;     /* Inode number */
	unsigned long  d_off;     /* Offset to next linux_dirent */
	unsigned short d_reclen;  /* Length of this linux_dirent */
	char           d_name[];  /* Filename (null-terminated) */
	/* length is actually (d_reclen - 2 -
	   offsetof(struct linux_dirent, d_name) */
	/*
	char           pad;       // Zero padding byte
	char           d_type;    // File type (only since Linux 2.6.4;
	                          // offset is (d_reclen - 1))
	*/

};
typedef unsigned int mode_t; // TODO: short or long?
typedef unsigned int uid_t;
typedef unsigned int gid_t;
struct rlimit {
	unsigned long	rlim_cur;
	unsigned long	rlim_max;
};
struct sysinfo {
	long uptime;			/* Seconds since boot */
	unsigned long loads[3];		/* 1, 5, and 15 minute load averages */
	unsigned long totalram;		/* Total usable main memory size */
	unsigned long freeram;		/* Available memory size */
	unsigned long sharedram;	/* Amount of shared memory */
	unsigned long bufferram;	/* Memory used by buffers */
	unsigned long totalswap;	/* Total swap space size */
	unsigned long freeswap;		/* swap space still available */
	unsigned short procs;		/* Number of current processes */
	unsigned short pad;		/* explicit padding for m68k */
	unsigned long totalhigh;	/* Total high memory size */
	unsigned long freehigh;		/* Available high memory size */
	unsigned int mem_unit;		/* Memory unit size in bytes */
	char _f[20-2*sizeof(long)-sizeof(int)];	/* Padding: libc5 uses this.. */
};
typedef long clock_t;
/* Structure describing CPU time used by a process and its children.  */
struct tms
  {
    clock_t tms_utime;		/* User CPU time.  */
    clock_t tms_stime;		/* System CPU time.  */

    clock_t tms_cutime;		/* User CPU time of dead children.  */
    clock_t tms_cstime;		/* System CPU time of dead children.  */
  };
enum __ptrace_request;
typedef short __s16;
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef unsigned long long __u64;
typedef long long __s64;
typedef struct __user_cap_header_struct {
	__u32 version;
	int pid;
} *cap_user_header_t;
typedef struct __user_cap_data_struct {
        __u32 effective;
        __u32 permitted;
        __u32 inheritable;
} *cap_user_data_t;

typedef struct siginfo {
  // TODO
} siginfo_t;

struct sched_param
  {
    int __sched_priority;
  };
struct __sysctl_args {
	int *name;
	int nlen;
	void *oldval;
	size_t *oldlenp;
	void *newval;
	size_t newlen;
	unsigned long __unused[4];
};
/*
 * syscall interface - used (mainly by NTP daemon)
 * to discipline kernel clock oscillator
 */
struct timex {
	unsigned int modes;	/* mode selector */
	long offset;		/* time offset (usec) */
	long freq;		/* frequency offset (scaled ppm) */
	long maxerror;		/* maximum error (usec) */
	long esterror;		/* estimated error (usec) */
	int status;		/* clock command/status */
	long constant;		/* pll time constant */
	long precision;		/* clock precision (usec) (read only) */
	long tolerance;		/* clock frequency tolerance (ppm)
				 * (read only)
				 */
	struct timeval time;	/* (read only) */
	long tick;		/* (modified) usecs between clock ticks */

	long ppsfreq;           /* pps frequency (scaled ppm) (ro) */
	long jitter;            /* pps jitter (us) (ro) */
	int shift;              /* interval duration (s) (shift) (ro) */
	long stabil;            /* pps stability (scaled ppm) (ro) */
	long jitcnt;            /* jitter limit exceeded (ro) */
	long calcnt;            /* calibration intervals (ro) */
	long errcnt;            /* calibration errors (ro) */
	long stbcnt;            /* stability limit exceeded (ro) */

	int tai;		/* TAI offset (ro) */

	int  :32; int  :32; int  :32; int  :32;
	int  :32; int  :32; int  :32; int  :32;
	int  :32; int  :32; int  :32;
};
typedef long long off64_t; // TODO
typedef unsigned long int __cpu_mask;
# define __CPU_SETSIZE	1024
# define __NCPUBITS	(8 * sizeof (__cpu_mask))
typedef struct
{
  __cpu_mask __bits[__CPU_SETSIZE / __NCPUBITS];
} cpu_set_t;
typedef unsigned long aio_context_t;
/* read() from /dev/aio returns these structures. */
struct io_event {
	__u64		data;		/* the data field from the iocb */
	__u64		obj;		/* what iocb this event came from */
	__s64		res;		/* result code for this event */
	__s64		res2;		/* secondary result */
};

#if defined(__LITTLE_ENDIAN)
#define PADDED(x,y)	x, y
#elif defined(__BIG_ENDIAN)
#define PADDED(x,y)	y, x
#else
#error edit for your odd byteorder.
#endif

/*
 * we always use a 64bit off_t when communicating
 * with userland.  its up to libraries to do the
 * proper padding and aio_error abstraction
 */

struct iocb {
	/* these are internal to the kernel/libc. */
	__u64	aio_data;	/* data to be returned in event's data */
	__u32	PADDED(aio_key, aio_reserved1);
				/* the kernel sets aio_key to the req # */

	/* common fields */
	__u16	aio_lio_opcode;	/* see IOCB_CMD_ above */
	__s16	aio_reqprio;
	__u32	aio_fildes;

	__u64	aio_buf;
	__u64	aio_nbytes;
	__s64	aio_offset;

	/* extra parameters */
	__u64	aio_reserved2;	/* TODO: use this for a (struct sigevent *) */

	/* flags for the "struct iocb" */
	__u32	aio_flags;

	/*
	 * if the IOCB_FLAG_RESFD flag of "aio_flags" is set, this is an
	 * eventfd to signal AIO readiness to
	 */
	__u32	aio_resfd;
}; /* 64 bytes */
/*
 * Note on 64bit base and limit is ignored and you cannot set DS/ES/CS
 * not to the default values if you still want to do syscalls. This
 * call is more for 32bit mode therefore.
 */
struct user_desc {
	unsigned int  entry_number;
	unsigned int  base_addr;
	unsigned int  limit;
	unsigned int  seg_32bit:1;
	unsigned int  contents:2;
	unsigned int  read_exec_only:1;
	unsigned int  limit_in_pages:1;
	unsigned int  seg_not_present:1;
	unsigned int  useable:1;
#ifdef __x86_64__
	unsigned int  lm:1;
#endif
};
typedef unsigned long long u64;
struct linux_dirent64; // TODO
typedef char * caddr_t;
struct kernel_sym; // TODO

/* semop system calls takes an array of these. */
struct sembuf {
	unsigned short  sem_num;	/* semaphore index in array */
	short		sem_op;		/* semaphore operation */
	short		sem_flg;	/* operation flags */
};

struct epoll_event {
	__u32 events;
	__u64 data;
} EPOLL_PACKED;

typedef int clockid_t;

typedef union sigval {
	int sival_int;
	void *sival_ptr;
} sigval_t;

/*
 * This works because the alignment is ok on all current architectures
 * but we leave open this being overridden in the future
 */
#ifndef __ARCH_SIGEV_PREAMBLE_SIZE
#define __ARCH_SIGEV_PREAMBLE_SIZE	(sizeof(int) * 2 + sizeof(sigval_t))
#endif

#define SIGEV_MAX_SIZE	64
#define SIGEV_PAD_SIZE	((SIGEV_MAX_SIZE - __ARCH_SIGEV_PREAMBLE_SIZE) \
		/ sizeof(int))

typedef struct sigevent {
	sigval_t sigev_value;
	int sigev_signo;
	int sigev_notify;
	union {
		int _pad[SIGEV_PAD_SIZE];
		 int _tid;

		struct {
			void (*_function)(sigval_t);
			void *_attribute;	/* really pthread_attr_t */
		} _sigev_thread;
	} _sigev_un;
} sigevent_t;

typedef int timer_t;
struct itimerspec {
	struct timespec it_interval;	/* timer period */
	struct timespec it_value;	/* timer expiration */
};
typedef int mqd_t;
struct mq_attr
{
  long int mq_flags;	/* Message queue flags.  */
  long int mq_maxmsg;	/* Maximum number of messages.  */
  long int mq_msgsize;	/* Maximum message size.  */
  long int mq_curmsgs;	/* Number of messages currently queued.  */
  long int __pad[4];
};
struct kexec_segment; // TODO
// RESUME
typedef enum
{
  P_ALL,		/* Wait for any child.  */
  P_PID,		/* Wait for specified process.  */
  P_PGID		/* Wait for members of process group.  */
} idtype_t;
typedef long id_t; // ???
typedef long key_serial_t; // ???

/*
 * Per-lock list entry - embedded in user-space locks, somewhere close
 * to the futex field. (Note: user-space uses a double-linked list to
 * achieve O(1) list add and remove, but the kernel only needs to know
 * about the forward link)
 *
 * NOTE: this structure is part of the syscall ABI, and must not be
 * changed.
 */
struct robust_list {
	struct robust_list *next;
};

/*
 * Per-thread list head:
 *
 * NOTE: this structure is part of the syscall ABI, and must only be
 * changed if the change is first communicated with the glibc folks.
 * (When an incompatible change is done, we'll increase the structure
 *  size, which glibc will detect)
 */
struct robust_list_head {
	/*
	 * The head of the list. Points back to itself if empty:
	 */
	struct robust_list list;

	/*
	 * This relative offset is set by user-space, it gives the kernel
	 * the relative position of the futex field to examine. This way
	 * we keep userspace flexible, to freely shape its data-structure,
	 * without hardcoding any particular offset into the kernel:
	 */
	long futex_offset;

	/*
	 * The death of the thread may race with userspace setting
	 * up a lock's links. So to handle this race, userspace first
	 * sets this field to the address of the to-be-taken lock,
	 * then does the lock acquire, and then adds itself to the
	 * list, and then clears this field. Hence the kernel will
	 * always have full knowledge of all locks that the thread
	 * _might_ have taken. We check the owner TID in any case,
	 * so only truly owned locks will be handled.
	 */
	struct robust_list *list_op_pending;
};
typedef long loff_t; // TODO long long
typedef unsigned int uint32_t;

/* Structure describing messages sent by
   `sendmsg' and received by `recvmsg'.  */
struct msghdr
  {
    void *msg_name;		/* Address to send to/receive from.  */
    socklen_t msg_namelen;	/* Length of address data.  */
  };

/* Structure which says how much of each resource has been used.  */
struct rusage
  {
    /* Total amount of user time used.  */
    struct timeval ru_utime;
    /* Total amount of system time used.  */
    struct timeval ru_stime;
    /* Maximum resident set size (in kilobytes).  */
    long int ru_maxrss;
    /* Amount of sharing of text segment memory
       with other processes (kilobyte-seconds).  */
    long int ru_ixrss;
    /* Amount of data segment memory used (kilobyte-seconds).  */
    long int ru_idrss;
    /* Amount of stack memory used (kilobyte-seconds).  */
    long int ru_isrss;
    /* Number of soft page faults (i.e. those serviced by reclaiming
       a page from the list of pages awaiting reallocation.  */
    long int ru_minflt;
    /* Number of hard page faults (i.e. those that required I/O).  */
    long int ru_majflt;
    /* Number of times a process was swapped out of physical memory.  */
    long int ru_nswap;
    /* Number of input operations via the file system.  Note: This
       and `ru_oublock' do not include operations with the cache.  */
    long int ru_inblock;
    /* Number of output operations via the file system.  */
    long int ru_oublock;
    /* Number of IPC messages sent.  */
    long int ru_msgsnd;
    /* Number of IPC messages received.  */
    long int ru_msgrcv;
    /* Number of signals delivered.  */
    long int ru_nsignals;
    /* Number of voluntary context switches, i.e. because the process
       gave up the process before it had to (usually to wait for some
       resource to be available).  */
    long int ru_nvcsw;
    /* Number of involuntary context switches, i.e. a higher priority process
       became runnable or the current process used up its time slice.  */
    long int ru_nivcsw;
  };


/* Length of the entries in `struct utsname' is 65.  */
#define _UTSNAME_LENGTH 65

/* Linux provides as additional information in the `struct utsname'
   the name of the current domain.  Define _UTSNAME_DOMAIN_LENGTH
   to a value != 0 to activate this entry.  */
#define _UTSNAME_DOMAIN_LENGTH _UTSNAME_LENGTH


#ifndef _UTSNAME_SYSNAME_LENGTH
# define _UTSNAME_SYSNAME_LENGTH _UTSNAME_LENGTH
#endif
#ifndef _UTSNAME_NODENAME_LENGTH
# define _UTSNAME_NODENAME_LENGTH _UTSNAME_LENGTH
#endif
#ifndef _UTSNAME_RELEASE_LENGTH
# define _UTSNAME_RELEASE_LENGTH _UTSNAME_LENGTH
#endif
#ifndef _UTSNAME_VERSION_LENGTH
# define _UTSNAME_VERSION_LENGTH _UTSNAME_LENGTH
#endif
#ifndef _UTSNAME_MACHINE_LENGTH
# define _UTSNAME_MACHINE_LENGTH _UTSNAME_LENGTH
#endif

/* Structure describing the system and machine.  */
struct utsname
  {
    /* Name of the implementation of the operating system.  */
    char sysname[_UTSNAME_SYSNAME_LENGTH];

    /* Name of this node on the network.  */
    char nodename[_UTSNAME_NODENAME_LENGTH];

    /* Current release level of this implementation.  */
    char release[_UTSNAME_RELEASE_LENGTH];
    /* Current version level of this release.  */
    char version[_UTSNAME_VERSION_LENGTH];

    /* Name of the hardware type the system is running on.  */
    char machine[_UTSNAME_MACHINE_LENGTH];

#if _UTSNAME_DOMAIN_LENGTH - 0
    /* Name of the domain of this node on the network.  */
# ifdef __USE_GNU
    char domainname[_UTSNAME_DOMAIN_LENGTH];
# else
    char __domainname[_UTSNAME_DOMAIN_LENGTH];
# endif
#endif
  };


typedef unsigned long __kernel_ulong_t;
/* Data structure used to pass permission information to IPC operations.  */
struct ipc_perm
  {
    key_t __key;			/* Key.  */
    uid_t uid;			/* Owner's user ID.  */
    gid_t gid;			/* Owner's group ID.  */
    uid_t cuid;			/* Creator's user ID.  */
    gid_t cgid;			/* Creator's group ID.  */
    unsigned short int mode;		/* Read/write permission.  */
    unsigned short int __pad1;
    unsigned short int __seq;		/* Sequence number.  */
    unsigned short int __pad2;
    unsigned long int __unused1;
    unsigned long int __unused2;
  };
typedef unsigned long int shmatt_t;
/* Data structure describing a shared memory segment.  */
struct shmid_ds
  {
    struct ipc_perm shm_perm;		/* operation permission struct */
    size_t shm_segsz;			/* size of segment in bytes */
    time_t shm_atime;			/* time of last shmat() */
#if __WORDSIZE == 32
    unsigned long int __unused1;
#endif
    time_t shm_dtime;			/* time of last shmdt() */
#if __WORDSIZE == 32
    unsigned long int __unused2;
#endif
    time_t shm_ctime;			/* time of last change by shmctl() */
#if __WORDSIZE == 32
    unsigned long int __unused3;
#endif
    pid_t shm_cpid;			/* pid of creator */
    pid_t shm_lpid;			/* pid of last shmop */
    shmatt_t shm_nattch;		/* number of current attaches */
    unsigned long int __unused4;
    unsigned long int __unused5;
  };
enum __ptrace_request { BLAH };
typedef struct sigaltstack {
	void *ss_sp;
	int ss_flags;
	size_t ss_size;
} stack_t;
typedef unsigned long __ino_t;
typedef int __daddr_t;
struct ustat
  {
    __daddr_t f_tfree;		/* Number of free blocks.  */
    __ino_t f_tinode;		/* Number of free inodes.  */
    char f_fname[6];
    char f_fpack[6];
  };

/* Types used in the structure definition.  */
typedef unsigned long int msgqnum_t;
typedef unsigned long int msglen_t;

/* Structure of record for one message inside the kernel.
   The type `struct msg' is opaque.  */
struct msqid_ds
{
  struct ipc_perm msg_perm;	/* structure describing operation permission */
  time_t msg_stime;		/* time of last msgsnd command */
#if __WORDSIZE == 32
  unsigned long int __unused1;
#endif
  time_t msg_rtime;		/* time of last msgrcv command */
#if __WORDSIZE == 32
  unsigned long int __unused2;
#endif
  time_t msg_ctime;		/* time of last change */
#if __WORDSIZE == 32
  unsigned long int __unused3;
#endif
  unsigned long int __msg_cbytes; /* current number of bytes on queue */
  msgqnum_t msg_qnum;		/* number of messages currently on queue */
  msglen_t msg_qbytes;		/* max number of bytes allowed on queue */
  pid_t msg_lspid;		/* pid of last msgsnd() */
  pid_t msg_lrpid;		/* pid of last msgrcv() */
  unsigned long int __unused4;
  unsigned long int __unused5;
};

struct utimbuf {
	time_t actime;
	time_t modtime;
};

typedef unsigned long dev_t; // TODO: short or long?
typedef unsigned short __kernel_old_dev_t;

/*
 * Most 64-bit platforms use 'long', while most 32-bit platforms use '__u32'.
 * Yes, they differ in signedness as well as size.
 * Special cases can override it for themselves -- except for S390x, which
 * is just a little too special for us. And MIPS, which I'm not touching
 * with a 10' pole.
 */
#ifndef __statfs_word
#if BITS_PER_LONG == 64
#define __statfs_word long
#else
#define __statfs_word __u32
#endif
#endif

typedef struct {
	int	val[2];
} __kernel_fsid_t;

struct statfs {
	__statfs_word f_type;
	__statfs_word f_bsize;
	__statfs_word f_blocks;
	__statfs_word f_bfree;
	__statfs_word f_bavail;
	__statfs_word f_files;
	__statfs_word f_ffree;
	__kernel_fsid_t f_fsid;
	__statfs_word f_namelen;
	__statfs_word f_frsize;
	__statfs_word f_spare[5];
};

/* Internet address.  */
typedef uint32_t in_addr_t;
struct in_addr
  {
    in_addr_t s_addr;
  };

#define NFS_MAXPATHLEN	1024
#define NFS_FHSIZE	32
#define NFS4_FHSIZE		128

/*
 * This is the old "dentry style" Linux NFSv2 file handle.
 *
 * The xino and xdev fields are currently used to transport the
 * ino/dev of the exported inode.
 */
struct nfs_fhbase_old {
	__u32		fb_dcookie;	/* dentry cookie - always 0xfeebbaca */
	__u32		fb_ino;		/* our inode number */
	__u32		fb_dirino;	/* dir inode number, 0 for directories */
	__u32		fb_dev;		/* our device */
	__u32		fb_xdev;
	__u32		fb_xino;
	__u32		fb_generation;
};

/*
 * This is the new flexible, extensible style NFSv2/v3 file handle.
 * by Neil Brown <neilb@cse.unsw.edu.au> - March 2000
 *
 * The file handle is seens as a list of 4byte words.
 * The first word contains a version number (1) and four descriptor bytes
 * that tell how the remaining 3 variable length fields should be handled.
 * These three bytes are auth_type, fsid_type and fileid_type.
 *
 * All 4byte values are in host-byte-order.
 *
 * The auth_type field specifies how the filehandle can be authenticated
 * This might allow a file to be confirmed to be in a writable part of a
 * filetree without checking the path from it upto the root.
 * Current values:
 *     0  - No authentication.  fb_auth is 0 bytes long
 * Possible future values:
 *     1  - 4 bytes taken from MD5 hash of the remainer of the file handle
 *          prefixed by a secret and with the important export flags.
 *
 * The fsid_type identifies how the filesystem (or export point) is
 *    encoded.
 *  Current values:
 *     0  - 4 byte device id (ms-2-bytes major, ls-2-bytes minor), 4byte inode number
 *        NOTE: we cannot use the kdev_t device id value, because kdev_t.h
 *              says we mustn't.  We must break it up and reassemble.
 *     1  - 4 byte user specified identifier
 *     2  - 4 byte major, 4 byte minor, 4 byte inode number - DEPRECATED
 *     3  - 4 byte device id, encoded for user-space, 4 byte inode number
 *     4  - 4 byte inode number and 4 byte uuid
 *     5  - 8 byte uuid
 *     6  - 16 byte uuid
 *     7  - 8 byte inode number and 16 byte uuid
 *
 * The fileid_type identified how the file within the filesystem is encoded.
 * This is (will be) passed to, and set by, the underlying filesystem if it supports
 * filehandle operations.  The filesystem must not use the value '0' or '0xff' and may
 * only use the values 1 and 2 as defined below:
 *  Current values:
 *    0   - The root, or export point, of the filesystem.  fb_fileid is 0 bytes.
 *    1   - 32bit inode number, 32 bit generation number.
 *    2   - 32bit inode number, 32 bit generation number, 32 bit parent directory inode number.
 *
 */
struct nfs_fhbase_new {
	__u8		fb_version;	/* == 1, even => nfs_fhbase_old */
	__u8		fb_auth_type;
	__u8		fb_fsid_type;
	__u8		fb_fileid_type;
	__u32		fb_auth[1];
/*	__u32		fb_fsid[0]; floating */
/*	__u32		fb_fileid[0]; floating */
};

struct knfsd_fh {
	unsigned int	fh_size;	/* significant for NFSv3.
					 * Points to the current size while building
					 * a new file handle
					 */
	union {
		struct nfs_fhbase_old	fh_old;
		__u32			fh_pad[NFS4_FHSIZE/4];
		struct nfs_fhbase_new	fh_new;
	} fh_base;
};

/*
 * Important limits for the exports stuff.
 */
#define NFSCLNT_IDMAX		1024
#define NFSCLNT_ADDRMAX		16
#define NFSCLNT_KEYMAX		32

/*
 * Version of the syscall interface
 */
#define NFSCTL_VERSION		0x0201

/*
 * These are the commands understood by nfsctl().
 */
#define NFSCTL_SVC		0	/* This is a server process. */
#define NFSCTL_ADDCLIENT	1	/* Add an NFS client. */
#define NFSCTL_DELCLIENT	2	/* Remove an NFS client. */
#define NFSCTL_EXPORT		3	/* export a file system. */
#define NFSCTL_UNEXPORT		4	/* unexport a file system. */
/*#define NFSCTL_UGIDUPDATE	5	/ * update a client's uid/gid map. DISCARDED */
/*#define NFSCTL_GETFH		6	/ * get an fh by ino DISCARDED */
#define NFSCTL_GETFD		7	/* get an fh by path (used by mountd) */
#define	NFSCTL_GETFS		8	/* get an fh by path with max FH len */

/* SVC */
struct nfsctl_svc {
	unsigned short		svc_port;
	int			svc_nthreads;
};

/* ADDCLIENT/DELCLIENT */
struct nfsctl_client {
	char			cl_ident[NFSCLNT_IDMAX+1];
	int			cl_naddr;
	struct in_addr		cl_addrlist[NFSCLNT_ADDRMAX];
	int			cl_fhkeytype;
	int			cl_fhkeylen;
	unsigned char		cl_fhkey[NFSCLNT_KEYMAX];
};

/* EXPORT/UNEXPORT */
struct nfsctl_export {
	char			ex_client[NFSCLNT_IDMAX+1];
	char			ex_path[NFS_MAXPATHLEN+1];
	__kernel_old_dev_t	ex_dev;
	__ino_t		ex_ino;
	int			ex_flags;
	uid_t		ex_anon_uid;
	gid_t		ex_anon_gid;
};

/* GETFD */
struct nfsctl_fdparm {
	struct sockaddr		gd_addr;
	char			gd_path[NFS_MAXPATHLEN+1];
	int			gd_version;
};

/* GETFS - GET Filehandle with Size */
struct nfsctl_fsparm {
	struct sockaddr		gd_addr;
	char			gd_path[NFS_MAXPATHLEN+1];
	int			gd_maxlen;
};

/*
 * This is the argument union.
 */
struct nfsctl_arg {
	int			ca_version;	/* safeguard */
	union {
		struct nfsctl_svc	u_svc;
		struct nfsctl_client	u_client;
		struct nfsctl_export	u_export;
		struct nfsctl_fdparm	u_getfd;
		struct nfsctl_fsparm	u_getfs;
		/*
		 * The following dummy member is needed to preserve binary compatibility
		 * on platforms where alignof(void*)>alignof(int).  It's needed because
		 * this union used to contain a member (u_umap) which contained a
		 * pointer.
		 */
		void *u_ptr;
	} u;
#define ca_svc		u.u_svc
#define ca_client	u.u_client
#define ca_export	u.u_export
#define ca_getfd	u.u_getfd
#define	ca_getfs	u.u_getfs
};

union nfsctl_res {
	__u8			cr_getfh[NFS_FHSIZE];
	struct knfsd_fh		cr_getfs;
};

/*
 * Hardware event_id to monitor via a performance monitoring event:
 */
struct perf_event_attr {

	/*
	 * Major type: hardware/software/tracepoint/etc.
	 */
	__u32			type;

	/*
	 * Size of the attr structure, for fwd/bwd compat.
	 */
	__u32			size;

	/*
	 * Type specific configuration information.
	 */
	__u64			config;

	union {
		__u64		sample_period;
		__u64		sample_freq;
	};

	__u64			sample_type;
	__u64			read_format;

	__u64			disabled       :  1, /* off by default        */
				inherit	       :  1, /* children inherit it   */
				pinned	       :  1, /* must always be on PMU */
				exclusive      :  1, /* only group on PMU     */
				exclude_user   :  1, /* don't count user      */
				exclude_kernel :  1, /* ditto kernel          */
				exclude_hv     :  1, /* ditto hypervisor      */
				exclude_idle   :  1, /* don't count when idle */
				mmap           :  1, /* include mmap data     */
				comm	       :  1, /* include comm data     */
				freq           :  1, /* use freq, not period  */
				inherit_stat   :  1, /* per task counts       */
				enable_on_exec :  1, /* next exec enables     */
				task           :  1, /* trace fork/exit       */
				watermark      :  1, /* wakeup_watermark      */

				__reserved_1   : 49;

	union {
		__u32		wakeup_events;	  /* wakeup every n events */
		__u32		wakeup_watermark; /* bytes before wakeup   */
	};
	__u32			__reserved_2;

	__u64			__reserved_3;
};

// DEFINE SIGNALS

_syscall3(ssize_t, read, __NR_read, int, fd, void *, buf, size_t, count);
_syscall3(ssize_t, write, __NR_write, int, fd, void *, buf, size_t, count);
_syscall3(int, open, __NR_open, const char *, pathname,
	  int, flags, mode_t, mode);
_syscall1(int, close, __NR_close, int, fd);
_syscall2(int, stat, __NR_stat, const char *, path, struct stat *, buf);
_syscall2(int, fstat, __NR_fstat, int, fd, struct stat *, buf);
_syscall2(int, lstat, __NR_lstat, const char *, path, struct stat *, buf);
_syscall3(int, poll, __NR_poll, struct pollfd *, fds, nfds_t, nfds,
	  int, timeout);
_syscall4(int, ppoll, __NR_ppoll, struct pollfd *, fds, nfds_t, nfds,
	  const struct timespec *, timeout, const sigset_t *, sigmask);
_syscall3(off_t, lseek, __NR_lseek, int, fd, off_t, offset, int, whence);
_syscall6(void, mmap, __NR_mmap, void *, addr, size_t, length,
	  int, prot, int, flags, int, fd, off_t, offset);
_syscall3(int, mprotect, __NR_mprotect, const void *, addr, size_t, len,
	  int, prot);
_syscall4(void, mremap, __NR_mremap, void *, old_address,
	  size_t, old_size, size_t, new_size, int, flags);
_syscall3(int, msync, __NR_msync, void *, addr, size_t, length,
	  int, flags);
_syscall2(int, munmap, __NR_munmap, void *, addr, size_t, length);
_syscall1(int, brk, __NR_brk, void *, addr);
_syscall3(int, sigaction, __NR_rt_sigaction, int, signum,
	  const struct sigaction *, act, struct sigaction *, oldact);
_syscall3(int, sigprocmask, __NR_rt_sigprocmask, int, how,
	  const sigset_t *, set, sigset_t *, oldset);
_syscall1(int, sigreturn, __NR_rt_sigreturn, unsigned long, __unused);
// _syscall3(int, ioctl, __NR_ioctl, int, d, int, request, ...); ???
_syscall4(ssize_t, pread64, __NR_pread64, int, fd, void *, buf,
	  size_t, count, off_t, offset);
_syscall4(ssize_t, pwrite64, __NR_pwrite64, int, fd, void *, buf,
	  size_t, count, off_t, offset);
_syscall3(ssize_t, readv, __NR_readv, int, fd,
	  const struct iovec *, iov, int, iovcnt);
_syscall3(ssize_t, writev, __NR_writev, int, fd,
	  const struct iovec *, iov, int, iovcnt);
_syscall2(int, access, __NR_access, const char *, pathname, int, mode);
_syscall1(int, pipe, __NR_pipe, int *, pipefd); // [2]
_syscall2(int, pipe2, __NR_pipe2, int *, pipefd, int, flags);
#if !defined(_MIPS)
_syscall5(int, select, __NR_select, int, nfds, fd_set *, readfds,
	  fd_set *, writefds, fd_set *, exceptfds,
	  struct timeval *, timeout);
#endif
_syscall6(int, pselect, __NR_pselect6, int, nfds, fd_set *, readfds,
	  fd_set *, writefds, fd_set *, exceptfds,
	  struct timespec *, timeout, const sigset_t *, sigmask);
_syscall0(int, sched_yield, __NR_sched_yield);
_syscall3(int, mincore, __NR_mincore, void *, addr, size_t, length,
	  unsigned char *, vec);
_syscall3(int, madvise, __NR_madvise, void *, addr, size_t, length,
	  int, advice);
#if !defined(_I386) && _MIPS_SIM != _MIPS_SIM_ABI32
_syscall3(int, shmget, __NR_shmget, key_t, key, size_t, size,
	  int, shmflag);
_syscall3(void *, shmat, __NR_shmat, int, shmid,
	  const void *, shmaddr, int, shmflg);
_syscall1(int, shmdt, __NR_shmdt, const void *, shmaddr);
_syscall3(int, shmctl, __NR_shmctl, int, shmid, int, cmd,
	  struct shmid_ds *, buf);
#endif
_syscall1(int, dup, __NR_dup, int, oldfd);
_syscall2(int, dup2, __NR_dup2, int, oldfd, int, newfd);
_syscall3(int, dup3, __NR_dup3, int, oldfd, int, newfd, int, flags);
_syscall0(int, pause, __NR_pause);
_syscall2(int, nanosleep, __NR_nanosleep, const struct timespec *, req,
	  struct timespec *, rem);
_syscall2(int, getitimer, __NR_getitimer, int, which,
	  struct itimerval *, curr_value);
_syscall1(unsigned int, alarm, __NR_alarm, unsigned int, seconds);
_syscall3(int, setitimer, __NR_setitimer, int, which,
	  struct itimerval *, new_value, struct itimerval *, old_value);
_syscall0(int, getpid, __NR_getpid);
_syscall0(int, getppid, __NR_getppid);
_syscall4(ssize_t, sendfile, __NR_sendfile, int, out_fd, int, in_fd,
	  off_t *, offset, size_t, count);
#ifdef _I386
/* Only on i386: */
_syscall2(int, socketcall, __NR_socketcall, int, call,
	  unsigned long *, args);
#else
/* All modern architectures */
_syscall3(int, socket, __NR_socket, int, domain, int, type,
	  int, protocol);
_syscall3(int, connect, __NR_connect, int, sockfd,
	  const struct sockaddr *, addr, socklen_t, addrlen);
_syscall3(int, accept, __NR_accept, int, sockfd,
	  struct sockaddr *, addr, socklen_t *, addrlen);
_syscall6(ssize_t, sendto, __NR_sendto, int, sockfd,
	  const void *, buf, size_t, len, int, flags,
	  const struct sockaddr *, dest_addr, socklen_t, addrlen);
_syscall6(ssize_t, recvfrom, __NR_recvfrom, int, sockfd,
	  const void *, buf, size_t, len, int, flags,
	  const struct sockaddr *, src_addr, socklen_t, addrlen);
_syscall3(ssize_t, sendmsg, __NR_sendmsg, int, sockfd,
	  const struct msghdr *, msg, int, flags);
_syscall3(ssize_t, recvmsg, __NR_recvmsg, int, sockfd,
	  struct msghdr *, msg, int, flags);
_syscall2(int, shutdown, __NR_shutdown, int, sockfd, int, how);
_syscall3(int, bind, __NR_bind, int, sockfd,
	  const struct sockaddr *, addr, socklen_t, addrlen);
_syscall2(int, listen, __NR_listen, int, sockfd, int, backlog);
_syscall3(int, getsockname, __NR_getsockname, int, sockfd,
	  struct sockaddr *, addr, socklen_t *, addrlen);
_syscall3(int, getpeername, __NR_getpeername, int, sockfd,
	  struct sockaddr *, addr, socklen_t *, addrlen);
_syscall4(int, socketpair, __NR_socketpair, int, domain, int, type,
	  int, protocol, int *, sv); // [2]
_syscall5(int, setsockopt, __NR_setsockopt, int, sockfd, int, level,
	  int, optname, void *, optval, socklen_t *, optlen);
_syscall5(int, getsockopt, __NR_getsockopt, int, sockfd, int, level,
	  int, optname, void *, optval, socklen_t *, optlen);
_syscall4(int, accept4, __NR_accept4, int, sockfd,
	  struct sockaddr *, addr, socklen_t *, addrlen, int, flags);
#endif
// ??? FIX function pointer, var args???
// _syscall6(int, clone, __NR_clone, int (*)(void *), fn, void *,
//	     child_stack, int, flags, void *, arg, ...);
_syscall0(pid_t, fork, __NR_fork);
_syscall0(pid_t, vfork, __NR_vfork);
_syscall3(int, execve, __NR_execve, const char *, filename,
	  char *const *, argv, char *const *, envp); // []
_syscall1(void, _exit, __NR_exit, int, status);
_syscall4(pid_t, wait4, __NR_wait4, pid_t, pid, int *, status,
	  int, options, struct rusage *, rusage);
_syscall2(int, kill, __NR_kill, pid_t, pid, int, sig);
_syscall1(int, uname, __NR_uname, struct utsname *, buf);
#if !defined(_I386) && _MIPS_SIM != _MIPS_SIM_ABI32
_syscall3(int, semget, __NR_semget, key_t, key, int, nsems,
	  int, semflg);
_syscall3(int, semop, __NR_semop, int, semid, struct sembuf *, sops,
	  size_t, nsops);
// _syscall3(int, semctl, __NR_semctl, int, semid, int, semmnum,
//           int, cmd, ...); ???
_syscall4(int, semtimedop, __NR_semtimedop, int, semid,
	  struct sembuf *, sops, size_t, nsops,
	  struct timespec *, timeout);
_syscall2(int, msgget, __NR_msgget, key_t, key, int, msgflg);
_syscall4(int, msgsnd, __NR_msgsnd, int, msgid, const void *, msgp,
	  size_t, msgsz, int, msgflg);
_syscall5(ssize_t, msgrcv, __NR_msgrcv, int, msgid, void *, msgp,
	  size_t, msgsz, long, msgtyp, int, msgflg);
_syscall3(int, msgctl, __NR_msgctl, int, msgid, int, cmd,
	  struct msqid_ds *, buf);
#endif
// _syscall3(int, fcntl, __NR_fcntl, int, fd, int, cmd, ...); // ???
_syscall2(int, flock, __NR_flock, int, fd, int, operation);
_syscall1(int, fsync, __NR_fsync, int, fd);
_syscall1(int, fdatasync, __NR_fdatasync, int, fd);
_syscall2(int, truncate, __NR_truncate, const char *, path,
	  off_t, length);
_syscall2(int, ftruncate, __NR_ftruncate, int, fd, off_t, length);
_syscall3(int, getdents, __NR_getdents, unsigned int, fd,
	  struct linux_dirent *, dirp, unsigned int, count);
_syscall2(char *, getcwd, __NR_getcwd, char *, buf, size_t, size);
_syscall1(int, chdir, __NR_chdir, const char *, path);
_syscall1(int, fchdir, __NR_fchdir, int, fd);
_syscall2(int, rename, __NR_rename, const char *, oldpath,
	  const char *, newpath);
_syscall2(int, mkdir, __NR_mkdir, const char *, pathname,
	  mode_t, mode);
_syscall1(int, rmdir, __NR_rmdir, const char *, pathname);
_syscall2(int, creat, __NR_creat, const char *, pathname,
	  mode_t, mode);
_syscall2(int, link, __NR_link, const char *, oldpath,
	  const char *, newpath);
_syscall1(int, unlink, __NR_unlink, const char *, pathname);
_syscall2(int, symlink, __NR_symlink, const char *, oldpath,
	  const char *, newpath);
_syscall3(ssize_t, readlink, __NR_readlink, const char *, path,
	  char *, buf, size_t, bufsiz);
_syscall2(int, chmod, __NR_chmod, const char *, path, mode_t, mode);
_syscall2(int, fchmod, __NR_fchmod, int, fd, mode_t, mode);
_syscall3(int, chown, __NR_chown, const char *, path, uid_t, owner,
	  gid_t, group);
_syscall3(int, fchown, __NR_fchown, int, fd, uid_t, owner,
	  gid_t, group);
_syscall3(int, lchown, __NR_lchown, const char *, path, uid_t, owner,
	  gid_t, group);
_syscall1(mode_t, umask, __NR_umask, mode_t, mask);
_syscall2(int, gettimeofday, __NR_gettimeofday, struct timeval *, tv,
	  struct timezone *, tz);
_syscall2(int, getrlimit, __NR_getrlimit, int, resource,
	  struct rlimit *, rlim);
_syscall2(int, getrusage, __NR_getrusage, int, who,
	  struct rusage *, usage);
_syscall1(int, sysinfo, __NR_sysinfo, struct sysinfo *, info);
_syscall1(clock_t, times, __NR_times, struct tms *, buf);
_syscall4(long, ptrace, __NR_ptrace, enum __ptrace_request, request,
	  pid_t, pid, void *, addr, void *, data);
_syscall0(uid_t, getuid, __NR_getuid);
_syscall3(int, syslog, __NR_syslog, int, type, char *, bufp, int, len);
_syscall0(gid_t, getgid, __NR_getgid);
_syscall1(int, setuid, __NR_setuid, uid_t, uid);
_syscall1(int, setgid, __NR_setgid, gid_t, gid);
_syscall0(uid_t, geteuid, __NR_geteuid);
_syscall0(gid_t, getegid, __NR_getegid);
_syscall2(int, setpgid, __NR_setpgid, pid_t, pid, pid_t, pgid);
_syscall0(pid_t, getpgrp, __NR_getpgrp);
_syscall0(pid_t, setsid, __NR_setsid);
_syscall2(int, setreuid, __NR_setreuid, uid_t, ruid, uid_t, euid);
_syscall2(int, setregid, __NR_setregid, gid_t, rgid, gid_t, egid);
_syscall2(int, getgroups, __NR_getgroups, int, size,
	  gid_t *, list); // []
_syscall2(int, setgroups, __NR_setgroups, size_t, size,
	  const gid_t *, list);
_syscall3(int, setresuid, __NR_setresuid, uid_t, ruid, uid_t, euid,
	  uid_t, suid);
_syscall3(int, getresuid, __NR_getresuid, uid_t, ruid, uid_t, euid,
	  uid_t, suid);
_syscall3(int, setresgid, __NR_setresgid, gid_t, rgid, gid_t, egid,
	  gid_t, sgid);
_syscall3(int, getresgid, __NR_getresgid, gid_t, rgid, gid_t, egid,
	  gid_t, sgid);
_syscall1(pid_t, getpgid, __NR_getpgid, pid_t, pid);
_syscall1(int, setfsuid, __NR_setfsuid, uid_t, fsuid);
// setfsgid: Mistake in Linux manual!  gid_t
_syscall1(int, setfsgid, __NR_setfsgid, gid_t, fsgid);
_syscall1(pid_t, getsid, __NR_getsid, pid_t, pid);
_syscall2(int, capget, __NR_capget, cap_user_header_t, hdrp,
	  cap_user_data_t, datap);
_syscall2(int, capset, __NR_capset, cap_user_header_t, hdrp,
	  const cap_user_data_t, datap);
_syscall1(int, sigpending, __NR_rt_sigpending, sigset_t *, set);
_syscall3(int, sigtimedwait, __NR_rt_sigtimedwait,
	  const sigset_t *, set, siginfo_t *, info,
	  const struct timespec *, timeout);
_syscall3(int, rt_sigqueueinfo, __NR_rt_sigqueueinfo, pid_t, tgid,
	  int, sig, siginfo_t *, uinfo);
_syscall1(int, sigsuspend, __NR_rt_sigsuspend, const sigset_t *, mask);
_syscall2(int, sigaltstack, __NR_sigaltstack, const stack_t *, ss,
	  stack_t *, oss);
_syscall2(int, utime, __NR_utime, const char *, filename,
	  const struct utimbuf *, times);
_syscall3(int, mknod, __NR_mknod, const char *, pathname,
	  mode_t, mode, dev_t, dev);
#if _MIPS_SIM != _MIPS_SIM_ABI64 && _MIPS_SIM != _MIPS_SIM_NABI32
_syscall1(int, uselib, __NR_uselib, const char *, library);
#endif
_syscall1(int, personality, __NR_personality, unsigned long, personal);
_syscall2(int, ustat, __NR_ustat, dev_t, dev, struct ustat *, ubuf);
_syscall2(int, statfs, __NR_statfs, const char *, path,
	  struct statfs *, buf);
_syscall2(int, fstatfs, __NR_fstatfs, int, fd, struct statfs *, buf);
_syscall3(int, sysfs, __NR_sysfs, int, option,
	  void *, arg2, char *, buf); // ??? varargs
_syscall2(int, getpriority, __NR_getpriority, int, which, int, who);
_syscall3(int, setpriority, __NR_setpriority, int, which, int, who,
	  int, prio);
_syscall2(int, sched_setparam, __NR_sched_setparam, pid_t, pid,
	  const struct sched_param *, param);
_syscall2(int, sched_getparam, __NR_sched_getparam, pid_t, pid,
	  struct sched_param *, param);
_syscall3(int, sched_setscheduler, __NR_sched_setscheduler,
	  pid_t, pid, int, policy, const struct sched_param *, param);
_syscall1(int, sched_getscheduler, __NR_sched_getscheduler, pid_t, pid);
_syscall1(int, sched_get_priority_max, __NR_sched_get_priority_max,
	  int, policy);
_syscall1(int, sched_get_priority_min, __NR_sched_get_priority_min,
	  int, policy);
_syscall2(int, sched_rr_get_interval, __NR_sched_rr_get_interval,
	  pid_t, pid, struct timespec *, tp);
_syscall2(int, mlock, __NR_mlock, const void *, addr, size_t, len);
_syscall2(int, munlock, __NR_munlock, const void *, addr, size_t, len);
_syscall1(int, mlockall, __NR_mlockall, int, flags);
_syscall0(int, munlockall, __NR_munlockall);
_syscall0(int, vhangup, __NR_vhangup);
#ifndef _ARM
_syscall3(int, modify_ldt, __NR_modify_ldt, int, func, void *, ptr,
	  unsigned long, bytecount);
#endif
_syscall2(int, pivot_root, __NR_pivot_root, const char *, new_root,
	  const char *, put_old);
_syscall1(int, sysctl, __NR__sysctl, struct __sysctl_args *, args);
_syscall5(int, prctl, __NR_prctl, int, option, unsigned long, arg2,
	  unsigned long, arg3, unsigned long, arg4, unsigned long, arg5);
#if !defined(_I386) && !defined(_ARM) && !defined(_MIPS)
_syscall2(int, arch_prctl, __NR_arch_prctl, int, code, unsigned long, addr);
#endif
_syscall1(int, adjtimex, __NR_adjtimex, struct timex *, buf);
_syscall2(int, setrlimit, __NR_setrlimit, int, resource,
	  const struct rlimit *, rlim);
_syscall1(int, chroot, __NR_chroot, const char *, path);
_syscall0(void, sync, __NR_sync);
_syscall1(int, acct, __NR_acct, const char *, filename);
_syscall2(int, settimeofday, __NR_settimeofday,
	  const struct timeval *, tv, const struct timezone *, tz);
_syscall5(int, mount, __NR_mount, const char *, source,
	  const char *, target, const char *, filesystemtype,
	  unsigned long, mountflags, const void *, data);
_syscall2(int, umount2, __NR_umount2, const char *, target, int, flags);
_syscall2(int, swapon, __NR_swapon, const char *, path, int, swapflags);
_syscall1(int, swapoff, __NR_swapoff, const char *, path);
_syscall4(int, reboot, __NR_reboot, int, magic, int, magic2,
	  int, cmd, void *, arg);
_syscall2(int, sethostname, __NR_sethostname, const char *, name,
	  size_t, len);
_syscall2(int, setdomainname, __NR_setdomainname, const char *, name,
	  size_t, len);
#if !defined(_ARM) && _MIPS_SIM != _MIPS_SIM_ABI64 && _MIPS_SIM != _MIPS_SIM_NABI32
_syscall1(int, iopl, __NR_iopl, int, level);
_syscall3(int, ioperm, __NR_ioperm, unsigned long, from,
	  unsigned long, num, int, turn_on);
#endif

#ifndef _ARM
_syscall2(caddr_t, create_module, __NR_create_module,
	  const char *, name, size_t, size);
// ??? fd or char* to module?
_syscall3(int, init_module, __NR_init_module, void *, module_image,
	  unsigned long, len, const char *, param_values);
_syscall2(int, delete_module, __NR_delete_module,
	  const char *, name, int ,flags);
_syscall1(int, get_kernel_syms, __NR_get_kernel_syms,
	  struct kernel_sym *, table);
_syscall5(int, query_module, __NR_query_module, const char *, name,
	  int, which, void *, buf, size_t, bufsize, size_t *, ret);
#endif

_syscall4(int, quotactl, __NR_quotactl, int, cmd,
	  const char *, special, int, id, caddr_t, addr);
_syscall3(long, nfsservctl, __NR_nfsservctl, int, cmd,
	  struct nfsctl_arg *, argp, union nfsctl_res *, resp);
// _syscall?(?, getpmsg, __NR_getpmsg); ??? not implemented
// _syscall?(?, putpmsg, __NR_putpmsg); ??? not implemented
// _syscall?(?, afs_syscall, __NR_afs_syscall); ??? not implemented
// _syscall?(?, tuxcall, __NR_tuxcall); ??? not implemented
// _syscall?(?, security, __NR_security); ??? not implemented
_syscall0(pid_t, gettid, __NR_gettid);
// ??? How to pass 64-bit argument on 32-bit systems?
// _syscall3(ssize_t, readahead, __NR_readahead, int, fd,
//	     off64_t, offset, size_t, count);
_syscall5(int, setxattr, __NR_setxattr, const char *, path,
	  const char *, name, const void *, value, size_t, size,
	  int, flags);
_syscall5(int, lsetxattr, __NR_lsetxattr, const char *, path,
	  const char *, name, const void *, value, size_t, size,
	  int, flags);
_syscall5(int, fsetxattr, __NR_fsetxattr, int, fd,
	  const char *, name, const void *, value, size_t, size,
	  int, flags);
_syscall4(ssize_t, getxattr, __NR_getxattr, const char *, path,
	  const char *, name, void *, value, size_t, size);
_syscall4(ssize_t, lgetxattr, __NR_lgetxattr, const char *, path,
	  const char *, name, void *, value, size_t, size);
_syscall4(ssize_t, fgetxattr, __NR_fgetxattr, int, fd,
	  const char *, name, void *, value, size_t, size);
_syscall3(ssize_t, listxattr, __NR_listxattr, const char *, path,
	  char *, list, size_t, size);
_syscall3(ssize_t, llistxattr, __NR_llistxattr, const char *, path,
	  char *, list, size_t, size);
_syscall3(ssize_t, flistxattr, __NR_flistxattr, int, fd,
	  char *, list, size_t, size);
_syscall2(int, removexattr, __NR_removexattr, const char *, path,
	  const char *, name);
_syscall2(int, lremovexattr, __NR_lremovexattr, const char *, path,
	  const char *, name);
_syscall2(int, fremovexattr, __NR_fremovexattr, int, fd,
	  const char *, name);
_syscall2(int, tkill, __NR_tkill, int, tid, int, sig);
#if _MIPS_SIM != _MIPS_SIM_ABI64 && _MIPS_SIM != _MIPS_SIM_NABI32
_syscall1(time_t, time, __NR_time, time_t *, t);
#endif
_syscall6(int, futex, __NR_futex, int *, uaddr, int, op, int, val,
	  const struct timespec *, timeout, int *, uaddr2, int, val3);
_syscall3(int, sched_setaffinity, __NR_sched_setaffinity,
	  pid_t, pid, size_t, cpusetsize, cpu_set_t *, mask);
_syscall3(int, sched_getaffinity, __NR_sched_getaffinity,
	  pid_t, pid, size_t, cpusetsize, cpu_set_t *, mask);
#ifndef _ARM
_syscall1(int, set_thread_area, __NR_set_thread_area,
	  struct user_desc *, u_info);
#endif
_syscall2(int, io_setup, __NR_io_setup, unsigned, rr_events,
	  aio_context_t *, ctxp);
_syscall1(int, io_destroy, __NR_io_destroy, aio_context_t, ctx);
_syscall5(int, io_getevents, __NR_io_getevents, aio_context_t, ctx_id,
	  long, min_nr, long, nr, struct io_event *, events,
	  struct timespec *, timeout);
_syscall3(int, io_submit, __NR_io_submit, aio_context_t, ctx_id,
	  long, nr, struct iocb *, iocbpp);
_syscall3(int, io_cancel, __NR_io_cancel, aio_context_t, ctx_id,
	  struct iocb *, iocb, struct io_event *, end_result);
#if !defined(_ARM) && !defined(_MIPS)
_syscall1(int, get_thread_area, __NR_get_thread_area,
	 struct user_desc *, u_info);
#endif
// ??? How to pass 64-bit argument on 32-bit systems?
// _syscall3(int, lookup_dcookie, __NR_lookup_dcookie, u64, cookie,
//	     char *, buffer, size_t, len);
_syscall1(int, epoll_create, __NR_epoll_create, int, size);
// _syscall?(?, epoll_ctl_old, __NR_epoll_ctl_old); ???
// _syscall?(?, epoll_wait_old, __NR_epoll_wait_old); ???
_syscall5(int, remap_file_pages, __NR_remap_file_pages,
	  void *, addr, size_t, size, int, prot, ssize_t, pgoff,
	  int, flags);
_syscall3(int, getdents64, __NR_getdents64, unsigned int, fd,
	  struct linux_dirent64 *, dirp, unsigned int, count);
_syscall1(long, set_tid_address, __NR_set_tid_address, int *, tidptr);
_syscall0(long, restart_syscall, __NR_restart_syscall);
#ifndef _ARM
_syscall4(int, posix_fadvise, __NR_fadvise64, int, fd, off_t, offset,
	  off_t, len, int, advice);
#endif
_syscall3(int, timer_create, __NR_timer_create, clockid_t, clockid,
	  struct sigevent *, evp, timer_t *, timerid);
_syscall4(int, timer_settime, __NR_timer_settime, timer_t, timerid,
	  int, flags, const struct itimerspec *, new_value,
	  struct itimerspec *, old_value);
_syscall2(int, timer_gettime, __NR_timer_gettime, timer_t, timer_id,
	  struct itimerspec *, curr_value);
_syscall1(int, timer_getoverrun, __NR_timer_getoverrun,
	  timer_t, timerid);
_syscall1(int, timer_delete, __NR_timer_delete, timer_t, timerid);
_syscall2(int, clock_settime, __NR_clock_settime, clockid_t, clk_id,
	  const struct timespec *, tp);
_syscall2(int, clock_gettime, __NR_clock_gettime, clockid_t, clk_id,
	  struct timespec *, tp);
_syscall2(int, clock_getres, __NR_clock_getres, clockid_t, clk_id,
	  struct timespec *, res);
_syscall4(int, clock_nanosleep, __NR_clock_nanosleep,
	  clockid_t, clock_id, int, flags,
	  const struct timespec *, request, struct timespec *, remain);
_syscall1(void, exit_group, __NR_exit_group, int, status);
_syscall4(int, epoll_wait, __NR_epoll_wait, int, epfd,
	  struct epoll_event *, events, int, maxevents, int, timeout);
_syscall4(int, epoll_ctl, __NR_epoll_ctl, int, epfd, int, op,
	  int, fd, struct epoll_event *, event);
_syscall3(int, tgkill, __NR_tgkill, int, tgid, int, tid, int, sig);
_syscall2(int, utimes, __NR_utimes, const char *, filename,
	  const struct timeval *, times); // [2]
// _syscall?(?, vserver, __NR_vserver); ??? not implemented
_syscall6(int, mbind, __NR_mbind, void *, addr, unsigned long, len,
	  int, mode, unsigned long *, nodemask, unsigned long, maxnode,
	  unsigned long, flags);
_syscall3(long, set_mempolicy, __NR_set_mempolicy, int, mode,
	  const unsigned long *, nodemask, unsigned long, maxnode);
_syscall5(long, get_mempolicy, __NR_get_mempolicy, int *, mode,
	  unsigned long *, nodemask, unsigned long, maxnode,
	  void *, addr, unsigned long, flags);
_syscall4(mqd_t, mq_open, __NR_mq_open, const char *, name, int, oflag,
	  mode_t, mode, struct mq_attr *, attr);
_syscall1(mqd_t, mq_unlink, __NR_mq_unlink, const char *, name);
_syscall5(mqd_t, mq_timedsend, __NR_mq_timedsend, mqd_t, mqdes,
	  const char *, msg_ptr, size_t, msg_len, unsigned, msg_prio,
	  const struct timespec *, abs_timeout);
_syscall5(ssize_t, mq_timedreceive, __NR_mq_timedreceive,
	  mqd_t, mqdes, char *, msg_ptr, size_t, msg_len,
	  unsigned *, msg_prio, const struct timespec *, abs_timeout);
_syscall2(mqd_t, mq_notify, __NR_mq_notify, mqd_t, mqdes,
	  const struct sigevent *, notification);
_syscall3(mqd_t, mq_getsetattr, __NR_mq_getsetattr, mqd_t, mqdes,
	  struct mq_attr *, newattr, struct mq_attr *, oldattr);
_syscall4(long, kexec_load, __NR_kexec_load, unsigned long, entry,
	  unsigned long, nr_segments, struct kexec_segment *, segments,
	  unsigned long, flags);
_syscall4(int, waitid, __NR_waitid, idtype_t, idtype, id_t, id,
	  siginfo_t *, infop, int, options);
_syscall5(key_serial_t, add_key, __NR_add_key, const char *, type,
	  const char *, description, const void *, payload,
	  size_t, plen, key_serial_t, keyring);
_syscall4(key_serial_t, request_key, __NR_request_key,
	  const char *, type, const char *, description,
	  const char *, callout_info, key_serial_t, dest_keyring);
_syscall5(long, keyctl, __NR_keyctl, int, operation,
	  __kernel_ulong_t, arg2, __kernel_ulong_t, arg3,
	  __kernel_ulong_t, arg4, __kernel_ulong_t, arg5);
_syscall3(int, ioprio_set, __NR_ioprio_set, int, which, int, who,
	  int, ioprio);
_syscall2(int, ioprio_get, __NR_ioprio_get, int, which, int, who);
_syscall0(int, inotify_init, __NR_inotify_init);
_syscall3(int, inotify_add_watch, __NR_inotify_add_watch,
	  int, fd, const char *, pathname, uint32_t, mask);
_syscall2(int, inotify_rm_watch, __NR_inotify_rm_watch,
	  int, fd, uint32_t, wd);
#ifndef _ARM
_syscall4(long, migrate_pages, __NR_migrate_pages, int, pid,
	  unsigned long, maxnode, const unsigned long *, old_nodes,
	  const unsigned long *, new_nodes);
#endif
_syscall4(int, openat, __NR_openat, int, dirfd, const char *, pathname,
	  int, flags, mode_t, mode);
_syscall3(int, mkdirat, __NR_mkdirat, int, dirfd, const char *, pathname,
	  mode_t, mode);
_syscall4(int, mknodat, __NR_mknodat, int, dirfd, const char *, pathname,
	  mode_t, mode, dev_t, dev);
_syscall5(int, fchownat, __NR_fchownat, int, dirfd,
	  const char *, pathname, uid_t, owner, gid_t, group, int, flags);
_syscall3(int, futimesat, __NR_futimesat, int, dirfd,
	  const char *, pathname, const struct timeval *, times); // [2]
#if defined(_I386) || defined(_ARM) || _MIPS_SIM == _MIPS_SIM_ABI32
_syscall4(int, fstatat, __NR_fstatat64, int, dirfd,
	  const char *, pathname, struct stat *, statbuf, int, flags);
#else
_syscall4(int, fstatat, __NR_newfstatat, int, dirfd,
	  const char *, pathname, struct stat *, statbuf, int, flags);
#endif
_syscall3(int, unlinkat, __NR_unlinkat, int, dirfd,
	  const char *, pathname, int, flags);
_syscall4(int, renameat, __NR_renameat, int, olddirfd,
	  const char *, oldpath, int, newdirfd, const char *, newpath);
_syscall5(int, linkat, __NR_linkat, int, olddirfd,
	  const char *, oldpath, int, newdirfd, const char *, newpath,
	  int, flags);
_syscall3(int, symlinkat, __NR_symlinkat, const char *, oldpath,
	  int, newdirfd, const char *, newpath);
_syscall4(int, readlinkat, __NR_readlinkat, int, dirfd,
	  const char *, pathname, char *, buf, size_t, bufsiz);
_syscall4(int, fchmodat, __NR_fchmodat, int, dirfd,
	  const char *, pathname, mode_t, mode, int, flags);
_syscall4(int, faccessat, __NR_faccessat, int, dirfd,
	  const char *, pathname, int, mode, int, flags);
_syscall1(int, unshare, __NR_unshare, int, flags);
_syscall2(long, set_robust_list, __NR_set_robust_list,
	  struct robust_list_head *, head, size_t, len);
_syscall3(long, get_robust_list, __NR_get_robust_list, int, pid,
	  struct robust_list_head **, head_ptr, size_t *, len_ptr);
_syscall6(ssize_t, splice, __NR_splice, int, fd_in, loff_t *, off_in,
	  int, fd_out, loff_t *, off_out, size_t, len,
	  unsigned int, flags);
_syscall4(ssize_t, tee, __NR_tee, int, fd_in, int, fd_out, size_t, len,
	  unsigned int, flags);
// ??? How to pass 64-bit argument on 32-bit systems?
// _syscall4(int, sync_file_range, __NR_sync_file_range, int, fd,
//	     off64_t, offset, off64_t, nbytes, unsigned int, flags);
_syscall4(ssize_t, vmsplice, __NR_vmsplice, int, fd,
	  const struct iovec *, iov, unsigned long, nr_segs,
	  unsigned int, flags);
_syscall6(long, move_pages, __NR_move_pages, int, pid,
	  unsigned long, count, void **, pages, const int *, nodes,
	  int *, status, int, flags);
_syscall4(int, utimensat, __NR_utimensat, int, dirfd,
	  const char *, pathname, const struct timespec *, times,
	  int, flags); // [2]
// __IGNORE_getcpu ???
_syscall5(int, epoll_pwait, __NR_epoll_pwait, int, epfd,
	  struct epoll_event *, events, int, maxevents, int, timeout,
	  const sigset_t *, sigmask);
_syscall3(int, signalfd, __NR_signalfd, int, fd, const sigset_t *, mask,
	  int, flags);
_syscall2(int, timerfd_create, __NR_timerfd_create, int, clockid,
	  int, flags);
_syscall2(int, eventfd, __NR_eventfd, unsigned int, initval, int, flags);
_syscall4(int, fallocate, __NR_fallocate, int, fd, int, mode,
	  off_t, offset, off_t, len);
_syscall4(int, timerfd_settime, __NR_timerfd_settime, int, fd, int, flags,
	  const struct itimerspec *, new_value,
	  struct itimerspec *, old_value);
_syscall2(int, timerfd_gettime, __NR_timerfd_gettime, int, fd,
	  struct itimerspec *, curr_value);
_syscall3(int, signalfd4, __NR_signalfd4, int, fd,
	  const sigset_t *, mask, int, flags);
_syscall2(int, eventfd2, __NR_eventfd2, unsigned int, initval, int, flags);
_syscall1(int, epoll_create1, __NR_epoll_create1, int, flags);
_syscall1(int, inotify_init1, __NR_inotify_init1, int, flags);
_syscall4(ssize_t, preadv, __NR_preadv, int, fd,
	  const struct iovec *, iov, int, iovcnt, off_t, offset);
_syscall4(ssize_t, pwritev, __NR_pwritev, int, fd,
	  const struct iovec *, iov, int, iovcnt, off_t, offset);
_syscall4(int, rt_tgsigqueueinfo, __NR_rt_tgsigqueueinfo, pid_t, tgid,
	  int, tid, int, sig, siginfo_t *, uinfo);
_syscall5(int, perf_event_open, __NR_perf_event_open,
	  struct perf_event_attr *, attr, pid_t, pid, int, cpu,
	  int, group_fd, unsigned long, flags);

#endif /* not ALLSYS_H */
