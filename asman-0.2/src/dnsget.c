/* DumDNS: An ultra-simple DNS client library.

2019

20190816/https://www.ietf.org/rfc/rfc1035.txt
20190816/https://tools.ietf.org/html/rfc3596

*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define OPCODE_QUERY 0
#define OPCODE_IQUERY 1
#define OPCODE_STATUS 2

#define TYPE_A 1
#define TYPE_NS 2
#define TYPE_MD 3 /* obsolete */
#define TYPE_MF 4 /* obsolete */
#define TYPE_CNAME 5
#define TYPE_SOA 6
#define TYPE_MB 7
#define TYPE_MG 8
#define TYPE_MR 9
#define TYPE_NULL 10
#define TYPE_WKS 11
#define TYPE_PTR 12
#define TYPE_HINFO 13
#define TYPE_MINFO 14
#define TYPE_MX 15
#define TYPE_TXT 16
#define TYPE_AAAA 28
#define QTYPE_AXFR 252 /* transfer entire zone */
#define QTYPE_MAILB 253 /* all mailbox related records */
#define QTYPE_MAILA 254 /* mail agent RRs (obsolete, see MX) */
#define QTYPE_ALL 255 /* all records */

#define CLASS_IN 1 /* Internet */
#define CLASS_CS 2 /* CSNET (obsolete) */
#define CLASS_CH 3 /* CHAOSNET (obsolete) */
#define CLASS_HS 4 /* Hesiod (obsolete version of NIS/LDAP) */
#define QCLASS_ANY 255 /* any class */

/* Resolver library flags */
#define RES_INIT 1
#define RES_DEBUG 2
#define RES_USEVC 4 /* Use TCP?  */
#define RES_RECURSE 8

/* Bit fields LSB must come first.  When sending across network,
   16-bit integers must be converted to Network Byte Order.  Structure
   padding/packing must be one byte.  */
struct DNSHead_tag
{
  unsigned short id;

  /* Byte 1 */
  unsigned char rd : 1; /* recursion desired */
  unsigned char tc : 1; /* truncation */
  unsigned char aa : 1; /* authoritative answer */
  unsigned char opcode : 4;
  unsigned char qr : 1; /* query (0) or response (1) */
  /* Byte 2 */
  unsigned char rcode : 4; /* response code */
  unsigned char z : 3; /* reserved for future use, must be zero */
  unsigned char ra : 1; /* recursion available */

  unsigned short qdcount; /* question count */
  unsigned short ancount; /* answer count */
  unsigned short nscount; /* name server response record count */
  unsigned short arcount; /* additional resource record count */
} __attribute__((packed));
typedef struct DNSHead_tag DNSHead;

struct QDTail_tag
{
  unsigned short qtype; /* A = 1, ... */
  unsigned short qclass; /* always 1 for our concerns */
} __attribute__((packed));
typedef struct QDTail_tag QDTail;

struct RRTail_tag
{
  unsigned short type;
  unsigned short class; /* always 1 for our concerns */
  unsigned int ttl; /* time to live in seconds */
  unsigned short rdlength;
} __attribute__((packed));
typedef struct RRTail_tag RRTail;

unsigned short
pack_name (unsigned char *getbuf, unsigned short goft,
	   char *name)
{
  unsigned char *gpos = getbuf + goft;
  unsigned short new_offset = 0;
  unsigned short old_dpos = 0;
  unsigned short new_dpos = 0;
  /* TODO: Enforce limit on getbuf max offset.  */
  /* Scan to the period or null character.  */
  while (name[old_dpos] != '\0') {
    unsigned char seglen;
    while (name[new_dpos] != '.' && name[new_dpos] != '\0')
      new_dpos++;
    seglen = new_dpos - old_dpos;
    if (seglen >= 0xc0)
      ; /* TODO ERROR */
    *gpos++ = seglen;
    new_offset++;
    if (seglen == 0)
      break; /* Already wrote the terminal.  */
    memcpy (gpos, name + old_dpos, seglen);
    if (name[new_dpos] == '.')
      new_dpos++; /* Skip the period now that we've parsed it.  */
    old_dpos = new_dpos;
    gpos += seglen;
    new_offset += seglen;
    if (name[new_dpos] == '\0' && seglen != 0) {
      *gpos++ = 0; /* Write the terminal.  */
      new_offset++;
    }
  }
  return new_offset;
}

unsigned short
parse_name (unsigned char *getbuf, unsigned short goft,
	    char *dest, unsigned short maxlen)
{
  unsigned char *gpos = getbuf + goft;
  unsigned short new_offset = 0;
  unsigned short dpos = 0;
  while (*gpos != '\0') {
    unsigned char seglen = *gpos;
    if ((seglen & 0xc0) == 0xc0) {
      /* Read the compressed segment recursively.  */
      unsigned short coft = *(unsigned short *) gpos;
      coft = ntohs (coft) & ~0xc000;
      parse_name (getbuf, coft, dest + dpos, maxlen - dpos);
      new_offset += 2;
      return new_offset;
    }
    /* else */
    /* TODO: Do not let these offsets overflow.  */
    gpos++;
    new_offset++;
    if (dpos + seglen + 1 < maxlen) {
      memcpy (dest + dpos, gpos, seglen);
      dpos += seglen;
      dest[dpos++] = '.';
    } else
      dpos += seglen + 1;
    gpos += seglen;
    new_offset += seglen;
  }
  gpos++;
  new_offset++;
  if (dpos > 0)
    dest[dpos-1] = '\0';
  else
    dest[0] = '\0';
  return new_offset;
}

unsigned short
concat_memcpy (void *dest, void *src, unsigned short size)
{
  memcpy (dest, src, size);
  return size;
}

struct DumDNSCtx_tag
{
  int sockfd;
  unsigned char flags;
};
typedef struct DumDNSCtx_tag DumDNSCtx;

/* Create our socket connection to the DNS server.  */
int
dumdns_open (DumDNSCtx *ctx, unsigned char *dns_addr,
	     int family, unsigned char flags)
{
  union {
    struct sockaddr_in in_sock;
    struct sockaddr_in6 in6_sock;
  } u;
  struct addrinfo hints;

  if (ctx == NULL)
    return -1;
  ctx->flags = RES_INIT | RES_RECURSE | flags;

  if (family == AF_INET6) {
    u.in6_sock.sin6_family = family;
    memcpy (&u.in6_sock.sin6_addr, dns_addr, sizeof (u.in6_sock.sin6_addr));
    u.in6_sock.sin6_port = htons (53); /* port 53 = DNS */
  } else { /* AF_INET */
    u.in_sock.sin_family = family;
    memcpy (&u.in_sock.sin_addr, dns_addr, sizeof (u.in_sock.sin_addr));
    u.in_sock.sin_port = htons (53); /* port 53 = DNS */
  }

  memset (&hints, 0, sizeof (hints));
  hints.ai_family = family;
  if ((ctx->flags & RES_USEVC) == RES_USEVC)
    hints.ai_socktype = SOCK_STREAM;
  else
    hints.ai_socktype = SOCK_DGRAM;
  hints.ai_addr = (struct sockaddr *)&u.in_sock;
  if (family == AF_INET6)
    hints.ai_addrlen = sizeof (struct sockaddr_in6);
  else /* AF_INET */
    hints.ai_addrlen = sizeof (struct sockaddr_in);

  if ((ctx->sockfd = socket (hints.ai_family, hints.ai_socktype,
			     hints.ai_protocol)) == -1) {
    perror ("client: socket");
    return -1;
  }

  if ((ctx->flags & RES_USEVC) != RES_USEVC) {
    /* We use a default UDP timeout-retransmit loop of 2 seconds.  */
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    if (setsockopt (ctx->sockfd, SOL_SOCKET, SO_RCVTIMEO,
		    (char *) &timeout, sizeof (timeout)) == -1) {
      close (ctx->sockfd);
      perror ("clinet: setsockopt");
    }
  }

  if (connect (ctx->sockfd, hints.ai_addr, hints.ai_addrlen) == -1) {
    close (ctx->sockfd);
    perror ("client: connect");
    return -1;
  }

  if ((ctx->flags & RES_DEBUG) == RES_DEBUG) {
    if (family == AF_INET6) {
      unsigned char *resp_data = (unsigned char *) &u.in6_sock.sin6_addr;
      fprintf (stderr, "client: connecting to "
	       "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
	       "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
	       resp_data[0], resp_data[1], resp_data[2], resp_data[3],
	       resp_data[4], resp_data[5], resp_data[6], resp_data[7],
	       resp_data[8], resp_data[9], resp_data[10], resp_data[11],
	       resp_data[12], resp_data[13], resp_data[14],
	       resp_data[15]);
    } else { /* AF_INET */
      fprintf (stderr, "client: connecting to %d.%d.%d.%d\n",
	       ((unsigned char *) &u.in_sock.sin_addr)[0],
	       ((unsigned char *) &u.in_sock.sin_addr)[1],
	       ((unsigned char *) &u.in_sock.sin_addr)[2],
	       ((unsigned char *) &u.in_sock.sin_addr)[3]);
    }
  }

  return 0;
}

int
dumdns_close (DumDNSCtx *ctx)
{
  if (ctx == NULL)
    return -1;
  return close (ctx->sockfd);
}

int
dumdns_getaddr (DumDNSCtx *ctx, char *name, int family,
		struct sockaddr *result)
{
  /* TODO: Make sure all buffer parsing limits are enforced.  */
  /* TODO: Check and handle response error conditions.  */
  unsigned char req_name[256];
  unsigned char getbuf[512];
  unsigned short getbuf_pos = 0;
  unsigned short getbuf_len = 0;

  DNSHead req_head;
  QDTail req_tail;
  RRTail resp_tail;
  unsigned char resp_name[256] = "NIL";
  unsigned char resp_data[256] =
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

  unsigned short i;

  strncpy (req_name, name, 255);
  req_name[255] = '\0';

  /* Build request.  */

  /* TODO: Determine a unique ID to use here.  */
  req_head.id = htons (0xcccc);
  req_head.qr = 0;
  req_head.opcode = OPCODE_QUERY;
  req_head.aa = 0;
  req_head.tc = 0;
  /* NOTE: If Recursion Desired is zero, then a query that cannot be
     immediately satisfied will instead return a list of nameserver
     resource records for which nameservers should be contacted next
     for the query.  */
  req_head.rd = ((ctx->flags & RES_RECURSE) == RES_RECURSE) ? 1 : 0;
  req_head.ra = 0;
  req_head.z = 0;
  req_head.rcode = 0;
  req_head.qdcount = htons (1);
  req_head.ancount = htons (0);
  req_head.nscount = htons (0);
  req_head.arcount = htons (0);

  if (family == AF_INET6)
    req_tail.qtype = htons (TYPE_AAAA);
  else /* AF_INET */
    req_tail.qtype = htons (TYPE_A);
  req_tail.qclass = htons (CLASS_IN);

  if ((ctx->flags & RES_USEVC) == RES_USEVC)
    getbuf_len += 2; /* Reserve space for message size header.  */
  getbuf_len +=
    concat_memcpy (getbuf + getbuf_len, &req_head, sizeof (req_head));
  getbuf_len += pack_name (getbuf, getbuf_len, req_name);
  getbuf_len +=
    concat_memcpy (getbuf + getbuf_len, &req_tail, sizeof (req_tail));
  if ((ctx->flags & RES_USEVC) == RES_USEVC)
    *(unsigned short *) getbuf = htons (getbuf_len - 2);

  /* Send/receive.  */

  /* TODO: Use `recvall ()', unless we are on UDP.  */
  if ((ctx->flags & RES_USEVC) == RES_USEVC) {
    /* fwrite (getbuf, getbuf_len, 1, stdout); */
    /* TODO: Use `sendall ()' */
    send (ctx->sockfd, getbuf, getbuf_len, 0);
    /* getbuf_len = fread (getbuf, 1, 512, stdin); */
    getbuf_len = recv (ctx->sockfd, getbuf, 512, 0);
  } else {
    unsigned short recvbuf_len;
    recvbuf_len = (unsigned short)-1;
    for (i = 0; i < 3 && recvbuf_len == (unsigned short)-1; i++) {
      /* TODO: Use `sendall ()' */
      send (ctx->sockfd, getbuf, getbuf_len, 0);
      recvbuf_len = recv (ctx->sockfd, getbuf, 512, 0);
      if (recvbuf_len == (unsigned short)-1 && errno != EWOULDBLOCK) {
	perror ("client: recv");
	break; /* TODO ERROR */
      }
    }
    if (recvbuf_len == (unsigned short)-1)
      perror ("client: final recv"); /* TODO ERROR */
    getbuf_len = recvbuf_len;
  }

  /* Parse response.  */

  req_name[0] = 'N';
  req_name[1] = 'I';
  req_name[2] = 'L';
  req_name[3] = '\0';

  if ((ctx->flags & RES_USEVC) == RES_USEVC)
    getbuf_pos += 2; /* Skip the size header.  */
  /* Read the header.  */
  getbuf_pos +=
    concat_memcpy (&req_head, getbuf + getbuf_pos, sizeof (req_head));
  req_head.id = ntohs (req_head.id);
  req_head.qdcount = ntohs (req_head.qdcount);
  req_head.ancount = ntohs (req_head.ancount);
  req_head.nscount = ntohs (req_head.nscount);
  req_head.arcount = ntohs (req_head.arcount);

  fprintf (stderr, "%d questions, %d answers\n", req_head.qdcount, req_head.ancount);

  /* Read all questions, ignore all but the last for our purposes.  */
  for (i = 0; i < req_head.qdcount; i++) {
    if ((ctx->flags & RES_USEVC) == RES_USEVC)
      getbuf_pos +=
	parse_name (getbuf + 2, getbuf_pos - 2, req_name, 256);
    else
      getbuf_pos +=
	parse_name (getbuf, getbuf_pos, req_name, 256);
    getbuf_pos +=
      concat_memcpy (&req_tail, getbuf + getbuf_pos, sizeof (req_tail));
    req_tail.qtype = ntohs (req_tail.qtype);
    req_tail.qclass = ntohs (req_tail.qclass);
  }

  /* Read all answers, ignore all but the last for our purposes.  */
  for (i = 0; i < req_head.ancount; i++) {
    if ((ctx->flags & RES_USEVC) == RES_USEVC)
      getbuf_pos +=
	parse_name (getbuf + 2, getbuf_pos - 2, resp_name, 256);
    else
      getbuf_pos +=
	parse_name (getbuf, getbuf_pos, resp_name, 256);
    getbuf_pos +=
      concat_memcpy (&resp_tail, getbuf + getbuf_pos, sizeof (resp_tail));
    resp_tail.type = ntohs (resp_tail.type);
    resp_tail.class = ntohs (resp_tail.class);
    resp_tail.ttl = ntohl (resp_tail.ttl);
    resp_tail.rdlength = ntohs (resp_tail.rdlength);
    getbuf_pos +=
      concat_memcpy (resp_data, getbuf + getbuf_pos, resp_tail.rdlength);
  }

  /* Ignore the rest of the resource records for our purposes.  */

  /* Send the resolved address to the user.  */
  if (family == AF_INET6)
    memcpy (result, resp_data, sizeof (struct sockaddr_in6));
  else /* AF_INET */
    memcpy (result, resp_data, sizeof (struct sockaddr_in));

  if ((ctx->flags & RES_DEBUG) == RES_DEBUG) {
    if (family == AF_INET6) {
      fprintf (stderr, "Q = %s, R = %s, "
	       "I = %02x%02x:%02x%02x:%02x%02x:%02x%02x:"
	       "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
	       req_name, resp_name, resp_data[0], resp_data[1],
	       resp_data[2], resp_data[3], resp_data[4],
	       resp_data[5], resp_data[6], resp_data[7], resp_data[8],
	       resp_data[9], resp_data[10], resp_data[11],
	       resp_data[12], resp_data[13], resp_data[14],
	       resp_data[15]);
    } else { /* AF_INET */
      fprintf (stderr, "Q = %s, R = %s, I = %d.%d.%d.%d\n",
	       req_name, resp_name, resp_data[0], resp_data[1],
	       resp_data[2], resp_data[3]);
    }

    { /* Save the response data to a file for analysis.  */
      FILE *fp = fopen ("gview.txt", "wb");
      if (fp != NULL) {
	fwrite (getbuf, getbuf_len, 1, fp);
	fclose (fp);
      }
    }
  }

  return 0;
}

int
main (int argc, char *argv[])
{
  DumDNSCtx ctx;
  unsigned char dns_addr[16] = { 192, 168, 1, 1 };
    /* { 0xfd, 0xaf, 0x96, 0x92, 0x1e, 0x7e, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }; */
  unsigned char dns_af = AF_INET;
  unsigned char flags = RES_DEBUG;
  int family = AF_INET;
  char *req_name;
  struct sockaddr_in6 result;

  /* Parse command line arguments.  */
  if (argc != 3)
    return 1;
  if (argv[1][0] == '4')
    family = AF_INET;
  else if (argv[1][0] == '6')
    family = AF_INET6;
  req_name = argv[2];

  /* Create our socket connection to the DNS server.  */
  if (dumdns_open (&ctx, dns_addr, dns_af, flags) != 0)
    return 1;

  /* Resolve name.  */
  if (dumdns_getaddr (&ctx, req_name, family,
		      (struct sockaddr *) &result) != 0)
    return 1;

  if (family == AF_INET6) {
    unsigned char *resp_data = (unsigned char *) &result;
    fprintf (stderr, "Result: %02x%02x:%02x%02x:%02x%02x:%02x%02x:"
	     "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
	     resp_data[0], resp_data[1], resp_data[2], resp_data[3],
	     resp_data[4], resp_data[5], resp_data[6], resp_data[7],
	     resp_data[8], resp_data[9], resp_data[10], resp_data[11],
	     resp_data[12], resp_data[13], resp_data[14],
	     resp_data[15]);
  } else { /* AF_INET */
    fprintf (stderr, "Result: %d.%d.%d.%d\n",
	     ((unsigned char *) &result)[0],
	     ((unsigned char *) &result)[1],
	     ((unsigned char *) &result)[2],
	     ((unsigned char *) &result)[3]);
  }

  /* Close.  */
  if (dumdns_close (&ctx) != 0)
    return 1;

  return 0;
}
