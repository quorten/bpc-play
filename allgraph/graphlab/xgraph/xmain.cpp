// libX11 + GLX main startup for the Rubik's Cube.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "GfxOpenGL.h"

bool exiting = false;
bool drawFrame = false;
bool renderLoop = false;
bool leftArrowDwn = false, rightArrowDwn = false; //Arrow keys
bool upArrowDwn = false, dwnArrowDwn = false;
const int windowWidth = 800;
const int windowHeight = 600;
const int windowBits = 32;

bool fullscreen = false;
Atom wmDeleteMessage;
int lastTime;

GfxOpenGL* g_glRender = NULL;

static XVisualInfo *
SetupVisual (Display *mydisplay, int myscreen)
{
	int attribs[64];
	int i = 0;
	// Singleton attributes.
	attribs[i++] = GLX_RGBA;
	attribs[i++] = GLX_DOUBLEBUFFER;

	// Key/value attributes.
	attribs[i++] = GLX_RED_SIZE;
	attribs[i++] = 1;
	attribs[i++] = GLX_GREEN_SIZE;
	attribs[i++] = 1;
	attribs[i++] = GLX_BLUE_SIZE;
	attribs[i++] = 1;
	attribs[i++] = GLX_DEPTH_SIZE;
	attribs[i++] = 1;
	attribs[i++] = None;

	return glXChooseVisual (mydisplay, myscreen, attribs);
}

inline void
EnableRenderLoop ()
{
	if (renderLoop == false)
	{
		renderLoop = true;
		struct timespec tp;
		clock_gettime (CLOCK_MONOTONIC, &tp);
		lastTime = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
	}
}

inline void
DisableRenderLoop ()
{
	if ((leftArrowDwn || rightArrowDwn ||
		dwnArrowDwn || upArrowDwn) == false)
		renderLoop = false;
}

int
MyMainWindowProc (XEvent *myevent)
{
	static bool lBtnDown = false, rBtnDown = false;
	switch (myevent->type)
	{
	case Expose:
		if (myevent->xexpose.count == 0)
			drawFrame = true;
		break;

	case ClientMessage:
		if (myevent->xclient.data.l[0] == wmDeleteMessage)
			exiting = true;
		break;

	case ConfigureNotify:
		g_glRender->SetupProjection (myevent->xconfigure.width,
									 myevent->xconfigure.height);
		drawFrame = true;
		break;

	case ButtonPress:
		switch (myevent->xbutton.button)
		{
		case Button1:
			/* g_rubiksCube->FindMousePick (myevent->xbutton.x,
										 myevent->xbutton.y); */
			lBtnDown = true;
			drawFrame = true;
			break;
		case Button3:
			g_glRender->MouseRotate (myevent->xbutton.x,
									 myevent->xbutton.y, true);
			rBtnDown = true;
			break;
		case Button4: // wheel up
			g_glRender->incZoom();
			drawFrame = true;
			break;
		case Button5: // wheel down
			g_glRender->decZoom();
			drawFrame = true;
			break;
		}
		break;

	case ButtonRelease:
		switch (myevent->xbutton.button)
		{
		case Button1:
			lBtnDown = false;
			break;
		case Button3:
			rBtnDown = false;
			break;
		}
		break;

	case MotionNotify:
		if (lBtnDown)
		{
			/* g_rubiksCube->RotateFace (myevent->xmotion.x, myevent->xmotion.y); */
			drawFrame = true;
		}
		if (rBtnDown)
		{
			g_glRender->MouseRotate (myevent->xmotion.x, myevent->xmotion.y, false);
			drawFrame = true;
		}
		break;

	case KeyRelease:
		switch (XLookupKeysym (&myevent->xkey, 0))
		{
		case XK_Up:
			upArrowDwn = false;
			if (dwnArrowDwn == false)
				g_glRender->RotatingUp (false);
			DisableRenderLoop ();
			break;
		case XK_Down:
			dwnArrowDwn = false;
			if (upArrowDwn == false)
				g_glRender->RotatingDown (false);
			DisableRenderLoop ();
			break;
		case XK_Left:
			leftArrowDwn = false;
			if (rightArrowDwn == false)
				g_glRender->RotatingLeft (false);
			DisableRenderLoop ();
			break;
		case XK_Right:
			rightArrowDwn = false;
			if (leftArrowDwn == false)
				g_glRender->RotatingRight (false);
			DisableRenderLoop ();
			break;
		case XK_Control_L:
		case XK_Control_R:
			g_glRender->SetRotSpeed (false);
			break;
		}
		break;

	case KeyPress:
		switch (XLookupKeysym (&myevent->xkey, 0))
		{
		case XK_Escape:
			exiting = true;
			break;
		case XK_r:
			g_glRender->incZoom();
			drawFrame = true;
			break;
		case XK_e:
			g_glRender->decZoom();
			drawFrame = true;
			break;
		case XK_Up:
			upArrowDwn = true;
			g_glRender->RotatingUp (true);
			EnableRenderLoop ();
			break;
		case XK_Down:
			dwnArrowDwn = true;
			g_glRender->RotatingDown (true);
			EnableRenderLoop ();
			break;
		case XK_Left:
			leftArrowDwn = true;
			g_glRender->RotatingLeft (true);
			EnableRenderLoop ();
			break;
		case XK_Right:
			rightArrowDwn = true;
			g_glRender->RotatingRight (true);
			EnableRenderLoop ();
			break;
		case XK_Control_L:
		case XK_Control_R:
			g_glRender->SetRotSpeed (true);
			break;
		}
		break;
	}
	return 0;
}

int
main (int argc, char *argv[])
{
	Display *mydisplay;
	int myscreen;
	Window myroot;
	Bool supportable_rtm;

	XVisualInfo *myvisinfo;
	XSizeHints myhint;
	XSetWindowAttributes attr;
	unsigned long mask;
	Window mywindow;
	char title[] = "OpenGL 3D Graphics Lab";
	Cursor mycursor;

	GLXContext hRC;

	/* X11 requires extra work to get a sane setup, first by needing
	   to open the display.  */
	mydisplay = XOpenDisplay ("");
	myscreen = DefaultScreen (mydisplay);
	myroot = DefaultRootWindow (mydisplay);
	/* X11 does not have a builtin equivalent of WM_CLOSE, so we have
	   to do extra work to get the WM_DELETE_WINDOW message.  */
	wmDeleteMessage = XInternAtom(mydisplay, "WM_DELETE_WINDOW", False);
	/* By default, X11 generates annoying KeyRelease events during
	   autorepeat, use this to disable it.  */
	XkbSetDetectableAutoRepeat (mydisplay, True, &supportable_rtm);

	/* Unlike Windows API, we cannot change the pixel format after we
	   create a window, so setup `glXChooseVisual ()' before creating
	   the window.  */
	myvisinfo = SetupVisual (mydisplay, myscreen);
	if (!myvisinfo)
	{
		fprintf (stderr, "Error: couldn't get an RGB, Double-buffered visual\n");
		return 1;
	}

	// Now we can create our window.
	myhint.x = 0; myhint.y = 0;
	myhint.width = windowWidth; myhint.height = windowHeight;
	myhint.flags = PSize;

	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap (mydisplay, myroot, myvisinfo->visual,
									 AllocNone);
	attr.event_mask = StructureNotifyMask
				  | ExposureMask
				  | ButtonPressMask
				  | ButtonReleaseMask
				  | PointerMotionMask
				  | KeyReleaseMask
				  | KeyPressMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	mywindow = XCreateWindow (mydisplay,		// display
		myroot,									// handle to parent
		myhint.x, myhint.y,						// x,y coordinate
		myhint.width, myhint.height,			// width, height
		0,										// border width
		myvisinfo->depth,						// color depth
		InputOutput,							// "window class"
		myvisinfo->visual,						// visual
		mask,									// attribute mask
		&attr);									// attributes

	// Set the window title and hints.
	XSetStandardProperties (mydisplay, mywindow, title, title,
							None, argv, argc, &myhint);
	/* Set the window cursor (only necessary if running without a
	   window manager).  */
	mycursor = XCreateFontCursor (mydisplay, XC_left_ptr);
	XDefineCursor (mydisplay, mywindow, mycursor);
	// Configure the WM_DELETE_WINDOW message.
	XSetWMProtocols(mydisplay, mywindow, &wmDeleteMessage, 1);

	// TODO FIXME: Add support for fullscreen.
	/* if (fullscreen)
		no_border (mydisplay, mywindow); */

	/* Although X11 does have a CreateNotify event, it's mainly meant
	   for detecting the creation of child windows, so we do our
	   WM_CREATE OpenGL rendering context setup here.  */
	hRC = glXCreateContext (mydisplay, myvisinfo, NULL, True);
	if (!hRC)
	{
		fprintf (stderr, "Error: glXCreateContext failed\n");
		XFree (myvisinfo);
		return 1;
	}
	XFree (myvisinfo);
	glXMakeCurrent (mydisplay, mywindow, hRC);

	XMapRaised (mydisplay, mywindow); // make the window visible

	g_glRender = new GfxOpenGL;

	if (!g_glRender->Init ())
		goto cleanup;
	/* When running without a window manager, we will not get an
	   initial ConfigureNotify event.  So, call `SetupProjection ()'
	   here to guarantee that it happens.  */
	g_glRender->SetupProjection (myhint.width, myhint.height);

	drawFrame = true; // draw the first frame
	while (!exiting) {
		XEvent myevent;
		if (drawFrame || renderLoop)
		{
			struct timespec tp;
			int thisTime;
			clock_gettime (CLOCK_MONOTONIC, &tp);
			thisTime = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
			g_glRender->Prepare ((float)(thisTime - lastTime)/1000.0f);
			lastTime = thisTime;

			g_glRender->Render ();
			glXSwapBuffers (mydisplay, mywindow);
			drawFrame = false;
		}

		while (!exiting &&
			   (!(drawFrame || renderLoop) || XPending (mydisplay) > 0))
		{
			XNextEvent (mydisplay, &myevent);
			/* X11 doesn't have builtin dispatching to registered
			   window processes since that is the responsibility of
			   toolkits, so we must do our own dispatching since we
			   don't want to require toolkit dependencies.  But since
			   we only have one window, we can send the messages
			   directly to that one window process.  */
			MyMainWindowProc (&myevent);
		}
	}

cleanup:
	delete g_glRender;
	glXDestroyContext (mydisplay, hRC);
	XDestroyWindow (mydisplay, mywindow);
	XCloseDisplay (mydisplay);

	return 0;
}
