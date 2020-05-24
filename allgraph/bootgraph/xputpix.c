/* $Id: hello.c 1.1 1994/01/12 04:36:59 ulrich Exp $ */
/*
 * hello.c
 *
 * Say "Hello, World." demo program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include <X11/bitmaps/stipple>

#include "bootgraph.h"

/* TODO: Define both clipped and unclipped requests.  Why?
   Particularly, when we embed in higher-level primitives, we've
   already performed clipping so it doesn't need to be done twice.  */

/* DISCUSSION: How will the code be designed to be reasonably generic
   but still optimal?  Here is the idea I will put forward.  I will
   use code templating using `sed' substitutions to avoid defining
   ugly macros.  This will mainly be about baking down the
   bits-per-pixel configuration into the whole function, for the
   common combinations:

   32 bits per pixel
   24 bits per pixel
   16 bits per pixel
   8 bits per pixel
   4 bits per pixel
   2 bits per pixel
   1 bit per pixel

   Uncommon combinations take the slow path.

*/

void
bg_put_pixel (RTImageBuf *rti,
	      Point2D pt,
	      unsigned long color)
{
  unsigned char *fbdata = rti->image_data;
  unsigned short pitch = rti->pitch;
  unsigned short width = rti->hdr.width;
  unsigned short height = rti->hdr.height;
  unsigned short bpp = rti->hdr.bpp;
  unsigned long addr;
  if (pt.x < 0 || pt.y < 0 || pt.x >= width || pt.y >= height)
    return; /* Clip out of bounds requests.  */
  addr = pitch * pt.y + (bpp >> 3) * pt.x;
  if (bpp == 32)
    *(unsigned long*)(fbdata + addr) = (unsigned long)color;
}

/* TODO: Copy scanline.  */

/* TODO: Define line iterator subroutines.  That is, we want things
   like GetFirstPoint(), GetNextPoint().  These can be easily embedded
   within higher level functions like DrawTriangle().  In all cases of
   use thus far for graphics, we step vertically, filling scanlines as
   we go.  */

/*

So, how many different dimensions of variables do we have?

* Scalar format: int8, int16, int32, int64, etc.  Always equal to
  int16 if targeting simple computers.

* Dimension is always equal to 2 for screen drawing primitives.

* Unclipped versus clipped drawing primitives.

* Color: Monochrome, solid-filled colors at bit-depth N (8, 16, 24,
  32, etc), Goraud smooth shading, texture filling, lookup table color
  blending, etc.

Okay, so that's not too bad.  Clipping can be done independently of
the underlying data format, so that can be implemented in a single
independent set of functions.  So it really just comes down to
different functions for different color formats.

With clever segmentation of fundamental features and functions,
polymorphism can be avoided while still providing a powerful range of
features.

 */

/* Plot a cubic curve using a similar technique to the parabola curve.
   The scale factor is similar to the parabola case.  If you want
   9/9 == (9/9)^3, then set the scale factor to 9.

   However, please note that the internal mathematics must square the
   quantity you give in the scale factor due to the following
   equations.

   9/9 == (9/9)^3
   9/9 == 1/9^3 * 9^3
   9 == 1/9^2 * 9^3

*/
void
plot_cubic_fast (RTImageBuf *rti, unsigned long color,
		 Point2D vertex, int scale, int x_max)
{
  Point2D cur;
  int x0 = 0, y0 = 0, y0_scale = 0;
  int x_3odd = 1, x_6odd = 3, x3_2q = 0, x_3q = 0;
  int scale_2q = scale * scale;
  /* Gotta go half a step forward to get symmetry.  */
  x3_2q += 2;
  x_6odd += 3;
  x_3odd += 2;
  while (x0 < x_max) {
    if (y0_scale < x_3q) {
      while (y0_scale < x_3q) {
	y0++;
	y0_scale += scale_2q;
	cur.x = vertex.x + x0;
	cur.y = vertex.y + y0;
	bg_put_pixel (rti, cur, color);
	cur.x = vertex.x - x0;
	cur.y = vertex.y - y0;
	bg_put_pixel (rti, cur, color);
      }
    } else {
      cur.x = vertex.x + x0;
      cur.y = vertex.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = vertex.x - x0;
      cur.y = vertex.y - y0;
      bg_put_pixel (rti, cur, color);
    }

    x_3q += x3_2q + x_3odd;
    x3_2q += x_6odd;
    x_6odd += 6;
    x_3odd += 3;
    x0++;
  }
}

/* Plotting a hyperbola is like plotting an ellipse, but easier.
   Unfortunately, it is less practical... almost no one desires to
   draw a hyperbola in a painting program.  Literally, I just changed
   two lines of code!  Major x-axis.  */
void
plot_hyperbola_fast (RTImageBuf *rti, unsigned long color,
		     Point2D center, int rx, int ry, int limit)
{
  Point2D cur;
  int x0 = rx, y0 = 0;
  int x_odd = 1, x_2q = 0; /* subtract from x */
  int y_odd = 1, y_2q = 0; /* add to y */
  int rx_2 = 2 * rx * ry * ry;
  int x2n = 0;
  int i = 0;

  /* Now, the trick here is to take half a step forward at the
     beginning to get some nice 50% rounding.  Think of what we do in
     a full iteration.  Now, perform only half of all those
     increments.  */
  /* x_2q += 0.5; */ x_odd += 1 * ry * ry;
  /* y_2q += 0.5; */ y_odd += 1 * rx * rx;
  x2n += rx * ry * ry;

  while (i < 250 && x0 < limit) {
    int y0_2q = (x2n + x_2q);
    if (y_2q >= y0_2q) {
      cur.x = center.x + x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x + x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);
    }
    while (y_2q < y0_2q) {
      cur.x = center.x + x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x + x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);

      y0++;
      y_2q += y_odd;
      y_odd += 2 * rx * rx;
      i++;
    }

    x0++;
    x2n += rx_2;
    x_2q += x_odd;
    x_odd += 2 * ry * ry;
    i++;
  }
}

/* NOTE: Scale is the quantity by which to divide the input and the
   output.  For example, if you want your parabola equation to be like
   9/9 == (9/9)^2, yoiu would set the scale to 9.  Such an equation is
   functionally equivalent to 9 == 1/9*(9)^2.  And that's also how you
   compute a table of squares to determine square roots.  Linear
   approximation between table entries has good accuracy, given a
   sufficiently high resolution table.  */
void
plot_parabola_fast (RTImageBuf *rti, unsigned long color,
		    Point2D vertex, int scale, int x_max)
{
  Point2D cur;
  int x0 = 0, y0 = 0, y0_scale = 0;
  int x_odd = 1, x_2q = 0;
  /* Gotta go half a step forward to get symmetry.  Explanation: Since
     thresholding is done every loop, if you increase x_odd by 1, then
     you consistently alter the thresholding logic, even as the final
     numbers grow.  But adding 1 to the staritng absolute value is
     negligible.  */
  x_odd += 1;
  while (x0 < x_max) {
    if (y0_scale < x_2q) {
      while (y0_scale < x_2q) {
	y0++;
	y0_scale += scale;
	cur.x = vertex.x + x0;
	cur.y = vertex.y + y0;
	bg_put_pixel (rti, cur, color);
	cur.x = vertex.x - x0;
	cur.y = vertex.y + y0;
	bg_put_pixel (rti, cur, color);
      }
    } else {
      cur.x = vertex.x + x0;
      cur.y = vertex.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = vertex.x - x0;
      cur.y = vertex.y + y0;
      bg_put_pixel (rti, cur, color);
    }

    x_2q += x_odd;
    x_odd += 2;
    x0++;
  }
}

/* Congratulations!  You've just pretty much reinvented windows GDI
   and Macintosh QuickDraw, although the drawing will not be as fast
   as QuickDraw on the original Macintosh.  But, point in hand.  I
   have my own public domain implementation written in C that can
   provide bindings for early style graphics API libraries for the
   three major windowing system platforms: Macintosh, X11, and
   Windows.  And, vice versa, provide an automatic wrapper library to
   target any of the three such drawing systems as "native."  Oh and,
   wait for it... PostScript.  */

/* Now what?  Game engine unification between the Mohawk and Doom game
   engines?  */

void
flood_fill (char *fbdata, Point2D start, unsigned int fill_color)
{
#define MAX_EDGE 65534
  /* Keep two edge lists, one for the old, the other for the new.  Use
     an index integer to specify which one is old and new.  */
  /* TODO FIXME: This static memory allocation is way too large.
     Heck, it's larger than all the memory in a Macintosh 128K!  This
     has ought to be improved.  */
  Point2D edge[2][MAX_EDGE+1];
  unsigned short edge_len[2] = { 0, 0 };
  short cur_edge = 0;
  unsigned int start_color;
  /* Initialize the start color.  */
  /* GetPixel (cur); */
  start_color = *(unsigned int*)&fbdata[(start.y*320+start.x)*4+0];
  if (start_color == fill_color)
    /* Does nothing!  We have to stop early otherwise we get a
       recursion bomb where we add the same points multiple times to
       the flood fill edge.  */
    return;
  /* Fill the first pixel.  */
  *(unsigned int*)&fbdata[(start.y*320+start.x)*4+0] = fill_color;
  /* Initialize the edge.  */
  edge[cur_edge][edge_len[cur_edge]].x = start.x;
  edge[cur_edge][edge_len[cur_edge]].y = start.y;
  edge_len[cur_edge]++;
  while (edge_len[cur_edge] > 0) {
    short next_edge = (cur_edge + 1) & 1;
    unsigned i;
    edge_len[next_edge] = 0;
    for (i = 0; i < edge_len[cur_edge]; i++) {
      Point2D cur = edge[cur_edge][i];
      Point2D test[4] = { { cur.x - 1, cur.y },
			  { cur.x, cur.y - 1 },
			  { cur.x + 1, cur.y },
			  { cur.x, cur.y + 1 } };
      unsigned j;
      /* Test the four surrounding points.  If they match the starting
	 color, fill and add them to the next edge list.  Make sure to
	 perform clipping with the image bounds.  */
      for (j = 0; j < 4; j++) {
	if (test[j].x >= 0 && test[j].x < 320 &&
	    test[j].y >= 0 && test[j].y < 240) {
	  unsigned int test_val =
	    *(unsigned int*)&fbdata[(test[j].y*320+test[j].x)*4+0];
	  if (test_val == start_color) {
	    edge[next_edge][edge_len[next_edge]].x = test[j].x;
	    edge[next_edge][edge_len[next_edge]++].y = test[j].y;
	    *(unsigned int*)&fbdata[(test[j].y*320+test[j].x)*4+0] = fill_color;
	    if (edge_len[next_edge] > MAX_EDGE) {
	      fputs("Flood fill edge length exceeded!\n", stderr);
	      abort ();
	    }
	  }
	}
      }
    }
    cur_edge = next_edge;
  }
}

void
plot_ellipse_fast (RTImageBuf *rti, unsigned long color,
		   Point2D center, int rx, int ry)
{
  Point2D cur;
  int x0 = rx, y0 = 0;
  int x_odd = 1, x_2q = 0; /* subtract from x */
  int y_odd = 1, y_2q = 0; /* add to y */
  int rx_2 = 2 * rx * ry * ry;
  int x2n = 0;
  int i = 0;

  /* Now, the trick here is to take half a step forward at the
     beginning to get some nice 50% rounding.  Think of what we do in
     a full iteration.  Now, perform only half of all those
     increments.  */
  /* x_2q += 0.5; */ x_odd += 1 * ry * ry;
  /* y_2q += 0.5; */ y_odd += 1 * rx * rx;
  x2n += rx * ry * ry;

  while (i < 250 && y0 <= ry + 1 && x0 >= 0) {
    int y0_2q = (x2n - x_2q);
    if (y_2q >= y0_2q) {
      cur.x = center.x + x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x + x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);
    }
    while (y_2q < y0_2q) {
      cur.x = center.x + x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x + x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);

      y0++;
      y_2q += y_odd;
      y_odd += 2 * rx * rx;
      i++;
    }

    x0--;
    x2n += rx_2;
    x_2q += x_odd;
    x_odd += 2 * ry * ry;
    i++;
  }
}

void
plot_circle_fast (RTImageBuf *rti, unsigned long color,
		  Point2D center, int size)
{
  Point2D cur;
  int x0 = size, y0 = 0;
  int x_odd = 1, x_2q = 0; /* subtract from x */
  int y_odd = 1, y_2q = 0; /* add to y */
  int size_2 = 2 * size;
  int x2n = 0;
  int i = 0;

  /* Now, the trick here is to take half a step forward at the
     beginning to get some nice 50% rounding.  Think of what we do in
     a full iteration.  Now, perform only half of all those
     increments.  */
  /* x_2q += 0.5; */ x_odd += 1;
  /* y_2q += 0.5; */ y_odd += 1;
  x2n += size;

  while (i < 250 && y0 <= size && x0 >= 0) {
    int y0_2q = x2n - x_2q;
    if (y_2q >= y0_2q) {
      cur.x = center.x + x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - y0;
      cur.y = center.y + x0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x + y0;
      cur.y = center.y - x0;
      bg_put_pixel (rti, cur, color);
    }
    while (y_2q < y0_2q) {
      cur.x = center.x + x0;
      cur.y = center.y + y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - y0;
      cur.y = center.y + x0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x - x0;
      cur.y = center.y - y0;
      bg_put_pixel (rti, cur, color);
      cur.x = center.x + y0;
      cur.y = center.y - x0;
      bg_put_pixel (rti, cur, color);

      y0++;
      y_2q += y_odd;
      y_odd += 2;
      i++;
    }

    x0--;
    x2n += size_2;
    x_2q += x_odd;
    x_odd += 2;
    i++;
  }
}

void
draw_geom (RTImageBuf *rti, BgPixIter *pit)
{
  unsigned long color = 0x00000000;

  /* { /\* Now plot our line, safe and intuitive way.  *\/ */
  /*   Point2D p1 = { 10, 17 }; */
  /*   Point2D p2 = { 130, 210 }; */

  /*   Point2D delta = { p2.x - p1.x, p2.y - p1.y }; */
  /*   Point2D cur = { p1.x, p1.y }; */
  /*   Point2D signs = { (delta.x > 0) ? 1 : -1, (delta.y > 0) ? 1 : -1 }; */
  /*   unsigned limit = delta.x * delta.y; */
  /*   unsigned i; */
  /*   for (i = 0; i < limit; i++) { */
  /*     if (i % delta.y == 0) { */
  /* 	/\* Step by one x pixel.  *\/ */
  /* 	cur.x += signs.x; */
  /*     } */
  /*     if (i % delta.x == 0) { */
  /* 	/\* Step by one y pixel.  *\/ */
  /* 	cur.y += signs.y; */
  /*     } */
  /*     /\* PutPixel (cur); *\/ */
  /*     fbdata[(cur.y*320+cur.x)*4+0] = 0x00; */
  /*     fbdata[(cur.y*320+cur.x)*4+1] = 0x00; */
  /*     fbdata[(cur.y*320+cur.x)*4+2] = 0x00; */
  /*     fbdata[(cur.y*320+cur.x)*4+3] = 0x00; */
  /*   } */
  /* } */

  { /* Plot our line the fast way.  */
    Point2D p1 = { 10, 17 };
    Point2D p2 = { 130, 210 };

    bg_pit_moveto (pit, *(IPoint2D*)&p1);
    bg_pit_lineto64 (pit, *(IPoint2D*)&p2, color);
  }

  /* TODO FIXME: This segfaults with 50% rounding.  */
  if (1)
  { /* Plot our line the fast way.  */
    Point2D p1 = { 10, 210 };
    Point2D p2 = { 200, 170 };

    bg_pit_moveto (pit, *(IPoint2D*)&p1);
    bg_pit_lineto64 (pit, *(IPoint2D*)&p2, color);
  }

  { /* Plot our line the fast way.  */
    Point2D p1 = { 10, 150 };
    Point2D p2 = { 200, 150 };

    bg_pit_moveto (pit, *(IPoint2D*)&p1);
    bg_pit_lineto64 (pit, *(IPoint2D*)&p2, color);
  }

  { /* Plot our line the fast way.  */
    Point2D p1 = { 280, 50 };
    Point2D p2 = { 280, 190 };

    bg_pit_moveto (pit, *(IPoint2D*)&p1);
    bg_pit_lineto64 (pit, *(IPoint2D*)&p2, color);
  }

  { /* Plot a triangle.  */
    Point2D p1 = { 20, 20 };
    Point2D p2 = { 20, 90 };
    Point2D p3 = { 150, 10 };

    /*bg_pit_tri_line64 (pit,
		       *(IPoint2D*)&p1,
		       *(IPoint2D*)&p2,
		       *(IPoint2D*)&p3,
		       color);*/
    bg_pit_tri_fill64 (pit,
		       *(IPoint2D*)&p1,
		       *(IPoint2D*)&p2,
		       *(IPoint2D*)&p3,
		       color);
  }

  { /* Plot a triangle.  */
    Point2D p1 = { 20, 90 };
    Point2D p2 = { 150, 10 };
    Point2D p3 = { 130, 70 };

    /*bg_pit_tri_line64 (pit,
		       *(IPoint2D*)&p1,
		       *(IPoint2D*)&p2,
		       *(IPoint2D*)&p3,
		       color);*/
    bg_pit_tri_fill64 (pit,
		       *(IPoint2D*)&p1,
		       *(IPoint2D*)&p2,
		       *(IPoint2D*)&p3,
		       color);
  }

  bg_pit_flush_all (pit);

  { /* Plot a circle.  */
    Point2D p1 = { 150, 120 };
    plot_circle_fast (rti, color, p1, 60);
  }

  { /* Plot an ellipse.  */
    Point2D p1 = { 150, 120 };
    plot_ellipse_fast (rti, color, p1, 30, 65);
  }

  { /* Plot a parabola.  */
    Point2D p1 = { 200, 20 };
    plot_parabola_fast (rti, color, p1, 36, 80);
  }

  { /* Plot a hyperbola.  */
    Point2D p1 = { 150, 120 };
    plot_hyperbola_fast (rti, color, p1, 10, 10, 30);
  }

  { /* Plot a cubic curve.  */
    Point2D p1 = { 220, 120 };
    plot_cubic_fast (rti, color, p1, 36, 50);
  }

  { /* Flood fill!  */
    Point2D p1 = { 150, 170 };
    flood_fill (rti->image_data, p1, 0x00000000);
  }
}

int
main (int argc, char *argv[])
{
  Display *mydisplay;
  int myscreen;
  unsigned long myforeground, mybackground;
  XSizeHints myhint;
  Window   mywindow;
  char title[] = "Hello, World.";
  Cursor mycursor;
  Atom wmDeleteMessage;

  XVisualInfo myvisinfo;
  GC mygc;
  Pixmap mypixmap;
  XGCValues mygcvalues;
  XColor red;

  int fbwidth = 320;
  int fbheight = 240;
  /* ??? We request 24-bit, but we must plot 32-bit data.  Pixels in
     BGRA format.  Scan lines are in top-down order.  */
  char fbdata[4*320*240];
  XImage *myimage;
  RTImageBuf rti;
  BgPixIter pit;

  int done;
  int is_pressed = 0;
  int line_start = 0;
  Point2D last_pt, line_pt;

  /* { /\* Test patterns to determine the parameters of the image.  *\/
    unsigned i;
    unsigned char j;
    for (i = 0, j = 0; i < 4*320*240; i++) {
      if (((i / (320 * 4)) & 1) == 0 /\* && (i & (4 - 1)) == 2 *\/) {
	fbdata[i] = j;
	j = (j + 1) & (256 - 1);
	fbdata[i] = rand ();
      }
    }
  } */

  mydisplay = XOpenDisplay ("");
  myscreen = DefaultScreen (mydisplay);
  mybackground = WhitePixel (mydisplay, myscreen);
  myforeground = BlackPixel (mydisplay, myscreen);

  myhint.x = 200; myhint.y = 300;
  myhint.width = 350; myhint.height = 250;
  myhint.flags = PPosition|PSize;

  mywindow = XCreateSimpleWindow (mydisplay,
				  DefaultRootWindow (mydisplay),
				  myhint.x, myhint.y, myhint.width, myhint.height,
				  5, myforeground, mybackground);
  XSetStandardProperties (mydisplay, mywindow, title, title,
			  None, argv, argc, &myhint);

  mycursor = XCreateFontCursor (mydisplay, XC_left_ptr);
  XDefineCursor (mydisplay, mywindow, mycursor);

  wmDeleteMessage = XInternAtom(mydisplay, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(mydisplay, mywindow, &wmDeleteMessage, 1);

  XSelectInput (mydisplay, mywindow,
		ButtonPressMask
		| ButtonReleaseMask
		| PointerMotionMask
		| KeyPressMask
		| ExposureMask
		| EnterWindowMask
		| LeaveWindowMask);

  mygc = XCreateGC (mydisplay, mywindow, 0, 0);
  mypixmap = XCreateBitmapFromData (mydisplay, mywindow, 
				    stipple_bits, stipple_width, stipple_height);

  if (XMatchVisualInfo (mydisplay, myscreen, 24, TrueColor, &myvisinfo) == 0)
    fprintf (stderr, "error: no visuals\n");

  mygcvalues.foreground = myforeground;
  mygcvalues.background = mybackground;
  mygcvalues.fill_style = FillSolid;
  /* mygcvalues.fill_style = FillStippled; */
  /* mygcvalues.stipple = mypixmap; */
  XChangeGC (mydisplay, mygc,
	     GCForeground | GCBackground | GCFillStyle /* | GCStipple */, &mygcvalues);

  XAllocNamedColor (mydisplay,
		    DefaultColormap (mydisplay, myscreen),
		    "red", &red, &red);

  myimage = XCreateImage (mydisplay, myvisinfo.visual, 24, ZPixmap, 0, fbdata, fbwidth, fbheight, 8, 0);
  if (myimage == NULL)
    fprintf (stderr, "error: could not create image\n");

  rti.image_data = fbdata;
  rti.image_size = fbwidth * fbheight * 4;
  rti.pitch = fbwidth * 4;
  rti.hdr.width = fbwidth;
  rti.hdr.height = fbheight;
  rti.hdr.bpp = 32;
  rti.hdr.image_desc = TOP_LEFT;

  bg_pit_bind (&pit, &rti, 0, 0);

  bg_pit_clear_img64 (&pit, 0xffffffff);
  draw_geom (&rti, &pit);

  XMapRaised (mydisplay, mywindow);

  done = 0;
  while (done == 0) {
    XEvent myevent;
    char hi[] = "Hi!";
    char text[10];
    KeySym mykey;
    int i;

    /* fvwm(); */
    /* if (!XPending(mydisplay)) continue; */

    XNextEvent (mydisplay, &myevent);

#ifdef DEBUG
    _XPrintEvent (mydisplay, &myevent);
#endif

    switch (myevent.type) 
      {
      case Expose:
	if (myevent.xexpose.count == 0)
	  {
	    XWindowAttributes attribs;
	    XGetWindowAttributes (myevent.xexpose.display,
				  myevent.xexpose.window,
				  &attribs);
	    /* XFillRectangle (myevent.xexpose.display,
			    myevent.xexpose.window,
			    mygc,
			    0, 0,
			    attribs.width, attribs.height); */
	    XClearArea (myevent.xexpose.display,
			myevent.xexpose.window,
			0, 0,
			attribs.width, attribs.height,
			False);
	    XPutImage (myevent.xexpose.display,
		       myevent.xexpose.window,
		       mygc,
		       myimage,
		       0, 0,
		       0, 0,
		       rti.hdr.width, rti.hdr.height);
	    XDrawImageString (myevent.xexpose.display,
			      myevent.xexpose.window,
			      mygc,
			      50, 50,
			      title, strlen (title));
	    XDrawRectangle (myevent.xexpose.display,
			    myevent.xexpose.window,
			    mygc,
			    300, 10,
			    10, 10);
	    XFillRectangle (myevent.xexpose.display,
			    myevent.xexpose.window,
			    mygc,
			    300, 30,
			    10, 10);
	  }
	break;

      case ButtonPress:
	if (myevent.xbutton.button == Button1) {
	  XDrawImageString (myevent.xbutton.display,
			    myevent.xbutton.window,
			    mygc,
			    myevent.xbutton.x, myevent.xbutton.y,
			    hi, strlen (hi));
	}
	XDrawPoint (myevent.xbutton.display,
		    myevent.xbutton.window,
		    mygc,
		    myevent.xbutton.x, myevent.xbutton.y);
	if (myevent.xbutton.x < 320 && myevent.xbutton.y < 240) {
	  Point2D this_pt = { myevent.xbutton.x, myevent.xbutton.y };
	  *(unsigned int*)&fbdata[(this_pt.y*320+this_pt.x)*4+0] = 0x00000000;
	}
	is_pressed = 1;
	last_pt.x = myevent.xbutton.x;
	last_pt.y = myevent.xbutton.y;
	break;

      case ButtonRelease:
	is_pressed = 0;
	break;

      case MotionNotify:
	if (is_pressed) {
	  Point2D this_pt = { myevent.xmotion.x, myevent.xmotion.y };
	  XDrawLine (myevent.xmotion.display,
		     myevent.xmotion.window,
		     mygc,
		     last_pt.x, last_pt.y,
		     myevent.xmotion.x, myevent.xmotion.y);
	  if (last_pt.x < 320 && last_pt.y < 240 &&
	      myevent.xmotion.x < 320 && myevent.xmotion.y < 240 &&
	      last_pt.x >= 0 && last_pt.y >= 0 &&
	      myevent.xmotion.x >= 320 && myevent.xmotion.y >= 0)
	    {
	      bg_pit_moveto (&pit, *(IPoint2D*)&last_pt);
	      bg_pit_lineto64 (&pit, *(IPoint2D*)&this_pt, 0x00000000);
	      bg_pit_flush_all (&pit);
	    }
	}
	last_pt.x = myevent.xmotion.x;
	last_pt.y = myevent.xmotion.y;
	break;

      case KeyPress:
	i = XLookupString (&myevent.xkey, text, 10, &mykey, 0);
	if (i == 1) {
	  switch (text[0]) {
	  case 'q': done = 1; break;
	  case 'c':
	    bg_pit_clear_img64 (&pit, 0xffffffff);
	    XPutImage (mydisplay,
		       mywindow,
		       mygc,
		       myimage,
		       0, 0,
		       0, 0,
		       rti.hdr.width, rti.hdr.height);
	    break;
	  case 'd':
	    draw_geom (&rti, &pit);
	    XPutImage (mydisplay,
		       mywindow,
		       mygc,
		       myimage,
		       0, 0,
		       0, 0,
		       rti.hdr.width, rti.hdr.height);
	    break;
	  case 'f':
	    flood_fill (fbdata, last_pt, 0x00000000);
	    XPutImage (mydisplay,
		       mywindow,
		       mygc,
		       myimage,
		       0, 0,
		       0, 0,
		       rti.hdr.width, rti.hdr.height);
	    break;
	  case 'l':
	    if (!line_start) {
	      line_pt = last_pt;
	      line_start = 1;
	    } else {
	      if (line_pt.x < 320 && line_pt.y < 240 &&
		  last_pt.x < 320 && last_pt.y < 240 &&
		  line_pt.x >= 0 && line_pt.y >= 0 &&
		  last_pt.x >= 0 && last_pt.y >= 0) {
		bg_pit_moveto (&pit, *(IPoint2D*)&line_pt);
		bg_pit_lineto64 (&pit, *(IPoint2D*)&last_pt, 0x00000000);
		bg_pit_flush_all (&pit);
	      }
	      XPutImage (mydisplay,
			 mywindow,
			 mygc,
			 myimage,
			 0, 0,
			 0, 0,
			 rti.hdr.width, rti.hdr.height);
	      line_start = 0;
	    }
	    break;
	  }
	}
	XDrawImageString (myevent.xbutton.display,
			  myevent.xbutton.window,
			  mygc,
			  5, 15,
			  text, strlen (text));
	break;

      case EnterNotify:
	XSetWindowBorder (mydisplay, myevent.xcrossing.window, red.pixel);
	break;

      case LeaveNotify:
	XSetWindowBorder (mydisplay, myevent.xcrossing.window, myforeground);
	break;

      case ClientMessage:
	if (myevent.xclient.data.l[0] == wmDeleteMessage)
	  done = 1;
	break;
      }
  }
  /* Set data to NULL before calling `XDestroyImage()' since this was
     not allocated on the heap and should not be freed.  */
  myimage->data = NULL;
  XDestroyImage (myimage);
  XFreeColors (mydisplay, DefaultColormap (mydisplay, myscreen),
	       &red.pixel, 1, 0);
  XFreePixmap (mydisplay, mypixmap);
  XFreeGC (mydisplay, mygc);
  XDestroyWindow (mydisplay, mywindow);
#ifdef DEBUG
  _XPrintTree (mydisplay, DefaultRootWindow (mydisplay), 0);
#endif
  XCloseDisplay (mydisplay);
  return 0;
}

/*

How the midpoint circle algorithm works.

1  1  = 1^2
3  4  = 2^2
5  9  = 3^2
7  16 = 4^2
9  25 = 5^2
11 36 = 6^2

x^2 + y^2 = r^2

r = 10

x = 10
y = 0

100 + 0 = 100

(x - 1)^2
(x - 2)^2
(x - 3)^2

(y + 1)^2
(y + 2)^2
(y + 3)^2

(x^2 - 2*x*1 + 1) + (y^2 + 2*y*1 + 1) = r^2

So what do we want to do?  We want to step by one pixel along the
x-axis, and determine what value should be used for the y-axis.  Base
case, x^2 == r^2, y^2 == 0.

(x^2 - 2*x*1 + 1) + y^2 = r^2
(-2*x*1 + 1) + y^2 = 0
(-2*10*1 + 1) + y^2 = 0
-19 + y^2 = 0
y^2 = 19

Okay, so what does this mean?  We keep computing consecutive squares
and plotting pixels until we exceed 19.  Which is computed how?

2*x*n - n^2

So we don't need to compute squares when we start.  "n" is the number
of times we want to decrement on the x-axis.  First iteration is easy
since n = 1.  But subsequent iterations?  Do we really have to
multiply?  Well, no, we can keep adding 2*x as we go along, and that
will tantamount to multiplication.  Okay, great.  Well... technically
2*r as we go along.  Ha, but it's the same thing at the start.

Finally, this is the real trick.  You can use the midpoint circle
algorithm to compute pi and generate lookup tables for the sine,
cosine, tangent, arc-sign, arc-cosine, arc-tangent, and atan2()
functions.  Here's how.  Keep track of the number of horizontal,
vertical, and diagonal pixels you move.  This is a distance of 1 for
horizontal and vertical pixels, and a distance of sqrt(2) for diagonal
pixels.  This is also the distance you've traveled across the edge of
the circle, or the circumference of an arc.  The circumference, of
course, can be used to compute the angle in radians, and also pi
itself.

Computing a binary fixed-point square root of two is fairly easy.  You
start the less-than approximation, 1.25, and you sequentially test
which remaining bits should be set to get as close to the square root
of two as possible.  The idea is to always stay less then so that you
can simply add more bits without needing to subtract from the whole
number.

But alas, if you test this out, you'll get some funny stuff at each
extrema of the circle, off by one pixel?  Well, technically no.  What
is happening is that this depends on your threshold of rounding, but
if you check the algorithms carefully, you'll see that everything is
correct.  So basically, what you want to to is offset the thresholding
by 50% of a pixel in order to get a plot with symmetric rounding.  How
do you do this?  Think about how you increment your counters for a
full step.  Now, simply take a half-step forward instead.

With the symmetry available from taking a half-step forward at the
start of rasterization, you now only need to compute one octant of the
circle rather than a full quadrant, and you can mirror it to get the
other octant.  So this saves you even more computation.

How about plotting ellipses?  Well, this is a fairly easy extension.
Start with the equation of an ellipse:

x^2 / a^2 + y^2 / b^2 = 1

Now multiply to get rid of those silly denominators.

b^2 * x^2 + a^2 * y^2 = a^2 * b^2

Starting equation, y^2 = 0, looks like this:

b^2 * x^2 = a^2 * b^2
x^2 = a^2

So really you start out the same here as you do in the circle case,
and the additional multiplier constants can simply be handled in a
single-step manner by adding the values to the running totals.  You do
have to compute both these squares initially, though.

 */

/*

Pixel-perfect quadratic Bezier plotting.

((1 - t) + t)^2
(1 - t)^2 + 2*(1 - t)*t + t^2
a*(1 - t)^2 + b*2*(1 - t)*t + c*t^2
a*(1 - t)^2 + b*2*t - b*2*t^2 + c*t^2

Then we only need special handling for the square term.  We
know the maximum value, and we step down to zero.  So it is
just like Bresenham's circle algorithm.  We just need to know
the limits of t.

If a == 0 and c == 2*b, then those terms cancel out and we are
left with a straight line.

b*2*t

If a != 0, then we can still get similar metrics by shifting
the curve to zero.

a - 2*a*t + a*t^2 + b*2*t - b*2*t^2 + c*t^2
a - 2*a*t + b*2*t + a*t^2 - b*2*t^2 + c*t^2
a + (b - a)*2*t + (a + c - 2*b)*t^2

So as you can see here, if we define b_l and c_l as the offsets from
`a' to get to `b' and `c', you get the following:

a + ((b_l + a) - a)*2*t + (a + (c_l + a) - 2*(b_l + a))*t^2

Simplifying, we get the following:

a + b_l*2*t + (c_l - 2*b_l)*t^2

And we're almost right back to where we started if we set c == 2*b.

So anyways, we can compute the straight line stepping distance given
this.  Our goal is to define a t-space where if you step by only a
single unit, you will never skip over pixels.  This, we can find out
by the beginning and end terms, which define how much faster we might
step than linear pace.  We step forward/backward by one unit from the
beginning/end, then if we determined that we've stepped on the output
by more than one unit, we scale the t-space accordingly, and try again
until we verify that we cannot step too quickly.

Okay, that sounds a bit inefficient.  Take the derivative?  Well,
hey, it's the same thing!  Okay, no not quite.  Yeah, if you scale
the t-space, you also have to scale the coefficients, or divide
by a number throughout.

Here's how it works, for scaling parabolas in fixed-point.

y = x^2
y/100 = (x/100)^2
y/100 = 1/10000*x^2
y = 1/100*x^2

Yep, simple as that.  Divide by the scaling factor.  The minimum
step size can be determined from the derivative.

2/b*a*x

So now you simply want to make sure that if a > b, then you correspondingly
adjust the scaling factor to include it too.

The thing about the derivative, since your curve function is your
position function, it tells your the instantaneous velocity at that
point.  The unknown you want to solve for is the sampling density
required such that one step on the input will result in one step on
the output.  With the derivative, this is all a linear solution
process.

Once you know your sampling density, it is all a matter of variable
stepping through the space for the non-linear portions.  The linear
portions are a constant stepping through the space.

Start with no multiplier, take the derivative at the beginning and
end points.  When you move one t, how many x's do you move?  Now
simply set your multiplier to the maximum.  When you step by a single
unit in the new scaled t input space, you are then guaranteed to
step by a single unit or less in the x space.

Simply put, you then use your derivative to find out how big your
steps should be on subsequent points of the curve.  Step by the
inverse of the derivative... works okay if you are rasterizing a
large curve, small curves may have bad numerics.

Yeah, seriously, start by simply coding up parabola graphing at
any resolution.

PARABOLAS

Need to add a linear term on top of the quadratic term?  No worries,
remember from basic Algebra, this is really just shifting the parabola
left and right.  Completing the square is used to determine by how
much, along with a corresponding vertical shift to compensate.
Don't believe me?  Try this experiment.  Create a parabola
from a cubic Bezier curve in Inkscape by setting the control points
at the 1/3 and 2/3 curve positions.  Then skew the curve vertically.
This is in effect rather similar to adding a line to the curve
function.  You'll notice that the parabola shape is maintained, it's
just that the curve and its endpoints are shifted left and right,
with a small vertical shift too.

Now, if you skew horizontally, you get a different picture.  What
exactly is happening here?  You're not adding a line function to
the y-values in the traditional sense, but rather to the x-values,
and that's why it has the different effect.

Cubic curves


Delta functions are a bit complicated:

x^3
(x + 1)^3 = x^3 + 3*x^2 + 3*x + 1

So, you can count up thirds, yes, but you must also count up odd
numbers to get squares since there is a quadratic term also.

3*(x + 1)^2 = 3*x^2 + 6*x + 3

So, let's do an example.

 n  3-odd   6-odd   3*x^2   x^3
 0      1       3       0     0
 1      4       9       3     1
 2      7      15      12     8
 3     10      21      27    27
 4     13      27      48    64
 5     16      33      75   125
 6     19      39     108   216
 7     22      45     147   343
 8     25      51     192   512
 9     28      57     243   729
10     31      63     300  1000
11     34      69     363  1331
12     37      75     432  1728

Wow, that's perfect.  And it almost feels like magic once you know
how to do it.  But yeah, to work with cubic curves in an integer
perfect manner, it sure does require large numbers.

That being  said, in the interest of numerical stability, there is
quite a strong incentive to use de Casteljau's algorithm on a cubic
Bezier curve until the control points reach 1/3 and 2/3 positions
where the segment can be reasonably approximated using a quadratic
Bezier curve, and then hte quadratic segments can be rendered
pixel-perfect.

However, the crux of the matter is this.  The whole idea with the
delta methods is that you can eliminate the t-value position
book-keeping, and once you do, the magnitude of t no longer matters.
All that matters is that the thresholds are within reasonable
integer bounds, and that is the simplifying assumption that is the
saving grace.

So, yes... speaking of that, let's revisit the parabola case.
In the parabola case, we only need to sum up odd numbers, so the
thresholding is as simple as keeping track of the current odd
number, no need to keep track of the current square.

However, in the case of cubic curves, we do need to keep track of a
running sum of squares since a square term is also used in the
delta computations.  So, unfortunately, as the powers become higher
and the integers become larger, it becomes less and less viable to
perform pixel-perfect computations using integer arithmetic.

*/
