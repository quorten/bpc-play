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

IVPoint2D_i32 p1 = { 50, 50 };
IVPoint2D_i32 p2 = { 200, 150 };
IVPoint2D_i32 p3 = { 150, 200 };
IVPoint2D_i32 p4 = { 150, 190 };

IVPoint2D_i32 reg_pts_stor[32];
IVPoint2D_i32_array reg_pts = { reg_pts_stor, 0 };

/* Index of selected mouse point, or (IVuint16)-1 if none.  */
IVuint16 isect_drag_idx = (IVuint16)-1;
IVuint16 reg_drag_idx = (IVuint16)-1;

/* Quality factor view mode, 0 = disable, 1 = isect, 2 = linreg.  */
IVuint8 qf_view = 0;

void
draw_geom (Display *display, Window window, GC mygc)
{
  IVVec2D_i32 v1, v2, v3;
  IVPoint2D_i32 pc;

  char isect_qf_str[32] = "";
  char linreg_qf_str[32] = "";
  char mat_line1[64] = "";
  char mat_line2[64] = "";

  iv_sub3_v2i32 (&v1, &p2, &p1);
  iv_sub3_v2i32 (&v2, &p3, &p1);
  iv_sub3_v2i32 (&v3, &p4, &p3);

  XDrawLine (display, window, mygc,
	     p1.x, p1.y, p2.x, p2.y);
  XDrawLine (display, window, mygc,
	     p2.x, p2.y, p3.x, p3.y);
  XDrawLine (display, window, mygc,
	     p3.x, p3.y, p4.x, p4.y);

  /* Now we're getting interesting.  Project p3 onto the line p1 - p2,
     through the shortest-distance perpendicular, and draw the
     line.  */
  {
    IVVec2D_i32 vt = { -v1.y, v1.x }; /* Perpendicular */
    IVPoint2D_i32 pp; /* projected perpendicular point on line */
    IVNLine_v2i32 plane = { vt, p1 };
    IVRay_v2i32 v3_ray = { v3, p3 };

    /* Use our spiffy new subroutines to compute perpendiculars and
       solutions.  */
    iv_proj3_p2i32_NLine_v2i32 (&pp, &p3, &plane);
    if (!iv_is_nosol_v2i32 (&pp)) {
      XDrawLine (display, window, mygc,
		 p3.x, p3.y, pp.x, pp.y);
      /* XFillRectangle (display, window, mygc,
		      pp.x - 5, pp.y - 5, 10, 10); */
    }

    iv_isect3_Ray_NLine_v2i32 (&pp, &v3_ray, &plane);

    if (!iv_is_nosol_v2i32 (&pp)) {
      XFillRectangle (display, window, mygc,
		      pp.x - 5, pp.y - 5, 10, 10);
    }

    if (qf_view == 1) {
      /* Compute the equation solving quality factor on our setup.  */
      IVSys2_Eqs_v2i32 sys;
      IVint32 qualfac;
      sys.d[0].v = v1;
      sys.d[1].v = v3;
      qualfac = iv_aprequalfac_i32q16_s2_Eqs_v2i32 (&sys);
      sprintf (isect_qf_str, "isect qf = %f", (float)qualfac / 0x10000);
    }
  }

  /* Draw all the linear regression points.  */
  {
    IVuint16 i;
    for (i = 0; i < reg_pts.len; i++) {
      XFillRectangle (display, window, mygc,
		      reg_pts.d[i].x - 5,
		      reg_pts.d[i].y - 5,
		      10, 10);
    }
  }

  if (reg_pts.len >= 2) {
    /* Compute and draw the linear regression line.  */
    /* y = m*x + b, vector format `[b, m]` */
    IVSys2_Eqs_v2i32q16 sys;
    IVint32 qualfac;
    IVPoint2D_i32 coeffs;
    IVPoint2D_i32 r_p1, r_p2;
    iv_pack_linreg_s2_Eqs_v2i32q16_v2i32 (&sys, &reg_pts, 0);
    qualfac = iv_prequalfac_i32q16_s2_Eqs_v2i32 (&sys);
    iv_solve2_s2_Eqs_v2i32 (&coeffs, &sys);
    r_p1.x = 0;
    r_p1.y = (coeffs.y * (r_p1.x - 0) + coeffs.x) >> 0x10;
    r_p2.x = 600;
    r_p2.y = (coeffs.y * (r_p2.x - 0) + coeffs.x) >> 0x10;
    XDrawLine (display, window, mygc,
	       r_p1.x, r_p1.y, r_p2.x, r_p2.y);

    if (qf_view == 2) {
      /* Show the quality factor of the internal system of equations
	 built to compute the linear regression coefficients.  */
      sprintf (linreg_qf_str, "linreg qf = %f", (float)qualfac / 0x10000);
      sprintf (mat_line1, "[ %f %f %f",
	       (float)sys.d[0].v.x / 0x10000,
	       (float)sys.d[0].v.y / 0x10000,
	       (float)sys.d[0].offset / 0x100000000LL);
      sprintf (mat_line2, "  %f %f %f ]",
	       (float)sys.d[1].v.x / 0x10000,
	       (float)sys.d[1].v.y / 0x10000,
	       (float)sys.d[1].offset / 0x100000000LL);
    }
  }

  /* Defer actually drawing text until the very end so that it is
     drawn on top of all graphics.  */
  if (qf_view == 1) {
    XDrawImageString (display, window, mygc,
		      5, 15,
		      isect_qf_str, strlen (isect_qf_str));
  } else if (qf_view == 2) {
    XDrawImageString (display, window, mygc,
		      5, 15,
		      linreg_qf_str, strlen (linreg_qf_str));
    XDrawImageString (display, window, mygc,
		      300, 15,
		      mat_line1, strlen (mat_line1));
    XDrawImageString (display, window, mygc,
		      300, 30,
		      mat_line2, strlen (mat_line2));
  }
}

void
redraw_geom (Display *display, Window window, GC mygc)
{
  XWindowAttributes attribs;
  XGetWindowAttributes (display, window, &attribs);
  XClearArea (display, window,
	      0, 0,
	      attribs.width, attribs.height,
	      False);
  draw_geom(display, window, mygc);
}

/* Find out which point we're selecting with the mouse based off of
   the closest match.  N.B.: We compare the distance squared so that
   we don't have to compute the square root.

   Actually, we don't even need to use fancy vector math to pick the
   point with the mouse, just check if the mouse point is within a
   bounding box around the test point, and pick the first such
   matching point.

   Admittedly, however, it does make for an easier user interface if
   you don't have to be within the bounding box to click and drag a
   point.  */
IVuint16
pick_point (IVint64 *result_dist_2,
	    IVPoint2D_i32_array *pts, IVPoint2D_i32 *mouse_pt)
{
  IVPoint2D_i32 *pts_d = pts->d;
  IVuint16 pts_len = pts->len;
  IVint64 pick_dist_2 = 0x0fffffffffffffffLL;
  IVuint16 pick = (IVuint16)-1;
  const IVuint8 fancy_pick_math = 1;
  IVuint16 i;
  for (i = 0; i < pts_len; i++) {
    if (fancy_pick_math) {
      IVint64 i_dist_2 = iv_dist2q2_p2i32 (mouse_pt, &pts_d[i]);
      if (i_dist_2 < pick_dist_2) {
	pick_dist_2 = i_dist_2;
	pick = i;
      }
    } else {
      IVVec2D_i32 diff;
      iv_sub3_v2i32 (&diff, mouse_pt, &pts_d[i]);
      if (iv_abs_i32 (diff.x) <= 5 &&
	  iv_abs_i32 (diff.y) <= 5) {
	pick = i;
	break;
      }
    }
  }
  if (fancy_pick_math)
    *result_dist_2 = pick_dist_2;
  else {
    if (pick != (IVuint16)-1)
      *result_dist_2 = 0;
    else
      *result_dist_2 = 0x0fffffffffffffffLL;
  }
  return pick;
}

void
drag_point (Display *display, Window window, GC mygc, int x, int y)
{
  IVPoint2D_i32 mouse_pt = { x, y };

  if (isect_drag_idx == (IVuint16)-1 && reg_drag_idx == (IVuint16)-1) {
    /* Find out which point we should select.  */
    IVPoint2D_i32 isect_stor[] = { p1, p2, p3, p4 };
    IVPoint2D_i32_array isect; isect.d = isect_stor; isect.len = 4;
    IVint64 isect_dist_2;
    IVint64 reg_dist_2;
    isect_drag_idx = pick_point (&isect_dist_2, &isect, &mouse_pt);
    reg_drag_idx = pick_point (&reg_dist_2, &reg_pts, &mouse_pt);
    if (isect_dist_2 < reg_dist_2)
      reg_drag_idx = (IVuint16)-1;
    else
      isect_drag_idx = (IVuint16)-1;
  }

  /* Drag the selected point.  */
  if (isect_drag_idx != (IVuint16)-1) {
    switch (isect_drag_idx) {
    case 0:
      /* Drag p1 */
      p1.x = mouse_pt.x; p1.y = mouse_pt.y;
      break;
    case 1:
      /* Drag p2 */
      p2.x = mouse_pt.x; p2.y = mouse_pt.y;
      break;
    case 2:
      /* Drag p3 */
      p3.x = mouse_pt.x; p3.y = mouse_pt.y;
      break;
    case 3:
      /* Drag p4 */
      p4.x = mouse_pt.x; p4.y = mouse_pt.y;
      break;
    }
  }
  if (reg_drag_idx != (IVuint16)-1) {
    reg_pts.d[reg_drag_idx].x = mouse_pt.x;
    reg_pts.d[reg_drag_idx].y = mouse_pt.y;
  }

  /* TODO: Now invalidate the affected region of the drawing area.
     Maybe we should use double-buffering or clipping for flicker-free
     display updates.  Note, however, that the flicker is only visible
     on non-compositing window managers.  */
  redraw_geom(display, window, mygc);
}

int
main (int argc, char *argv[])
{
  Display *mydisplay;
  int myscreen;
  unsigned long myforeground, mybackground;
  XSizeHints myhint;
  Window   mywindow;
  char title[] = "Graphics Lab";
  Cursor mycursor;
  Atom wmDeleteMessage;

  XVisualInfo myvisinfo;
  GC mygc;
  XGCValues mygcvalues;
  XColor red;

  int done;
  int is_pressed = 0;
  IVPoint2D_i32 last_pt;

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

  XMapRaised (mydisplay, mywindow);

  done = 0;
  while (done == 0) {
    XEvent myevent;
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
	  redraw_geom (myevent.xexpose.display, myevent.xexpose.window, mygc);
	break;

      case ButtonPress:
	if (myevent.xbutton.button == Button1)
	  XDrawPoint (myevent.xbutton.display,
		      myevent.xbutton.window,
		      mygc,
		      myevent.xbutton.x, myevent.xbutton.y);
	is_pressed = 1;
	last_pt.x = myevent.xbutton.x;
	last_pt.y = myevent.xbutton.y;
	break;

      case ButtonRelease:
	is_pressed = 0;
	isect_drag_idx = (IVuint16)-1;
	reg_drag_idx = (IVuint16)-1;
	break;

      case MotionNotify:
	if (is_pressed) {
	  if ((myevent.xmotion.state & Button1Mask))
	    XDrawLine (myevent.xmotion.display,
		       myevent.xmotion.window,
		       mygc,
		       last_pt.x, last_pt.y,
		       myevent.xmotion.x, myevent.xmotion.y);
	  else if ((myevent.xmotion.state & Button3Mask))
	    drag_point (myevent.xmotion.display, myevent.xmotion.window,
			mygc, myevent.xmotion.x, myevent.xmotion.y);
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
	  case 'e':
	    /* Clear all linear regression data points.  */
	    reg_pts.len = 0;
	    redraw_geom (myevent.xkey.display, myevent.xkey.window, mygc);
	    break;
	  case 'd':
	    draw_geom (myevent.xkey.display, myevent.xkey.window, mygc);
	    break;
	  case 'p':
	    /* Add a linear regression data point.  */
	    if (reg_pts.len < 25) {
	      reg_pts.d[reg_pts.len].x = myevent.xkey.x;
	      reg_pts.d[reg_pts.len].y = myevent.xkey.y;
	      reg_pts.len++;
	    }
	    redraw_geom (myevent.xkey.display, myevent.xkey.window, mygc);
	    break;
	  case 'x':
	    if (reg_pts.len > 0) {
	      /* Delete a linear regression data point.  */
	      IVPoint2D_i32 mouse_pt = { myevent.xkey.x, myevent.xkey.y };
	      IVint64 result_dist_2;
	      IVuint16 pick_idx =
		pick_point(&result_dist_2, &reg_pts, &mouse_pt);
	      /* Overwrite the deleted point with the last point since
		 the order of the array is not important, plus this is
		 faster and easier to code than moving all elements
		 over.  */
	      reg_pts.d[pick_idx] = reg_pts.d[--reg_pts.len];
	      redraw_geom (myevent.xkey.display, myevent.xkey.window, mygc);
	    }
	    break;
	  case 'v':
	    /* Change quality factor view mode.  */
	    qf_view++;
	    if (qf_view > 2)
	      qf_view = 0;
	    redraw_geom (myevent.xkey.display, myevent.xkey.window, mygc);
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
  XFreeColors (mydisplay, DefaultColormap (mydisplay, myscreen),
	       &red.pixel, 1, 0);
  XFreeGC (mydisplay, mygc);
  XDestroyWindow (mydisplay, mywindow);
#ifdef DEBUG
  _XPrintTree (mydisplay, DefaultRootWindow (mydisplay), 0);
#endif
  XCloseDisplay (mydisplay);
  return 0;
}
