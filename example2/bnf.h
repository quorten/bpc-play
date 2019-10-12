/*

N.B.: Now here's a good question.  True, this parser engine is very
simple, and there is virtue in that.  But how do you generate a valid
pattern table from a BNF grammar?  Here's how.

1. Note all terminal rules in your BNF grammar.  That is, the lowest
   level rules that match numbers and symbols directly.  There will
   need to be at least one parse rule in the pattern table for each of
   these rules.

   Please note the precedence of these rules/operators too.  Only the
   highest precedence rules can be evaluated immediately under all
   conditions.  All other rules can only be immediately evaluated on
   end-of-file conditions.  Also, higher-precedence rules should be
   coded earlier in the table so that they will be guaranteed to be
   matched before lower precedence rules.

2. Take note of the required look-ahead of your grammar.  All terminal
   rules that are not of highest precedence will need a variety of
   similar rules that include the valid numbers/symbols that may
   appear following the rule.  However, if a rule matches a literal
   number/symbol at the end (like a semicolon), then there need not be
   any look-ahead because, well, there's no other possibilities that
   could be matched with higher precedence!

3. What about invalid combinations and syntax errors?  Here is where
   the choice for creativity comes into play.  If you want fancy error
   handling, then you also code up rules for ALL number/symbol
   combinations possible with the required look-ahead.  All invalid
   combinations will have respective error handling coded up.

   Otherwise, if you are lazy, then you simply need not code up any
   matching rules for syntax errors.  On a syntax error, the parser
   will find that there are no matching rules and it will indicate a
   generic syntax error at the point of the match failure.

4. How do we handle higher-level rules?  Remember this simple rule on
   the operation of the parser.  We first match the lowest-level
   possible rules, then when we match one such rule, we throw away the
   original expression and replace it with a new symbol.  That is, we
   do a "reduction computation."

   So, if we need any special symbolic directives for higher level
   matching to function properly, such as a list of statements, after
   processing one statement, we push a STMT symbol onto the stack.
   This symbol can then be used in the matching of a STMTS rule, after
   which point we replace the STMT symbol with a STMTS symbol.  That
   is how we parse a list of statements in a BNF compliant manner.

   Another thing, about rules with an "empty" production.  This is
   rather difficult to handle with our simple-minded parser.  In
   practice, this means that the higher-level rule needs to be
   rewritten to assume that there may be nothing in place of the
   referenced lower-level rule.  When possible, I would recommend such
   rules be rewritten so that they can be expanded without requiring
   an empty expansion.

That's all there is to it!

N.B. Chomsky normal form simplification is a very similar idea.
Matter of fact, thinking it through, Chomsky normal form really does a
lot to simplfy the parsing. though it may create a few problems of its
own when it comes to executing the parsed statements.

Okay, with that being said, let's try to go and correct the last few
bugs in my parse rules for the simple calculator.  We start by
defining a formal grammar in BNF.

FILE: EOF | STMTS EOF
STMTS: STMT | STMTS STMT
STMT: EXPR '='

EXPR: TERM | ADD | SUB

ADD: EXPR '+' TERM
SUB: EXPR '-' TERM
MUL: TERM '*' FACTOR
DIV: TERM '/' FACTOR

TERM: FACTOR | MUL | DIV
FACTOR: NUM | PHP
PHP: '(' EXPR ')'

Our practical implementation clears the stack completely after reading
a full statement.  So, the first two rules need not be coded, thus
simplifying the implementation.  Also, we can simplify in light of the
fact that higher precedence operators are computed first and perform a
reduction computation, thereby reducing the number of numbers/symbols
that need to be matched by the higher-level rules.  So, let's rewrite
the rules.

STMT: NUM '=' -> CLEAR_STACK

ADD: NUM '+' NUM -> NUM
SUB: NUM '-' NUM -> NUM
MUL: NUM '*' NUM -> NUM
DIV: NUM '/' NUM -> NUM

PHP: '(' NUM ')' -> NUM

This grammar needs to look-ahead one symbol.  So, let's rewrite the
rules to handle valid syntax in light of this look-ahead.

STMT: NUM '=' -> CLEAR_STACK

ADD: NUM '+' NUM '=' -> NUM '='
ADD: NUM '+' NUM '+' -> NUM '+'
ADD: NUM '+' NUM '-' -> NUM '-'
ADD: NUM '+' NUM '*' -> IGNORE
ADD: NUM '+' NUM '/' -> IGNORE
ADD: NUM '+' NUM ')' -> NUM ')'
SUB: NUM '-' NUM '=' -> NUM '='
SUB: NUM '-' NUM '+' -> NUM '+'
SUB: NUM '-' NUM '-' -> NUM '-'
SUB: NUM '-' NUM '*' -> IGNORE
SUB: NUM '-' NUM '/' -> IGNORE
SUB: NUM '-' NUM ')' -> NUM ')'
MUL: NUM '*' NUM '=' -> NUM '='
MUL: NUM '*' NUM '+' -> NUM '+'
MUL: NUM '*' NUM '-' -> NUM '-'
MUL: NUM '*' NUM '*' -> NUM '*'
MUL: NUM '*' NUM '/' -> NUM '/'
MUL: NUM '*' NUM ')' -> NUM ')'
DIV: NUM '/' NUM '=' -> NUM '='
DIV: NUM '/' NUM '+' -> NUM '+'
DIV: NUM '/' NUM '-' -> NUM '-'
DIV: NUM '/' NUM '*' -> NUM '*'
DIV: NUM '/' NUM '/' -> NUM '/'
DIV: NUM '/' NUM ')' -> NUM ')'

PHP: '(' NUM ')' -> NUM

We're almost there.  Now we need to add valid match rules for partial
reads so that our parser does not error out.

STMT: NUM -> IGNORE

ADD: NUM '+' -> IGNORE
ADD: NUM '+' NUM -> IGNORE
SUB: NUM '-' -> IGNORE
SUB: NUM '-' NUM -> IGNORE
MUL: NUM '*' -> IGNORE
MUL: NUM '*' NUM -> IGNORE
DIV: NUM '/' -> IGNORE
DIV: NUM '/' NUM -> IGNORE

PHP: '(' -> IGNORE
PHP: '(' NUM -> IGNORE

Taking this all together, we can make some final simplifications.

STMT: NUM -> IGNORE
STMT: NUM '=' -> CLEAR_STACK

ADD: NUM '+' -> IGNORE
ADD: NUM '+' NUM '=' -> NUM '='
ADD: NUM '+' NUM '+' -> NUM '+'
ADD: NUM '+' NUM '-' -> NUM '-'
ADD: NUM '+' NUM ')' -> NUM ')'
SUB: NUM '-' -> IGNORE
SUB: NUM '-' NUM '=' -> NUM '='
SUB: NUM '-' NUM '+' -> NUM '+'
SUB: NUM '-' NUM '-' -> NUM '-'
SUB: NUM '-' NUM ')' -> NUM ')'
MUL: NUM '*' -> IGNORE
MUL: NUM '*' NUM '=' -> NUM '='
MUL: NUM '*' NUM '+' -> NUM '+'
MUL: NUM '*' NUM '-' -> NUM '-'
MUL: NUM '*' NUM '*' -> NUM '*'
MUL: NUM '*' NUM '/' -> NUM '/'
MUL: NUM '*' NUM ')' -> NUM ')'
DIV: NUM '/' -> IGNORE
DIV: NUM '/' NUM '=' -> NUM '='
DIV: NUM '/' NUM '+' -> NUM '+'
DIV: NUM '/' NUM '-' -> NUM '-'
DIV: NUM '/' NUM '*' -> NUM '*'
DIV: NUM '/' NUM '/' -> NUM '/'
DIV: NUM '/' NUM ')' -> NUM ')'

PHP: '(' -> IGNORE
PHP: '(' NUM ')' -> NUM

That's all there is to it!  All of these patterns need only check for
matches at the top of the stack, there is no need for patterns that
check the whole stack with this specification.

Also, the final step would be to organize these rules into a few
primary "subroutines" in order to reduce the matching overhead with
this the simple-minded parsing engine.

*/

/*

Verifications on my previous written code.  It's more complicated due
to error handling.


This grammar needs to look-ahead one symbol.  So, let's rewrite the
rules to handle valid syntax in light of this look-ahead.

X STMT: NUM '=' -> CLEAR_STACK

X ADD: NUM '+' NUM '=' -> NUM '='
X ADD: NUM '+' NUM '+' -> NUM '+'
X ADD: NUM '+' NUM '-' -> NUM '-'
? ADD: NUM '+' NUM '*' -> IGNORE
? ADD: NUM '+' NUM '/' -> IGNORE
X ADD: NUM '+' NUM ')' -> NUM ')'
X SUB: NUM '-' NUM '=' -> NUM '='
X SUB: NUM '-' NUM '+' -> NUM '+'
X SUB: NUM '-' NUM '-' -> NUM '-'
? SUB: NUM '-' NUM '*' -> IGNORE
? SUB: NUM '-' NUM '/' -> IGNORE
X SUB: NUM '-' NUM ')' -> NUM ')'
X MUL: NUM '*' NUM '=' -> NUM '='
? MUL: NUM '*' NUM '+' -> NUM '+'
? MUL: NUM '*' NUM '-' -> NUM '-'
? MUL: NUM '*' NUM '*' -> NUM '*'
? MUL: NUM '*' NUM '/' -> NUM '/'
? MUL: NUM '*' NUM ')' -> NUM ')'
X DIV: NUM '/' NUM '=' -> NUM '='
? DIV: NUM '/' NUM '+' -> NUM '+'
? DIV: NUM '/' NUM '-' -> NUM '-'
? DIV: NUM '/' NUM '*' -> NUM '*'
? DIV: NUM '/' NUM '/' -> NUM '/'
? DIV: NUM '/' NUM ')' -> NUM ')'

PHP: '(' NUM ')' -> NUM

We're almost there.  Now we need to add valid match rules for partial
reads so that our parser does not error out.

E STMT: NUM -> IGNORE

E ADD: NUM '+' -> IGNORE
X ADD: NUM '+' NUM -> IGNORE
E SUB: NUM '-' -> IGNORE
x SUB: NUM '-' NUM -> IGNORE
X MUL: NUM '*' -> IGNORE
? MUL: NUM '*' NUM -> IGNORE
X DIV: NUM '/' -> IGNORE
? DIV: NUM '/' NUM -> IGNORE

E PHP: '(' -> IGNORE
X PHP: '(' NUM -> IGNORE

So, what's the problem?  We try to get away wihtout look-ahead one
when we process multiply and divide operators, and it must work most
of time, but maybe apparently in there are the few errors that give me
the incorrect results.

Or does it?  Did I fix all the bugs in the pattern table by now?

 */

/*

Okay, let's build out the grammar rules for a more full C programming
language implementation.


FILE: EOF | STMTS EOF
STMTS: STMT | STMTS STMT
STMT: ';' | EXPRS ';'

EXPRS: EXPR | EXPRS ',' EXPR
EXPR: TERM | ADD | SUB

VASN HERE += -= *= /= %=
TERNOP: EXPR '?' EXPR ':' EXPR
LOR: EXPR "||" EXPR
LAND: EXPR "&&" EXPR
BOR: EXPR '|' EXPR
BXOR: EXPR '^' EXPR
BAND: EXPR '&' EXPR
LEQ: EXPR "==" EXPR
LNE: EXPR "!=" EXPR
LT: EXPR '<' EXPR
LTE: EXPR "<=" EXPR
GT: EXPR '>' EXPR
GTE: EXPR '>=' EXPR
SHL: EXPR '<<' EXPR
SHR: EXPR '>>' EXPR

== !=
< <= > >=
<< >>

ADD: EXPR '+' TERM
SUB: EXPR '-' TERM
MUL: TERM '*' FACTOR
DIV: TERM '/' FACTOR
MOD: TERM '%' FACTOR

! ~ ++ -- NEG(-) (type) PTR(*) ADDR(&) sizeof
NEST() [] -> .

LNOT: '!' EXPR
BNOT: '~' EXPR
NEG: '-' EXPR
INCPRE: "++" EXPR
INCPOST: EXPR "++"
DECPRE: "--" EXPR
DECPOST: EXPR "--"
EXPRPTR: '*' EXPR
EXPRADDR: '&' EXPR
SZOF: "sizeof" EXPR
ARIDX: EXPR '[' EXPR ']'
MEMBR: EXPR '.' NAME
PTRMEMBR: EXPR '->' NAME

TERM: FACTOR | MUL | DIV | MOD
FACTOR: ECAST | PHP
ECAST: NUM | NAME | NAMEPTR | FUNC
  | '(' TYPE ')' NUM | '(' TYPE ')' NAME | '(' TYPE ')' FUNC
PHP: '(' EXPR ')' | '(' TYPE ')' '(' EXPR ')'

// TODO: Support all the other C operators with correct
// operator precedence.

SIGNMOD: "unsigned" | "signed"
SZMOD: "short" | "long" | "long" "long"
BTYPE: "char" | "short" | "int" | "long" | "long" "long"
TYPE: BTYPE | SIGNMOD BTYPE | SZMOD "int" | SIGNMOD SZMOD "int"
  | "const" BTYPE | "const" SIGNMOD BTYPE | "const" SZMOD "int"
  | "const" SIGNMOD SZMOD "int"
NAMEPTR: '*' NAME
ANYNAME: NAME | NAMEPTR
VASN: ANYNAME '=' EXPR
VASNS: VASN | VASNS ',' VASN
VACCUM: ANYNAME "|=" EXPR
  | ANYNAME "^=" EXPR
  | ANYNAME "&=" EXPR
  | ANYNAME "+=" EXPR
  | ANYNAME "-=" EXPR
  | ANYNAME "*=" EXPR
  | ANYNAME "/=" EXPR
  | ANYNAME "%=" EXPR
DVAR: TYPE VASNS ';'
DVARS: DVAR | DVARS DVAR
FUNC: NAME '(' EXPRS ')'
DECLARG: TYPE | TYPE NAME
DECLARGS: "void" | DECLARG | DECLARGS ',' DECLARG
DECLFUN: TYPE NAME '(' DECLARGS ')'
KNRDECLFUN: TYPE NAME '(' ')'
FNPTR: TYPE '(' '*' NAME ')' '(' DECLARGS ')'
SCOPE: '{' DVARS STMTS '}' | '{' STMTS '}'
DEFUN: DECLFUN SCOPE
KNRDEFUN: KNRDECLFUN DVARS SCOPE

RETSTMT: "return" EXPR ';'
STMTSCOPE: STMT | SCOPE
IFELSSTMT: "if" '(' EXPR ')' STMTSCOPE
  | "if '(' EXPR ')' STMTSCOPE "else" STMTSCOPE
  | "if '(' EXPR ')' STMTSCOPE "else" IFELSSTMT
WHILE: "while" '(' EXPR ')' STMTSCOPE
DOWHILE: "do" STMTSCOPE "while" '(' EXPR ')' ';'
FOR: "for" '(' EXPRS ';' EXPRS ';' EXPRS ')' STMTSCOPE
BRKSTMT: "break" ';'
CONTSTMT: "continue" ';'
SWSTMT: "switch" '(' EXPR ')' '{' CASES '}'
CASES: CASE | CASES CASE
CASE: "case" EXPR ':' STMTSCOPE
 | "default" ':' STMTSCOPE

Well, that's quite a bit of grammatical definitions to cover some of
the most commonly used C operators, which is basically all of them.
But, what can we say.  It's not as basic of a language as BASIC.
Please note that BASIC generally DOES support subroutines!  But not
local variables.

So, what features do we delete?

* Less integer types
* No pointers, no function pointers
    * Instead of pointers, PEEK and POKE functions are provided
    * Instead of function pointers, CALL is provided
* No bit-wise operators
* No structures or type defines
* No local variables, though the choice is up to you

Hierarchy of C symbol operator precedence:

EXPRS: Comma-separated list of expressions
EXPR: Single expression, in between commas
LVAL: L-value, things to the left of =, +=, -=, etc.
RVAL: R-value, things to the right of =, +=, -=, etc.
TERNEXPR: Things in between ternary operator expressions
LOREXPR: Things  in between logical OR expressions
LANDEXPR: Things in between logical AND expressions
BOREXPR: Things in between bit-wise OR expressions
BXOREXPR: Things in between bit-wise exclusive OR expressions
BANDEXPR: Things in between bit-wise AND expressions
EQEXPR: Things in between == or != expressions
LGEEXPR: Things in between <, <=, >, >= expressions
SHEXPR: Things in between << or >> expressions
TERM: Things in between + or - expressions
FACTOR: Things in between * / or % expressions
UNAEXPR: Things operated on by !, ~, ++, --, (type), VALUE(*) ADDR(&),
  or sizeof expressions
PARENEXPR: Last things inside NEST(), ARRAY[], or MEMBER expressions

----------------------------------------

Okay, let's rewrite the expressions with precedence rules proper.


LNOT: '!' EXPR
BNOT: '~' EXPR
NEG: '-' EXPR
INCPRE: "++" EXPR
INCPOST: EXPR "++"
DECPRE: "--" EXPR
DECPOST: EXPR "--"
EXPRPTR: '*' EXPR
EXPRADDR: '&' EXPR
SZOF: "sizeof" EXPR
ARIDX: EXPR '[' EXPR ']'
MEMBR: EXPR '.' NAME
PTRMEMBR: EXPR '->' NAME

TERM: FACTOR | MUL | DIV | MOD
FACTOR: ECAST | PHP
ECAST: NUM | NAME | NAMEPTR | FUNC
  | '(' TYPE ')' NUM | '(' TYPE ')' NAME | '(' TYPE ')' FUNC
PHP: '(' EXPR ')' | '(' TYPE ')' '(' EXPR ')'

// TODO: Support all the other C operators with correct
// operator precedence.

SIGNMOD: "unsigned" | "signed"
SZMOD: "short" | "long" | "long" "long"
BTYPE: "char" | "short" | "int" | "long" | "long" "long"
TYPE: BTYPE | SIGNMOD BTYPE | SZMOD "int" | SIGNMOD SZMOD "int"
  | "const" BTYPE | "const" SIGNMOD BTYPE | "const" SZMOD "int"
  | "const" SIGNMOD SZMOD "int"
NAMEPTR: '*' NAME
ANYNAME: NAME | NAMEPTR
VASN: ANYNAME '=' EXPR
VASNS: VASN | VASNS ',' VASN
LVAL: ANYNAME
XVASN: LVAL '=' EXPR
VACCUM: LVAL "|=" EXPR
  | LVAL "^=" EXPR
  | LVAL "&=" EXPR
  | LVAL "+=" EXPR
  | LVAL "-=" EXPR
  | LVAL "*=" EXPR
  | LVAL "/=" EXPR
  | LVAL "%=" EXPR
DVAR: TYPE VASNS ';'
DVARS: DVAR | DVARS DVAR
FUNC: NAME '(' EXPRS ')'
DECLARG: TYPE | TYPE NAME
DECLARGS: "void" | DECLARG | DECLARGS ',' DECLARG
DECLFUN: TYPE NAME '(' DECLARGS ')'
KNRDECLFUN: TYPE NAME '(' ')'
FNPTR: TYPE '(' '*' NAME ')' '(' DECLARGS ')'
SCOPE: '{' DVARS STMTS '}' | '{' STMTS '}'
DEFUN: DECLFUN SCOPE
KNRDEFUN: KNRDECLFUN DVARS SCOPE

TODO: FIX ECAST

EXPRS: EXPR | EXPRS ',' EXPR
EXPR: XVASN | VACCUM | RVAL
RVAL: TERNEXPR | TERNEXPR '?' TERNEXPR ':' RVAL
TERNEXPR: LOREXPR | TERNEXPR "||" LOREXPR
LOREXPR: LANDEXPR | LOREXPR "&&" LANDEXPR
LANDEXPR: BOREXPR | LANDEXPR '|' BOREXPR
BOREXPR: BXOREXPR | BOREXPR '^' BXOREXPR
BXOREXPR: BANDEXPR | BXOREXPR '&' BANDEXPR
BANDEXPR: EQEXPR | BANDEXPR "==" EQEXPR | BANDEXPR "!=" EQEXPR
EQEXPR: LGEEXPR | EQEXPR '<' LGEEXPR | EQEXPR "<=" LGEEXPR
  | EQEXPR '>' LGEEXPR | EQEXPR ">=" LGEEXPR
LGEEXPR: SHEXPR | LGEEXPR "<<" SHEXPR | LGEEXPR ">>" SHEXPR
SHEXPR: TERM | SHEXPR '+' TERM | SHEXPR '-' TERM
TERM: FACTOR | TERM '*' FACTOR | TERM '/' FACTOR | TERM '%' FACTOR
FACTOR: UNAEXPR
UNAEXPR: PARENEXPR | '!' UNAEXPR | '~' UNAEXPR | "++" UNAEXPR | UNAEXPR "++"
  | "--" UNAEXPR | UNAEXPR "--" | '-' UNAEXPR | ECAST
  | '*' UNAEXPR | '&' UNAEXPR | "sizeof" UNAEXPR
PARENEXPR: NUM | NAME | '(' EXPRS ')' | NAME '[' EXPRS ']'
  | NAME '.' NAME | NAME '->' NAME



RETSTMT: "return" EXPR ';'
STMTSCOPE: STMT | SCOPE
IFELSSTMT: "if" '(' EXPR ')' STMTSCOPE
  | "if '(' EXPR ')' STMTSCOPE "else" STMTSCOPE
  | "if '(' EXPR ')' STMTSCOPE "else" IFELSSTMT
WHILE: "while" '(' EXPR ')' STMTSCOPE
DOWHILE: "do" STMTSCOPE "while" '(' EXPR ')' ';'
FOR: "for" '(' EXPRS ';' EXPRS ';' EXPRS ')' STMTSCOPE
BRKSTMT: "break" ';'
CONTSTMT: "continue" ';'
SWSTMT: "switch" '(' EXPR ')' '{' CASES '}'
CASES: CASE | CASES CASE
CASE: "case" EXPR ':' STMTSCOPE
 | "default" ':' STMTSCOPE



*/

/*

C is such a complex grammar!  Alas, if only prefix or postfix operator
notation was used, there would be no need for operator precedence
rules in the parser.  Alas, as I keep telling myself, nobody programs
in those languages of Lisp or Forth, though.  So if anything, you'd
only use such a language to help you bootstrap a C compiler.  Probably
you'd use a prefix notation as that closely coincides with the
structure of a macro assembler.  An assembler is a must-have for a C
programming environment.

Literally, software source code is currency, and you want to be
compatible with the currency that has the biggest economy.  Of course,
the twist here is that you also want to be able to choose the simplest
such programming language for the sake of bootstrapping.

Okay, okay, let's take that twisted route that you've alluded to.
"Functional macro assembler extensions."  How do these work?  In
principle, they're not too different from the concept of C functions.
The return value of a function may only be of an integer type, and
that value is stored in the accumulator register.  Functions can be
defined similar to how macros are defined.  Arguments are all of
integer types.  Then we add an extension, subroutine invocations can
invoke additional subroutines for the arguments inline.  Instructions,
however, must operate with direct register values, so you'd define a
suite of macros if you wanted similar behavior on direct instructions.

While we're at it, we can also add support for local variables, stack
management, scoping, and recursion.  The system adds additional
instructions for pack/unpack arguments as needed.  To avoid the
complexity of an optimizing compiler, we can also add directives to
allow the user to do register management themselves, load and unload
variables to/from registers.  After which, the variable can simply be
referenced by name and will automatically be mapped to the register.

Varargs are not supported.

N.B. This does allow you to write clever C code that can end up being
compiled either by a full C compiler or by the macro assembler.

#define subr int
subr fnadd ()
  int a;
  int b;
{
  ADC a, b
}

subr main ()
{
  fnadd (AL, BL);
}

So this leaves us with data structures to address.  What is our
stance on this?  Oh yeah, we've got this one covered from previous
discussion.  You can simply define macros to get structure fields,
which compute the corresponding offsets.  But, that being said,
we can still easily provide syntactic sugar to declare a structure,
but accessing the fields will require the accessor macro syntax.

Not too foreign of a concept for high-level programming languages,
it's similar to an accessor function.

Function pointers, that's just a subroutine call in assembler.  Well,
yeah, we can fancy this up a bit too.

Now, now, we don't simply want to have a macro assembler for one
architecture, we want a portable macro assembler.  How do we do
this?  Simple, any tme we need an architecture-specific feature, we
rely on the user to define all such macros for the local architecture.
Here is our list of required macros:

* Read nth value from stack.
* Write nth value to stack.
* Load register from stack.
* Store register to stack.
* Load register from direct address.
* Store register to direct address.
* Change stack allocation.
* Subroutine call, direct address.
* Subroutine call, indirect address.
* Compute load-effective address (for structures).
* Compute load-effective address and load (for structures)
* Copy string (for initializing strings on the stack).
* Conditional jumps, unconditional jumos for control flow structures.

Return structures?  Okay, we can also support returning values
as stack-allocated data for this.  Allow returning data larger
than integer type.  Oh, and returning more than one variable,
a similar approach for that too.

Arrays are supported by convention: define standard macro names
to get nth array element.

Object oriented extensions?  Okay, fine fine,
I guess it's not that hard to support dot-structure notation.
Suffice it to say, we translate dot-structure notation to macro
accessor function notation.  And through this, we can also add
special support for calling the proper class routines.  This
allows you to use closure-style object-oriented syntax.

Wow, I guess we all have to admit, you can do a surprisingly good lot
of coding without any infix operator syntax.

Generics programming?  We've got the macro processor, that does
well enough, double down to make sure it has all the required
features.  Token-pasting and stringification are big ones.

Hey!  We made it this far without supporting strings proper?
I'd be tempted to say no need, international programming would
discourage hard-coding strings anyways, but we do in fact have
ubiquitous machine-readable data formats like XML and HTML that
require the parsing and recognition of special strings.  Luckily,
support is fairly easy to add.  A string constant simply gets
allocated in a string table and passed around as a memory address.

If-else statements are easy to implement.  A function is required
for the condition, instructions are not allowed.  Likewise, while
and do-while loops can also be implemented.  Goto?  That's implemented
directly as a macro.  For loops?  Well, thinking about that, it's not
too hard to implement, just allow two more function statements
before the "begin" directive and interpret them accordingly.

But, I do have to say.  From what you've described so far, you
are designing a macro assembler that packs a punch!

 */

/*

Now, to avoid getting ahead of ourselves, we must define the features
in extension sets proper

1. Basic macro processor.  ONLY define and substitute macros.  Support
   simple definition of constants and parameterized macros.

2. Support for token-pasting, to assist with generics programming.

3. "Functional macro assembler extensions."  Support defining and
   using subroutines with functional syntax.

4. Control flow: If-else, while, do-while, goto, for

5. Local variables, stack management, scoping, recursion.  Pin/unpin
   variables to registers.

6. `sizeof' directive, to assist with building data structure accessor
   macros.

7. Declarative syntax for data structures, auto-generate accessor
   macros.

8. Auto-generate global variable allocation, instead of requiring
    macros that compute sizes.

9. String constants and initialization.

10. Fancy syntax function pointer calls.

11. Return structures and multiple arguments.

12. Object-oriented syntax extensions.

13. Polymorphism extensions.

Compile-time evaluation of code bodies when all variables
are constants known at compile-time, even when there is looping.

*/

/*

Now that I mention this, another statement is also valid.  You can
easily set up a portable C compiler by a similar fashion.  Simply, the
C compiler translates the high-level syntax to macro assembler, then
you substitute macros for the operatores.  Also, this method allows
you to support operator overloading with ease.  How?  Well this is a
matter of token-pasting the type name with the operator macro name.

Inheritance and polymorphism?  Inheritance can be implemented via
macros, no biggie there.  Polymorphism, well, the requires a v-table.
The compiler can keep track of whether methods are marked as virtual,
and if so, it can translate the code to use the v-table.

Well, there you have it, I guess.  I've basically designed everything
I need for not only a C compiler, but a restricted subset C++
compiler.

C++ is nice in particular for its support of object destructor
functions and manual memory management.

Lambda functions?  Essentially a special scope usage.  Only
particularly useful in conjunction with lazy evaluation macros, which
is basically what all macros are.

Lazy evaluation macros/functions?  This is literally what if-else is.
In practice, lambda functions with lazy evaluation only work with
either macros or functions where arguments that are not compile-time
constants are function pointers.  If that is what the programmer wants
in functions, they can code that up explicitly.

Type inference?

Function currying?

Garbage collection?  Now you're getting ahead of yourself.

What features of C++ are we missing?  template<>, static_cast<>, and
so on, and all modern features of C++.  Essentially, we eliminate
those syntactic features of C++ that would require a grammar with
infinite look-ahead.

Point in hand, when implemented properly, macros allow quite a
powerful range of advanced programming language features, but with a
remarkably simple implementation.  And that's exactly what we want.

*/

/*

So, the critical question is.  How hard is it to write the pattern
tables for a full C grammar with correct operator precedence?  If it
is too difficult or too prone to error, we use the functional
assembler syntax for building up the preliminary tools before
implementing the full C compiler.

Another good option that is still readable but not prone to error is
to simply implement the C grammar without any operator precedence.
Just use parentheses for all your operators in the bootstrap toolchain
source code.

So actually, I've pointed out a few good extension sets by which to
design the first high-level programming language of the bootstrap
toolchain.

1. Assembler only, no label translation
2. Assembler with label translation
3. Macro assembler
4. Functional macro assembler
5. Infix notation macro assembler, no operator precedence
6. Infix notation macro assembler with operator precedence
7. Full ANSI C/C89 compiler
8. Restricted subset C++ compiler

Oh, and seriously!  I've been consulting the C operator precedence
chart a number of times.  Do you really need to consult an operator
precedence chart to program in this programming language?  Think about
it, are we still talking about a simple programming language?

And how does this fit in with you you'd practically implement these
tools?  Where in the process are you using multiple tools and require
some sort of disk operating system or operating system of a sort to
handle piping input?

The first option can easily be done directly into RAM.

Second option?  You're going to have dual buffers in RAM, one for the
source, the other for the binary code.

So, point in hand, all options can be implemented via multiple RAM
buffers and running separate programs in sequence.  Of course you
don't have all the RAM in the world to work with in a bootstrapping
environment.

Pipes can be implemented procedurally, without a full operating system
implementation.  The trick is setjmp, longjmp, and stack limits.

Okay, so it looks like before we even have a disk operating system,
there are a number of reasons why we'd want to create a cothreading
library first.  Cooperative multitasking kernel.  This is useful
both for the bootloader and for the first high-level language.

Okay, let's now list extension sets for operating system features in
light of the requirements of our boot and programming language
environments.

1. Basic memory management
2. Cooperative cothreading
3. Disk operating system

In place of a very basic shell to setup cothreads, the user must write
a small program to do so.

*/

/*

So, an assembler with label translation?  Anecdotal evidence from GCC
toolchain steps suggests this is best done in a two-pass method.

* Pass #1: This is almost entirely an assemble without label
  translation, except that we construct a table where we take note of
  every label that is used and the byte address we find it to be at.

  When we need to reference a label in an instruction, we translate
  the label so that the jump address is an ID number in our label
  table rather than an address.

  At the very end, we output the computed label table.

* Pass #2: Now we read the label table and use it to go back and edit
  our output so that all label references use the correct addresses.

There's also another alternative two-pass method.

* Pass #1: We go through and assemble the instructions, but we THROW
  AWAY the results since we are only interested in computing the
  instruction lengths for byte offsets.  We use these to build our
  label table, and only output the label table.

* Pass #2: Now that we have our complete label table in advance, we
  can go through and output the assembled instructions.  Any time a
  label is used, we can consult our label table to output the correct
  address.

When computers are slow, the choice is obvious.  Use style #1.  If we
need more ID numbers, we can construct an external table of
relocations.  Matter of fact, this is probably the fastest method.
Assemble with null addresses, output a relocation table at the end.
Then, read the relocation table to fill in the addresses proper.

label entry: ID, name, list of relocs
reloc entry: type (direct address, jump offset), address of use

*/

/*

Basic memory management and cothreading design.  Actually, we support
coprocesses.  Why?  Well, threads can only be defined in the context
of processes, as a thread must reference a process to indicate the
shared values.  We allow relocation of global memory and basic sharing
of read-only memory.

struct Coproc_tag
{
  unsigned status;
  unsigned const_base;
  unsigned const_limit;
  unsigned global_base;
  unsigned global_limit;
  unsigned stack_base;
  unsigned stack_limit;
  unsigned stack_guard_limit;
  char regmem[REGMEM_SIZE];
};
typedef Coproc_tag Coproc;

Coproc coprocs[MAX_COPROCS];
unsigned coprocs_len;

setjmp ()
longjmp ()

coproc_init ()
int coproc_create (void (*start)(void))
coproc_exec ()
coproc_switch_to (int pid)
coproc_switch ()
coproc_switch_rti (jmp_buf *old_regmem)
coproc_destroy (int pid)
coproc_exit ()
coproc_abort_all ()

rti = return from interrupt.  When we switch, we check that the
stack limit has not been exceeded.  If it has been exceeded but
not exceeded its guard space, we terminate that process on stack
overflow.  If it exceeded the guard limit, that is a critical
exception where we must abort all processes in the system.

How do we get started?  We initialize, create the processes without
running them, then switch to the first process to start running.
The system will then switch back and forth as necessary to finish
execution, then finally return from `coproc_exec ()' when it is done.

coproc_init ();
coproc_create (...);
coproc_create (...);
coproc_exec ();

What if you're in the middle of a running system, and you want
to create more processes?  As soon as you call coproc_create (),
it becomes runnable.  We have not coded support for sleeping
processes as of yet, so they otherwise have polling waiting
loops when there is no input/output.  We can add support for
sleeping processes and waking them.

coproc_sleep ()
coproc_wake ()

Sleeping a fixed number of time is more complicated to implement.
Essentially, the naive implementation is HALT + IRQ.  More advanced
assumes effectively a PIT, which is likely software-managed
so it can be shared.

And support for getting the process ID, if it might matter.

coproc_getpid ()

Oh, and we might want to support fork, exec, and wait.  How do we
do that?  we need coproc_create () to return the process ID created.
Then, we need to be able to wait on a PID.

coproc_waitpid ()

And now we're going to want a SIGCHLD signal handler and that stuff.
Okay, fine, now we can add support for signals.

How are they implemented?  Basically, it's like how a process
would receive a hardware interrupt.  Return address and registers
are pushed onto stack, and signal handler code is invoked.  When
it returns, we go back to normal process code.  For more Unix-like
behavior, we insert a "thunk" that calls the user signal
handler, and the system signal handler decides what to do next
based off of the return code.  Also, another advantage is this
allows us to insert `setjmp ()'/`longjmp ()' to set up and tear
down the signal handler.

If there is realtime scheduled processes, we allocate constant
time slices for them and call them at constant periods, possibly
with a little jitter.  The max number of realtime processes s is limited
by the period scheduling, and all free time must be waited out
unless there are flex processes to fill it out.

It is possible for realtime threads to allocate more than one
unit of time slice, but please note that in simple implementations,
the realtime thread will still be interrupted for each timeslice.

How do we support `brk ()' and `sbrk ()' without virtual memory?
We use a heuristic in positioning processes that do or don't request it.
Non-requestors are lined up at the front of memory.  Requestors
are spaced throughout all of memory in a binary division manner, since
we know nothing else in general.

General algorithm.

total_allocs = 0;
exp_2 = 0;
next_pow_2 = 1;
while (next_pow_2 < total_allocs) {
  next_pow_2 <<= 1; exp_2++;
}
// Now, here's the trick.  We evenly space the divisions
// by next_pow_2, but skip every first such position already
// taken by the previous ones.
spacing = max_mem / next_pow_2;
if (total_allocs == 0)
  start = 0;
else {
  i = total_allocs - (next_pow_2 >> 1);
  start = 1 + i * 2 * spacing;
}

TODO: Please test this algorithm!

Now if this fails because one of the older processes already
grew larger, then we keep counting up total_allocs until we find
a space that fits, until we can count no longer because we divided
up the space too small.

What if we have processes being freed?  Well, then we need a list
to keep track of which spaces are free.  Okay, fine, so we define
a max number of allocs, and we can statically allocate the book-keeping
list.

Finally, with virtual memory support, either in hardware or in
software, we can flexibly grow both the stack and a dynamic memory
allocation region.

But seriously!  Windows resources have a thing being "discardable"
which means they need not be pinned to RAM, and I believe this
also came from the Macintosh memory manager.  So the model
I presented is still even a bit too simplistic even for early
personal computers.

N.B.: With globals, a process can also have

init_globals ()
load_globals ()
save_globals ()

functions for the sake of keeping compiled C code
unchanged but supporting multiprocessing.  These are function
pointers in the Coproc object.

How do we support exec ()?  That's pretty easy, basically
refactor coproc_create () into two stages.  How about fork ()?
That one is tougher.  First create a clone
of the Coproc object.  Clone the global variables and the stack.
The last part is tricky since we need to return with one of
two possible return values.  We set a status flag to indicate
if this is a newly forked process, then we update setjmp ()/longjmp ().
The final stages of the code, we check the fork flag, and decide
whether we should return zero or the value of the child process
PID previously saved in a stack variable.  The fork flag is cleared
and never used again.

Now, the big bummer that makes this really thorny.  With this
implementation of fork, any stack variable that is a pointer is
invalidated, since all pointers will need to be rebased.  So if you
want to use pointers together with fork (), you need a practical way
to rebase all pointers, of which the most practical is to simply store
all pointers as an offset from a base address variable and dynamically
compute them when you need them.  For this reason, vfork () is better.
exec () is easy and nice.

Now you ask about passing arguments to processes?  Okay, fine, we can
do this simply by pushing them onto the stack ahead of the function
that we call, and then it is a simple matter of unpacking the
arguments.

Exit codes and the exit () system call?  Well, we know where the
return address is since we save the stack base pointer, so we can
easily do a remote return from the top-level procedure.  As for
the exit code, we can store that in the Coproc object and leave
it laying around as a zombie process until it is reaped.

Okay, that's too complicated.  We need something more fundamental,
like the most minimal thing you'd use inside a bootloader.
In this case, there will not be multiple instances of the same
code executing, so we can make some simplifications.

struct Coboot_tag
{
  char regmem[REGMEM_SIZE];
};
typedef Coboot_tag Coboot;

In this extreme case, we don't even bother checking for stack overflows.

 */

/*

TODO:

1. Get the time synced

2. Get the systemd unit file synced

3. Merge in pending TODO for Raspberry Pi scripts

4. Finish your Raspberry Pi install scripts

5. Merge together pending blog notes

6. Finish pending blog notes

7. Update your password

I have to do all that, before I update my password, which
is due in 2 weeks?!!!


 */
