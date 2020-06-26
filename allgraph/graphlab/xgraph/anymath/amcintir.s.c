/* Irrational operations for integers.  */

/* OKAY, how should we better organize this aspect?

Probability, statistics, and logarithms: Factorials, permutations,
combinations, compute number e. 1/0! + 1/1! + 1/2! + ...

Compute binomials raised to a power efficiently using the binomial
theorem.

----------

FACTORS are a totally independent problem.

So, here's the idea.  A factored number is simply a list of numbers
to multiply together, easy enough.  Now, to define multiplication
on a factored number, you simply concatenate a number to the list,
rather than multiply.

Now actually, that's a great idea if you're operating on a CPU without
a hardware multiply instruction.

Okay, so what makes working with factors so special then?  Basically,
just a few subroutines that take in the types and perform common operations,
like eliminate common factors and... gosh!  No, there really is nothing
special about factor lists once they've been built.

After that, treat them as multi-sets and provide boolean operators on
that.  Intersection is determine common factors, difference is
"reduce" i.e.  eliminate common factors, union is multiply, and now
for least common denominator.  Basically, a modified union, union*,
include all multi-set items the max number of times, rather than total
number of times.  And also determining the multipliers.

Exclusive-OR to determine non-common factors.

Greatest common factor?  Again, that is an intersect, followed by a
difference.  Or just do a difference straight pn each one, is
"exclusive-OR" if you consider the two together.

Okay, so clearly, the point is, if we have a robust set of primitives
for working with multi-sets, that totally covers most of the factor
operations required.  Then, the only thing missing is, simply the
operation itself directly, factor().  And, of course, multiply().

Also important as subroutine of factor().  Prime number sieving and
testing.

Okay, so that's one end.  The other end, statistics.

and(), or(), sum(), ar_mean(), max(), min(), min_max()

VECTOR operation: sigfig()

mode(), median()

Okay, GOOD POINT!  The reason why statistical operations should be
done separately.  Because, most of them cannot be run in parallel
except by means of binary tree reduction.  So, we need a different
way of coding.

NEIGHBOR COMBINATOR.  Then the underlying engine that combines those
two numbers, that is configurable.  You only define the 2-to-1
reducer.  That's the way to code it up correctly.

min_max()?  That is a 3-to-2 reducer, but it can be used very similar
to 2-to-1 reducers.

mode()?  Now that is a tough one, like a conversion to a multi-set
that counts each element, then mode() is easy.  Convert the
representational form.  Otherwise, you must sort.

arithmetic mean

PLEASE NOTE: Bit-wise combinators are, in a conceptual sense, also
statistical, but they are special because they are so ubiquitous.
Likewise with product() combinator.

Of course, the practical problem and limit with product() combinator
and factoring.  The number of numbers you multiply together, and how
many bits that creates.  A lot.  Which means there are practical
considerations about how many primes you need to consider, and max
number of factors.

OKAY, so how do we gracefully handle both potential sequential and
parallel computation?  DEFINE STAT OBJ.  For sequential computation,
the STAT OBJ is just a single accumulator.  For parallel computation,
it is a "whole set of registers and threads," or an array of max
required size, in other words.

OKAY, so NEW TAGS in light of this!

PARALLEL (implied)

REDUCER

If a higher-order operation is comprised only out of PARALLEL
operations on the components, it can be sliced apart into simultaneous
threads that compute on each component in parallel.

In essence, any REDUCER acts as a BARRIER to parallel threads.
Reducers can operate in most log(n) times for an n-element vector.

For SEQUENTIAL computation, temp variables that are a vector-wide can
be reduced to a single component wide if slicing can be used on
normally parallel computation to compute each individual component
wholly sequentially instead.

BUT WAIT, there's one more classification:

PARALLEL-REDUCERS

This is, of course, if you do use reducer subroutines, but they can
still be parallelized.  But if they can't be parallelized?

NONPAR-REDUCERS

As a warning that the reducers have further slowdowns on top of the
log(n) baseline.

----------

EVEN FURTHER RIGOR.  Reduce is a core sub-task of all statistical
computations, and it is general beyond simply statistics.  Wow, now
I'm re-inventing Google MapReduce.

Oh, dang, I should have known.  I could have specified all those
vector operations in terms of a map.  Or, by generalization, a map
that takes n input arguments and produces a single output.

Google MapReduce, that is the way to go.  It very clearly allows you
to analyze the parallel structure of your code.

*/
