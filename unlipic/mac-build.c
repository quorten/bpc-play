/* Read a macro-based hardware design source, output an ASCII gate
   array listing.  Actually, given the right definitions, this can
   also be a basic macro processor for assembly language too.

   If you want to include files, use the Unix `cat' command.  */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "bool.h"
#include "xmalloc.h"
#include "exparray.h"

/* N.B. These "MAX" definitions are one beyond the allowed values.
   (x < MAX).  */
/* Yes, it's true.  We don't allow users of this simple tool to be
   working with "over one million lines of code."  */
#define MAX_FILE_LINES ((unsigned short)65535)
#define MAX_IDENT_LEN ((unsigned char)32)
#define MAX_HEADER_FIELDS ((unsigned char)255)
#define MAX_ARGS ((unsigned char)255)
#define MAX_MACRO_INSTS ((unsigned char)65535)
#define MAX_MACROS ((unsigned short)65535)
#define MAX_TRUTH_TBL_LINES ((unsigned char)255)
#define MAX_WIRES ((unsigned long)-1)
/* Maximum unnamed wire value, greater than or equal to this and we
   treat this specially as a named argument.  */
#define MAX_UNNAMED (MAX_WIRES - MAX_ARGS)

/* Symbolically define the names of special directives, since we may
   want slight changes to the "flavor" of our macro processor.  */
#define DIR_GATE "Gate"
#define DIR_MACRO "Macro"
#define DIR_INPUTS "Inputs"
#define DIR_OUTPUTS "Outputs"
#define DIR_INTERMEDS "Intermediates"
#define DIR_TRUTH "Truth"
#define DIR_TRUTH_TBL "Truth table"
#define DIR_SEQUENTIAL "Sequential"
#define DIR_BEGIN "Begin"
#define DIR_END "End"

/* Parse mode flags */
#define MODE_MAC_DEFINE 1
#define MODE_MAC_HEADER 2

/* Macro header field flags */
#define GOT_TYPE 1
#define GOT_INPUTS 2
#define GOT_OUTPUTS 4
#define GOT_INTERMEDS 8
#define GOT_TRUTH_TBL 16
#define GOT_SEQUENTIAL 32

/* ArgIDs are ordered as follows: inputs, outputs, intermediates */
typedef unsigned char ArgID;
EA_TYPE(ArgID);
typedef char* char_ptr;
EA_TYPE(char_ptr);
typedef unsigned long ulong;
EA_TYPE(ulong);

struct MacroInst_tag
{
  unsigned short type_id; /* ID of gate or macro to use */
  ArgID_array args;
  unsigned short line_num; /* for reporting errors */
};
typedef struct MacroInst_tag MacroInst;
EA_TYPE(MacroInst);

enum MacroDefType_tag {
  MD_GATE,
  MD_MACRO
};
typedef enum MacroDefType_tag MacroDefType;

struct MacroDef_tag
{
  unsigned char type; /* MD_GATE or MD_MACRO */
  char_ptr name;
  char_ptr_array arg_names;
  ArgID_array ins;
  ArgID_array outs;
  ArgID_array mids; /* Intermediates */
  /* For now, we skip the truth table.  */
  MacroInst_array body;
};
typedef struct MacroDef_tag MacroDef;
EA_TYPE(MacroDef);

MacroDef_array macros;
unsigned long num_wires = 0;

/* TODO: Code refactor.  Use macros for translation computations
   between ArgID and Input ID, Output ID, Intermediate ID.  */

/* TODO: Fix all "return 1" code paths so that they cleanly deallocate
   all dynamic memory allocations.  */

#define CHECK_MAX_LINES() \
  if (line_num >= MAX_FILE_LINES) { \
    fprintf (stderr, \
	     "Error: Too many lines of code in file.\n"); \
    return 1; \
  }

#define SKIP_LEADING_WHITESPACE() \
  while ((c = getchar ()) == ' ' || c == '\t'); \
  if (c != EOF) \
    ungetc (c, stdin);

#define BEGIN_MAC_BODY() \
  parse_mode &= ~MODE_MAC_HEADER; \
  /* Check that all necessary fields have been defined.  */ \
  if (got_hdr_flags != need_hdr_flags) { \
    unsigned char miss_fields = need_hdr_flags & ~got_hdr_flags; \
    fprintf (stderr, \
	     "Syntax error on line %u: Missing header fields, expected", \
	     line_num); \
    if ((miss_fields & GOT_TYPE)) \
      fputs (" " DIR_GATE "/" DIR_MACRO ",", stderr); \
    if ((miss_fields & GOT_INPUTS)) \
      fputs (" " DIR_INPUTS ",", stderr); \
    if ((miss_fields & GOT_OUTPUTS)) \
      fputs (" " DIR_OUTPUTS ",", stderr); \
    if ((miss_fields & GOT_INTERMEDS)) \
      fputs (" " DIR_INTERMEDS ",", stderr); \
    if ((miss_fields & GOT_TRUTH_TBL)) \
      fputs (" " DIR_TRUTH_TBL ",", stderr); \
    if ((miss_fields & GOT_SEQUENTIAL)) \
      fputs (" " DIR_SEQUENTIAL ",", stderr); \
    putc ('\n', stderr); \
    return 1; \
  } \
  new_body = &new_macro->body; \
  EA_INIT(MacroInst, *new_body, 16);

#define END_MAC_BODY() \
  if (new_macro->type != MD_GATE && (parse_mode & MODE_MAC_HEADER)) { \
    fprintf (stderr, \
	     "Syntax error on line %u: " \
	     "Incomplete macro definition.\n", \
	     line_num); \
    return 1; \
  } \
  parse_mode &= ~MODE_MAC_DEFINE; \
  EA_ADD(macros); \
  if (macros.len >= MAX_MACROS) { \
    /* Subtract two from line number due to blank lines.  */ \
    fprintf (stderr, \
	     "Syntax error on line %u: " \
	     "Macro %s: Too many macro definitions.\n", \
	     line_num - 2, macros.d[macros.len-1].name); \
    return 1; \
  } \
  got_hdr_flags = 0;

/* Read a space-separated list.  Note that we also skip over comma
   separators.  By treating parentheses also as comma-style
   whitespace, we can transparently parse the function notation styles
   of Lisp, BASIC/C/C++/Java/Javascript/Python, Go, etc.  */
#define READ_SS_LIST(arg_names, array, init_args) \
  decl_len = 0; \
  while (decl_len < MAX_IDENT_LEN && (c = getchar ()) != EOF) { \
    if (isspace (c) || c == ',' || c == '(' || c == ')') { \
      decl_buf[decl_len] = '\0'; \
      /* Do not add zero-length arguments.  */ \
      if (decl_len == 0) { \
        if (c == '\n') { \
          /* End of list.  */ \
          line_num++; \
	  CHECK_MAX_LINES(); \
          break; \
        } \
        continue; \
      } \
      if (init_args) { \
	/* Verify that the name is not already used.  */ \
	ArgID i; \
	for (i = 0; i < (arg_names).len; i++) { \
	  if (strcmp (decl_buf, (arg_names).d[i]) == 0) { \
	    fprintf (stderr, \
		    "Syntax error on line %u: " \
		    "Duplicate argument definition: %s\n", \
		    line_num, decl_buf); \
	    return 1; \
	  } \
	} \
	(arg_names).d[(arg_names).len] = xstrdup (decl_buf); \
	(array).d[(array).len] = (ArgID)(arg_names).len; \
	EA_ADD(arg_names); \
	EA_ADD(array); \
	if ((arg_names).len >= MAX_ARGS) { \
	  fprintf (stderr, \
		   "Syntax error on line %u: " \
		   "Too many arguments: %s\n", \
		   line_num, decl_buf); \
	  return 1; \
	} \
      } else { \
	/* Search for the ArgID to use.  */ \
	ArgID i; \
	for (i = 0; i < (arg_names).len; i++) { \
	  if (strcmp (decl_buf, (arg_names).d[i]) == 0) \
	    break; \
	} \
	if (i >= (arg_names).len) { \
	  fprintf (stderr, \
		   "Syntax error on line %u: Undefined argument: %s\n", \
		   line_num, decl_buf); \
	  return 1; \
	} \
	(array).d[(array).len] = i; \
	EA_ADD(array); \
      } \
      decl_len = 0; \
      if (c == '\n') { \
        /* End of list.  */ \
        line_num++; \
	CHECK_MAX_LINES(); \
        break; \
      } \
    } else \
      decl_buf[decl_len++] = (char)c; \
  } \
  if (decl_len >= MAX_IDENT_LEN) { \
    fprintf (stderr, \
	     "Syntax error on line %u: Identifier too long.\n", \
	     line_num); \
    return 1; \
  }

#define ALLOC_WIRE() num_wires++

#define CHECK_MAX_WIRES() \
  if (num_wires >= MAX_UNNAMED) { \
    fprintf (stderr, \
	     "Macro expansion error: Too many wires.\n"); \
    return 1; \
  }

int expand_macro (unsigned short type_id,
		  ulong_array *inputs, ulong_array *outputs,
		  char_ptr_array *arg_names);

int
main (int argc, char *argv[])
{
  bool debug_structs = false;
  bool auto_main_mac = true;

  int retval;
  int c;
  unsigned short line_num = 1;
  char decl_buf[MAX_IDENT_LEN];
  unsigned char decl_len = 0;
  unsigned char parse_mode = 0;
  MacroDef *new_macro;
  unsigned char need_hdr_flags = 0;
  unsigned char got_hdr_flags = 0;
  MacroInst_array *new_body = NULL;
  EA_INIT(MacroDef, macros, 16);

  if (argc == 2 && strcmp (argv[1], "-d") == 0)
    debug_structs = true;

  /* TODO: Also support backslash line continuation.  */

  /* Read a list of directives.  */
  while ((c = getchar ()) != EOF) {
    unsigned char do_read_name = 0;

    /* Skip leading whitespace, if applicable.  */
    if (isspace (c) || c == '(' || c == ')') {
      if (c == '\n') {
	line_num++;
	CHECK_MAX_LINES();
      }
      continue;
    }
    ungetc (c, stdin);

    /* Read the directive name.  */
    decl_len = 0;
    while (decl_len < MAX_IDENT_LEN && (c = getchar ()) != EOF &&
	   !isspace (c) && c != ':' && c != '(' && c != ')')
      decl_buf[decl_len++] = (char)c;
    if (decl_len >= MAX_IDENT_LEN) {
      fprintf (stderr,
	       "Syntax error on line %u: Directive name too long.\n",
	       line_num);
      return 1;
    }
    if (c == '\n') {
      line_num++;
      CHECK_MAX_LINES();
    }
    if (c != EOF && isspace(c))
      ungetc (c, stdin);
    decl_buf[decl_len] = '\0';

    /* Check if this is a special directive.  */
    if (strcmp (decl_buf, DIR_GATE) == 0) {
      /* Close existing macro definition if applicable.  */
      if ((parse_mode & MODE_MAC_DEFINE)) {
	END_MAC_BODY();
      }
      if ((got_hdr_flags & GOT_TYPE)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 "Duplicate type definition: " DIR_GATE ".\n",
		 line_num);
	return 1;
      }
      parse_mode = MODE_MAC_DEFINE | MODE_MAC_HEADER;
      new_macro = &macros.d[macros.len];
      new_macro->type = MD_GATE;
      EA_INIT(char_ptr, new_macro->arg_names, 16);
      EA_INIT(ArgID, new_macro->ins, 16);
      EA_INIT(ArgID, new_macro->outs, 16);
      new_macro->mids.d = NULL;
      new_macro->mids.len = 0;
      new_macro->body.d = NULL;
      new_macro->body.len = 0;
      /* We skip the truth table.  */
      need_hdr_flags = GOT_TYPE | GOT_INPUTS | GOT_OUTPUTS |
	GOT_INTERMEDS;
      got_hdr_flags = GOT_TYPE;

      do_read_name = 1;

    } else if (strcmp (decl_buf, DIR_MACRO) == 0) {
      /* Close existing macro definition if applicable.  */
      if ((parse_mode & MODE_MAC_DEFINE)) {
	END_MAC_BODY();
      }
      if ((got_hdr_flags & GOT_TYPE)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 "Duplicate type definition: " DIR_MACRO ".\n",
		 line_num);
	return 1;
      }
      parse_mode = MODE_MAC_DEFINE | MODE_MAC_HEADER;
      new_macro = &macros.d[macros.len];
      new_macro->type = MD_MACRO;
      EA_INIT(char_ptr, new_macro->arg_names, 16);
      EA_INIT(ArgID, new_macro->ins, 16);
      EA_INIT(ArgID, new_macro->outs, 16);
      EA_INIT(ArgID, new_macro->mids, 16);
      need_hdr_flags = GOT_TYPE | GOT_INPUTS | GOT_OUTPUTS |
	GOT_INTERMEDS | GOT_TRUTH_TBL;
      got_hdr_flags = GOT_TYPE;

      do_read_name = 1;

    } else if (strcmp (decl_buf, DIR_INPUTS) == 0) {
      char_ptr_array *arg_names = &new_macro->arg_names;
      ArgID_array *ins = &new_macro->ins;

      if (!(parse_mode & MODE_MAC_DEFINE) ||
	  !(parse_mode & MODE_MAC_HEADER)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_INPUTS " definition not allowed outside macro header.\n",
		 line_num);
	return 1;
      }
      if ((got_hdr_flags & GOT_INPUTS)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 "Duplicate " DIR_INPUTS " definition not allowed.\n",
		 line_num);
	return 1;
      }
      if ((got_hdr_flags & GOT_OUTPUTS)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_INPUTS " must be declared before " DIR_OUTPUTS ".\n",
		 line_num);
	return 1;
      }
      if ((got_hdr_flags & GOT_INTERMEDS)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_INPUTS " must be declared before " DIR_INTERMEDS ".\n",
		 line_num);
	return 1;
      }
      got_hdr_flags |= GOT_INPUTS;

      /* Skip leading space.  */
      SKIP_LEADING_WHITESPACE();
      /* Read a space-separated list.  */
      READ_SS_LIST(*arg_names, *ins, true);

    } else if (strcmp (decl_buf, DIR_OUTPUTS) == 0) {
      char_ptr_array *arg_names = &new_macro->arg_names;
      ArgID_array *outs = &new_macro->outs;

      if (!(parse_mode & MODE_MAC_DEFINE) ||
	  !(parse_mode & MODE_MAC_HEADER)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_OUTPUTS " definition not allowed outside macro header.\n",
		 line_num);
	return 1;
      }
      if (!(got_hdr_flags & GOT_INPUTS)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_INPUTS " must be declared before " DIR_OUTPUTS ".\n",
		 line_num);
	return 1;
      }
      if ((got_hdr_flags & GOT_OUTPUTS)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 "Duplicate " DIR_OUTPUTS " definition not allowed.\n",
		 line_num);
	return 1;
      }
      if ((got_hdr_flags & GOT_INTERMEDS)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_OUTPUTS " must be declared before " DIR_INTERMEDS ".\n",
		 line_num);
	return 1;
      }
      got_hdr_flags |= GOT_OUTPUTS;

      /* Skip leading space.  */
      SKIP_LEADING_WHITESPACE();
      /* Read a space-separated list.  */
      READ_SS_LIST(*arg_names, *outs, true);

    } else if (strcmp (decl_buf, DIR_INTERMEDS) == 0) {
      char_ptr_array *arg_names = &new_macro->arg_names;
      ArgID_array *mids = &new_macro->mids;

      if (!(parse_mode & MODE_MAC_DEFINE) ||
	  !(parse_mode & MODE_MAC_HEADER)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_INTERMEDS " definition not allowed "
		 "outside macro header.\n",
		 line_num);
	return 1;
      }
      if (!(got_hdr_flags & GOT_INPUTS)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_INPUTS " must be declared before " DIR_OUTPUTS " and "
		 DIR_INTERMEDS ".\n",
		 line_num);
	return 1;
      }
      if (!(got_hdr_flags & GOT_OUTPUTS)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_OUTPUTS " must be declared before " DIR_INTERMEDS ".\n",
		 line_num);
	return 1;
      }
      if ((got_hdr_flags & GOT_INTERMEDS)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 "Duplicate " DIR_INTERMEDS " definition not allowed.\n",
		 line_num);
	return 1;
      }
      got_hdr_flags |= GOT_INTERMEDS;

      /* Skip leading space.  */
      SKIP_LEADING_WHITESPACE();
      /* Read a space-separated list.  */
      READ_SS_LIST(*arg_names, *mids, true);

    } else if (strcmp (decl_buf, DIR_TRUTH) == 0) { /* "Truth table" */
      /* We simply compute the number of lines in the truth table
	 based off the number of inputs and skip them all, one header
	 line plus 2^n truth table lines.  */
      unsigned char num_tbl_lines =
	1 + (1 << new_macro->ins.len);

      if (!(parse_mode & MODE_MAC_DEFINE) ||
	  !(parse_mode & MODE_MAC_HEADER)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_TRUTH_TBL " definition not allowed "
		 "outside macro header.\n",
		 line_num);
	return 1;
      }
      if ((got_hdr_flags & GOT_TRUTH_TBL)) {
	fprintf (stderr,
		 "Syntax error on line %u: Duplicate truth table.\n",
		 line_num);
	return 1;
      }
      got_hdr_flags |= GOT_TRUTH_TBL;

      if (new_macro->ins.len >= 8) {
	fprintf (stderr,
		 "Syntax error on line %u: Too many truth table lines.\n",
		 line_num);
	return 1;
      }
      if (new_macro->ins.len == 0) {
	fprintf (stderr,
		 "Syntax error on line %u: " DIR_TRUTH_TBL " must come after "
		 "specification.\n",
		 line_num);
	return 1;
      }
      /* Skip to the end of the current line.  */
      while ((c = getchar ()) != EOF && c != '\n');
      if (c == EOF) {
	fprintf (stderr,
		 "Syntax error on line %u: Missing truth table.\n",
		 line_num);
	return 1;
      }
      line_num++;
      CHECK_MAX_LINES();
      while (num_tbl_lines > 0) {
	c = getchar ();
	if (c == '\n') {
	  line_num++;
	  CHECK_MAX_LINES();
	  num_tbl_lines--;
	} else if (c == EOF)
	  break;
      }
      if (num_tbl_lines != 0) {
	fprintf (stderr,
		 "Syntax error on line %u: " DIR_TRUTH_TBL " too short.\n",
		 line_num);
	return 1;
      }

    } else if (strcmp (decl_buf, DIR_SEQUENTIAL) == 0) {
      /* Place holder to indicate that sequential logic does not
	 have a truth table.  */
      if (!(parse_mode & MODE_MAC_DEFINE) ||
	  !(parse_mode & MODE_MAC_HEADER)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_SEQUENTIAL " definition not allowed "
		 "outside macro header.\n",
		 line_num);
	return 1;
      }
      if ((got_hdr_flags & GOT_SEQUENTIAL)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 "Duplicate " DIR_SEQUENTIAL " definition.\n",
		 line_num);
	return 1;
      }
      got_hdr_flags |= GOT_SEQUENTIAL;
      /* We don't need a truth table for sequential logic.  */
      need_hdr_flags &= ~GOT_TRUTH_TBL;
      need_hdr_flags |= GOT_SEQUENTIAL;

      /* Skip to end of line.  */
      decl_len = 0;
      while ((c = getchar ()) != EOF && c != '\n')
	decl_buf[decl_len++] = (char)c;
      if (c == '\n') {
	line_num++;
	CHECK_MAX_LINES();
      }

    } else if (strcmp (decl_buf, DIR_BEGIN) == 0) {
      if (!(parse_mode & MODE_MAC_DEFINE)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_BEGIN " directive not allowed "
		 "outside macro definition.\n",
		 line_num);
	return 1;
      }
      if (!(parse_mode & MODE_MAC_HEADER)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 "Duplicate " DIR_BEGIN " directive not allowed "
		 "in macro body.\n",
		 line_num);
	return 1;
      }
      BEGIN_MAC_BODY();

    } else if (strcmp (decl_buf, DIR_END) == 0) {
      if (!(parse_mode & MODE_MAC_DEFINE)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_END " directive not allowed "
		 "outside macro definition.\n",
		 line_num);
	return 1;
      }
      if ((parse_mode & MODE_MAC_HEADER)) {
	fprintf (stderr,
		 "Syntax error on line %u: "
		 DIR_END " directive not allowed "
		 "in macro header.\n",
		 line_num);
	return 1;
      }
      END_MAC_BODY();

    } else {
      /* Exit macro header mode upon definition of a body macro.  */
      unsigned short new_type_id;
      if ((parse_mode & MODE_MAC_HEADER)) {
	BEGIN_MAC_BODY();
      }

      { /* Search for the macro ID.  */
	unsigned short i;
	new_type_id = (unsigned short)-1;
	for (i = 0; i < macros.len; i++) {
	  if (strcmp (decl_buf, macros.d[i].name) == 0)
	    { new_type_id = i; break; }
	}
	if (new_type_id == (unsigned short)-1) {
	  fprintf (stderr,
		   "Syntax error on line %u: Unrecognized macro name: %s\n",
		   line_num, decl_buf);
	  return 1;
	}
      }

      /* Skip leading space.  */
      SKIP_LEADING_WHITESPACE();

      if ((parse_mode & MODE_MAC_DEFINE)) {
	/* Extend macro body.  */

	/* Now read all macro args until end of line.  */
	MacroInst *new_inst = &new_body->d[new_body->len];
	new_inst->type_id = new_type_id;
	new_inst->line_num = line_num;
	EA_INIT(ArgID, new_inst->args, 16);
	READ_SS_LIST(new_macro->arg_names, new_inst->args, false);

	{ /* Verify that the number of supplied arguments is
	     correct.  */
	  unsigned short type_id = new_inst->type_id;
	  unsigned char req_args =
	    macros.d[type_id].ins.len + macros.d[type_id].outs.len;
	  if (new_inst->args.len != req_args) {
	    /* Point to the previous line number since we have already
	       advanced one line.  */
	    fprintf (stderr,
		     "Syntax error on line %u: "
		     "Incorrect number of macro arguments: "
		     "%u given, %u needed\n",
		     line_num - 1, new_inst->args.len, req_args);
	    return 1;
	  }
	}

	EA_ADD(*new_body);
	if (new_body->len >= MAX_MACRO_INSTS) {
	  /* Point to the previous line number since we have already
	     advanced one line.  */
	  fprintf (stderr,
		   "Syntax error on line %u: "
		   "Macro %s: Too many macro instances in body.\n",
		   line_num - 1, new_macro->name);
	  return 1;
	}
      } else {
	/* Expand macro, possibly with string arguments.  */
	unsigned char ins_len = macros.d[new_type_id].ins.len;
	unsigned char outs_len = macros.d[new_type_id].outs.len;
	unsigned char args_len = ins_len + outs_len;
	unsigned char i;
	char_ptr_array arg_names;
	ArgID_array args;
	ulong_array inputs;
	ulong_array outputs;
	EA_INIT(char_ptr, arg_names, args_len + 1);
	EA_INIT(ArgID, args, args_len + 1);
	EA_INIT(ulong, inputs, ins_len + 1);
	EA_INIT(ulong, outputs, outs_len + 1);
	READ_SS_LIST(arg_names, args, true);
	if (args.len != args_len) {
	  fprintf (stderr,
		   "Syntax error on line %u: "
		   "Incorrect number of macro arguments: "
		   "%u given, %u needed\n",
		   line_num, args.len, args_len);
	  return 1;
	}
	/* TODO: If we were given all numeric arguments, pass those
	   instead, and reserve the wires.  Otherwise, just pass
	   through the string arguments.  */
	EA_SET_SIZE(inputs, ins_len);
	EA_SET_SIZE(outputs, outs_len);
	for (i = 0; i < ins_len; i++)
	  inputs.d[i] = MAX_UNNAMED + args.d[i];
	for (i = 0; i < outs_len; i++)
	  outputs.d[i] = MAX_UNNAMED + args.d[ins_len+i];
	expand_macro (new_type_id, &inputs, &outputs, &arg_names);
	for (i = 0; i < arg_names.len; i++)
	  xfree (arg_names.d[i]);
	EA_DESTROY(arg_names);
	EA_DESTROY(args);
	EA_DESTROY(inputs);
	EA_DESTROY(outputs);
      }
    }

    if (do_read_name) {
      /* Now read the name of the gate or macro.  */
      /* Skip leading whitespace, if applicable.  */
      SKIP_LEADING_WHITESPACE();
      decl_len = 0;
      /* TODO: Only read a single word, skip over functional notation
	 characters, additional arguments specify macro inputs.  */
      while (decl_len < MAX_IDENT_LEN && (c = getchar ()) != EOF &&
	     c != '\n')
	decl_buf[decl_len++] = (char)c;
      if (decl_len >= MAX_IDENT_LEN) {
	fprintf (stderr,
		 "Syntax error on line %u: Identifier too long.\n",
		 line_num);
	return 1;
      }
      line_num++;
      CHECK_MAX_LINES();
      decl_buf[decl_len] = '\0';

      { /* Verify that the name is not already used.  */
	unsigned short i;
	for (i = 0; i < macros.len; i++) {
	  if (strcmp (decl_buf, macros.d[i].name) == 0) {
	    fprintf (stderr,
		     "Syntax error on line %u: "
		     "Duplicate macro name definition: %s\n",
		     line_num, decl_buf);
	    return 1;
	  }
	}
      }
      new_macro->name = xstrdup (decl_buf);
    }
  }
  /* If at EOF and in the middle of defining a macro body, finish
     up.  */
  if ((parse_mode & MODE_MAC_DEFINE)) {
    END_MAC_BODY();
  }

  /* DEBUG: Now dump out everything that we've read in.  */
  if (debug_structs) {
    unsigned short i;
    for (i = 0; i < macros.len; i++) {
      unsigned short j;
      fprintf (stderr, "  %s: %s\n",
	       (macros.d[i].type) ? DIR_MACRO : DIR_GATE,
	       macros.d[i].name);
      fprintf (stderr, "  " DIR_INPUTS ":");
      for (j = 0; j < macros.d[i].ins.len; j++) {
	ArgID arg_id = macros.d[i].ins.d[j];
	char *arg_name = macros.d[i].arg_names.d[arg_id];
	fprintf (stderr, " %s", arg_name);
      }
      fprintf (stderr, "\n");
      fprintf (stderr, "  " DIR_OUTPUTS ":");
      for (j = 0; j < macros.d[i].outs.len; j++) {
	ArgID arg_id = macros.d[i].outs.d[j];
	char *arg_name = macros.d[i].arg_names.d[arg_id];
	fprintf (stderr, " %s", arg_name);
      }
      fprintf (stderr, "\n");
      fprintf (stderr, "  " DIR_INTERMEDS ":");
      for (j = 0; j < macros.d[i].mids.len; j++) {
	ArgID arg_id = macros.d[i].mids.d[j];
	char *arg_name = macros.d[i].arg_names.d[arg_id];
	fprintf (stderr, " %s", arg_name);
      }
      fprintf (stderr, "\n");
      for (j = 0; j < macros.d[i].body.len; j++) {
	unsigned short type_id = macros.d[i].body.d[j].type_id;
	unsigned char k;
	fprintf (stderr, "%s", macros.d[type_id].name);
	for (k = 0; k < macros.d[i].body.d[j].args.len; k++) {
	  ArgID arg_id = macros.d[i].body.d[j].args.d[k];
	  char *arg_name = macros.d[i].arg_names.d[arg_id];
	  fprintf (stderr, " %s", arg_name);
	}
	fprintf (stderr, "\n");
      }

      fprintf (stderr, "\n");
    }
  }

  if (auto_main_mac)
  {
    /* Automatically treat the last macro as the "main" macro and
       expand it.  */
    ulong_array inputs;
    ulong_array outputs;
    EA_INIT(ulong, inputs, 16);
    EA_INIT(ulong, outputs, 16);
    { /* Allocate arbitrary wires for all inputs.  */
      unsigned char i;
      MacroDef *main_macro = &macros.d[macros.len-1];
      unsigned char main_macro_ins_len = main_macro->ins.len;
      unsigned char main_macro_outs_len = main_macro->outs.len;
      fprintf (stderr, "info: Expanding main macro %s\n" ,main_macro->name);
      for (i = 0; i < main_macro_ins_len; i++) {
	EA_APPEND(inputs, ALLOC_WIRE());
	CHECK_MAX_WIRES();
      }
      /* Pre-set all output wires to undefined.  */
      EA_SET_SIZE(outputs, main_macro_outs_len);
      for (i = 0; i < main_macro_outs_len; i++)
	outputs.d[i] = (unsigned long)-1;
    }
    retval = expand_macro (macros.len - 1, &inputs, &outputs, NULL);
    EA_DESTROY(inputs);
    EA_DESTROY(outputs);
  }

 cleanup:
  /* Cleanup.  */
  {
    unsigned short i;
    for (i = 0; i < macros.len; i++) {
      unsigned short j;
      xfree (macros.d[i].name);
      for (j = 0; j < macros.d[i].arg_names.len; j++)
	xfree (macros.d[i].arg_names.d[j]);
      EA_DESTROY(macros.d[i].arg_names);
      EA_DESTROY(macros.d[i].ins);
      EA_DESTROY(macros.d[i].outs);
      EA_DESTROY(macros.d[i].mids);
      for (j = 0; j < macros.d[i].body.len; j++)
	EA_DESTROY(macros.d[i].body.d[j].args);
      EA_DESTROY(macros.d[i].body);
    }
    EA_DESTROY(macros);
  }

  return retval;
}

int
expand_macro (unsigned short type_id,
	      ulong_array *inputs, ulong_array *outputs,
	      char_ptr_array *arg_names)
{
  int retval = 0;
  MacroDef *macro = &macros.d[type_id];
  unsigned char macro_ins_len = macro->ins.len;
  unsigned char macro_outs_len = macro->outs.len;
  ulong *inputs_d = inputs->d;
  ulong *outputs_d = outputs->d;
  if (macro->type == MD_GATE) {
    unsigned char i;
    /* Special case on output wires, mainly to support the crisscross
       structure of SR flip-flops.  An output wire may also be an
       intermediate wire at the same time, in which case it may have
       been pre-allocated.  */
    for (i = 0; i < macro_outs_len; i++) {
      if (outputs_d[i] == (unsigned long)-1)
	outputs_d[i] = ALLOC_WIRE();
	CHECK_MAX_WIRES();
    }
    /* Now that we know both the input and output wires for an
       individual gate, we can output the gate.  */
    fputs (macro->name, stdout);
    for (i = 0; i < macro_ins_len; i++) {
      if (arg_names == NULL || inputs_d[i] < MAX_UNNAMED)
	printf (" %lu", inputs_d[i]);
      else
	printf (" %s", arg_names->d[inputs_d[i]-MAX_UNNAMED]);
    }
    for (i = 0; i < macro_outs_len; i++) {
      if (arg_names == NULL || outputs_d[i] < MAX_UNNAMED)
	printf (" %lu", outputs_d[i]);
      else
	printf (" %s", arg_names->d[outputs_d[i]-MAX_UNNAMED]);
    }
    putchar ('\n');
  } else { /* (macro->type == MD_MACRO) */
    MacroInst_array *body = &macro->body;
    unsigned short body_len = macro->body.len;
    unsigned char macro_mids_len = macro->mids.len;
    ulong_array intermeds;
    unsigned short i;
    outputs_d = outputs->d;
    EA_INIT(ulong, intermeds, macro_mids_len + 1);
    EA_SET_SIZE(intermeds, macro_mids_len);
    for (i = 0; i < macro_mids_len; i++)
      intermeds.d[i] = (unsigned long)-1;
    /* Recursively expand the macro body.  */
    for (i = 0; retval == 0 && i < body_len; i++) {
      MacroInst *inst = &body->d[i];
      unsigned short inst_type_id = inst->type_id;
      ArgID *inst_args_d = inst->args.d;
      unsigned char inst_args_len = inst->args.len;
      unsigned char sub_ins_len = macros.d[inst_type_id].ins.len;
      unsigned char sub_outs_len = macros.d[inst_type_id].outs.len;
      ulong_array sub_inputs;
      ulong_array sub_outputs;
      unsigned char j;
      EA_INIT(ulong, sub_inputs, sub_ins_len + 1);
      EA_SET_SIZE(sub_inputs, sub_ins_len);
      EA_INIT(ulong, sub_outputs, sub_outs_len + 1);
      EA_SET_SIZE(sub_outputs, sub_outs_len);
      for (j = 0; j < sub_outs_len; j++)
	sub_outputs.d[j] = (unsigned long)-1;
      /* Fill in the inputs.  */
      for (j = 0; j < sub_ins_len; j++) {
	ArgID arg_id = inst_args_d[j];
	if (arg_id < macro_ins_len) {
	  /* It's an input.  */
	  sub_inputs.d[j] = inputs_d[arg_id];
	} else if (arg_id >= macro_ins_len + macro_outs_len) {
	  /* It's an intermediate, allocate a wire if necessary.  */
	  unsigned char intermed_id =
	    arg_id - (macro_ins_len + macro_outs_len);
	  if (intermeds.d[intermed_id] == (unsigned long)-1) {
	    intermeds.d[intermed_id] = ALLOC_WIRE();
	    CHECK_MAX_WIRES();
	  }
	  sub_inputs.d[j] = intermeds.d[intermed_id];
	} else {
	  /* It's an output being rerouted as an internal input... if
	     the output wire has not already been pre-assigned, we
	     follow a special case code path to pre-allocate it.  */
	  unsigned char output_id = arg_id - macro_ins_len;
	  unsigned long output_wire = outputs_d[output_id];
	  if (output_wire == (unsigned long)-1) {
	    outputs_d[output_id] = ALLOC_WIRE();
	    CHECK_MAX_WIRES();
	    output_wire = outputs_d[output_id];
	    /* fprintf (stderr, "Macro expansion error: %s: Line %u: "
		     "Attempting to route unassigned output to input.\n",
		     macro->name, inst->line_num);
	    retval = 1; goto cleanup; */
	  }
	  sub_inputs.d[j] = output_wire;
	}
      }
      /* Check for pre-allocated outputs, and fill them in if
	 applicable.  */
      for (j = sub_ins_len; j < inst_args_len; j++) {
	ArgID arg_id = inst_args_d[j];
	if (arg_id < macro_ins_len) {
	  /* It's an input, not applicable.  BUT, we need to provide a
	     special case for main macro inputs.  */
	  fprintf (stderr, "Macro expansion warning: %s: Line %u: "
		   "Sub-output should not pre-allocate with macro input.\n",
		   macro->name, inst->line_num);
	  /* retval = 1; goto cleanup; */
	  sub_outputs.d[j-sub_ins_len] = inputs_d[arg_id];
	} else if (arg_id >= macro_ins_len + macro_outs_len) {
	  /* It's an intermediate.  */
	  unsigned char intermed_id =
	    arg_id - (macro_ins_len + macro_outs_len);
	  unsigned long intermed_wire = intermeds.d[intermed_id];
	  if (intermed_wire != (unsigned long)-1) {
	    sub_outputs.d[j-sub_ins_len] = intermed_wire;
	  }
	} else {
	  /* It's an output.  */
	  unsigned char output_id = arg_id - macro_ins_len;
	  unsigned long output_wire = outputs_d[output_id];
	  if (output_wire != (unsigned long)-1)
	    sub_outputs.d[j-sub_ins_len] = output_wire;
	}
      }
      /* Expand the macro.  */
      if (expand_macro (inst_type_id, &sub_inputs, &sub_outputs,
			arg_names) != 0) {
	retval = 1; goto cleanup;
      }
      /* Copy the outputs to the master macro's outputs and
	 intermediates.  */
      for (j = sub_ins_len; j < inst_args_len; j++) {
	ArgID arg_id = inst_args_d[j];
	if (arg_id < macro_ins_len) {
	  /* It's an erroneous input BUT, we need to provide a special
	     case for main macro inputs..  */
	  fprintf (stderr, "Macro expansion warning: %s: Line %u: "
		   "Sub-output should not wire with macro input.\n",
		   macro->name, inst->line_num);
	  /* retval = 1; goto cleanup; */
	  /* Verify the pre-allocation is consistent.  */
	  if (inputs_d[arg_id] != sub_outputs.d[j-sub_ins_len]) {
	      fprintf (stderr, "Macro expansion error: %s: Line %u: "
		       "Attempting to reassign pre-allocated output wire.\n",
		       macro->name, inst->line_num);
	      retval = 1; goto cleanup;
	  }
	} else if (arg_id >= macro_ins_len + macro_outs_len) {
	  /* It's an intermediate, verify the wire is not already
	     assigned.  */
	  unsigned char intermed_id =
	    arg_id - (macro_ins_len + macro_outs_len);
	  if (intermeds.d[intermed_id] != (unsigned long)-1) {
	    /* If the wire was pre-allocated, verify that it is
	       consistent.  */
	    if (intermeds.d[intermed_id] != sub_outputs.d[j-sub_ins_len]) {
	      fprintf (stderr, "Macro expansion error: %s: Line %u: "
		       "Attempting to reassign intermediate wire.\n",
		       macro->name, inst->line_num);
	      retval = 1; goto cleanup;
	    }
	  }
	  intermeds.d[intermed_id] = sub_outputs.d[j-sub_ins_len];
	} else {
	  /* It's an output, check for pre-allocation consistency.  */
	  unsigned char output_id = arg_id - macro_ins_len;
	  if (outputs_d[output_id] != (unsigned long)-1 &&
	      outputs_d[output_id] != sub_outputs.d[j-sub_ins_len]) {
	    fprintf (stderr, "Macro expansion error: %s: Line %u: "
		     "Attempting to reassign pre-allocated output wire.\n",
		     macro->name, inst->line_num);
	    retval = 1; goto cleanup;
	  }
	  outputs_d[output_id] = sub_outputs.d[j-sub_ins_len];
	}
      }
    cleanup:
      EA_DESTROY(sub_inputs);
      EA_DESTROY(sub_outputs);
    }
    EA_DESTROY(intermeds);
  }
  return retval;
}
