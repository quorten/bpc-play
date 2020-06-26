/* Compute a sequence of integers in parallel.

   How do we do this nifty trick?  First we must seed a chunk of
   consecutive integers, up to the maximum width of our parallel
   compute.  This is essentially a _binary tree unfold_, the reverse
   of a binary tree reduce/fold.  Then, we can compute the next set of
   consecutive integers by adding an integer constant base to the set
   in parallel.

   NOW, here's the kicker.  I generalized these routines so not only
   can they compute consecutive integers, they can also compute
   consecutive powers.  */

#include <stdio.h>
#include <stdlib.h>

#define BASE 1
#define COMBIN +

/* Since the max size of an unfold is determined by the capabilities
   of the parallel hardware, typically this is computed just once and
   the results are stored somewhere permanent relative to the duration
   of the process (program instance).  */
void unfold_n(int *data, unsigned log2_n)
{
  unsigned n, c;
  /* unsigned num_seq = 0; */
  n = (unsigned)1 << log2_n;
  data[0] = BASE; /* BASE VALUE */
  c = 1;
  while (c < n) {
    /* BEGIN MAP */
    unsigned i;
    unsigned add_val = data[c-1];
    for (i = 0; i < c; i++) {
      data[i+c] = data[i] COMBIN add_val;
    }
    /* END MAP */
    /* OpenCL barrier ensures that all threads are synced in execution
       and local memory consistency state by this point, for the sake
       of the sequential outer loop.  */
    /* barrier(CLK_LOCAL_MEM_FENCE); */
    c += c;
    /* num_seq++; */
  }
  /* printf("%d sequential iterations\n", num_seq); */
}

/* NOW, here's the real kicker-deal.  Do we really have to do an
   unfold in OpenCL to get a list of consecutive integers?  Absolutely
   not.  Since we get thread IDs for indices, we can just use that
   smack right as our seed value.  Then in one fell swoop we generate
   our list of consecutive integers.  Like the other unfold, this
   routine is confined to a single block, once a single block is
   initialized you can use the multi-block routine.

   Technically, that means our inner loop is not a "map," strictly
   speaking, but we seriously need to take advantage of this low-level
   optimization!  */
void gpu_unfold_n(int *data, unsigned log2_n)
{
  unsigned n = (unsigned)1 << log2_n;
  unsigned i;
  for (i = 0; i < n; i++) {
    data[i] = i + 1;
  }
}

void next_n(int *data, unsigned add_idx, unsigned len)
{
  unsigned i;
  unsigned add_val = data[add_idx-1];
  /* BEGIN MAP */
  for (i = 0; i < len; i++) {
    data[i+add_idx] = data[i] COMBIN add_val;
  }
  /* END MAP */
}

/* Enhanced with block-wise (work-group-wise) multiplicity.
   Essentially, we nest two maps.  */
void next_blk_n(int *data, unsigned add_idx, unsigned blk_len, unsigned n_blks)
{
  unsigned i;
  /* BEGIN MAP */
  for (i = 0; i < n_blks; i++) {
    unsigned blk_add_idx = add_idx + blk_len * i;
    next_n(data, blk_add_idx, blk_len);
  }
  /* END MAP */
}

int main(int argc, char *argv[])
{
  unsigned log2_n = (argc > 1) ? atoi(argv[1]) : 0;
  unsigned n = (unsigned)1 << log2_n;
  unsigned n_blks = (argc > 2) ? atoi(argv[2]) : 0;
  unsigned num_alloc = n * (1 + n_blks);
  int *data = (int*)malloc(sizeof(int) * num_alloc);
  unsigned i;

  if (data == NULL)
    return 1;
  unfold_n(data, log2_n);
  if (n_blks)
    next_blk_n(data, n, n, n_blks);

#ifndef NOPRINT
  printf("%d", data[0]);
  for (i = 1; i < num_alloc; i++) {
    printf(" %d", data[i]);
  }
  puts("");
#endif

  return 0;
}

/*

compute pow(x, n) in parallel?

Start with the need to seed by unfolding.

x
x, x^2
x, x^2, x^3, x^4

Each iteration, multiply previous chunk by x.

Unfortunately, looks like we cannot eliminate unfold.

How do we step block-wise?  Our blockwise stepping is limited by
block-size times block-size.  To find the multiplier, we use the
unfold chunk as a lookup table.  Then we take that chunk and multiply
it by our cached multiplier.  Easy as that.

Compute factorials in parallel?  That's fairly easy, generate a list
of consecutive integers in parallel, then multiply-reduce them all in
parallel.  Use block-wise reduction, followed by gather and repeat
steps.

Limit, of course, to the maximum extent of parallelism.  Beyond that,
it's sequential computation, use the last computed value.

Okay, that's for computing one factorial, what about computing
multiple in parallel?  More or less, you can start out by having
everything be independent, once you have a largest value computed that
is the winner for seeding larger values.

Likewise, for computing powers in parallel.  As long as you have
enough parallel compute units, you just spawn more threads, even if
some computations turn out to be redundant.  Why not try to share more
results?  Because otherwise, you can create memory synchronization
problems.  If memory synchronization weren't a problem, yeah, it would
clearly be more efficient.

So, we pretty much have everything that is required to compute number
e in parallel.  The final divides and additions are all downhill for
parallel compute.

Na... is this really the fastest way?  Or is computing the area much
better for parallel compute?  That's purportedly the better way of
computing pi in parallel.

Actually, to be honest, when computing factorials in parallel?  Here's
how we can compute a whole series rather than just one factorial.

T1:

1 2 3 4 5 6 7 8
->
1*2 3*4 5*6 7*8

T2:

1*2 3*4 5*6 7*8
->
1*2*3*4 5*6*7*8

T3:

1*2*3*4 5*6*7*8
->
1*2*3*4*5*6*7*8

So, what's the magic trick that needs to be applied to get the whole
sequence?  Save all of our lower-level sub-computations.  Then,
use the following knowledge:

2! = (1*2)
3! = (1*2) * 3
4! = (1*2*3*4)
5! = (1*2*3*4) * 5
6! = (1*2*3*4) * (5*6)
7! = (1*2*3*4) * (5*6) * 7

So far, so good, it's still pretty efficient to compute in parallel,
to fill in the gaps.  Also it looks like we don't need to save all
intermediate results.  Okay, let's extend this to double-wide.

T1:

01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
->
01*02 03*04 05*06 07*08 09*10 11*12 13*14 15*16

T2:

01*02 03*04 05*06 07*08 09*10 11*12 13*14 15*16
->
01*02*03*04 05*06*07*08 09*10*11*12 13*14*15*16

T3:

01*02*03*04 05*06*07*08 09*10*11*12 13*14*15*16
->
01*02*03*04*05*06*07*08 09*10*11*12*13*14*15*16

T4:

01*02*03*04*05*06*07*08 09*10*11*12*13*14*15*16
->
01*02*03*04*05*06*07*08*09*10*11*12*13*14*15*16

2! = (01*02)
3! = (01*02) * 3
4! = (01*02*03*04)
5! = (01*02*03*04) * 5
6! = (01*02*03*04) * (05*06)
7! = (01*02*03*04) * (05*06) * 7
8! = (01*02*03*04*05*06*07*08)
9! = (01*02*03*04*05*06*07*08) * 9
10! = (01*02*03*04*05*06*07*08) * (09*10)
11! = (01*02*03*04*05*06*07*08) * (09*10) * 11
12! = (01*02*03*04*05*06*07*08) * (09*10*11*12)
13! = (01*02*03*04*05*06*07*08) * (09*10*11*12) * 13
14! = (01*02*03*04*05*06*07*08) * (09*10*11*12) * (13*14)
15! = (01*02*03*04*05*06*07*08) * (09*10*11*12) * (13*14) * 15

So, looks like worst-case multiplications here to fill in the gaps are
`log2(n) - 1`.  That's excellent!  And, the best part about this is
that this procedure can also occur in parallel with the primary
factorial computation code.

So, what's the rule for what values get discarded?  After we combine
the two halfs together, we never need the second half, only the first
half. Of course, this is the COMBINED second half, the second half
split apart had a first half and second half.  That split apart first
half will still be needed, until we are only using the wholly combined
part, then even that is no longer needed.

Yeah, so the keep/discard rule is easy enough to follow.  It's
basically like a kind of modulo remainder type of counting.

So, overwrite the last end, not the first end, when storing.  If you
just always overwrite the last entry of the current slice when
reducing, you will be just fine.  Worthy of note is that you'll need
power of two padding at the end of non-power-of-two vectors for this
to work nicely.

How do you phrase together the actual program to compute this?  Think
about this, this is basically a binary power-of-two expanded number
notation decomposition.  You can use parallel lookup tables to load
the factors that need to be multiplied, then a parallel reduce on
those.

About area methods, they still require a log(n) summary step, so are
they really that much faster?  Doesn't look like it from a big-O
notation standpoint.  You just need to get really good at doing folds
and unfolds in parallel using log2(n) binary tree methods.

*/
