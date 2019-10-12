/* These are simple macro definitions for the `ctype' header, defined
   only for ASCII systems.  */
/* See the COPYING file for license details.  */
#ifndef CTYPE_H
#define CTYPE_H

#define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || \
                    (c) == '\r' || (c) == '\f' || (c) == '\v')
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
#define islower(c) ((c) >= 'a' && (c) <= 'z')
#define isalpha(c) (isupper(c) || islower(c))
#define isalnum(c) (isdigit(c) || isalpha(c))
#define isxdigit(c) (isdigit(c) || ((c) >= 'A' && (c) <= 'F') || \
                     ((c) >= 'a' && (c) <= 'f'))
#define ispunct(c) (((c) >= '!' && (c) < '0') || \
                    ((c) > '9' && (c) < 'A') || \
                    ((c) > 'Z' && (c) < 'a') || \
                    ((c) >= '{' && (c) <= '~'))
#define isgraph(c) (isalnum(c) || ispunct(c))
#define isprint(c) ((c) == ' ' || isgraph(c))
#define iscntrl(c) (((c) >= '\x00' && (c) < ' ') || (c) == '\x7f')

#define toupper(c) (islower(c) ? (c - 'a' + 'A') : c)
#define tolower(c) (isupper(c) ? (c - 'A' + 'a') : c)

#endif
