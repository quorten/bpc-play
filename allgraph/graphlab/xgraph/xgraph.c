/* xgraph.c: Simpler graphics lab implementation that depends only on
   X11.  Easier to compile and less dependencies than the GTK+
   version.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "ivecmath.h"

char title[] = "Graphics Lab";

IVPoint2D_i32 p1 = {{ 50, 50 }};
IVPoint2D_i32 p2 = {{ 200, 150 }};
IVPoint2D_i32 p3 = {{ 150, 200 }};
IVPoint2D_i32 p4 = {{ 150, 190 }};

void
draw_geom (Display *display, Window window, GC mygc)
{
  IVVec2D_i32 v1, v2, v3;
  IVPoint2D_i32 pc;

  iv_sub3_v2i32 (&v1, &p2, &p1);
  iv_sub3_v2i32 (&v2, &p3, &p1);
  iv_sub3_v2i32 (&v3, &p4, &p3);

  XDrawLine (display, window, mygc,
	     p1.d[IX], p1.d[IY], p2.d[IX], p2.d[IY]);
  XDrawLine (display, window, mygc,
	     p2.d[IX], p2.d[IY], p3.d[IX], p3.d[IY]);
  XDrawLine (display, window, mygc,
	     p3.d[IX], p3.d[IY], p4.d[IX], p4.d[IY]);

  /* Now we're getting interesting.  Project p3 onto the line p1 - p2,
     through the shortest-distance perpendicular, and draw the
     line.  */
  {
    IVVec2D_i32 vt = {{ -v1.d[IY], v1.d[IX] }}; /* Perpendicular */
    IVPoint2D_i32 pp; /* projected perpendicular point on line */
    IVNLine_v2i32 plane = { vt, p1 };
    IVRay_v2i32 v3_ray = { v3, p3 };

    /* Use our spiffy new subroutines to compute perpendiculars and
       solutions.  */
    iv_proj3_p2i32_NLine_v2i32 (&pp, &p3, &plane);
    XDrawLine (display, window, mygc,
	       p3.d[IX], p3.d[IY], pp.d[IX], pp.d[IY]);
    /* XFillRectangle (display, window, mygc,
		    pp.d[IX] - 5, pp.d[IY] - 5, 10, 10); */

    iv_isect3_Ray_NLine_v2i32 (&pp, &v3_ray, &plane);

    if (pp.d[IX] != IVINT32_MIN) {
      XFillRectangle (display, window, mygc,
		      pp.d[IX] - 5, pp.d[IY] - 5, 10, 10);
    }
  }
}

void
drag_point (Display *display, Window window, GC mygc, int x, int y)
{
  IVPoint2D_i32 mouse_pt = {{ x, y }};

  { /* Find out which point we're selecting and moving based off of
       the closest match.  N.B.: We compare the distance squared so
       that we don't have to compute the square root.  */
    IVint64 p1_dist_2, p2_dist_2, p3_dist_2, p4_dist_2;
    p1_dist_2 = iv_dist2q2_p2i32 (&mouse_pt, &p1);
    p2_dist_2 = iv_dist2q2_p2i32 (&mouse_pt, &p2);
    p3_dist_2 = iv_dist2q2_p2i32 (&mouse_pt, &p3);
    p4_dist_2 = iv_dist2q2_p2i32 (&mouse_pt, &p4);
    if (p1_dist_2 < p2_dist_2 && p1_dist_2 < p3_dist_2 &&
	p1_dist_2 < p4_dist_2)
      {
	/* Select p1 */
	p1.d[IX] = mouse_pt.d[IX]; p1.d[IY] = mouse_pt.d[IY];
      }
    else if (p2_dist_2 < p1_dist_2 && p2_dist_2 < p3_dist_2 &&
	     p2_dist_2 < p4_dist_2)
      {
	/* Select p2 */
	p2.d[IX] = mouse_pt.d[IX]; p2.d[IY] = mouse_pt.d[IY];
      }
    else if (p3_dist_2 < p2_dist_2 && p3_dist_2 < p1_dist_2 &&
	     p3_dist_2 < p4_dist_2)
      {
	/* Select p3 */
	p3.d[IX] = mouse_pt.d[IX]; p3.d[IY] = mouse_pt.d[IY];
      }
    else /* if (p4_dist_2 < p1_dist_2 && p4_dist_2 < p2_dist_2 &&
	     p4_dist_2 < p3_dist_2) */
      {
	/* Select p4 */
	p4.d[IX] = mouse_pt.d[IX]; p4.d[IY] = mouse_pt.d[IY];
      }
  }

  /* TODO: Now invalidate the affected region of the drawing area.
     Maybe we should use double-buffering or clipping for flicker-free
     display updates.  */
  {
    XWindowAttributes attribs;
    XGetWindowAttributes (display, window, &attribs);
    XClearArea (display, window,
		0, 0,
		attribs.width, attribs.height,
		False);
    draw_geom(display, window, mygc);
  }
}

int
main (int argc, char *argv[])
{
  Display *mydisplay;
  Window   mywindow;
  GC mygc;
  XGCValues mygcvalues;
  XEvent myevent;
  KeySym mykey;
  XSizeHints myhint;
  int myscreen;
  unsigned long myforeground, mybackground;
  int i;
  char text[10];
  int done;
  XColor red;
  Cursor mycursor;
  Atom wmDeleteMessage;
  int is_pressed = 0;
  int line_start = 0;
  IVPoint2D_i32 last_pt;

  XVisualInfo myvisinfo;

  mydisplay = XOpenDisplay ("");
  myscreen = DefaultScreen (mydisplay);
  /* mybackground = WhitePixel (mydisplay, myscreen);
  myforeground = BlackPixel (mydisplay, myscreen); */
  mybackground = 0xff000000; /* Black BGRA little endian */
  myforeground = 0xff00ff00; /* Green BGRA little endian */

  myhint.x = 0; myhint.y = 0;
  myhint.width = 600; myhint.height = 400;
  myhint.flags = /*PPosition|*/PSize;

  mywindow = XCreateSimpleWindow (mydisplay,
				  DefaultRootWindow (mydisplay),
				  myhint.x, myhint.y, myhint.width, myhint.height,
				  5, myforeground, mybackground);
  XSetStandardProperties (mydisplay, mywindow, title, title,
			  None, argv, argc, &myhint);

  mycursor = XCreateFontCursor (mydisplay, XC_left_ptr);
  XDefineCursor (mydisplay, mywindow, mycursor);

  mygc = XCreateGC (mydisplay, mywindow, 0, 0);

  if (XMatchVisualInfo (mydisplay, myscreen, 24, TrueColor, &myvisinfo) == 0)
    fprintf (stderr, "error: no visuals\n");

  mygcvalues.foreground = myforeground;
  mygcvalues.background = mybackground;
  mygcvalues.fill_style = FillSolid;
  XChangeGC (mydisplay, mygc,
	     GCForeground | GCBackground | GCFillStyle /* | GCStipple */, &mygcvalues);

  XAllocNamedColor (mydisplay,
		    DefaultColormap (mydisplay, myscreen),
		    "red", &red, &red);

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

  XMapRaised (mydisplay, mywindow);

  done = 0;
  while (done == 0) {

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
	    XClearArea (myevent.xexpose.display,
			myevent.xexpose.window,
			0, 0,
			attribs.width, attribs.height,
			False);
	    draw_geom (myevent.xexpose.display, myevent.xexpose.window, mygc);
	  }
	break;

      case ButtonPress:
	if (myevent.xbutton.button == Button1)
	  XDrawPoint (myevent.xbutton.display,
		      myevent.xbutton.window,
		      mygc,
		      myevent.xbutton.x, myevent.xbutton.y);
	is_pressed = 1;
	last_pt.d[IX] = myevent.xbutton.x;
	last_pt.d[IY] = myevent.xbutton.y;
	break;

      case ButtonRelease:
	is_pressed = 0;
	break;

      case MotionNotify:
	if (is_pressed) {
	  if ((myevent.xmotion.state & Button1Mask))
	    XDrawLine (myevent.xmotion.display,
		       myevent.xmotion.window,
		       mygc,
		       last_pt.d[IX], last_pt.d[IY],
		       myevent.xmotion.x, myevent.xmotion.y);
	  else if ((myevent.xmotion.state & Button3Mask))
	    drag_point (myevent.xmotion.display, myevent.xmotion.window,
			mygc, myevent.xmotion.x, myevent.xmotion.y);
	}
	last_pt.d[IX] = myevent.xmotion.x;
	last_pt.d[IY] = myevent.xmotion.y;
	break;

      case KeyPress:
	i = XLookupString (&myevent.xkey, text, 10, &mykey, 0);
	if (i == 1) {
	  switch (text[0]) {
	  case 'q': done = 1; break;
	  case 'c':
	  {
	    XWindowAttributes attribs;
	    XGetWindowAttributes (myevent.xkey.display,
				  myevent.xkey.window,
				  &attribs);
	    XClearArea (myevent.xkey.display,
			myevent.xkey.window,
			0, 0,
			attribs.width, attribs.height,
			False);
	    break;
	  }
	  case 'd':
	    draw_geom (myevent.xkey.display, myevent.xkey.window, mygc);
	    break;
	  }
	}
	/* XDrawImageString (myevent.xbutton.display,
			  myevent.xbutton.window,
			  mygc,
			  5, 15,
			  text, strlen (text)); */
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
  XFreeGC (mydisplay, mygc);
  XDestroyWindow (mydisplay, mywindow);
#ifdef DEBUG
  _XPrintTree (mydisplay, DefaultRootWindow (mydisplay), 0);
#endif
  XCloseDisplay (mydisplay);
  exit (0);
}
