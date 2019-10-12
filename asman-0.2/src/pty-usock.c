/* pty-usock.c -- Open a Unix-domain socket and expose it by a
   PTY.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <arpa/inet.h>
#include <pty.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>

#define BUFSIZE 1

int sockfd;
int pty_master;

/* Set the `O_NONBLOCK' flag of DESC if VALUE is nonzero, or clear the
   flag if VALUE is 0.  Return 0 on success, or -1 on error with
   `errno' set.  */
int set_nonblock_flag (int desc, int value)
{
	int oldflags = fcntl(desc, F_GETFL, 0);
	if (oldflags == -1)
		return -1;
	if (value != 0)
		oldflags |= O_NONBLOCK;
	else
		oldflags &= ~O_NONBLOCK;
	return fcntl(desc, F_SETFL, oldflags);
}

long
write_all (int fd, const void *buf, unsigned long count)
{
  const unsigned char *cur_pos = buf;
  unsigned long num_pending = count;
  do {
    unsigned long result = write (fd, cur_pos, num_pending);
    if (result == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
        continue;
      else
        return -1;
    }
    cur_pos += result;
    num_pending -= result;
  } while (num_pending > 0);
  return count;
}

void int_handler(int signum)
{
	puts("Closing pty master");
	if (close(pty_master) == -1)
	{
		perror("close pty master");
	}
	close(sockfd);
	exit(0);
}

int main(int argc, char *argv[])
{
	int numbytes;  
	char buf[BUFSIZE];
	struct sockaddr_un un_sock;
	struct addrinfo hints;
	int rv;
	int pty_slave;
	char pty_name[PATH_MAX];
	fd_set pty_fdset;
	int max_fd;

	if (argc != 2)
	{
	    fprintf(stderr,"usage: client sockfile\n");
	    exit(1);
	}

	un_sock.sun_family = AF_LOCAL;
	strncpy(un_sock.sun_path, argv[1], sizeof(un_sock.sun_path));
	un_sock.sun_path[sizeof(un_sock.sun_path)-1] = '\0';

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_LOCAL;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_addr = (struct sockaddr *)&un_sock;
	hints.ai_addrlen = sizeof(struct sockaddr_un);

	/* Connect to the Unix-domain socket.  */
	if ((sockfd = socket(hints.ai_family, hints.ai_socktype,
						 hints.ai_protocol)) == -1)
	{
		perror("client: socket");
		exit(2);
	}

	if (connect(sockfd, hints.ai_addr, hints.ai_addrlen) == -1)
	{
		close(sockfd);
		perror("client: connect");
		exit(2);
	}

	printf("client: connecting to %s\n", un_sock.sun_path);

	/* Setup the pseudo-terminal that routes the socket.  */
	if (openpty(&pty_master, &pty_slave, pty_name, NULL, NULL) == -1)
	{
		perror("openpty");
		exit(1);
	}
	if (close(pty_slave) == -1)
	{
		perror("close pty slave");
		exit(1);
	}
	signal(SIGINT, int_handler);
	printf("Connect to pty '%s'\n", pty_name);

	/* Now we just wait forever, keeping our socket routed to the
	   PTY.  */
	set_nonblock_flag(sockfd, 1);
	set_nonblock_flag(pty_master, 1);
	max_fd = (pty_master > sockfd) ? pty_master : sockfd;
	max_fd++;
	FD_ZERO(&pty_fdset);
	FD_SET(sockfd, &pty_fdset);
	FD_SET(pty_master, &pty_fdset);
	while (1)
	{
		/* `select()' does not work reliably on Unix-domain sockets?  */
		/* select(FD_SETSIZE, &pty_fdset, NULL, NULL, NULL); */

		/* Pipe socket input.  */
		rv = read(sockfd, buf, BUFSIZE);
		if (/* rv == 0 || */ rv == -1 &&
			errno != EAGAIN && errno != EWOULDBLOCK)
		{ perror("read"); break; }
		if (rv != 0 && rv != -1)
		{
			rv = write_all(pty_master, buf, rv);
			if (rv == -1)
			{ perror("write"); break; }
		}

		/* Pipe socket output.  */
		/* Ignore failure to read from slave.  */
		rv = read(pty_master, buf, BUFSIZE);
		if (/* rv == 0 || */ rv == -1 &&
			errno != EAGAIN && errno != EWOULDBLOCK &&
			errno != EIO)
		{ perror("read"); break; }
		if (rv != 0 && rv != -1)
		{
			rv = write_all(sockfd, buf, rv);
			if (rv == -1)
			{ perror("write"); break; }
		}
	}

	int_handler(SIGINT);
}
