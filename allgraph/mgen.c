/* Make music according to a technical specification.

   This program and the resulting music are in public domain without
   restrictions.

   How to generate random music.

   1. Decide on the total length (32 beats).

   2. Decide on the total octave span for the melody (1 octave).
      Decide on the minimum and maximum notes.  Min note (C4), max
      note (B5).

   3. Decide on the length of the principle pattern that repeats with
      minimal or no modifications (10 notes).  In other words, this
      could be called a "measure," although I think you know the way
      that "measures" are often times use in sheet music: they aren't
      used in a meaningful way in relation to the music.

   4. Decide on the number of "key notes" that are used in one
      "principle pattern" section (7).

   5. Decide on the number of "key notes" that are allowed to change
      from one principle pattern section to another (3).

   6. Decide on what "key" the piece should be set in, if any.  This
      piece will be set in no "key" whatsoever, using sharps and flats
      freely without restriction.

   6. Pick the random number corresponding to the notes and their
      actual length.  Of course, notes can be shorter and longer than
      one beat.  Let's keep things simple here, notes can only be one
      shorter or one longer, but it is 7/10 likely notes will be of
      normal length, 2/10 likely that notes will be shorter, and 1/10
      likely that notes will be longer.

   Now, this is where I use FreeBSD libc srand(32251073), included in
   this program's source code for platform-independent
   reproducibility.  Execute this program to get the rest of the
   sequence.

 */

#include <stdio.h>
#include <string.h>
/* BEGIN FreeBSD <stdlib.h> */
#ifndef NULL
#define NULL ((void*)0)
#endif
#define	RAND_MAX	0x7fffffff
int rand(void);
void srand(unsigned);
/* END FreeBSD <stdlib.h> */

#ifndef __cplusplus
enum bool_tag { false, true };
typedef enum bool_tag bool;
#endif

#define ABS(x) (((x) >= 0) ? (x) : -(x))
struct Coord2D_tag { int x; int y; };
typedef struct Coord2D_tag Coord2D;

unsigned calc_note_duration();
void print_note(unsigned num, unsigned note_dur);
void ly_engrave_note(unsigned num, unsigned note_dur);
void flush_ly_engraving();
void punch_note_holes(unsigned num, unsigned note_dur);
void flush_piano_roll();
void cubic_bezier_eval(Coord2D *r, Coord2D t, Coord2D max,
		       Coord2D a, Coord2D b, Coord2D c, Coord2D d);
void test_bezier_curve();
void test_line_rasterizer();
void test_smooth_bezier();
void test_recursive_bezier();

int
main ()
{
  unsigned total_beats = 32;
  unsigned base_note = 4 * 12;
  unsigned note_range = 1 * 12;
  unsigned princ_pat_len = 10;
  unsigned num_princ_patterns = total_beats / princ_pat_len;
  unsigned num_key_notes = 7;
  unsigned change_key_notes = 3;
  unsigned chosen_key_notes[7];

  unsigned i, j;

  srand(32251073);

  /* Initialize the chosen key notes.  */
  for (i = 0; i < num_key_notes; i++)
    chosen_key_notes[i] = base_note + rand() % note_range;

  /* Sequentially generate the principle patterns.  */
  for (i = 0; i < num_princ_patterns; i++) {
    for (j = 0; j < princ_pat_len; j++) {
      unsigned note_key = chosen_key_notes[rand()%num_key_notes];
      unsigned note_dur = calc_note_duration();
      /* print_note(note_key, note_dur); */
      ly_engrave_note(note_key, note_dur);
      punch_note_holes(note_key - base_note, note_dur);
    }
    /* Mutate the pattern now.  */
    for (j = 0; j < change_key_notes; j++) {
      chosen_key_notes[rand()%num_key_notes] =
	base_note + rand() % note_range;
    }
  }

  /* Do the "finisher notes," any notes remaining after the principle
     pattern.  Generate these totally randomly.  */
  i *= num_princ_patterns;
  while (i < total_beats) {
    unsigned note_key = base_note + rand() % note_range;
    unsigned note_dur = calc_note_duration();
    /* print_note(note_key, note_dur); */
    ly_engrave_note(note_key, note_dur);
    punch_note_holes(note_key - base_note, note_dur);
    i++;
  }

  flush_ly_engraving();
  putchar('\n');
  flush_piano_roll();

  test_bezier_curve();
  test_line_rasterizer();
  test_smooth_bezier();
  test_recursive_bezier();
  return 0;
}

/* What if we listen to that curve directly?  Lesson learned: straight
   lines work better for synthesis.  Use straight lines to vary the
   pitch for octave climbing, and use straight lines to vary the
   tempo/speed of a certain part of a piece.  */

/* So is the "Happy Birthday" octave climbing pattern a curve?  No,
   it's a series of straight line segments!  Ah, "simplicity" as they
   say.  Or, in other words, lack of copyrightability.  No, it's not a
   bell curve, it's actually just a series of straight line segments.
   And no, making it a curve would not make it sound more musical, it
   would actually sound worse than a series of straight line
   segments.  */

/* One of the biggest problems in regard to the copyrightability of
   music is that what makes music sound musical and memorable is the
   lack of originality present in the song, not the expression of it.
   As has been revealed by my music generation software.  "Restrict
   the output so that it follows some basic patterns."  So, in other
   words, the field of original and copyrightable music is much
   smaller than it may seem at first, from a quantitative
   perspective.  */

/* Return 1 for half note, 2 for whole note, 4 for double note.  */
unsigned
calc_note_duration()
{
  unsigned r_num = rand() % 10;
  if (r_num < 7) return 2;
  else if (r_num < 9) return 1;
  else return 4;
}

/* Print a note by its name.  */
void
print_note(unsigned num, unsigned note_dur)
{
  const char *note_names[] =
    { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
  const char *dur_names[] =
    { "0.5", "1.0", "2.0" };
  unsigned octave = num / 12;
  unsigned note = num % 12;
  printf(" %s%u %s", note_names[note], octave, dur_names[note_dur/2]);
}

/* Print a note as a LilyPond typesetting command.  */
void
ly_engrave_note(unsigned num, unsigned note_dur)
{
  const char *note_names[] =
    { "c", "cis", "d", "dis", "e", "f", "fis", "g", "gis",
      "a", "ais", "b" };
  const char *dur_names[] =
    { "8", "4", "2" };
  static bool ly_init = false;
  unsigned octave = num / 12;
  unsigned note = num % 12;
  unsigned i;

  if (!ly_init) {
    fputs("\\version \"2.12.3\"\n{", stdout);
    ly_init = true;
  }
  putchar(' ');
  fputs(note_names[note], stdout);
  /* Note: Use comma `,' to lower the octave.  Here, we only handle
     the case of higher octaves than the base octave.  */
  if (octave < 3) {
    for (i = octave; i < 3; i++)
      putchar(',');
  } else {
    for (i = 0; i < octave - 3; i++)
      putchar('\'');
  }
  fputs(dur_names[note_dur/2], stdout);
}

/* Print the closing statements needed for a complete LilyPond
   input.  */
void
flush_ly_engraving()
{
  fputs(" }", stdout);
}

#define ROLL_HEIGHT 12
#define ROLL_WIDTH 4 * 32
/* Roll lines run vertially, with the first starting at the leftmost
   edge of the piano roll.  */
unsigned cur_roll_line = 0;
char roll_buffer[ROLL_HEIGHT][ROLL_WIDTH];

/* Print a note as it would appear on piano roll.  This is also great
   for debugging the Bezier curve renderer!  */
void
punch_note_holes(unsigned num, unsigned note_dur)
{
  unsigned target_col = cur_roll_line + note_dur;
  while (cur_roll_line < ROLL_WIDTH && cur_roll_line < target_col) {
    unsigned i;
    for (i = 0; i < ROLL_HEIGHT; i++) {
      char value;
      if (i == num) value = 'X';
      else value = ' ';
      roll_buffer[i][cur_roll_line] = value;
    }
    cur_roll_line++;
  }
}

/* Print out the piano roll "framebuffer."  This is starting to feel
   more like graphics programming than music programming!  */
void
flush_piano_roll()
{
  unsigned i, j;
  for (i = 0; i < ROLL_HEIGHT; i++) {
    for (j = 0; j < cur_roll_line; j++)
      putchar(roll_buffer[ROLL_HEIGHT-1-i][j]);
    putchar('\n');
  }
}

/*

Additional information on curves.

Let's get some mathematical insight into why 1/3 2/3 control point
positioning is so lineaer.

a = 0, b = 1/3, c = 2/3, d = 1

a*(1 - t)^3 + b*3*(1 - t)^2*t + c*3*(1 - t)*t^2 + d*t^3
= (1 - t)^2*t + 2*(1 - t)*t^2 + t^3
= t - 2t^2 + t^3 + 2*(t^2 - t^3) + t^3
= t - 2t^2 + t^3 + 2*t^2 - 2*t^3 + t^3
= t

Huh!  They cancel!

dt = 1
ddt = 0
dddt = 0

Okay, we better do a proof to show that having the two middle control
points equal in y-value and equal-spaced in x-value really is a
parabola.

a = 0, b = 1, c = 1, d = 0

a*(1 - t)^3 + b*3*(1 - t)^2*t + c*3*(1 - t)*t^2 + d*t^3
= 3*(1 - t)^2*t + 3*(1 - t)*t^2
= 3*(t - 2t^2 + t^3) + 3*(t^2 - t^3)
= 3*(t - t^2)

Huh!  The t^3 terms cancel!  That's how we know the result is indeed
a parabola: x(t) = t, y(t) = 3*(t - t^2).

y(t) = -3*(t^2 - t)
Complete the square
= -3*(t^2 - t + 1/4 - 1/4)
= -3*((t - 1/2)^2 - 1/4)
= -3*(t - 1/2)^2 + 3/4
Vertex: (1/2, 3/4)
Parabola opens downward

Huh!  So there's another reason alluding to the 0.75 * y that you've
previously calculated.  And this time, Calculus was not required.

Also, we should do a proof with quadratic Bezier curves.

a = 0, b = 1, c = 0
a*(1 - t)^2 + b*2*(1 - t)*t + c*t^2
= 2*(1 - t)*t
= 2*(t - t^2)
= -2*(t^2 - t)
Complete the square
= -2*(t^2 - t + 1/4 - 1/4)
= -2*(t^2 - 1/2)^2 - 2*(-1/4)
= -2*(t^2 - 1/2)^2 + 1/2
Vertex: (1/2, 1/2)
Parabola opens downward

Yes, and everything you learned about parabolas in Algebra applies
right here.

Okay, now finally time for the calculation that you've all been
waiting for.  Let's get some mathematical insight into these nonlinear
positionings of control points.

a = 0, b = 1/8, c = 7/8, d = 1

a*(1 - t)^3 + b*3*(1 - t)^2*t + c*3*(1 - t)*t^2 + d*t^3
= 3/8*(1 - t)^2*t + 21/8*(1 - t)*t^2 + t^3
= 3/8*(t - 2t^2 + t^3) + 21/8*(t^2 - t^3) + t^3
= 3/8*t - 3/4*t^2 + 3/8*t^3 + 21/8*t^2 - 21/8*t^3 + t^3
= -13/8*t^3 + 3/8*t^3 + 15/8*t^2 + 3/8*t
= -10/8*t^3 + 15/8*t^2 + 3/8*t

What does this mean?  Well, let's try plotting the curve.  Remember,
we only care about the curve in the area between zero and one.  In my
plot of the curve, I only see some smooth-waving curve with equal
symmetric changes in curvature around t = 0.5.  "But why?" is the
better question.  Oh, sure, take the derivatives of this curve!

dx = -30/8*t^2 + 30/8*t + 3/8
Now that's looking interesting.  Suspicious, I should say.
ddx = -60/8*t + 30/8
dddx = -60/8 = -15/2 = -7.5

The highest derivative will always just give us some kind of constant.
The first derivative can tell us where the extrema are, the turning
points of the curve, where the slope is equal to zero (located a
little bit outside the t-bounds).  The second derivative can tell us
where the inflection points are (t = 0.5).  The third derivative?
This just tells us some kind of constant rate of change in curvature.

Here's the thing: we want to have a function that tells us the density
of the curve.  Where the rate of change increases and decreases.
That, of course, would be the first derivative.  So, what are we
looking for in the first derivative?  Peaks and troughs.  That
information, of course, can be found in the zeros of the second
derivative.  Actually, we're not interested in peaks and troughs,
we're only interested in the rate of change.  We use the results of
computation in the rate of change to adjust for the density at which
we plot points at.  Well, okay, fine, we use peaks and troughs to
determine the rate to sample the rate of change function at and size
the accumulator appropriately.  (We only care about peaks and troughs
between t = 0 and t = 1).  Since the second derivative has to be a
straight line, it can only have one zero.  Beyond this, we can
determine the max rate(s) of change surrounding this zero, then size
the accumulator appropriately.  How is that?  At the max rate of
change, it must not overflow more than once per iteration.  So, from
our previous calculation above, we've determined the min rate of
change and the max rate of change, so we can setup the size of our
accumulator.  Then we use the rate of change function chained to the
accumulator to determine when to plot a point.  We plot a point if the
accumulator either exceeds the max or falls below the min.  Otherwise,
we just keep stepping the function until the limits are exceeded.

Obviously, given the plotting algorithm above, we can make the most
use of our CPU cycles if we have the least extreme variation in curve
plotting density.  That way, we won't be cycling and looping over
nothing over areas of low rates of change.

* And, that's the reason why the "stretch a short curve into a long
  one" while plotting algorithm is not very efficient.

Again, I reiterate the reason why this works.  In areas of high rates
of change, this algorithm will cause more points to be plotted, but in
areas of low rates of change, less points will be plotted.  Also, when
the direction of the curve changes, we'll go from adding to
subtracting, so we'll properly account for the fact that we have
overlapping plotting points due to crossing the turning point and so
won't plot excess points in these regions of the curve.

Yes, yes, that's great!  Borrowing ideas from the good line plotting
algorithm to create a good curve plotting algorithm.  Of course, you
really do want to use the optimizations mentioned previously as much
as possible to get the best mix of high performance curve plotting and
accurate curve plotting.

So, let me put things this way.  First, you want to make the curve as
close to a parabola as possible.  This is now merely an optimization
for the next step that monitors the rate of change.  The optimization
is that you'll minimize the number of dead values that you compute in
the rate of change loop.  Yet, by using the rate of change loop, you
still get perfectly accurate curve plotting.

Now I just need to figure out an efficient way to do the "good Bezier
curve" detection calculations to plug into the de Casteljau's
recursive subdivision algorithm.

Yes, yes, yes, and for those who ask, I can provide every imaginable
variation of the curve plotting algorithms for all of those
specialties where you evaluate the function directly at fixed
intervals etc.  Of course, those will already be strict subsets of my
fully optimized algorithms that I'll be using.

Yes, yes, yes, again, that works perfectly.  We're talking about the
rate of change on the output (X-Y coordinate plane distances) based
off of the input (t-value).  This is exactly what we're looking for.

Okay, here's how to do the checks.  First, truncate the precision of
the coordinates.  Then, rotate the coordinates to a nominal
orientation.  Once in the nominal orientation, do the checks to see if
the point positions are satisfactory.

Or, hey, you said pixels, so fight pixels with pixels.  Only do the
fast algorithms when you are working with short curves.  So, if the
curve is too long, you always use de Casteljau's algorithm to render
it at full precision.  Then you only use the fast algorithms when you
are working with a sufficiently small number of pixels.  Otherwise,
accuracy is paramount, even if it comes at the expense of speed.
Don't want that much accuracy?  Hey, remind the user that this is,
after all, integer arithmetic.  Truncate integers on input then!

Okay, okay, I guess for some use cases, curvature accuracy isn't quite
so important beyond the half-a-degree limit, so this is another
parameter you can take into account during plotting.  For some
applications, as soon as the change in angle drops below a certain
threshold (half-a-degree recommendation for graphics rendering without
any additional special effects), then the rest of the curve can be
plotted using a line approximation.

That being said, I guess something similar can apply with parabola
approximations too.  Now I have to think between pure parabola
approximation versus rate-of-change near-parabola plotters.

Okay, this is how it goes.  Shallow parabolas can be plotted just like
lines.  Steep parabolas need either a rate-of-change plotter or they
need to be subdivided before plotting.  If the rate-of-change extrema
is too steep, then the parabola must be subdivided for optimal
rendering performance.

Okay, so let's overview the rendering process end-to-end.

1. You get an arbitrary cubic Bezier curve.

2. Begin recursive subdivision and decision-based rendering.

   * Require subdivision if any of the following conditions are true:
   ** The middle control points are on opposite sides of the endpoints
      line
   ** The middle control points are not within the perpendicular bounds
      of the endpoints lines (draw picture)
   ** The endpoints line is very short, but the middle control points
      are relatively far away

   * If we have gotten to this point, that means we've got a mostly
     well-behaved curve.  In order to make the final rendering
     optimizations, we want to verify that the middle control points
     are sufficiently close to a 1/3-2/3 spacing between the
     endpoints.  If not, require subdivision to be a better-behaved
     curve.

   * Now we are guaranteed a curve that takes a form that is very
     close to a parabola.
   ** If the curve is sufficiently close to a parabola, we can jump
      directly to the parabola plotting code.  Again, the well-behaved
      parabola plotting algorithm basically just does a straight-line
      plot in t-values, with the number of steps equal to the number
      of positions between the endpoints.
   ** Otherwise, if the curve isn't close enough, we instead must use
      the slightly more expensive but iterative and still fairly
      well-behaved plotting code for near-parabolas, adjusting for
      differences in t-plotting density.  This algorithm was described
      above.

3. Finally, in the parabola-plotting and near-parabola-plotting
   algorithms, we also do a check to see if the change in angle is
   sufficiently small over a large sweep of the curve to simplify the
   plotting algorithm to a line plot algorithm.  Again, the line plot
   algorithm was already described by this code.

Change in angle or max distance error?  That of course is the question
to ask the programmer.  If the programmer goes by max distance error,
then the simplify-to-line code is almost invariably limited to only
short distances, whereas the simplify-by-angle algorithm is
proportional to the overall shape of the object.  Of course, this
depends on the user's target application, mere decorative visual
graphics or precise simulations.

* Good point!  Another way to compute a Bezier circle.  Start by
  plotting circle points to a requisite density, then recursively
  simplify a spline until the max error bounds are reached.

NOTE: In order to determine whether you should reduce a parabola down
to a series of lines, you need to render the parabola recursively in
wider steps.

NOTE: For very low-resolution Bezier renderings, the best performance
is achieved when using non-recursive algorithms whenever possible.
Perhaps recursive algorithms should only be used for rendering really
bad-behaved Bezier curves.

----------------------------------------

Oh yeah.  Good point!  On anti-aliasing.  Basically, there's two
decisions that you both want to consider simultaneously.  Minimizing
the error of the color and minimizing the error of the line position.
If, for example, plotting a line at its perfectly accurate position
would result in huge color error, then you probably want to fudge the
position where the line is plotted at to minimize the color error.  Of
course, you're only allowed to move lines by +/- 0.5 pixels to achieve
this goal.

Anti-aliasing.  You can smudge the shape and position of lines a
little bit, but not too much.  Otherwise, the error will be too
noticeable.  Also, you want similar distances to be smudged by the
same thickness on the canvas, otherwise the user will notice all kinds
of weird rasterization errors.

JJJ TODO Limits/distance measurements on derivatives and rasterization
evaluations.  Explanation of the straight-line case.

Here, here, realize what we are doing.  Basically, we are rasterizing
curves at even intervals such that the region of the curve where the
acceleration is highest will not be undersampled.  Or, in other words,
we are technically oversampling.  So, what were you saying about the
well-behaved parabola case?  That being said, your oversampling
technique is in fact the "right way" to do things.

Okay, let's think of things this way, then.  Rather than plotting a
pixel if the line passes through it, you only plot the pixel if the
line passes through it and the center of the pixel is above the line.
Okay, I tried simulating that on graph paper, and the results were
disasterous.  What simple geometric rule can I state that works for
all cases?  We're trying to create a minimal bounding line.

Only plot the pixel when you get a change in at least one coordinate.
No, we'll still have L-bends in this case.

Try, as hard as possible, to delay plotting the point until you get a
change in both coordinates.  If that is not possible, then relegate to
plotting a pixel with a change in one coordinate and move on.

Aright, perfect!  That works.  But, this means we need a pending pixel
plot buffer if we are planning on plotting multiple points in the same
pixel.

Yes, it works, but looking at the results, I see a bias in which
pixels are plotted versus which pixels the majority of the line passes
through.  Oh, so you want to do things that way?  Well, you can try
that way too then!

Okay, so let's put it this way.  Sometimes, you'll have two candidate
points that you can plot.  Which one do you plot?  The one which the
majority of the line passes through.  Consider in both horizontal and
veritcal directions.  But remember, the way which a straight line
works, it can never pass through three or more pixels.  So you at most
have to consider two potential candidates for plotting, and you only
have to plot one of them.  (The next two potential plot points.  And
sometimes, the previous unpicked potential plot point, if it is
adjacent.)

What about when you have two choices that are exactly equal?  In order
to make the best choice when all other choices are arbitrary, you have
to "look around" a little bit.  Otherwise, you can just choose a
random of the two choices, or use rounding rules.  Look, you should be
totally consistent.  Don't do random, use standard rounding rules.

Filling regions with scan-fill.  What if the bounding line is off the
edge of the viewport?  What you must do is keep track of fill trees
when zooming in on the viewport.  This will allow you to do fast
renders even when zoomed in.

Remember, since we're using integers as coordinates, we're not doing
that silly Macintosh method of saying that lines connect edges between
pixel grid points.  Nope, lines are always on pixel coordinates.  But
what about fill rules?  Okay, fine, let's put it this way.  You can
use that picture only as a mental model of how the rounding algorithms
in fill rules work.  But, we always plot boundaries as pixel points on
an even-spaced integer grid.  Always.

The fact that diagonal DOES NOT count as adjacent for fill rules is
how we improve the rasterization of diagonal lines.  Interesting,
isn't it?  How do you know that?  Why do you use that fill rule?
Visually, it looks more accurate.  Can you show mathematically that
really is so?  Yes, I can compute the thickness of the plotted region
in one dimension, the longer dimension, and I get only "one pixel
thick."  Think of it this way.  The base case is horizontal and
vertical lines, and the most extreme case is diagonal lines.  What is
the relative error in thickness?  sqrt(2).  But that's less than 0.5!
Which means, according to correct rounding rules, we should round that
down to 1.  And sqrt(2)/2?  That's greater than 0.5, so it rounds up
to one.  That's the mathematics behind why this kind of point plotting
makes sense.

For 45 degree diagonal lines, the distance between diagonals of a
pixel is sqrt(2).  But, the width of the diagonal line is only 1.  So,
how many pixels should you plot for the width?  Definitely less than
one!  So, you should never plot more than one pixel thick for diagonal
lines.  And, of course, "one pixel thick" means you're stroking with a
brush thicker than one pixel, which means in one point, you can move
both horizontally and vertically in a filled region.

Diagonal IS adjacent.  sqrt(2) rounds to one unit.  BUT, for line
rasterization and flood fills, things don't quite work correctly, so
we have to make a compromise.  Remember, 45 degree diagonal lines are
the extreme case that we need to handle properly.  They never pass
through more than two adjacent pixels!  You don't want flood fills to
pass right through these, do you?

So, for line rasterization.  You've got your three base cases, and you
want to continue the pattern in the most logical manner in between
those three base cases.  Hence the algorithmic decisions above.  See
my picture `line-pixels.png'.  When the line passes through three
pixels where an "L-bend" could be plotted, we break down the line into
one of three base forms: a horizontal line, a vertical line, and a
diagonal line.  Because we have to have this strict mapping reduction,
that's why we get the particular shape of low-resolution lines.
(Technically we have four base cases.)

45 degree diagonal lines.  Base case, easiest to prove: the line never
passes through any of the other adjacent grid coordinates.

So, what we're literally doing is taking arbitrary straight lines and
recursively subdividing them until we can represent them piece-wise
using the above base-case forms.  Although, I must admit, looking at
my example, it appears that it's not really so much that it's a
"clean" subdivision, but that you can trace out all of those forms
from the straight line and nothing else.  Wait, no, I was wrong.  What
I meant to say is that the start and end points must overlap.  Yeah,
that makes sense if you plot both the start and end points.  If the
lines share the same start and end point.  Or, in other words, of the
next line starts at the same point that the previous line ended at.
Yeah, that makes a whole lot more sense.  If you want to continue the
same line, you must pick up exactly where the last line left off, not
almost where the last line left off.

This is a really great way of putting things!  Now I know how to
oversample line plots yet still get exactly the same results as I
would get from a lower sampling of the line.

* But wait!  This will only work correctly if you uniformly sample the
  line.  Otherwise, points with high plotting bias will be favored in
  place of points with low plotting bias.  Hey, maybe there's a
  solution to this problem too.  Just do a weighted average, the
  weight being the derivative of the curve at that point.  This will
  give slow points the same bias as fast points.  Plus, this way, you
  don't have to realize you've skipped over a fast part of the curve,
  then backtrace to sample it at a higher rate.

** Remember, again, we're using integer and rational arithmetic, so
   backtracing means that you've got to recompute the numerator and
   denominator, which is expensive.

So, how do you build a system such that the language it is written in
can be understood by one who does not know the language?  Throughout
the system, you use consistent "up-pointer" links and symbols.  So,
the user can become familiar with those symbols and figure out how
they serve as a navigational guide.  Eventually, they can follow those
symbols to the "START" point, which uses basic forms to explain
exactly what the more advanced symbols are.  Those and their
interpretation should be straightforward obvious, made obvious through
pattern display and association.  From there, are the rest of the
definitions necessary for the system are made obvious.

If you subdivide a curve, how do you know how much precision to use
for the sub-parts?  However much precision is necessary to result in
absolutely no loss in calculational accuracy.  If you really do want
lossy calculations, you must warn the user in advance.  It should be
there choice whether they want lossy but fast versus lossless
calculations.

Or, put it this way.  Base case.  Even when you are working on
sub-parts of the curve, you never throw away the information that you
used to work with the highest level of the curve.  It's called
continuing the pattern.  You know what?  I like that approach.  I
think that's a great idea, a great way of putting it.

The question, of course, rests in how accurate they want their
calculations to be.

And yes, don't forget.  Arbitrary precision in computers.  Yes,
computers can do it, but they can't do it as fast as fixed precision.

----------------------------------------

Bezier curve too complicated and too big to rasterize at full
accuracy?  No problem, when that is the case, if the user allows, you
only rasterize "pixel-perfect" at a finite resolution, then you use
other lossy rasterization algorithms to interpolate for higher
plotting resolution, albeit lower accuracy.

So, another important question.  What are you using the curves for?
Human visual graphics only?  In that case, you can formulate practical
limitations on the accuracy of the rendering.  But, if you are
transforming graphics (hence providing mathematical input to
additional computer steps), you might not be able to sacrifice
accuracy.

----------------------------------------

Wait, let's think this through.  What are you really doing when you
multiply by the derivative to take a weighted average?  Think about
the shape of the curve.  For parts where there is a steep slope,
you've plotted too few points, so what you are doing by multiplying by
the derivative is "stretching out" these parts.  For parts where there
is a shallow slope, you've plotted too many points, so you are
"squashing" these parts.  When you graph the modified t-x plot, what
do you get?  You should get a straight line!  That means we're only
concerned about the distance between the endpoints on the x and y
values, so why even bother with t?  Because that's the value that ties
the x and y functions together.  But, for the purpose of weighting, if
you can find the x and y values where the curve escapes the pixel,
along with the turning points in the function, you can just do adds
and subtracts to find the proper weighting of the curve in between.

Remember, you're trying to reduce the curve into a piece-wise join of
the four base cases of simple straight lines.  So, given the
candidates, you just have to figure out which piece you should use in
preference to the other pieces.

Since we're assuming we have a "well-behaved curve," straight line
approximations for inner-pixel rendering should be adequate.

But really, why is this adequate?  Because for sub-pixel
rasterization, all pixels end up being plotted on the same pixel.
Okay, /that's/ why it makes sense!

So then, you're saying you have all these neighboring single pixel
rasterizations of the curve, and you're trying to find which one to
use to build the next case of simplest shortest line rasterization.
So, you use the weighing of the single-pixel rasterizations to make
that determination.

Now /this/ is making sense!  You're only going to have more sub-pixel
rasterizations in a single pixel if a line crosses over itself.
Otherwise, the result is always the same: 1.

* No, that doesn't work.  The problem you're encountering is that you
  DID want the length of those sub-pixel rasterizations to matter in
  determining whether you should plot this pixel or that pixel.

* Look, we're trying to keep the math simple here, and you're making
  it more complicated!  How do you get both simple and correct?

* The point is, let's consider this simple case.  If you over-sample a
  line, which samples should you actually plot?  If you over-sample
  only a tiny bit, will you still get exactly the same results?

** Maybe that's a good test case for starters.  The answer is "no."
   If you over-sample only a tiny bit, the rounding WILL change for
   all of the pixels.  If you over-sample at least 2x, there will be
   far less of an issue if you over-sample just a tiny bit more.

* You want to calculate the actual length of the line in the pixel,
  using that complicated arc-length formula that takes into account
  both x and y coordinates.

It would be easiest if you could just "plot all pixels"
indiscriminantly, but that doesn't correlate with the mapping
reduction requirement you mandated earlier.

The problem, again, is that you can have multiple mapping reduction
decisions while plotting the curve, and you've got to choose the best
one by some better-than-random determinant.

So wait, how many choices did you say there were?  Four.  Only four?

In regard to what you were worrying about above.  Again, the problem
is that when we have a portion of the curve that crosses between
multiple pixels, we in effect have an aliasing artifact on our
sampling of the curve.

Derivative of a square root function?  That's complicated.

Okay, here's what we're going to do.  We're going to have to do a
compromise out of necessity.  Don't bother with where the curve may
cross over itself, let's assume that is handled in higher level code
for the recursive subdivision rendering.

Okay, let's think of things this way.  When you have one point that
you know you should definitely plot, there are only two next points
that you could potentially plot.  Which one do you plot?  The one that
is a better approximation of the line, of course!  That is, the one
that has more line length traveling through it.  That is, more points
would naturally be plotted in that pixel.  How do you know for sure
you are making an accurate assertion of this using only a sampling of
the line?

Remember, we're assuming very close to a straight line for this case,
so all that needs to be done is compute the fractional distance
through the pixel.  But how accurate do you need to be?  Is there a
practical limit you can specify for the accuracy?  If you are talking
99 versus 100, do you really need to care?

Here's the answer.  Remember straight line rasterization?  The degree
of precision possible in the slope is directly correlated to the size
of the line.  Bigger lines, more opportunity for finer slopes.  Hence,
you only need to rasterize to the degree of sub-pixel accuracy
necessary to capture the slope and positioning of a line in a
subpixel.  Beyond that, additional rasterization within the correct
error control bounds should not result in a different rounding result,
provided that you adjust for numerically correct results.

Oh yeah, and how many pixels are there rasterized in the line?  That
is the max of the two dimensions.

Okay, first things first, I've got to write the test case for handling
this for straight lines, then we can move onto the curve case, which
is slightly more complicated.

But, let's start by simplifying it!  Base case: well-behaved shallow
parabolas, the same number of pixels are plotted as would be used for
a straight line between the two endpoints?  That is very easy right
off the bat.

So what about when you subdivide?  Here, you must handle fractional
control point positions, as you have previously realized.  So, total
precision = integer + fractional bits of precision.  Then, once you've
got well-behaved curves, you can just simplify to straight lines at
the sub-pixel level.  What about when the curve rises and falls in the
same point?  Simplify!  Don't worry about this, since we're talking
well-behaved shallow curves, so we can basically assume this point to
be so shallow it might as well be a straight line.

Okay, but this doesn't quite factor in accurately capturing the
curvature of the line.  Okay, remember parabolas.  0.75 * y.  Or, in
other words, just add the control point bounds to the distance span
required for precision calculations.

Yes, this all results in very correct curve evaluations, but there's
an overwhelming dilemma in this: large curves become proportionally
more complicated to evaluate at the sub-pixel level.  Well, yes, that
makes sense, because that's how our calculations ended up working with
even straight lines!  So why should be be able to get the "easy way
out" on curves?  Exactly, we shouldn't if we're talking about
mandatory accuracy.  Yes, and this also means no greatest common
factors to simplify your residual calculations.

----------------------------------------

Explain things this way.  Why don't you plot every single point?  Yes,
although you would still be able to show that is mapping reducible to
the base cases given, compared to using jagged lines and a thinner
profile, the mapping reduction of the line is not as accurate to the
ideal line.  So that's the main problem with the other option.

OR, best approach: show what happens if you run the algorithm on a
well-behaved slope like 2:1 and then you tweak the line just a little
bit.  What do you get?  Yeah, exactly, something ugly that seems to
have extra pixels come out of nowhere.  So, that's why.

----------------------------------------

Alright, we're doing good.  You start with equations of lines on graph
paper.  ax + by + c = 0.  Sounds good, right?  Now you want to turn
that graph paper into a pixel grid.  Now how do you rasterize the
line?  Oh, the simplest progression is to start by saying either y =
mx + b or x = y/m + b, depending on the steepness of the line.  Then,
you just evaluate the line at all pixel coordinates.

However, this does not extend nearly so neatly to more complex
polynomial functions for smooth curves, so let's take a detour by
defining lines as a sub-definition of Bezier curves.

For lines, you only have two candidates for the next point to plot.
For parabolas, you have three candidates.  For higher-order curves,
you can have the maximum of 9 candidates, which includes even the
current pixel!

----------------------------------------

Notes:

20160405/https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
20160405/https://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm
20160404/https://en.wikipedia.org/wiki/Midpoint_circle_algorithm

Be CAREFUL about the newest version of the last article, it has a
wacky "Summary" section up front.

Is there really not an equivalent method for Bezier curves?  And I
thought these were supposed to be simpler to evaluate than circles!
Apparently not.  Wait, no, there is.  See below in my mathematical
analysis.

20160404/https://en.wikipedia.org/wiki/Talk:Midpoint_circle_algorithm

Ah ha!  XPaint doesn't work?  What does work?  Wow, and this is a very
interesting path to how I found it!  I found it in the link of the
talk page of the Wikipedia article on the midpoint circle algorithm.
That's really interesting!

20160404/http://kolourpaint.sourceforge.net/about.html
20160404/http://kolourpaint.sourceforge.net/about-product-comparison.html

And the product comparison?  That basically answers all my questions
on why there are so many other painting programs out there that simply
don't work.  But JJJ TODO Make sure you download all such painting
programs mentioned here!

Given that Microsoft Paint was mentioned, what does Wikipedia have to
say about Microsoft Paint?  Oh, that's really interesting.  So that
explains all the .pal files on Old_WA_Desktop in Nadia's folder.  This
was a feature that was dropped from Paint in Windows 98.

20160404/https://en.wikipedia.org/wiki/Microsoft_Paint

A variation on Paint dot net, if you will.  Very new, and unlike Paint
dot net, it is cross platform.  Yeah, but I personally didn't like
Paint dot net.

20160404/https://en.wikipedia.org/wiki/Pinta_%28software%29

So what about PCX?  PC Paintbrush, an early MS-DOS PC painting
program.  It was quite literally based off of early x86 graphics
hardware.  There you go, language in action.  The symbolic format is
based off of the concrete device that makes the means to that end
possible.

20160404/https://en.wikipedia.org/wiki/PCX

*/

/*

----------------------------------------

Parabolas.

y = x^2
(x + 1)^2 = x^2 + 2x + 1

Let's try it out.

  x   x^2  +term
  0   0    1
  1   1    3
  2   4    5
  3   9    7
  4  16    9
  5  25   11
  6  36   13
  7  49   15
  8  64   17
  9  81   19
 10 100   21
 11 121   23
 12 144   25

Sorry, the sum of odd numbers part just makes it so easy it's
addicting.  The sum of odd numbers computes consecutive squares.

And why does this make sense mathematically?  Because we know that
(2x + 1) defines the mathematical sequence of odd numbers.

Okay, that makes sense.

(x + 1)^3 = x^3 + 3x^2 + 3x + 1

That's similarly easy to evaluate using the x^2 optimization above.

= x^3 + 2x^2 + x + (x^2 + 2x + 1)

x
x + 1
2x + 1 = 2*(x + 1) - 1
2(x + 1) + 1 = (2x + 1) + 2
(x + 1)^2 = x^2 + (2x + 1)
(x + 1)^3 = x^3 + (x + 1)^2 + 2x^2 + x
(x + 1)^3 = x^3 + 3x^2 + 3x + 1

3(x + 1)^2 = 3x^2 + 6x + 3
3x + 1 + 3(x + 1)^2 = 3x^2 + 9x + 4

(x + 1)^3

pre_mid_term += 9;
mid_term += pre_mid_term;
x_1_3 += mid_term;

So wait, it looks like I've figured out a more efficient way to
evaluate Bezier curves than the Allegro library's functions.  No need
to compute the derivatives.

No, I believe both method's results are exactly identical.  It's just
that they are presented in a slightly different way.  I've done it
without any reference to derivatives, as this has to be integer
arithmetic, of course!

ddx += dddx;
dx + ddx;
x += dx;

Technically, the derivative method does work and yes, I can say that
it is correct.  However, I'd rather explain things using the algebra
above as it is more direct to explaining the fact that for step-wise
/discrete/ computations, you can simplify the math.  Taking a detour
through the difference quotient and the simplified rule of
differentiation for monomials seems like you're overdoing it on a
conceptual level.

----------------------------------------

Oh yeah!  Ideal steep curve handling for parabolas.  Divide the curve
up into three segments if it is too steep.  Then, you don't need to do
any more subdivision checks.  You can skip right to iterative
rendering.

Okay, after an experiment in my Bezier curves editor, it turned out
that this is actually not the case.  For the two edges, further
subdivision is still necessary to get well-behaved curves, even after
you handle the case for skinny curves.  Don't even bother with the
special middle subdivision, normal subdivision will do just as fine.

BUT, like you were saying, you need to carry rounding residual from
one segment to the next before you plot the points that join segments
to each other.  You need to have correct base case reduction!  It
should be easy for the calling function to pass around this additional
information.

----------------------------------------

Hey!  I didn't see this?  It was linked from the Wikipedia midpoint
circle algorithm article.

20160407/http://members.chello.at/~easyfilter/bresenham.html

This is great.  This contains the algorithm for totally perfect
rasterization of quadratic Bezier curves.  I already know how to find
the extrema and split up a Bezier curve, so I could run the segments
through that algorithm as the final step.  Or alternately, for curve
analysis, I could somehow run these step-wise algorithms in reverse
for a highly efficient curve analysis.

Okay, but let's try to derive things on my own, similar to how the
midpoint circle algorithm works.

ERROR: I missed a "2" for the b term!

x(t) = a*(1 - t)^2 + b*(1 - t)*t + c*t^2
y(t) = a*(1 - t)^2 + b*(1 - t)*t + c*t^2

x(t) = a - 2at + at^2 + bt - bt^2 + ct^2
x(t) = at^2 - bt^2 + ct^2 - 2at + bt + a
x(t) = (a - b + c)t^2 + (-2a + b)t + a
y(t) = (a - b + c)t^2 + (-2a + b)t + a

But in order to remove the parameter, you have to solve for t.  That
means you'll be using the quadratic formula, and in general, you'll
get two answers.  But, according to the page above, we make some
simplifications so that we only have to consider one of the answers.
This is also the case for the midpoint circle algorithm.  Then, in
order to avoid square roots, we square both sides and work step-wise
one pixel at a time.

The main problem, here, is that we want to have an idea of what degree
of t-precision we need for the calculations.  I think I've covered
that above.  Okay, so we can use a rather different approach for these
equations then.

a = (a_x - b_x + c_x)
b = (-2a_x + b_x)
c = a_x

x = (-b + sqrt(b^2 - 4ac)) / 2a
x = (-(-2a_x + b_x) + sqrt((-2a_x + b_x)^2 - 4*(a_x - b_x + c_x)*a_x)) /
      2*(a_x - b_x + c_x)

Hey, why go through the complexity of squaring both sides from the
quadratic formula when you just did the quadratic formula below?  And
you know what?  You've observed there's a simpler intermediary on the
way to the end of the derivation of the formula.

ax^2 + bx + c = 0
ax^2 + bx = -c
x^2 + b/a*x = -c/a
x^2 + b/a*x + (b/2a)^2 = -c/a + (b/2a)^2
(x + b/2a)^2 = -c/a + (b/2a)^2
x + b/2a = +/- sqrt(-c/a + (b/2a)^2)
x + b/2a = +/- sqrt(-c/a + b^2/4a^2)
x + b/2a = +/- sqrt(-c/a * (4a/4a) + b^2/4a^2)
x + b/2a = +/- sqrt((-4ca/4a^2) + b^2/4a^2)
x + b/2a = +/- sqrt((-4ca + b^2)/4a^2)
x = (-b +/- sqrt(b^2 - 4ac)) / 2a

Don't know what your t-precision should be?  Just say it has to be a
product of the x and y precision displacements.  The other option?
Rather than products and divisors, you just keep track of multiple
independent counters and compare their values.

Anyways, like you were saying.  You need t-precision that is both a
factor of the two axes.  Then, you only store the minimal delta
difference that you still need to travel.  That's to avoid numeric
overflows and such.

----------------------------------------

How do you split and join line segments with correct rounding for
recursive rendering?  That's what I want to know.  Okay, so lines have
candidate endpoints.  You don't know what pixel exactly they will
appear in, but you know they will be localized to only one of a few
choices.  So, using those candidate endpoints, you can determine if
you need to subdivide to get some candidate midpoints or not.  Once
you've got sufficient connectivity, you can make the final evaluation
decisions.  Oh yeah, and you must also adjust for variations in point
density.  Ideally, you want all plot points to be evaluated with
uniform density.

Whoa!  That's complicated.  Yes, complicated, but at the same time,
necessary if you want to get a pixel-perfect plot of a curve.

----------------------------------------

Okay, where did I leave off?  I must be getting into a bad habit of
write-only documents, where my documents are too big to go back and
read upon to sync them up.  But look, I really do know this, sometimes
I really am adding detail that I know to not be found anywhere else!

Anyways, here's how you plot lines in any number of dimensions.
Basically, you cannot use only one accumulator variable, because you'd
have to add and subtract in multiple different dimensions.

* But maybe you can always divide the number of required accumulators
  by two by matching them up into pairs.  Maybe.

Anyways, here's how you do it without the use of subtraction on the
same accumulator variable.

Think of it this way.  You have two different coordinate system maxes,
but you want to be able to step across them at the same rate.  How do
you do this?  The easiest way to get setup is to multiply the two
maxes.  Then, you just single-step in the new dimension.  Every single
time you exceed a threshold point of one dimension, you update the
plot point and plot a point there.  If you exceed both at the same
time, then you update both simultaneously.

Now, for more than two dimensions, this is just as easy, only you
introduce a third axis.  Then you can plot lines in three dimensions
with voxel accuracy.

For rasterizing Bezier curves, you have a bit of a complication
because there is not a linear relationship between the input and the
output.

x(t) = a*(1 - t)^2 + b*2*(1 - t)*t + c*t^2
x(t) = a - 2*a*t + t^2 + 2*b*t - 2*b*t^2 + c*t^2
x(t) = c*t^2 - 2*b*t^2 + t^2 + 2*b*t - 2*a*t + a
x(t) = (c - 2*b + 1)t^2 + (2*b - 2*a)t + a

dt = 2(c - 2*b + 1)t + 2*b - 2*a
ddt = 2(c - 2*b + 1)

Divide `t' by its derivative to get a uniform progression function.
But the problem with this is that the derivative includes `t'!

Alright, how about this.  You have a "compensation vector" that you
use based off of the derivative.

Okay, fine, let's operate off of this realization instead.  De
Casteljau's algorithm.  You slide those points across the linear edges
of the envelope.  And that does what?  That subtends a more precise
slope that is less than one pixel in size.  So, you're already
oversampling.

* But that's not correct!  You'll still miss some points.

Okay, I better understand exactly what that other algorithm is doing
to plot Bezier curves.

----------------------------------------

NOTE: I am concerned about the methods of using the derivative as a
proxy for line lengths.  Don't be.  Think of it this way.  If the
curve is symmetric on both sides, the rounding error will be
equivalent regardless of the evaluation accuracy.

Come on, stop just talking about this, and let's get to work in trying
out an implementation!

Yes, I'll have to program out derivative curve evaluation and see how
it goes.  Then I can work out the kinks interactively.

----------------------------------------

How do you know that a curve really shouldn't have any "L-bends" when
it is plotted?  If it is one of the "well-behaved" curves as covered
in my previous documentation, you should not have to worry about that
being an issue.  The line will head only in one direction, it will not
have any of Runge's phenomenon, and it will not cross over itself.
Therefore, it can be rasterized using the well-behaved curve
algorithms and pixel density comparison.

----------------------------------------

Again, if you only care about proportional details, then you can
render a curve as a series of simpler curves or lines.

----------------------------------------

What if some of your algorithms run out of memory?  Then if you want
to continue, you have to have the permission of the user (the
programmer) to use less memory intensive algorithms.  Otherwise, you
must fail due to lack of calculational accuracy for the purposes of
the including application.

----------------------------------------

TODO Write compact numerical libraries too!  And if asked why?  For
the purpose of minimizing the lines of code and assembly language
size, not so much for maximizing performance on modern architectures.
If that's what you want, use one of the popular modern libre software
libraries to do the same task on modern architectures.  This is for
the sake of portability to simpler computer architectures.

Being able to include all code bundled up into a single portable
assembly language library.  Then, you just wire up the last
connectives.

----------------------------------------

How do you make a clean one-pixel-thick line?  Remember, define what a
boundary is.  A boundary is a path from one adjacent pixel to another.
Adjacent only includes strictly horizontal and vertical neighbors, not
diagonals.  So, the idea is that you want to create a boundary between
two regions that uses the least number of pixels to achieve this goal.
That is your ideal for line rendering.

Again, I reiterate.  We're using integers so that we can exactly
control the required degree of precision.


*/

/* TODO: Speed test of my line and Bezier rendering code in a slow
   computing environment!  Use emulators if necessary!  */

/* NOTE: How to do anti-aliasing with integer arithmetic.  Since you
   must use integers the whole way through, the easiest way is to
   simply divide up a pixel into subpixels, enough to account for all
   luminosity variations.  For example, if you must support 256
   luminosity variations, then you should divide each pixel into a
   grid of 16x16 subpixels.  Then you just do the same rendering
   process and use the pixel proportions to compute the color of the
   target full pixel.  Of course, this kind of rendering doesn't
   generally make sense in paletted displays, but when you extend it
   with your color margin of error logic (try to minimize the color
   "aliasing" of colors that don't actually exist in the source
   image), then it can be made to work quite well actually.  */

/* Important!  How do you rasterize various non-circular pen tips?
   Here's how, and this applies to circular pen tips too.  You plot
   the pen tip shape at each point of the curve you evaluate.  You
   then try to draw a tangent line between the two pen tips plotted.
   If this line length is greater than the tolerance, you know you
   need to subdivide in order to get greater precision.  */

/* Or, even better, as Donald Knuth did it in METAFONT.  Rather than
   using a brush with a smooth curve, use a polygonal brush.  That
   way, you reduce the problem to merely offsetting the curve for each
   point, and switching which offset curve you use based off of which
   one is farthest out from the curve surface.  */

/* If you can't think of better options now, you could just use brute
   force to fill in all of the offset curves.  Note: This is in effect
   kind of what you have to do to handle both sides of the curve
   around the center of the brush.  So, you have to determine which
   points of extruded curves to use and which ones not to use
   dynamically across the length of the curve.  Or, then again, maybe
   you don't, if you are using bruth force.  Or maybe you should use
   brute force to start, then see what you can go back and simplify.
   All curve segments entirely contained within the filled region can
   be removed.  In the end, you are left with only the boundary curve
   segments.  */

/* Yeah, I'm guessing you're right about the brute force at first,
   then backtrack to simplify method.  It seems to work pretty well,
   but you must take the entire brush shape into account, not just
   extruded subsets of points.  */

/* Good point!  Check for any areas where extruded segments cross over
   each other.  This is a potential transition point.  But how do you
   identify those areas?  Well, you'll be plotting points, and you'll
   know if a point was already plotted!  */

/* So, here's the way I see things.  After you extrude a pen into a
   series of Bezier curve segments, you want to cache the
   computational results to avoid having to do a complex computation
   again.  */

/* Scan fill a region.  The boundary line that is plotted is either up
   or down or neutral.  Then, you determine how you should scan-fill
   once you cross that boundary line.  That is, one that has a
   particular directionality, of course.  The neutral denotion is used
   to simplify the plotting algorithms.  Then, the rounding algorithms
   can determine whether to fill at that point or not.  */

/* Yes, yes, yes.  When you use circular brushes, all the calculations
   are easier.  */

/* To simplify recursive curve plotting.  When doing the plot, you tag
   each pixel with an ID number (or just "undetermined" in the case of
   non-parallel plots), then once you're done, you finish off by
   assigning the directionality tags by connecting the pixels in
   order.  */

/* Yes, regions you should call these things.  First you create a
   region based off of an outline, a region that is not a pixel
   buffer, then you can transform an outline region into a pixel
   region, with areas filled and transparent.  */

/* My motto: Carefully designed software to account for any and all
   numeric precision errors that must be inevitable.  But, be careful.
   This may mean that the data associated with some points will only
   grow larger in size.  To avoid this, you need to be a carefully
   trained editor who keeps in mind the residual data stored with each
   point to avoid errors in numeric precision, and discard the extra
   data when it is no longer necessary.  */

/* I guess I"m going to have to create an entire graphics software
   stack to suit my needs.  Plus, I'll be able to integrate that with
   Asman!  */

/* Better yet, create two different rotation and scaling tools: no
   precision loss mode and lossy mode.  State that lossy mode is the
   default for most software, and for beginners, will be good enough,
   but lossless mode is when you input some original measurement data
   in digital form and its important that the original lengths as
   measured are preserved.  However, the transformed data may not be
   necessary to preserve.  For this, often times the accepted practice
   is to use the original plus a transformation matrix, whose
   coefficients are lossy.  */

/* Most commodity graphics software does not take precision to this
   degree.  Rather, it is of a more ad-hoc informal design.  That's
   what they call "artistic"?  I guess so.  The really odd thing about
   this is that artists too need to do things to a certain degree of
   precision too, it's just that the degree of design precision is
   dictated by the artist rather than the laws of physics.  In the
   end, I think it's just marketing, albeit an inopportune kind of
   marketing.  */

/* How about this?  Visually tag points with a transform stack so that
   the user knows how much extra data is associated with particular
   points.  Or, let's make it more general.  Allow the user to tag any
   data to points.  Yes, free-form data tagging is good.  That's
   always something that computer users want to make computers and
   software easy to use.  */

/* To avoid rendering errors.  How do you figure out the highest
   pixel?  You do a binary search!  Oh, that makes sense.  Then once
   you find the extrema using the binary search, you can just use
   binary subdivision rendering to fill in the rest of the pixels.
   Really, in our case, only one kind of extrema matters: the max
   perpendicular distance from the line that connects the two
   endpoints.  Control points beyond the endpoints, not between them?
   Well, we've covered that we subdivide to solve that problem.
   Mandatory subdivision.  */

/* Evaluate a 2D cubic Bezier using only integer/fixed-point
   arithmetic, and store the output in `r'.  This only works for SMALL
   integers that will not overflow 32 bits in the intermediate
   calculations!

   I intentionally didn't use "long long" types here.  After all,
   basic music should not require such precise Bezier curve
   calculations!

   I must admit, though, that using integer arithmetic made the
   programming of this calculation more complicated.  Then again, the
   extra complexity allows for additional flexibility in being able to
   exactly control the precision of the fractional calculations.

   Remember, the cubic Bezier equation is the following:

   a*(1 - t)^3 + b*3*(1 - t)^2*t + c*3*(1 - t)*t^2 + d*t^3

   Note: The rasterizers that use this function repeatedly are
   inferior to other rasterization methods.  However, I'm leaving this
   code in here for posterity.  (I did have to take the time to write
   it, after all.)  */
void
cubic_bezier_eval(Coord2D *r, Coord2D t, Coord2D max,
		  Coord2D a, Coord2D b, Coord2D c, Coord2D d)
{
  long ix, iy;
  long x1_t = max.x - t.x;     long y1_t = max.y - t.y;
  long x1_t_2 = x1_t * x1_t;   long y1_t_2 = y1_t * y1_t;
  long x1_t_3 = x1_t_2 * x1_t; long y1_t_3 = y1_t_2 * y1_t;
  long xt_2 = t.x * t.x;       long yt_2 = t.y * t.y;
  long xt_3 = xt_2 * t.x;      long yt_3 = yt_2 * t.y;
  ix = (a.x * x1_t_3) + (b.x * 3 * x1_t_2 * t.x) +
    (c.x * 3 * x1_t * xt_2) + (d.x * xt_3);
  iy = (a.y * y1_t_3) + (b.y * 3 * y1_t_2 * t.y) +
    (c.y * 3 * y1_t * yt_2) + (d.y * yt_3);
  r->x = ix / (max.x * max.x * max.x);
  r->y = iy / (max.y * max.y * max.y);
}

/* TESTING of basic Bezier rendering code.  */
void
test_bezier_curve()
{
  unsigned i;
  memset(&roll_buffer, ' ', sizeof(roll_buffer));
  cur_roll_line = 73;
  for (i = 0; i <= 100; i++) {
    Coord2D t = { i, i };
    Coord2D max = { 100, 100 };
    Coord2D a = { 0, 0 }, b = { 24, 15 }, c = { 48, 15 }, d = { 72, 0 };
    Coord2D r;
    cubic_bezier_eval(&r, t, max, a, b, c, d);
    if (r.x >= 0 && r.x < ROLL_WIDTH && r.y >= 0 && r.y < ROLL_HEIGHT)
    roll_buffer[r.y][r.x] = 'X';
  }
  flush_piano_roll();
}

/* Actually that's rather a good idea.  What about when the height
   versus width of the curve does not make a well-behaved case for
   x-oriented evaluation?  Well, pretend that you're working with a
   squashed curve that does evaluate nicely instead!  Yes, simple and
   cheap, but not optimal.  */

/* Yes!  This is it!  Good way to compute the inverse without having
   to do an addition.  So, you have some max value.  You compute the
   derivative and add to an accumulator.  Every time that accumulator
   is exceeded, you plot a point.  This will result in more points
   being plotted in areas of higher acceleration and less points
   plotted in areas of lower acceleration.  Remember, in the case of a
   parabola, the highest acceleration in the axis of symmetry is at
   the endpoints.  And, when both endpoints are on the same side of
   the curve, you'll cross over the y-axis length twice.  But, in
   well-behaved cases, what is the proportion of the actual extrema
   compared to the equation?  Let's do a calculation.

   t = 0.5
   b_y = c_y
   a_y = d_y = 0

   a*(1 - t)^3 + b*3*(1 - t)^2*t + c*3*(1 - t)*t^2 + d*t^3
   = a * 0.125 + b * 3 * 0.125 + c * 3 * 0.125 + d * 0.125
   = 0.125 * (a + 3b + 3c + d)

   Considering only the y-coordinates:
   0.125 * (a + 6y + d)
   = 0.125 * 6y
   = 0.75 * y

   Wow, that's really convenient!  3/4 * y defines the extrema for
   this special case.  Actually, this can be made to practically
   always be the case by subdividing irregular Bezier curves to make
   them sufficiently close to this case for rounding errors to be
   negligible.  Then, you can finish off with the fast evaluation
   methods.

   So what scale do you use for the square root and square operation?
   You use the scale in y-axis pixels in one direction.  Oh, that
   makes sense.

   Very good, very good handling of splines we have here.  Textbook
   perfect, I must say.  */

/* If the control points are close enough to the line between the
   endpoints and are approximately equally spaced, then you can
   simplify to just evaluating as a straight line.  */

/* In terms of accuracy and performance, this appears to be the best
   method.  If you want more of one than the other, then you just tune
   the tolerances that you use for the various decision points between
   the algorithms.  */

/* Or, let's put it this way for parabolas.  The number of points that
   share the same y-coordinate are negligible, therefore, you should
   just set the X and Y evaluation density to be the same.  Oh, that
   makes sense.  That really helps explain my observation below in
   relation to evaluation density!  So, just set evaluation density to
   the length of the line through the endpoints.  */

/* So you know, adding up the derivative?  What value do you use as
   the max?  The height of the curve, of course.  */

/* Note: Tension of curves near endpoints versus center.  */

/* Note: Is is really okay for us to remove the boundary checks?  Yes
   Here's why.  We only move an integer number of coordinates.  So,
   whatever fractional number we add to the start, the number of
   integer boundary crossings will be the same regardless.  Try
   playing around with the diagram below, varying the number of spaces
   in front of the X's and counting the number of cells the "line"
   crosses.

   |   |   |   |   |   |
   X   X   X   X   X

*/

/* Efficient line rasterizer algorithm.  Add to an accumulator in
   increments of `dy', plotting an x pixel each time, until the value
   `dx' is met or exceeded, then subtract `dx' and plot y pixels until
   the accumulator is less than `dx', then repeat.  Main advantage of
   this algorithm: it does not need to divide to compute the slope, so
   it does not suffer from the related numerical and performance
   problems from doing so.

   If `line_only == true', then this is a straight line rasterizer
   only, using `cpts[0]' and `cpts[3]' as the starting and ending
   points.  (The intermediate points should be copies of either the
   starting or ending points in this case.)  If `line_only == 0', then
   this line rasterizer is used to drive an efficient cubic Bezier
   curve rasterizer instead.

   NOTE: The anisotropic Bezier curve rasterizer proves to be inferior
   to the recursive Bezier curve rasterizer.  Why is this the case?
   In the simplest possible explanation terms, think of it this way.
   A line only climbs horizontal or vertical in one direction, but a
   curve might climb in one direction, then turn around and go back
   the other direction.  And, a curve might make a sharp climb near
   the beginning, but then only a gradual climb near the end.  Thus,
   some parts of the curve may need to be evaluated with higher
   `t'-precision in one dimension, even if only for a very short
   `t'-interval.

   Note that I'm leaving the anisotropic Bezier curve rasterizer code
   in here for posterity, even though I could have just cut it
   out.  */
void
rasterize_line_internal(bool line_only, Coord2D cpts[4])
{
  Coord2D p1, p2;
  Coord2D min, max;
  Coord2D dl; /* Deltas */
  Coord2D dp; /* Store the sign of the deltas separately.  */
  int accum;
  Coord2D pi; /* Interpolated point */

  { /* Determine the bounding box.  */
    unsigned i;
    min.x = cpts[0].x; min.y = cpts[0].y;
    max.x = cpts[0].x; max.y = cpts[0].y;
    for (i = 1; i < 4; i++) {
      if (cpts[i].x < min.x) min.x = cpts[i].x;
      if (cpts[i].y < min.y) min.y = cpts[i].y;
      if (cpts[i].x > max.x) max.x = cpts[i].x;
      if (cpts[i].y > max.y) max.y = cpts[i].y;
    }
  }

  /* Setup `p1' and `p2'.  */
  if (line_only) {
    p1.x = cpts[0].x; p1.y = cpts[0].y;
    p2.x = cpts[3].x; p2.y = cpts[3].y;
  } else {
    p1.x = 0; p1.y = 0;
    p2.x = max.x - min.x; p2.y = max.y - min.y;
    /* See the reason I've mentioned above for this.  Actually, doing
       this turns this evaluation algorithm into the best of its
       breed, without any special handling to avoid "L-bends."  Okay,
       this is really great.  This means I can use the addition-based
       derivative form for evaluation.  Calculate the distance between
       the endpoints, and use that as your `t' precision evaluation
       value.  */
    /* Okay, this doesn't quite work for diagonal curves, or for
       curves with extreme heights.  I guess I'm saying use the max
       distance, then always evaluate non-anisotropically.

       Or again, if the distance is too far, subdivide.  Which means
       if the height is greater than one third the width (any higher
       would result in slopes greater than 1, which means we need more
       than one y sample for a single x sample).  Then you'll
       eventually get something that is fairly close to a line.  */
    /* Yeah, that makes sense.  The further away the curve is from
       otherwise a straight line, the more special handling you need
       to do on the curve.  */
    /* But, you can also figure out in advance how many times you're
       going to need to subdivide the curve based off of how well the
       control points are positioned.  Well, yes, you're right, sort
       of.  But the thing is, the subdivision is not uniform, so yes,
       you still need to do the recursive algorithm for optimal
       performance.  That is, so you can get the maximum benefits from
       the simpler, more performant algorithms doing the greatest
       amount of work possible.  */
    /* Integer square roots?  You can do that using a lookup table.
       No, you can do that using a spline.  Oh my, now we're drawing
       circles around ourselves.  Speed or accuracy?  If that's the
       case, we might as well go for a simpler algorithm even if it
       means over-evaluation.  No, we can't do that.  Okay, fine.  */
    /* To avoid square rooting, you can compare distance squared.  But
       then, you need to take into account numeric precision limits
       because of this.  Rasterize a line to compute the distance.
       Oh, that's a good one.  Then you can bucket the results into +1
       and +1.414..., and do bit shifting to add the 1.414... results
       to the unit results.  Okay, that's admittedly slow.  */
    p2.y = p2.x;
  }

  dl.x = p2.x - p1.x; dl.y = p2.y - p1.y;
  dp.x = (dl.x > 0) | (dl.x >> 31); dp.y = (dl.y > 0) | (dl.y >> 31);
  /* Make sure that `d' only contains positive values.  */
  dl.x = ABS(dl.x); dl.y = ABS(dl.y);
  /* Make sure we do correct rounding!  Initialize accumulator to
     "1/2" (metaphorically speaking), not zero.  */
  accum = (dl.x >> 1) + (dl.x & 1);

  pi.x = p1.x; pi.y = p1.y;
  while (((pi.x >= min.x && pi.x <= max.x) ||
	  (pi.y >= min.y && pi.y <= max.y)) &&
	 (pi.x != p2.x || pi.y != p2.y)) {
    if (line_only)
      roll_buffer[pi.y][pi.x] = 'X';
    else {
      Coord2D r;
      cubic_bezier_eval(&r, pi, dl, cpts[0], cpts[1], cpts[2],
			cpts[3]);
      roll_buffer[r.y][r.x] = 'X';
    }
    if (accum < dl.x) {
      pi.x += dp.x;
      accum += dl.y;
    }
    if (accum >= dl.x) {
      pi.y += dp.y;
      accum -= dl.x;
    }
  }
  if (line_only)
    roll_buffer[pi.y][pi.x] = 'X';
  else {
    Coord2D r;
    cubic_bezier_eval(&r, pi, dl, cpts[0], cpts[1], cpts[2], cpts[3]);
    roll_buffer[r.y][r.x] = 'X';
  }
}

void
rasterize_line(Coord2D p1, Coord2D p2)
{
  Coord2D cpts[4] = { p1, p1, p2, p2 };
  return rasterize_line_internal(true, cpts);
}

void
rasterize_bezier(Coord2D a, Coord2D b, Coord2D c, Coord2D d)
{
  Coord2D cpts[4] = { a, b, c, d };
  return rasterize_line_internal(false, cpts);
}

/* TODO: Implement the iterative evaluator that computes the
   derivatives via integer arithmetic and additions to compute an
   equal-t-spaced curve.  Again, this algorithm has similar numeric
   problems to the algorithms above.  */

/* TODO: Implement quadratic Bezier evaluation too.  */

/* TODO: Implement the evaluators in 3D and all that 3D math stuff
   too.  */

/* TODO: Implement useful NURBS tools.  */

/* TODO: Implement Wavefront OBJ file loading and saving.  */

/* TODO: Implement a system for generating the 2D graphs that you
   want, with nice Beziers for curved geometry.  */

/* TODO: Implement tangent and splines and fitting Beziers over
   patches of a mesh geometry.  Fill in the wireframe with NURBS.  */

/* TODO: Generate NURBS for cylinders and spheres at the correct
   accuracy.  */

/* TODO: CSG for NURBS!!!  */

/* TODO: Convert arbitrary meshes to NURBS.  Interactive curve
   simplification for NURBS!  */

/* TODO: Memory storage consumption meter for 3D geometry
   specifications.  For the artist.  */

/* For the house measurements.  Define line of length, angle, and
   anchoring with a specific point, referencing that point when
   applicable, or specifying an arbitrary point.  This way, you know
   that there is no loss in precision of the line lengths.  */

/* NOTE: How do you position the control points on a Bezier to get a
   straight line with uniform evaluation density?  Position the two
   control points 1/3 and 2/3 between the endpoints.  Suppose the
   endpoints form a horizontal line.  If you offset the control points
   vertically by the same distance, you get a parabola.  If you change
   the control point offset?  You get a linear sheared parabola.  */

/* The problem with dynamically dividing by the derivative to
   determine the evaluation density.  But then you need dynamic
   compensation.  That means that you cannot have nice,clean, looping
   algorithms.  Hey!  Maybe you can!  Look at the simplification
   mentioned just above!  If you can show that a curve is reasonably
   close to that simplification, you can just use simple square roots
   and such to evaluate a curve at an even density.  Hey, I just made
   up the proof for why square root works so well in the Allegro
   spline code!  Moral of the story: there's a lot that common people
   don't know about mathematics, even though its simplicity is in the
   way it governs the world around us, being ubiquitous, but,
   unfortunately, not fully understood by most humans.  Again, what
   you are now saying about both humans and computers, it is not
   necessary for a human or computer to fully understand and be able
   to explain what they do for them to be able to do it.  Very easy to
   explain using a computer analogy.  Though a computer may not
   contain a full digital copy of its documentation, that doesn't stop
   a computer from operating.  */

/* TODO: Implement drawing with brushes in 3D.  */

/* TODO: Implement snapping.  */

/* TODO: Implement CSG.  */

/* TODO: Why, implement your entire own graphics and CAD suite!
   Create libraries and tie with existing industry established
   standards as much as possible.  Oh, how I wish better file format
   converter libraries and feature categories were implemented with
   libre graphics software like they are with libre sound software.
   `libsndfile'.  I guess that's just too bad.  */

/* Compute the derivative to find out which parts of the curve need to
   be evaluated with higher t-precision.  The problem here is that
   when you need to evaluate with higher t-precision, you need to
   recompute the derivatives using finer grained increments.  But that
   is in fact the nature of a curve.  Okay, maybe not.  Anyways, the
   ideal is that you have the distance-based derivative of the curve
   be identical across its entire evaluation, but since this is not
   the case in Bezier curves, you have to do some extra compensation
   calculations to make this the case.  */

/* Again, like you were saying, the problem of all the problems in
   computer graphics is speed.  When speed is all that matters, people
   are willing to sacrifice quality for this.  Unfortunately, this
   ends up spiraling too far and even speed and efficiency end up
   getting sacrificed for implementation simplicity.  So, there you
   have it.  Why triangle meshes?  It's easier to implement hardware
   acceleration for this.  Why poor un-even evaluation of Bezier and
   NURBS surfaces?  It's faster to evaluate the curve this way, and
   besides, all the curve points will just get passed to the GPU for
   actual rendering, so it is in one's best interest to keep things as
   simple as possible on the CPU.  Why do you need so much GPU memory
   for meshes?  Because, all the curve evaluation was happening in
   advance on the CPU, and there's no way it could have been happening
   on the non-programmable GPU.  Why does this problem still keep up
   even with GPGPUs?  Because, people have to use legacy software, and
   the legacy software implementation was less efficient; therefore,
   continued use of that software has to be inefficient too.

   Wow!  Graphics software is basically a long list of problems.  But,
   this isn't just any list of problems.  It is a long list of
   problems that go unsolved because of lack of general business
   applicability.  For example, why would an accountant care about
   graphics rendering?  Sure, they might care about polynomial curves,
   but for the specific use case of graphics rendering, it doesn't
   matter to them, because their main job target does not need the
   results of that kind of work.  And so the same with 90% of the rest
   of the business world or so.

   I still think it's annoying, though, because sometimes, you really
   do need to use graphics.  */

/* Bits After Decimal for recursive Bezier rasterizers.  This was kind
   of a magic number selected based off of what produced similar
   results to the iterative evaluation rasterizer.  */
#define BAD 7

/* Rasterize a Bezier curve by recursively subdividing it, as opposed
   to calling `cubic_bezier_eval()' for iterative values of `t'.  This
   is known as De Casteljau's algorithm.  This has the benefit of
   better numeric stability, even though it comes at the cost of
   requiring a recursion stack.  Also, according to Wikipedia, this is
   slower than the direct approach for most architectures.

   This particular algorithm also has the caveat of not accurately
   rasterizing Bezier curves with extreme control point positioning.

   Solution (to be carried out by a pre-processor): Start by analyzing
   the curve to determine if it has extreme positioning of the control
   points (that is, very far from a plain linear line evaluation), and
   if it does, start by pre-subdividing it until you get a series of
   near-linear curves.  Then, recursive evaluation can be performed in
   a numerically stable manner.  Incidentally, this also significantly
   helps for the iterative Bezier evaluator.  */
void
raster_bez_rec_internal(Coord2D a, Coord2D b, Coord2D c, Coord2D d)
{
#define CALL_STACK_SIZE 31 - BAD
#define DO_S1 1
#define DO_S2 2
  unsigned sp = 0; /* Stack position */
  struct CallStack_tag {
    Coord2D a, d;
    Coord2D ab, bc, cd;
    Coord2D abbc, bccd;
    Coord2D curve_point;
    unsigned eval_flags; /* DO_S1 and DO_S2 */
  } cs[CALL_STACK_SIZE]; /* Call stack */
#define MIDPOINT(r, a, b) \
  ({ (r).x = ((a).x + (b).x) >> 1; \
     (r).y = ((a).y + (b).y) >> 1; })

  /* Special case to avoid extra plot points ("L-bends"): If `a' and
     'd' are adjacent diagonally, do not try to plot any
     sub-curves.  */
  if (ABS((a.x >> BAD) - (d.x >> BAD)) == 1 &&
      ABS((a.y >> BAD) - (d.y >> BAD)) == 1)
    return;
  /* 1. Compute the midpoints of the three line segments formed by
        connecting the control points in the order (a, b, c, d).  */
  cs[sp].a.x = a.x; cs[sp].a.y = a.y;
  cs[sp].d.x = d.x; cs[sp].d.y = d.y;
  MIDPOINT(cs[sp].ab, a, b);
  MIDPOINT(cs[sp].bc, b, c);
  MIDPOINT(cs[sp].cd, c, d);
  /* 2. Compute the two midpoints of the two lines that connect the
        points computed just above in the order (ab, bc, cd).  */
  MIDPOINT(cs[sp].abbc, cs[sp].ab, cs[sp].bc);
  MIDPOINT(cs[sp].bccd, cs[sp].bc, cs[sp].cd);
  /* 3. Compute the midpoint of the line (abbc, bccd).  This is a
     point (t = 0.5) on the Bezier curve, and the juncture between two
     sub-curves.  Therefore, we can plot this point.  */
  MIDPOINT(cs[sp].curve_point, cs[sp].abbc, cs[sp].bccd);
  /* Note: I have previously considered rounding before rendering, but
     that appears to produce worse results than just truncating.  */
  roll_buffer[cs[sp].curve_point.y>>BAD]
    [cs[sp].curve_point.x>>BAD] = 'X';

  /* 4. The inner-most "convex hulls" (only guaranteed convex when
     shape "abcd" is also convex) that are coincident on `curve_point'
     define the control points of the two sub-curves.  Recursively
     rasterize those if there is still "rasterizable spacing" between
     the points `a' and `d' in respect to the sub-curves.  */
  /* Note: For efficiency while maintaining code readability,
     we use `#define' rather than defining new C variables.  */
#define S1_A cs[sp-1].a
#define S1_B cs[sp-1].ab
#define S1_C cs[sp-1].abbc
#define S1_D cs[sp-1].curve_point
#define S2_A cs[sp-1].curve_point
#define S2_B cs[sp-1].bccd
#define S2_C cs[sp-1].cd
#define S2_D cs[sp-1].d
#define RASTER_SPACING(a, b) \
  (ABS(a.x - b.x) > (1 << BAD) || \
   ABS(a.y - b.y) > (1 << BAD))
  sp++;
  cs[sp-1].eval_flags = 0;
  if (RASTER_SPACING(S1_A, S1_D))
    cs[sp-1].eval_flags |= DO_S1; 
  if (RASTER_SPACING(S2_A, S2_D))
    cs[sp-1].eval_flags |= DO_S2;
  if (cs[sp-1].eval_flags == 0)
    sp--;

  /* NOTE: In order to have efficient looping code with minimal
     variable duplication, we have to do a little bit of repetition of
     the code above.  However, this time, we don't include the
     comments to save space.  */
  while (sp > 0 && sp < CALL_STACK_SIZE)  {
    if (cs[sp-1].eval_flags & DO_S1) {
      cs[sp-1].eval_flags &= ~DO_S1;
      if (ABS((S1_A.x >> BAD) - (S1_D.x >> BAD)) == 1 &&
	  ABS((S1_A.y >> BAD) - (S1_D.y >> BAD)) == 1)
	continue;
      cs[sp].a.x = S1_A.x; cs[sp].a.y = S1_A.y;
      cs[sp].d.x = S1_D.x; cs[sp].d.y = S1_D.y;
      MIDPOINT(cs[sp].ab, S1_A, S1_B);
      MIDPOINT(cs[sp].bc, S1_B, S1_C);
      MIDPOINT(cs[sp].cd, S1_C, S1_D);
    } else if (cs[sp-1].eval_flags & DO_S2) {
      cs[sp-1].eval_flags &= ~DO_S2;
      if (ABS((S2_A.x >> BAD) - (S2_D.x >> BAD)) == 1 &&
	  ABS((S2_A.y >> BAD) - (S2_D.y >> BAD)) == 1)
	continue;
      cs[sp].a.x = S2_A.x; cs[sp].a.y = S2_A.y;
      cs[sp].d.x = S2_D.x; cs[sp].d.y = S2_D.y;
      MIDPOINT(cs[sp].ab, S2_A, S2_B);
      MIDPOINT(cs[sp].bc, S2_B, S2_C);
      MIDPOINT(cs[sp].cd, S2_C, S2_D);
    } else {
      /* No more work left to do, pop this stack frame.  */
      sp--; continue;
    }
    MIDPOINT(cs[sp].abbc, cs[sp].ab, cs[sp].bc);
    MIDPOINT(cs[sp].bccd, cs[sp].bc, cs[sp].cd);
    MIDPOINT(cs[sp].curve_point, cs[sp].abbc, cs[sp].bccd);
    roll_buffer[cs[sp].curve_point.y>>BAD]
      [cs[sp].curve_point.x>>BAD] = 'X';
    if (cs[sp-1].eval_flags == 0) {
      /* No work left to do, we can overwrite the previous stack
	 frame.  */
      memcpy(&cs[sp-1], &cs[sp], sizeof(struct CallStack_tag));
      sp--;
    }
    sp++;
    cs[sp-1].eval_flags = 0;
    if (RASTER_SPACING(S1_A, S1_D))
      cs[sp-1].eval_flags |= DO_S1; 
    if (RASTER_SPACING(S2_A, S2_D))
      cs[sp-1].eval_flags |= DO_S2;
    if (cs[sp-1].eval_flags == 0)
      sp--;
  }
  if (sp >= CALL_STACK_SIZE) {
    printf("Call stack exceeded!  %s\n", __FUNCTION__);
    return; /* Error: Stack overflow.  */
  }
#undef CALL_STACK_SIZE
#undef DO_S1
#undef DO_S2
#undef RASTER_SPACING
#undef S1_A
#undef S1_B
#undef S1_C
#undef S1_D
#undef S2_A
#undef S2_B
#undef S2_C
#undef S2_D
}

void
rasterize_bezier_recursive(Coord2D a, Coord2D b, Coord2D c, Coord2D d)
{
  /* Plot the endpoints right away.  */
  roll_buffer[a.y][a.x] = 'X';
  roll_buffer[d.y][d.x] = 'X';
  /* Start by giving ourselves a little bit of fixed-point precision
     after the decimal point.  */
  a.x <<= BAD; a.y <<= BAD; b.x <<= BAD; b.y <<= BAD;
  c.x <<= BAD; c.y <<= BAD; d.x <<= BAD; d.y <<= BAD;
  /* In order to get correct rounding to screen coordinates, add 0.5
     to each coordinate in advance (rather than throughout the
     computations).  */
  a.x += 1 << (BAD - 1); a.y += 1 << (BAD - 1);
  b.x += 1 << (BAD - 1); b.y += 1 << (BAD - 1);
  c.x += 1 << (BAD - 1); c.y += 1 << (BAD - 1);
  d.x += 1 << (BAD - 1); d.y += 1 << (BAD - 1);
  return raster_bez_rec_internal(a, b, c, d);
}

/* Rasterize a line as per inverted equations and common t-value
   derivation.  */
void
alt_rasterize_line(Coord2D a, Coord2D b)
{
  Coord2D size = { ABS(a.x - b.x), ABS(a.y - b.y) };
  Coord2D sign = { (b.x - a.x >= 0) ? 1 : -1,
		   (b.y - a.y >= 0) ? 1 : -1 };
  unsigned tmax = size.x * size.y;
  /* Choose the min between size.x and size.y, as that will result in
     the max number of steps being plotted.  */
  unsigned step_size = (size.x < size.y) ? size.x : size.y;
  unsigned t = 0;
  Coord2D ofs = { 0, 0 };
  /* To plot EVERY conceivable point from the line equation, use
     step_size = 1.  In general, this is undesirable rasterization
     behavior.  */
  /* step_size = 1; */
  /* So what does it mean to avoid step sizes too small?  You never
     plot denser than the highest minimum sampling frequency.  */
  for (t = 0; t < tmax; t += step_size) {
    Coord2D p = { a.x + ofs.x, a.y + ofs.y };
    roll_buffer[p.y][p.x] = 'X';
    ofs.x = sign.x * (t / size.y);
    ofs.y = sign.y * (t / size.x);
  }
}

/* Use two counter variables for rasterization rather than just one
   shared t-variable.  The scales are still calibrated to be
   identical.

   Ooh, this one looks even better than the last.  Yeah, you're right,
   I should have done this one first rather than second.

   The final result with rounding?  That's beautiful, that's perfect.
   I couldn't have done any better than that.  */
void
alt_rasterize_line2(Coord2D a, Coord2D b)
{
  Coord2D size = { ABS(a.x - b.x), ABS(a.y - b.y) };
  Coord2D sign = { (b.x - a.x >= 0) ? 1 : -1,
		   (b.y - a.y >= 0) ? 1 : -1 };
  unsigned tmax = size.x * size.y * 2;
  Coord2D step_size = { size.y * 2, size.x * 2 };
  Coord2D t = { size.y, size.x };
  Coord2D ofs = { 0, 0 };
  while (t.x < tmax || t.y < tmax) {
    Coord2D p = { a.x + ofs.x, a.y + ofs.y };
    roll_buffer[p.y][p.x] = 'X';
    if (t.x <= t.y)
      { t.x += step_size.x; ofs.x += sign.x; }
    if (t.y < t.x)
      { t.y += step_size.y; ofs.y += sign.y; }
  }
  /* Make sure we plot the last point.  (Unless we want to behave like
     Win32 API.)  */
  roll_buffer[b.y][b.x] = 'X';
}

/* Highly experimental parabola rasterizer, based off of a similar
   concept of `alt_line_rasterizer()' above.  The only way to know if
   it really works is to try it out!

   y = x^2

   x     x^2     2x + 1    2

   Starting x = -3
   Ending x = 3

   The plot points will be shifted right by 3 units.

   Okay, it works, but only after heavy modifications and not working
   exactly how I originally intended it to.

*/
void
alt_rasterize_para()
{
  /* int x = -3, x_2 = 9, x2_1 = -5, c = 2; */
  int x = -4, x_2 = 16, x2_1 = -7, c = 2;
  /* x and y pixel window plotting sizes, used for the delta
     thresholds.  */
  Coord2D size = { 8, 32 };
  /* Coord2D size = { 6, 18 }; */
  /* Okay, here's how it works.  Each step we multiply the pixel delta
     by the max size of the opposite dimension.  For straight lines,
     the pixel delta is always 1.  For curves, it may be more than
     one.  This is precisely equal to the value of our (2x + 1) term
     above.  */
  /* Let's view a demonstration:
     x: 1, 1, 1, 1, 1, 1
     y: -5, -3, -1, 1, 3, 5

     So what do we do about the negatives?  We've decided to take the
     absolute value to make them positive.  So this is what we get:

     x_size: 6
     y_size: 18

     So what values do we add in order?

     x: 18 + 18 + 18 + 18 + 18 + 18
     x_max = 108
     y: 6 * 5 + 6 * 3 + 6 * 1 + 6 * 1 + 6 * 3 + 6 * 5
     y: 30 + 18 + 6 + 6 + 18 + 30
     y_max = 108

     Perfect!  That works perfectly!  Why must this work?

     x = 18 * (1 + 1 + 1 + 1 + 1 + 1)
     y =  6 * (5 + 3 + 1 + 1 + 3 + 5)
     x = 18 * 6
     y = 6 * 18

     That's why it must work.

     Okay, I guess you're right.  The reason why this isn't working is
     because I am in effect turning a parabola into a straight line.
     Yes, although that's what I intended to do, the way I am doing it
     here is causing things to not work.

     Oh, I see what I am doing wrong.  With this configuration, I must
     always increment both at the same time, right?  Well, yes.  What
     you've done above is unified the stepping interval.

     But that doesn't actually make sense...
   */

  /* One thing that makes these computations tricky is that the sign
     can (WILL!) change mid-way through the computation.  */
  Coord2D sign = { 1, -1 };
  /* unsigned tmax = size.x * size.y;
  Coord2D step_size = { size.x * x2_1, size.y }; */
  /* Note: size.x appears in these equations only as a placeholder for
     where you would insert the rational denominator by which you want
     to "divide" the curve by.  (That is, stretch it out by
     iteratively computing fractions rather than whole numbers.)  */
  unsigned tmax = size.x * size.y / size.x;
  Coord2D step_size = { x2_1, size.x / size.x };
  /* unsigned tmax = size.x * size.y;
  Coord2D step_size = { x2_1, size.x / 4 }; */
  Coord2D t = { 0, 0 };
  Coord2D ofs = { 0, x_2 };
  while (t.x < tmax || t.y < tmax) {
    Coord2D p = { ofs.x, ofs.y };
    /* Note: We plot with x and y transposed so that we get more
       space.  It also makes it easier to tell if the base of the
       parabola is at zero or not.  */
    /* roll_buffer[p.x][p.y] = '0' + ofs.x % 10; */
    if (t.x <= t.y) {
      /* This is where things get tricky.  It wouldn't be nearly so
         tricky if we didn't have to worry about correctly dealing
         with sign changes.  Also, these special cases unfortunately
         fail when we have to deal with rational curve points.  */
      if (step_size.x > 0) sign.y = 1;
      else sign.y = -1;
      t.x += sign.y * step_size.x; ofs.x += sign.x;
      printf("%d %d %d\n", ofs.x, ofs.y, step_size.x);
      step_size.x += c;
    }
    /* Weird, but we have to plot here for accurate results rather
       than just above where one would think it would make more sense.
       Oh, now I get it, it's some sort of rounding error.  I should
       have known!  I did, after all, go into this code knowing I
       would disable correct rounding.  */
    /* The actual points computed on the parabola are correct, it's
       just that the surronding connectives are rounded differently.
       I'm going to have to decide on a correct rounding mode for
       this.  */
    /* Okay, fine, maybe this is as close to a correct rounding mode
       as I can get for a parabola that is not evaluated using a
       mirror-image algorithm.  In terms of directionality, it is
       correct rounding behavior.  */
    p.x = ofs.x; p.y = ofs.y;
    roll_buffer[p.x][p.y] = '0' + ofs.x % 10;
    if (t.y < t.x)
      { t.y += step_size.y; ofs.y += sign.y; }
  }
}

/*

Oh yeah, don't forget the key point from your thought experiments
earlier on.  Now, you're wondering, how do you coordinate nonlinear
and linear coordinates?  Here's the key.  You can't just multiply in
order to coordinate the spaces.  Any time you multiply, you're just
going to get a linear function.  As in the case of a straight line.
So, you have to do something else, something of higher order powers.
Somewhere in the relationship equation, you've got to have something
raised to a power.

Okay, but if you are combining a linear with a non-linear, that means
that you can have one raised to a power, and the other multiplied,
right?  Wrong.  One has to raise the other to a power, and the other
can multiply.  Something like that.  Okay, that sounds tricky.

Look, there's only two rules to plotting a parabola.  Either you plot
a long stretch of points that are near-linear, or you plot tall steeps
where there are many points stacked up but not much motion.

Okay, I'm almost there.  The final step.  Adding a rational
denominator.  I think I'm there.  You multiply one of the opposites by
the right amounts to get the rational denominator of your choice.

Yes, in the choices above, you multiply y by the denominator x!

Okay, so far, so good, but there's a few rasterization errors of
L-bends that I still have to work out.  It's probably due to my
special cases for sign crossing, that's probably the cause of the
kinks.

Yeah, I think you're definitely right.  It's due to the change in
sign.  However, I tried to improve the troublesome section of code,
but my attempt seems to have failed.  Maybe what I'm seeing is indeed
correct as a matter of fact that we're dealing with the region at a
change of sign.

Okay, see above.  I've solved the problem.  It was due to rounding
error.  Or, should I say more accurately, my intentional decision to
not work on correct rounding in the code at the moment.

*/

/*

Okay, I finally figured out how to explain my solution.  Basically,
what I am doing is I am defining an equation and single stepping the
input variables.  The idea is that I want to step the values from
"0.0" to "1.0."  However, this creates the obvious problem that I'd
end up dealing with fractional values to step across the intermediary
pixels in the space.  So, how do we get rid of the fractions and deal
with only integer arithmetic?  Cross multiply, of course.  Then we
have the equations in terms of only integers.  Okay, let's try it.

Line from (14, 0) to (29, 36).

rise = 36 - 0 = 36
run = 29 - 14 = 15
m = 36/15
y = mx + b
0 = 36*14/15 + b
b = -504/15

y = mx + b
y = 36/15*x - 504/15
15*y = 36*x - 504
36*x - 15*y - 504 = 0

Alright, now we've got our equation.

Hey, maybe I've figured out another way to explain how to initialize
the values.

m = (y2 - y1)/(x2 - x1)
b = y1 - m*x1
y = mx + b
y = (y2 - y1)/(x2 - x1)*x + (y1 - m*x1)
(x2 - x1)*y = (y2 - y1)*x + (y1 - m*x1)
-(y2 - y1)*x + (x2 - x1)*y - (y1 - (y2 - y1)/(x2 - x1)*x1) = 0

Okay, but we don't really need to worry about what the exact value of
c is.  We know what the starting point is, and we only need to compute
deltas, for which the value of c will drop out of the equations.

Then, the goal here, if it is not already apparent, is to single step
the values x and y (or both at the same time) in such a way is to keep
the result as close to zero as possible at every single point plotted
along the length of the line.  Easy as pie.

Yeah, that's basically how the midpoint circle algorithm was
explained, but the Wikipedia page on Bresenham's line algorithm was
more verbose.

*/

/*

So, given the above observations, let's try to solve this for a
parabola.  Or, maybe not solve, but compute something good enough for
a demonstration.

y = x^2
ax^2 + by + c = 0

a(x + 1)^2 + by + c - ax^2 - by - c
= ax^2 + 2ax + a^2 + by + c - ax^2 - by - c
= 2ax + a^2

2a(x + 1) + a^2 - 2ax - a^2
= 2ax + 2a + a^2 - 2ax - a^2
= 2a

ax^2 + b(y + 1) + c - ax^2 - by - c
= by + b - by
= b

a = 1, b = -1, c = 0

x^2 - y = 0
y = x^2

So, do we have any rational parts that we have to remove from
the equation?  Not in this case, but we can recompute with
rational parts in there too for the general case.

b = -1
by = -25
b(y + 1) = by + b

How do we know what direction we want to go?  Up or down?  Seriously,
the only way we can know is from the sign of the other term.  We
know for certain that the other term moves only in one direction,
so we can use that as the determinant of which direction we need
to move the y-term.  I know, this is super-special case handling,
but it helps to get us started.

Alright, let's develop the equations for the rational version.

ax^2/d + by + c = 0
ax^2 + bdy + cd = 0

ax^2 + bd(y + 1) + cd - ax^2 - bdy - cd
= bdy + bd - bdy
= bd

*/

/* Trying to figure out correct rounding...  */
/* #define ABS_1(x) (((x) >= 0) ? (x) : -(x + 1)) */
#define ABS_1(x) (((x) >= 0) ? (x) : -(x))

void
alt_rasterize_para2()
{
  int a = 1, bd = -1;
  int a2 = 2 * a;
  const int x0 = -5;
  int ax2_a2 = 2 * x0 + 1;
  int ax2 = x0 * x0, bdy = -ax2;
  int y = ax2;
  Coord2D ofs = { 0, y };
  unsigned i;
  for (i = 0; i < 52 && ofs.x < 12; i++) {
    int error_sum = ABS_1(ax2 + bdy);
    int new_error_sum1, new_error_sum2;
    int new_ax2, new_bdy;
    int sign_y;
    /* We must always make progress on each iteration.  We use this
       variable to ensure that we single-step at least one of the two
       dimensions.  */
    int no_single_step = 1;

    /* Again, we are transposing here for saving screen space.  */
    printf("%d %d %d %d %d\n", ofs.x, ax2, ax2_a2, bdy, ofs.y);
    roll_buffer[ofs.x][ofs.y] = '0' + ofs.x % 10;

    /* First, figure out which direction the second term is supposed
       to advance in.  We wouldn't need to worry about this if we
       didn't handle sign changes.  */
    if (ax2_a2 < 0) sign_y = -1;
    else sign_y = 1;

    /* Wow, extra complex logic here to make sure we keep advancing in
       a smooth manner.  Really, my code above for parabola plotting
       was certainly simpler, even though I didn't understand it.  */
    /* We need to make sure we try to advance both to the greatest
       extent possible before plotting the pixel to avoid L-bends.  */

    /* Okay, final statement.  We don't need to necessarily calculate
       where we are in t-space, so long as we single-step the
       variables in the equation of the function.  */

    new_ax2 = ax2 + ax2_a2;
    new_error_sum1 = ABS_1(new_ax2 + bdy);
    new_bdy = bdy + sign_y * bd;
    new_error_sum2 = ABS_1(ax2 + new_bdy);

    if (new_error_sum1 < error_sum ||
	(new_error_sum1 >= error_sum &&
	 new_error_sum2 >= error_sum)) {
      ofs.x++;
      ax2 = new_ax2;
      ax2_a2 += a2;
      error_sum = new_error_sum1;
      no_single_step = 0;
    }

    new_bdy = bdy + sign_y * bd;
    new_error_sum2 = ABS_1(ax2 + new_bdy);
    if (new_error_sum2 < error_sum) {
      ofs.y += sign_y;
      bdy = new_bdy;
      error_sum = new_error_sum2;
      no_single_step = 0;
    }
  }
}

/*

For the case of a parabola, you can solve for t using the quadratic
formula and remove the t paramater from both equations to get an
implicit equation.  But for cubic Beziers, it's not so easy.

Okay, here's how to do the t-scale thing.  Compute the derivatives of
the equations, yes, instantaneous.  Then, find the max instantaneous
rate of change.  Use this to setup your t-scales for each equation
separately.  The combined t-scale is the product of these two
quantities.  Then, single-step the output variables, and try to find
out how many single-steps it takes to make the inputs break even, or I
should say with minimal error.  That's how you know what points to
plot, without skipping a beat.  As, the max instantaneous rate of
change will not exceed the delta that you must add to advance by a
single point, you know you'll be in good shape.  And, with your new
single-stepping algorithms, you know how to avoid oversampling, even
when you must oversample the input equations.

*/

/*

Let's try removing the parameter from quadratic Bezier curves.

x(t) = a*(1 - t)^2 + 2*b*(1 - t)*t + c*t^2
= a - 2*a*t + a*t^2 + 2*b*t - 2*b*t^2 + c*t^2
= (a - 2*b + c)*t^2 + (-2*a + 2*b)*t + a

quadratic formula, square both sides to remove radical
a = a - 2*b + c
b = -2*a + 2*b
c = a

x = (-b +/- sqrt(b^2 - 4ac)) / 2a
x^2 = (b^2 - 2*b*sqrt(b^2 - 4ac) + (b^2 - 4ac)) / 4a^2

What?!  We still end up having a square root in there.  Okay, this was
a failed attempt to remove the parameter from the equations.

Hey, how about this solution?  Add the two equations together so that
you don't need to worry so much about coordination, just error
minimization.

x + y = (2 - 2*(b_x + b_y) + (c_x + c_y))*t^2 +
        (-2*(a_x + a_y) + 2*(b_x + b_y))*t + 2a

Yes, simpler, but we're still looking to eliminate t.  Okay, how did
we do it in the linear case?  Well, we already knew what the linear x
and y ranges had to be, so we just combined those linearly (via
multiplication) to get a linear t-range.

Yes, that's true conceptually, but the formula, please?

x(t) = a_x + (b_x - a_x)*t
y(t) = a_y + (b_y - a_y)*t

x + y = (a_x + a_y) + ((b_x + b_y) - (a_x + a_y))*t

x(t) = a_x + (b_x - a_x)*p_x/d_x
y(t) = a_y + (b_y - a_y)*p_y/d_y

x + y = a_x + (b_x - a_x)*p_x/d_x + a_y + (b_y - a_y)*p_y/d_y
x + y = (a_x + a_y) + (b_x - a_x)*p_x*d_y/(d_x*d_y) +
                      (b_y - a_y)*p_y*d_x/(d_x*d_y)

d_x = b_x - a_x
d_y = b_y - a_y

x + y = (a_x + a_y) + d_x*p_x*d_y/(d_x*d_y) +
                      d_y*p_y*d_x/(d_x*d_y)
x + y = (a_x + a_y) + p_x + p_y

Yes, that's indeed how I originally derived things.  Although, I did
not originally explain it with as much mathematical notational
elegance.

Okay, but that's great.  So, the next step is to basically find some
sort of natural denominator that cancels out the numerator.  That's
how you simplify the calculations.

x(t) = (a - 2*b + c)*t^2 + (-2*a + 2*b)*t + a
x(t) = (a - b + c - b)*t^2 + 2*(b - a)*t + a
x(t) = (a - b + c - b)*t^2 + 2*(b - a)*t + a

Yes, this is starting to, in part, look very familiar.

t = p_x/d_x
d_x = 2*(b - a)
d_x^2 = 4*(b^2 - 2a + a^2)
x(t) = (a - 2*b + c)*p_x^2/d_x^2 + p_x + a

Well, let's try to break down (1 - 2*b + c) into something simpler.

(a - 2*b + c)
(a - b + c - b)
(a - b) + (c - b)
-2*(b - a)/2 + (c - b)
(a + c - 2*b)

x(t) = -p_x*t/2 + (c - b)*t^2 + p_x + a
x(t) = (-p_x/2 + (c - b)*t)*t + p_x + a
d_x = (b - a)(c - b)
x(t) = (-(c - b)*p_x + (b - a)*p_x)*t + 2*(c - b)*p_x + a
x(t) = (-(c - b)*p_x + (b - a)*p_x)*p_x/d_x + 2*(c - b)*p_x + a
x(t) = -(c - b)*p_x^2/d_x + (b - a)*p_x^2/d_x + 2*(c - b)*p_x + a
x(t) = -p_x^2/(b - a) + p_x^2/(c - b) + 2*(c - b)*p_x + a
x(t) = p_x^2/(c - b) - p_x^2/(b - a) + 2*(c - b)*p_x + a
x(t) = ((b - a) - (c - b))p_x^2/((b - a)(c - b)) + 2*(c - b)*p_x + a
x(t) = (2b - a - c)p_x^2/((b - a)(c - b)) + 2*(c - b)*p_x + a

Okay, sure, I can get rid of the t-parameter and get to be in only
terms of p_x, the problem is I haven't figured out the best value for
d_x... okay, fine, I guess I have.

Okay, let's summarize the progress I've made.  I've basically shown
that I can single-step the linear part using the same technique I've
demonstrated above.  I guess that goes to showing that maybe I can
show exactly the same thing for the nonlinear part using my successful
graphing algorithms so far.

*/

/*

So what?  What was the final modification?  Well, if you have to
single-step the t-values before you can step the other value, can't
you just cross-multiply the the equations and drop out the t-values?
I guess you're right, as long as you don't have to worry about a
sign change.

How would that make sense?  Well, if you're stepping multiple times
per one step of another variable, that's multiplication, right?  Oh,
yes it is.  So I guess in that case, I can just take the number of
steps per single movement and multiply it over as a factor of the
variable that is being stepped.  I guess that makes sense.

"For every 5 steps of t, you step x by one."

x + 1 = t + 5

derivatives...

1/5

integrate...

x = 1/5*t

check...

(x + 1) = 1/5*(t + 5)
x + 1 = 1/5*t + 1
x = 1/5*t

So I've got to successfully do some sort of analysis like that even in the
case of exponential behavior.

x = t^2

dx/dt = 2*t

Although the amount you step at the lowest degree varies, the higher
degrees have a constant stepping.  But what does that mean in terms of
rearranging single-steps?

Yes, this makes sense.  You can swap one of the two equations, then
you can either add or multiply both together.  You already tried
adding above (which you purported was not very successful), so
why don't we try multiplying?

x(t) = (a_x - b_x + c_x - b_x)*t^2 + 2*(b_x - a_x)*t + a_x
y(t) = (a_y - b_y + c_y - b_y)*t^2 + 2*(b_y - a_y)*t + a_y

(a_x*y - b_x*y + c_x*y - b_x*y)*t^2 + 2*(b_x*y - a_x*y)*t + a_x*y =
(a_y*x - b_y*x + c_y*x - b_y*x)*t^2 + 2*(b_y*x - a_y*x)*t + a_y*x

Well, we have t on both sides, now we just need to figure out how
to drop out all t-terms.  Really?  Or is that not necessary?  t
is our unified accumulator that we single-step to figure out x and
y?  No, we need to single-step the variables too.

a = a_x*y - b_x*y + c_x*y - b_x*y - (a_y*x - b_y*x + c_y*x - b_y*x)
b = 2*(b_x*y - a_x*y) - 2(b_y*x - a_y*x)
c = a_x*y - a_y*x

at^2 + bt + c = 0

Look, you need to be able to say, based off of the values you have
for x and y, you can solve for t.  Okay, fine, let's do this, we'll
end up getting plus or minuses, but then we just throw away the minus
so we can have a single equation.

a = (a_x - b_x + c_x - b_x)
b = (b_x - a_x)
c = a_x

t = (-b +/- sqrt(b^2 - 4ac))/(2a)
t = (sqrt((b_x - a_x)^2 - 4*(a_x - b_x + c_x - b_x)*a_x) -
     (b_x - a_x))/(2*(a_x - b_x + c_x - b_x))
t = (sqrt(b_x^2 - 2*b_x*a_x + a_x^2 - 4*a_x^2 +
          4*a_x*b_x - 4*c_x*a_x + 4*b_x*a_x) -
     b_x + a_x)/(2*a_x - 2*b_x + 2*c_x - 2*b_x)
t = (sqrt(b_x^2 - 3*a_x^2 + 6*a_x*b_x - 4*c_x*a_x) -
     b_x + a_x)/(2*a_x - 2*b_x + 2*c_x - 2*b_x)

t^2 = (b_x^2 - 3*a_x^2 + 6*a_x*b_x - 4*c_x*a_x)/
(2*a_x - 2*b_x + 2*c_x - 2*b_x)^2 - (2*sqrt(b_x^2 - 3*a_x^2 + 6*a_x*b_x - 4*c_x*a_x)*(a_x - b_x) + a_x^2 - 2*b_x*a_x + b_x^2)/(2*a_x - 2*b_x + 2*c_x - 2*b_x)^2

Technically, I've solved the t-problem at this point, but I've
introduced an ugly square root in the intermin.

Okay, let's revert back to adding and start over.

x(t) = (a_x - b_x + c_x - b_x)*t^2 + 2*(b_x - a_x)*t + a_x
y(t) = (a_y - b_y + c_y - b_y)*t^2 + 2*(b_y - a_y)*t + a_y

x + y = (a_x + a_y - b_x - b_y + c_x + c_y - b_x - b_y)*t^2 +
        2*(b_x + b_y - a_x - a_y)*t + (a_x + a_y)
---
a = (a_x + a_y - b_x - b_y + c_x + c_y - b_x - b_y)
b = 2*(b_x + b_y - a_x - a_y)
c = (a_x + a_y) + x + y

t = (sqrt(b^2 - 4ac) - b)/(2a)

You know how to compute square roots involving rationals, don't you?
Oh yeah, you just keep constructing that table iteratively.  Okay,
well, I guess if you do it in a step-wise manner, it's not that bad.
That is still, in effect, like there is an intermediary t-term.

Well, look at it this way.  You're basically doing a linear combination
of two parabolas, one on the y-axis, another on the x-axis.  Having
only one t^2 term to compute is better than having two to compute.

Okay, so I think I've got as many technical details of my formula pinned
down as I can, and I think what I have is as optimal as it can get.
But, look at the bright side.  The square root and rational just above
give you key insight as to what your denominators must be, don't
they?  Okay, I guess not entirely.  I have to get that from other means.

Yes, like I was saying, think of it this way.  Out of necessity, because
a curve is more complicated than a line, it has to be one step more
complicated.  So, you shouldn't expect to simplify the terms to something
simpler than logically makes sense.

Don't fool me, you can't say x^2 = t^2, because then you'd just get
linear!  Exactly, that's the point and the reason why one of your earlier
algorithms didn't work.

But you CAN say

x = t
y = t^2
x^2 = t^2
y = x^2

THAT works!  So if you can get that to work and always make progress
single-stepping at least one of the variables, then you've got to be
able to find a way to get the more general formulas to always work,
making progress single-stepping the variables.

Okay, here's my final verdict.  This whole time, you were worried
about undersampling, but I'll tell you the truth.  The most extreme
case is rotating to a diagonal, 45 degrees.  What does that do to the
resolution?  It actually /decreases/ it, not increases it.  So, that
being said, you have no reason to worry about undersampling parabolas
at the different angles.  It turns out that the standard sampling
procedure will work just as well.

s = t^2

Wait, wait, hold on.  What you've said above about the diagonals.
You were wrong.  With diagonals, the diagonal of the diagonal is
the straight line, which has a /higher/ resolution than the diagonal.
Shorter distance end-to-end, right?  Yes, that's right.

x(t) = a*(1 - t)^2 + 2*b*(1 - t)*t + c*t^2
x(t) = a*(1 - 2t + t^2) + 2*b*(t - t^2) + c*t^2
x(t) = a - 2*a*t + a*t^2 + 2*b*t - 2*b*t^2 + c*t^2
x(t) = a*t^2 - 2*b*t^2 + c*t^2 - 2*a*t + 2*b*t + a
x(t) = (a*t^2 - 2*b*t^2 + c*t^2) + (-2*a*t + 2*b*t) + a
x(t) = (a - 2*b + c)*t^2 + (2*b - 2*a)*t + a
x(t) = (a - 2*b + c)*t^2 + 2*(b - a)*t + a

x = (a - 2*b + c)*t^2 + 2*(b - a)*t + a
0 = (a - 2*b + c)*t^2 + 2*(b - a)*t + a - x
0 = (a - b + c - b)*t^2 + 2*(b - a)*t + a - x

ab + bc
b(a + c)

It would be simple to solve for t, but forget about t^2.

(x - a)/(2*(b - a)) = t + (a - b + c - b)*t^2/(2*(b - a))
x - a = (a - b + c - b)*t^2 + 2*(b - a)*t
x - a = ((a - b)*t + (c - b)*t)*t + 2*(b - a)*t
x - a = ((a - b)*t + (c - b)*t)*t + (b - a)*t + (b - a)*t
x - a = (((a - b)*t + (c - b)*t) + (b - a) + (b - a))*t

Are you sure it's possible using only algebra?

(x - a)/t = ((a - b)*t + (c - b)*t) + (b - a) + (b - a)
(x - a)/t - (b - a) - (b - a) = ((a - b)*t + (c - b)*t)
(x - a)/t - (b - a) - (b - a) = ((a - b) + (c - b))*t

That looks very elegant.  Only we still have two t's sprinkled in.

-(b - a) - (b - a) = ((a - b) + (c - b))*t - (x - a)/t
(b - a) + (b - a) = (x - a)/t - ((a - b) + (c - b))*t

Wow, that's it!  Perfect!  Since dividing by t only narrows the range
of the output, all we need to care about is using the term that t
multiplies to set its precision.  And, since this is the equation
that defines the error bounds, all we have to do is minimize it.
No need to solve for any more variables.  That's perfect!

Maybe, maybe not.  You do know, that ideally, in the end, you'll
remove the rational denominators and do calculations only on
integers, right?  Well, yes, but then we're back to square 1.

*/

/*

Wondering about rounding error?

Oh yeah, when you count forwards?

for (i = 0; i < max; i++)
  doit(i);

But when you count backwards?

i = max;
do {
  i--;
  doit(i);
} while (i > 0);

Otherwise, you'll get goofy behavior.

Remember this for rounding calculations.

Now you're asking, how do you avoid this using rounding?

for (i = 0.0; i < 4.5; i++);

0.0
1.0
2.0
3.0
4.0

(for (i = 4.0; i > -0.5; i--);

4.0
3.0
2.0
1.0
0.0

But if you use integers...

for (i = 0; i < 4; i++)

0
1
2
3
4

for (i = 4; i > 0; i++)

*/

/*

So, lesson learned on curve plotting.  Here's how the overall process
works.

* First, you need invertible functions for the x and y coordinates and
  some coordinating variable.  The idea is that you want to compute
  single steps on the x and y coordinates and have those map to some
  values on a linear integer number line of the coordinating variable.
  Each input must map to a unique t value, but the reverse need not be
  true.

* Since the initialization above will be independent for x and y, you
  will need to multiply together the denominators and single-step
  using integers larger than one for each of the axes.

* You need to compute the range for x and y to plot.  In the case of
  functions that bend back on themselves, you will need to split up
  the function and evaluate it piece-wise in the regions between the
  turning points.  (You could handle symmetry as a special case, but
  look, we're trying to concentrate only on handling the general case
  well here.)

* In the case of non-linear functions, you will need to calibrate the
  t-space to handle the worst-case max rate of change coming from the
  non-linear function.  You'll need to solve for the max derivative
  for this.

* In order to mitigate problems with highly non-linear functions when
  using the above evaluation method, you'll need to use de Casteljau's
  algorithm to subdivide the curve into better behaved, more linear
  functions.  Otherwise, you'll get problems with numeric instability,
  huge denominators, and huge numerators that may overflow and require
  slow arbitrary precision arithmetic, even over small integer spaces.

* You don't need to store the full t-values along the entire
  evaluation path.  You only need to store the fractional residuals,
  and the integer part you can relegate to the real coordinates that
  have been plotted.  Yes, this means that for the integer part, you
  have different coordinate axes, and the fractional part is kind of
  the combined residual.

* Do not sample at a rate higher than the maximum required rate; that
  is, the min rate of change between the t-variables, which results in
  the max number of plotted points.  Otherwise, you will get "barbs"
  in the plotted line.  If you sample too low, then you'll get "holes"
  in the plotted line.

* Remember to transfer over fractional parts when plotting the pieces
  of a subdivided curve.

* These same mechanisms to determine the necessary rational parts can
  be transferred to sparse plotting functions (used for tessellating
  3D NURBS surfaces into partial density meshes).  Then, for
  interoperability with commodity 3D software, the exact point
  calculations can be approximated with binary single-precision
  floating point numbers converted to ASCII representational form.

Okay, that's great.  Now I think I really understand how to evaluate
these curve functions properly.

* But wait!  One problem.  When you carry over error residuals, your
  fractional denominator is going to just keep increasing and
  increasing until it gets too big.  Look, that's kind of what you
  were already setup for to begin with.  Were you to evaluate it
  directly, you'd end up having the exact same problem.  There's no
  shortcuts here.  Yeah, I guess you're right.  So, I guess starting
  out by subdividing the curve is just a potential area to throw away
  data and get an approximate solution rather than going for the exact
  solution.  Sometimes, getting totally accurate results can be really
  complicated.  But, the way I see it, it can also be really important
  too.

** I guess this is the case when you /really/ need to do least common
   denominator factoring rather than just saying you "only handle the
   general case."

** "Rounding off" error residuals that the user deems not to be
   important.  So yes, I have indeed figured out the algorithms by
   which you can render an absolutely perfect rasterization of a
   Bezier curve.  However, for some very badly behaved Bezier curves,
   this comes at a high cost, perhaps higher than the user originally
   intended.  If that is the case, then you want to back off and throw
   away some of that excessively high precision in order to keep the
   calculations stable and fast.

** Perhaps the subdivision logic is better for warning the user in an
   interactive editor to have them fix the bad-behaved curves before
   they run it through the production rendering software pipeline.

** If the denominator would be too large for it to be practical to
   evaluate the curve directly, you have to give the user two options:
   either do it, using slow bignum arithmetic, or subdivide and throw
   away excess error residuals, in effect rasterizing a less accurate
   curve.

* Remember, quick way to test for "good curves."  Just check the
  slopes, then check the order of the distances, without doing the
  actual distance formula.  You don't need to do that as you've
  already done the slope test fairly accurately.

* Oh, don't forget.  Bits after decimal.  You can also have a
  truncation mode.  And, there are special options for graphics.
  Angular error, that is.  Half a degree.  And, also 7 bits after
  decimal.

* But, do you know what is an even better decision?  Limit the max
  number of subdivisions.  That way, you limit the number of bits
  after the decimal that you need to work with.  For practical
  reasons, you can assume that most curves will be well-behaved after
  a certain number of subdivisions.  And, when this is not the case,
  it is probably a very good idea to come back complaining to the user
  on error.

*/

/*

Good point (no pun intended), I like your statement above.  The
/least/ number of points that you are required to plot that still
gives you a connected line.

Plotting parabolas, from linear to nonlinear scale, that's easy.  But
the inverse, from nonlinear to linear, is a little bit harder.  But,
don't worry, there's an easy way to simplify it.  Just use a
multiplier that can sample the least number of samples required at the
highest rate of change.  And what is the highest rate of change?
Compute the derivative!

Okay, let's try to do this with a parabola.  First, let's do the
forward mode, then, let's figure out how to do the reverse mode.

x^2
(x + 1)^2 = x^2 + 2x + 1
2x + 1
2(x + 1) + 1 = (2x + 1) + 2

 x  x^2  delta
 0    0      1
 1    1      3
 2    4      5
 3    9      7
 4   16      9
 5   25     11
 6   36     13
 7   49     15
 8   64     17
 9   81     19
10  100     21
11  121     23
12  144     25

Okay, so on the x-side, we have a smooth increase, but on
the x^2 side, we have gaps between the plotted integers.  So,
we want to figure out how to eliminate the gaps on the x^2
side.  To do this, we of course must use rational numbers on
the x-side.  Important!  Our plotting window is the range [0, 144].

First, what is the highest rate of change?

f(x) = x^2
dx = 2x
max rate = 24

Okay, what does that mean?  I think it means that the integer stride
on the output between two integer inputs shall be less than this
number.  Or, it means we know that the max integer delta will be 23,
which it is!  Great, that's exactly what we wanted to know!  So, that
means we need a scale at least 24 times more dense to capture every
point in that range.

d = 24
x = p/d
x^2 = p^2/d^2
x + 1/d = (p + 1)/d
2x + 1/d = (2p + 1)/d
2(x + 1/d) + 1/d = (2p + 1 + 2)/d
(x + 1/d)^2 = (p/d + 1/d)^2 = p^2/d^2 + 2p/d^2 + 1/d^2
= p^2/d^2 + (2p + 1)/d^2

What a happy coincidence!  Although we don't have matching denominators,
we have matching numerators that we /do/ know how to compute using
simple arithmetic.  So now, it's just a matter of determining when
the result exceeds d^2 to plot an integer coordinate.  Or, I guess
put another way, I should have taken the most complex equation required
and then worked backwards to simplify it.

But you still need the fractional for the next fractional
calculation... okay, fine.  And, I guess you're also right.  I was
incrementing by the wrong base step value.

Okay, great, see below.  Things are working perfectly.  You know what
else?  Well, you were computing derivatives above.  What's the derivative
at a certain point on a parabola?  2x.  Of course, so if you know
what that slope must be, you can solve for what "x" must be at that
point.  Then, you can compute consecutive squares.  Ingenious!  So
that's how that quadratic Bezier plotting code works, sort of.

So, the question is, how do your rephrase a t^2 calculation into an
x^2 calculation?  Here's the thing.  It doesn't matter whether the
square is on your side or their side.  What matters is that you can
compute both starting values, then you can step in whichever direction
is required.

But the problem I was bringing up, time and time again, is that the
fractional t-values are what's causing you the trouble.

Okay, I figured it out.  Here's what you need to see.  f(x) = x^2 =>
x(t) = t^2.  See?  You have exactly what you want.  It's just that
you have some constants to multiply the t-value.  And that's exactly
the point!  Since you know the x-range, you know the x^2 range,
and the the t^2 range = x^2/a.  So, you can put all of this together
to get the t-range on an integer scale.

What were you doing below?  Let's try it on a smaller scale right
here.  Let's say d^2 = 16, and we compute in the range from zero
to 9, whole number.

  x       x^2      delta
  0/4       0/16       1/16
  1/4       1/16       3/16
  2/4       4/16       5/16
  3/4       9/16       7/16
  4/4      16/16       9/16
  5/4      25/16      11/16
  6/4      36/16      13/16
  7/4      49/16      15/16
  8/4      64/16      17/16
  9/4      81/16      19/16
 10/4     100/16      21/16
 11/4     121/16      23/16
 12/4     144/16      25/16

Okay, great!  Now let's display this in decimal numbers.

x         x^2      delta
0.00        0.0000     0.0625
0.25        0.0625     0.1875
0.50        0.2500     0.3125
0.75        0.5625     0.4375
1.00        1.0000     0.5625
1.25        1.5625     0.6875
1.50        2.2500     0.8125
1.75        3.0625     0.9375
2.00        4.0000     1.0625
2.25        5.0625     1.1875
2.50        6.2500     1.3125
2.75        7.5625     1.4375
3.00        9.0000     1.5625

Only one integer is skipped.  Pretty good, I must say.  And, all
fractional calculations are correct.

Okay, how about this?  A binary division approach.  Whenever you're
about to skip one point, you subdivide to compute the point that
you're about to skip over.  Are you moving an integer length plus
a fractional length?  Subdivide if greater than or equal to 1.5.

  x       x^2      delta
  0         0          1
  1         1          3/4
  2/2       4/4        5/4
  3/2       9/4        7/9
  4/3      16/9        9/9
  5/3      25/9       11/16
  6/4      36/16      13/16
  7/4      49/16      15/16
  8/4      64/16      17/16
  9/4      81/16      19/16
 10/4     100/16      21/25
 11/5     121/25      23/25
 12/5     144/25      25/25
 13/5     169/25      27/25
 14/5     196/25      29/25
 15/5     225/25      31/25

Perfect!  That is, when we require correct rounding.  Let's take
a look at the decimals (which I have actually been computing on
the back of my hand).

  x       x^2      delta
  0         0          1
  1         1          0.75
  1         1          1.25
  1.5       2.25       0.778
  1.333     1.778      1
  1.667     2.778      0.688
  1.5       2.25       0.812
  1.75      3.062      0.938
  2         4          1.062
  2.25      5.062      1.188
  2.5       6.25       0.84
  2.2       4.84       0.92
  2.4       5.76       1
  2.6       6.76       1.08
  2.8       7.84       1.16
  3         9          1.24

Anyways, interesting algorithm, but we're not going to need to use
it for rasterizing Bezier curves.  Point in hand, you know how to
do rational integer arithmetic with nonlinear functions efficiently,
at least when doing step-wise computations.  We can take the
independent coordinate spaces and map them back to a uniform t-space
using rational integer arithmetic.

And the long one is below:

  x     x^2      delta
  0       0/576      1/576
  1       1/576      3/576
  2       4/576      5/576
  3       9/576      7/576
  4      16/576      9/576
  5      25/576     11/576
  6      36/576     13/576
  7      49/576     15/576
  8      64/576     17/576
  9      81/576     19/576
 10     100/576     21/576
 11     121/576     23/576
 12     144/576     25/576
 13     169/576     27/576
 14     196/576     29/576
 15     225/576     31/576
 16     256/576     33/576
 17     289/576     35/576
 18     324/576     37/576
 19     361/576     39/576
 20     400/576     41/576
 21     441/576     43/576
 22     484/576     45/576
 23     529/576     47/576
 24     576/576     49/576
 25     625/576     51/576
 26     676/576     53/576
 27     729/576     55/576
 28     784/576     57/576
 29     841/576     59/576
 30     900/576     61/576
 31     961/576     63/576
 32    1024/576     65/576
 33    1089/576     67/576
 34    1156/576     69/576
 35    1225/576     71/576
 36    1296/576     73/576
 37    1369/576     75/576
 38    1444/576     77/576
 39    1521/576     79/576
 40    1600/576     81/576
 41    1681/576     83/576
 42    1764/576     85/576
 43    1849/576     87/576
 44    1936/576     89/576
 45    2025/576     91/576
 46    2116/576     93/576
 47    2209/576     95/576
 48    2304/576     97/576
 49    2401/576     99/576
 50    2500/576    101/576
 51    2601/576    103/576
 52    2704/576    105/576
 53    2809/576    107/576
 54    2916/576    109/576
 55    3025/576    111/576
 56    3136/576    113/576
 57    3249/576    115/576
 58    3364/576    117/576
 59    3481/576    119/576
 60    3600/576    121/576
 61    3721/576    123/576
 62    3844/576    125/576
 63    3969/576    127/576
 64    4096/576    129/576
 65    4225/576    131/576
 66    4356/576    133/576
 67    4489/576    135/576
 68    4624/576    137/576
 69    4761/576    139/576
 70    4900/576    141/576
 71    5041/576    143/576
 72    5184/576    145/576
 73    5329/576    147/576
 74    5476/576    149/576
 75    5625/576    151/576
 76    5776/576    153/576
 77    5929/576    155/576
 78    6084/576    157/576
 79    6241/576    159/576
 80    6400/576    161/576
 81    6561/576    163/576
 82    6724/576    165/576
 83    6889/576    167/576
 84    7056/576    169/576
 85    7225/576    171/576
 86    7396/576    173/576
 87    7569/576    175/576
 88    7744/576    177/576
 89    7921/576    179/576
 90    8100/576    181/576
 91    8281/576    183/576
 92    8464/576    185/576
 93    8649/576    187/576
 94    8836/576    189/576
 95    9025/576    191/576
 96    9216/576    193/576
 97    9409/576    195/576
 98    9604/576    197/576
 99    9801/576    199/576
100   10000/576    201/576
101   10201/576    203/576
102   10404/576    205/576
103   10609/576    207/576
104   10816/576    209/576
105   11025/576    211/576
106   11236/576    213/576
107   11449/576    215/576
108   11664/576    217/576
109   11881/576    219/576
110   12100/576    221/576
111   12321/576    223/576
112   12544/576    225/576
113   12769/576    227/576
114   12996/576    229/576
115   13225/576    231/576
116   13456/576    233/576
117   13689/576    235/576
118   13924/576    237/576
119   14161/576    239/576
120   14400/576    241/576
121   14641/576    243/576
122   14884/576    245/576
123   15129/576    247/576
124   15376/576    249/576
125   15625/576    251/576
126   15876/576    253/576
127   16129/576    255/576
128   16384/576    257/576
129   16641/576    259/576
130   16900/576    261/576
131   17161/576    263/576
132   17424/576    265/576
133   17689/576    267/576
134   17956/576    269/576
135   18225/576    271/576
136   18496/576    273/576
137   18769/576    275/576
138   19044/576    277/576
139   19321/576    279/576
140   19600/576    281/576
141   19881/576    283/576
142   20164/576    285/576
143   20449/576    287/576
144   20736/576    289/576
145   21025/576    291/576
146   21316/576    293/576
147   21609/576    295/576
148   21904/576    297/576
149   22201/576    299/576
150   22500/576    301/576
151   22801/576    303/576
152   23104/576    305/576
153   23409/576    307/576
154   23716/576    309/576
155   24025/576    311/576
156   24336/576    313/576
157   24649/576    315/576
158   24964/576    317/576
159   25281/576    319/576
160   25600/576    321/576
161   25921/576    323/576
162   26244/576    325/576
163   26569/576    327/576
164   26896/576    329/576
165   27225/576    331/576
166   27556/576    333/576
167   27889/576    335/576
168   28224/576    337/576
169   28561/576    339/576
170   28900/576    341/576
171   29241/576    343/576
172   29584/576    345/576
173   29929/576    347/576
174   30276/576    349/576
175   30625/576    351/576
176   30976/576    353/576
177   31329/576    355/576
178   31684/576    357/576
179   32041/576    359/576
180   32400/576    361/576
181   32761/576    363/576
182   33124/576    365/576
183   33489/576    367/576
184   33856/576    369/576
185   34225/576    371/576
186   34596/576    373/576
187   34969/576    375/576
188   35344/576    377/576
189   35721/576    379/576
190   36100/576    381/576
191   36481/576    383/576
192   36864/576    385/576
193   37249/576    387/576
194   37636/576    389/576
195   38025/576    391/576
196   38416/576    393/576
197   38809/576    395/576
198   39204/576    397/576
199   39601/576    399/576
200   40000/576    401/576
201   40401/576    403/576
202   40804/576    405/576
203   41209/576    407/576
204   41616/576    409/576
205   42025/576    411/576
206   42436/576    413/576
207   42849/576    415/576
208   43264/576    417/576
209   43681/576    419/576
210   44100/576    421/576
211   44521/576    423/576
212   44944/576    425/576
213   45369/576    427/576
214   45796/576    429/576
215   46225/576    431/576
216   46656/576    433/576
217   47089/576    435/576
218   47524/576    437/576
219   47961/576    439/576
220   48400/576    441/576
221   48841/576    443/576
222   49284/576    445/576
223   49729/576    447/576
224   50176/576    449/576
225   50625/576    451/576
226   51076/576    453/576
227   51529/576    455/576
228   51984/576    457/576
229   52441/576    459/576
230   52900/576    461/576
231   53361/576    463/576
232   53824/576    465/576
233   54289/576    467/576
234   54756/576    469/576
235   55225/576    471/576
236   55696/576    473/576
237   56169/576    475/576
238   56644/576    477/576
239   57121/576    479/576
240   57600/576    481/576
241   58081/576    483/576
242   58564/576    485/576
243   59049/576    487/576
244   59536/576    489/576
245   60025/576    491/576
246   60516/576    493/576
247   61009/576    495/576
248   61504/576    497/576
249   62001/576    499/576
250   62500/576    501/576
251   63001/576    503/576
252   63504/576    505/576
253   64009/576    507/576
254   64516/576    509/576
255   65025/576    511/576
256   65536/576    513/576
257   66049/576    515/576
258   66564/576    517/576
259   67081/576    519/576
260   67600/576    521/576
261   68121/576    523/576
262   68644/576    525/576
263   69169/576    527/576
264   69696/576    529/576
265   70225/576    531/576
266   70756/576    533/576
267   71289/576    535/576
268   71824/576    537/576
269   72361/576    539/576
270   72900/576    541/576
271   73441/576    543/576
272   73984/576    545/576
273   74529/576    547/576
274   75076/576    549/576
275   75625/576    551/576
276   76176/576    553/576
277   76729/576    555/576
278   77284/576    557/576
279   77841/576    559/576
280   78400/576    561/576
281   78961/576    563/576
282   79524/576    565/576
283   80089/576    567/576
284   80656/576    569/576
285   81225/576    571/576
286   81796/576    573/576
287   82369/576    575/576

288   82944/576    577/576
289   83521/576    579/576
290   84100/576    581/576
291   84681/576    583/576
292   85264/576    585/576
293   85849/576    587/576
294   86436/576    589/576
295   87025/576    591/576
296   87616/576    593/576
297   88209/576    595/576
298   88804/576    597/576
299   89401/576    599/576
300   90000/576    601/576
301   90601/576    603/576
302   91204/576    605/576
303   91809/576    607/576
304   92416/576    609/576
305   93025/576    611/576
306   93636/576    613/576
307   94249/576    615/576
308   94864/576    617/576
309   95481/576    619/576
310   96100/576    621/576
311   96721/576    623/576
312   97344/576    625/576
313   97969/576    627/576
314   98596/576    629/576
315   99225/576    631/576
316   99856/576    633/576
317  100489/576    635/576
318  101124/576    637/576
319  101761/576    639/576
320  102400/576    641/576
321  103041/576    643/576
322  103684/576    645/576
323  104329/576    647/576
324  104976/576    649/576
325  105625/576    651/576
326  106276/576    653/576
327  106929/576    655/576
328  107584/576    657/576
329  108241/576    659/576

  x         x^2          delta
  0 + 0/24    0 + 0/576      1/576
  0 + 1/24    0 + 1/576      3/576
  0 + 2/24    0 + 4/576      5/576
  0 + 2/24    0 + 9/576      7/576
  ...
11 + 0/24  121 + 0/576

*/

/*

Oh yeah.  Important comment.  Regarding taking derivatives.  The
difference quotient to the limit is in fact not conceptually correct
to the actual calculation that you want to do, it's just that it
happens to coincidentally work.  Well, not exactly.  See below for
where the calculation is in error.

f(x) = x^2
dx = 2x

d(x + 1) - d(x) = 2x + 2 - 2x = 2

What happens is that you ended up computing an intermediate value in
error, due to computing the instantaneous rate of change rather than
the actual change between two integer coordinates.

(f(x + 1) - f(x)) / 1 = x^2 + 2x + 1 - x^2 = 2x + 1
(g(x + 1) - g(x)) / 1 = 2x + 3 - 2x - 1 = 2

Okay, but this does give me a much better way of explaining my results
above.

So, let's put it this way.  It's Newton's difference quotient in
discrete space.  It does in fact have many useful applications, so
it's important not to forget about this tool when it provides a
simpler solution than derivative computations.

Note that we shouldn't be dividing by one above.

(f(x - 1) - f(x)) = x^2 - 2x + 1 - x^2 = -2x + 1
(g(x - 1) - g(x)) = -2x - 2 + 1 + 2x - 1 = -2

When going in the negative direction, your results get inverted to
become positive.

*/

/*

Okay, I've been kind of lost on the cubic Bezier curve efficient
computations.  Given everything that I've said about, this is the
right way to do the derivations:

(1 - t)^3
(1 - t)*(1 - 2t + t^2) = 1 - 2t + t^2 - t + 2t^2 - t^3
= 1 - 3t + 3t^2 - t^3

a*(1 - t)^3 + 3*b*(1 - t)^2*t + 3*c*(1 - t)*t^2 + d*t^3
= a - 3*a*t + 3*a*t^2 - a*t^3 + 3*b*t - 6*b*t^2 + 3*b*t^3 +
  3*c*t^2 - 3*c*t^3 + d*t^3
= - a*t^3 + 3*b*t^3 - 3*c*t^3 + d*t^3 + 3*a*t^2 - 6*b*t^2 + 3*c*t^2 -
  3*a*t + 3*b*t + a
= (-a + 3*b - 3*c + d)*t^3 + (3*a - 6*b + 3*c)*t^2 + (-3*a + 3*b)*t + a

let m = (-a + 3*b - 3*c + d)
    n = (3*a - 6*b + 3*c)
    p = (-3*a + 3*b)
    q = a
x(t) = m*t^3 + n*t^2 + p*t + q

x(t + 1) - x(t) = m*(t + 1)^3 + n*(t + 1)^2 + p*(t + 1) + q -
                  (m*t^3 + n*t^2 + p*t + q)
= m*(t^3 + 3*t^2 + 3*t + 1) + n*(t^2 + 2*t + 1) + p*t + p + q -
  m*t^3 - n*t^2 - p*t - q
= m*(3*t^2 + 3*t + 1) + n*(2*t + 1) + p
= m*3*t^2 + m*3*t + m + n*2*t + n + p
dx(t) = 3*m*t^2 + (3*m + 2*n)*t + m + n + p

dx(t + 1) - dx(t) = 3*m*(t + 1)^2 + (3*m + 2*n)*(t + 1) + m + n + p -
                    (3*m*t^2 + (3*m + 2*n)*t + m + n + p)
= 3*m*(t^2 + 2*t + 1) + (3*m + 2*n)*(t + 1) + m + n + p -
  3*m*t^2 - (3*m + 2*n)*t - m - n - p
= 3*m*(2*t + 1) + 3*m + 2*n
ddx(t) = 6*m*t + 6*m + 2*n

ddx(t + 1) - ddx(t) = 6*m*(t + 1) + 6*m + 2*n - (6*m*t + 6*m + 2*n)
okay
= 6*m*t + 12*m + 2*n - 6*m*t - 6*m - 2*n
= 6*m

*/

/*

Now, this is an interesting way to put things.  Here's another
notation and way of thinking about the delta computations.  Here, we
think of this as a polynomial division operation, and it is like we
are writing out a decimal number.  Then, we just add one to the
rightmost digit.  Further to the right, we just have a bunch of zeros.
Of course, this is like integers that have a fractional part of all
zeros after the decimal.  In the case of integers, the zero really
should not be specified.

(x + 1)^2 - x^2 = x^2 + 2x + 1 - x^2 = 2x + 1
(2(x + 1) + 1) - (2x + 1) = 2x + 3 - 2x - 1 = 2
2 - 2 = 0
0 - 0 = 0

x = 0, x^2 = 0, 2x + 1 = 1, 2 = 2

  x    x^2   2x + 1  2
  0      0        1  2
  1      1        3  2
  2      4        5  2
  3      9        7  2
  4     16        9  2
  5     25       11  2
  6     36       13  2
  7     49       15  2
  8     64       17  2
  9     81       19  2
 10    100       21  2
 11    121       23  2
 12    144       25  2
 13    169       27  2

Yes, and this is how you find integer sum of squares.  Using the
table above, you identify which deltas happen to also be square
numbers themselves.  Since the deltas are always odd numbers,
your selection can only come from the list of square numbers that
are also odd.

25 = 16 + 9
5^2 = 4^2 + 3^2
169 = 144 + 25
13^2 = 12^2 + 5^2

And, now that I've written all of this, I see another reason for
why detached documentation is a good idea.  Sometimes, it doesn't
make sense to do all this writing and documentation within
the software source code that implements it.  Rather, a separate
"book" document should be written.  Then, the source code can
make reference to that separate document.

Nowadays, of course, it would be ideal if that separate document
is in the computer too, but in the past, the separate document
had to be a printed book found in a library, out of technological
necessity.

How about running in reverse?  In that case, you have to remember
to subtract everything rather than add it.  You also have to subtract
using numbers from the current row rather than the previous row.

  x    x^2   2x + 1  2
  5     25       11  2
  4     16        9  2
  3      9        7  2
  2      4        5  2
  1      1        3  2
  0      0        1  2
 -1      1       -1  2
 -2      4       -3  2
 -3      9       -5  2
 -4     16       -7  2
 -5     25       -9  2

So, counting in reverse with this formula works too.  Because
you're subtracting everything, you end up subtracting negative numbers
too, which ends up being the addition of positive numbers.  Of course,
adding is faster than subtracting, so if you can rephrase this as only
adding numbers, that would give you the optimal performance.  Hence
starting at the bottom and counting up only!

  x    x^2   2x + 1  2
 -5     25       -9  2
 -4     16       -7  2
 -3      9       -5  2
 -2      4       -3  2
 -1      1       -1  2
  0      0        1  2
  1      1        3  2
  2      4        5  2
  3      9        7  2
  4     16        9  2
  5     25       11  2

Wow, that math is so easy, I could almost do it in my sleep.  That's
good!  That's what makes the computation fast for the computer too.

*/

/*

Alright, prepare to write another test case and rasterization
algorithm.  Here's the idea.  In regard to t-values and plotting
curves.  When you have a parabola, you have some sections of large
leaps followed by not much change, but the total number of pixels
traversed still stays the same.  So, here's your proposal.  With a
large changing y-value, you have a small-changing x-value.  So,
the y-t-value is the value you use to determine whether you should
increase by one x-value, and the x-t-value is the value you use
to determine whether you should increase by one y-value.  This will
result in long plots of y-values at the fast changing areas.  Okay,
okay, it's a leap, but the only way to know if it works is to try
it out.

*/

/*

First, the programmer is to setup their desired degree of precision.
Four choices: short integers, long integers, long long integers,
arbitrary precision.  If, for any of the choices, some limit is
exceeded, then they return a failure code: EOVERFLOW.

The problem with passing around bits after the decimal.  This in
general means that your denominators for your fractional calculations
will also have to be bigger.

*/

/* TESTING of line rasterizer.  */
void
test_line_rasterizer()
{
  Coord2D p1 = { 72, 0 }, p2 = { 0, 11 };
  memset(&roll_buffer, ' ', sizeof(roll_buffer));
  cur_roll_line = 73;
  rasterize_line(p1, p2);
  flush_piano_roll();
  memset(&roll_buffer, ' ', sizeof(roll_buffer));
  alt_rasterize_line(p1, p2);
  flush_piano_roll();
  memset(&roll_buffer, ' ', sizeof(roll_buffer));
  alt_rasterize_line2(p1, p2);
  flush_piano_roll();
  memset(&roll_buffer, ' ', sizeof(roll_buffer));
  alt_rasterize_para();
  flush_piano_roll();
  memset(&roll_buffer, ' ', sizeof(roll_buffer));
  alt_rasterize_para2();
  flush_piano_roll();
}

/* TESTING of anisotropic Bezier rasterizer.  */
void
test_smooth_bezier()
{
  Coord2D a = { 0, 0 }, b = { 24, 15 }, c = { 48, 15 }, d = { 72, 0 };
  memset(&roll_buffer, ' ', sizeof(roll_buffer));
  cur_roll_line = 73;
  rasterize_bezier(a, b, c, d);
  flush_piano_roll();
}

/* TESTING of recursive Bezier rasterizer.  */
void
test_recursive_bezier()
{
  Coord2D a = { 0, 0 }, b = { 24, 15 }, c = { 48, 15 }, d = { 72, 0 };
  /* Coord2D a = { 49, 6 }, b = { 50, 12 }, c = { 50, 0 }, d = { 51, 6 }; */
  memset(&roll_buffer, ' ', sizeof(roll_buffer));
  cur_roll_line = 73;
  rasterize_bezier_recursive(a, b, c, d);
  flush_piano_roll();
}

/* Yes, the ansiotropic Bezier curve rasterizer!  Again, like I said,
   that's the advantage of using integer arithmetic for the
   calculations.  How do you walk the t-values evenly from min to max
   with separate max bounds for x and y?  Easy, just imagine drawing a
   straight line through the corners of a rectangle, And we know how
   to do that, don't we?  Oh yes we do!  Use that well-known line
   rasterizer algorithm.  */

/********************************************************************/
/* rand.c: */

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Posix rand_r function added May 1999 by Wes Peters <wes@softweyr.com>.
 *
 * This random number generator from FreeBSD was trimmed for use with
 * `mgen.c'.
 */

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;

static int
do_rand(unsigned long *ctx)
{
#ifdef  USE_WEAK_SEEDING
/*
 * Historic implementation compatibility.
 * The random sequences do not vary much with the seed,
 * even with overflowing.
 */
	return ((*ctx = *ctx * 1103515245 + 12345) % ((u_long)RAND_MAX + 1));
#else   /* !USE_WEAK_SEEDING */
/*
 * Compute x = (7^5 * x) mod (2^31 - 1)
 * without overflowing 31 bits:
 *      (2^31 - 1) = 127773 * (7^5) + 2836
 * From "Random number generators: good ones are hard to find",
 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
 * October 1988, p. 1195.
 */
	long hi, lo, x;

	/* Can't be initialized with 0, so use another value. */
	if (*ctx == 0)
		*ctx = 123459876;
	hi = *ctx / 127773;
	lo = *ctx % 127773;
	x = 16807 * lo - 2836 * hi;
	if (x < 0)
		x += 0x7fffffff;
	return ((*ctx = x) % ((u_long)RAND_MAX + 1));
#endif  /* !USE_WEAK_SEEDING */
}


int
rand_r(unsigned int *ctx)
{
	u_long val = (u_long) *ctx;
	int r = do_rand(&val);

	*ctx = (unsigned int) val;
	return (r);
}


static u_long next = 1;

int
rand()
{
	return (do_rand(&next));
}

void
srand(seed)
u_int seed;
{
	next = seed;
}
