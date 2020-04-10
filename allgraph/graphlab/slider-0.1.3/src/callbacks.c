/* GTK+ widget signal handlers.

Copyright (C) 2011, 2012, 2013 Andrew Makousky
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.  */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <stdlib.h>

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "ivecmath.h"

/** Keeps track of whether the user has unsaved changes in the current
    file.  */
gboolean file_modified = TRUE;
/** Keeps track of the last visited folder in the GTK+ file chooser
    for convenience for the user.  */
gchar *last_folder = NULL;
/** Keeps track of the currently loaded filename.  */
gchar *loaded_fname = NULL;

GdkPoint last_pt;
GdkPoint p1 = { 50, 50 };
GdkPoint p2 = { 200, 150 };
GdkPoint p3 = { 150, 200 };
GdkPoint p4 = { 150, 190 };

/**
 * Ask the user if they want to save their file before continuing.
 *
 * Audio playback will be turned off when calling this function.
 *
 * @return TRUE to continue, FALSE to cancel
 */
gboolean check_save (gboolean closing)
{
  GtkWidget *dialog;
  const gchar *prompt_msg;
  const gchar *continue_msg;
  GtkResponseType result;

  if (!file_modified)
    return TRUE;

  if (closing)
    {
      prompt_msg = _("<b><big>Save changes to the current file before " \
       "closing?</big></b>\n\nIf you close without saving, " \
       "your changes will be discarded.");
      continue_msg = _("Close _without saving");
    }
  else
    {
      prompt_msg = _("<b><big>Save changes to the current file before " \
       "continuing?</big></b>\n\nIf you continue without saving, " \
       "your changes will be discarded.");
      continue_msg = _("Continue _without saving");
    }

  dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
				   NULL);
  gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog), prompt_msg);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
		  continue_msg, GTK_RESPONSE_CLOSE,
		  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		  GTK_STOCK_SAVE, GTK_RESPONSE_YES,
		  NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (result == GTK_RESPONSE_CLOSE)
    return TRUE;
  else if (result == GTK_RESPONSE_CANCEL)
    return FALSE;
  else if (result == GTK_RESPONSE_YES)
      return save_as ();
  /* Unknown response?  Do nothing.  */
  return FALSE;
}

/**
 * Save a file with a specific name from the GUI.
 */
gboolean
save_as (void)
{
  GtkFileFilter *filter = gtk_file_filter_new ();
  GtkWidget *dialog =
    gtk_file_chooser_dialog_new (_("Save File"),
				 GTK_WINDOW (main_window),
				 GTK_FILE_CHOOSER_ACTION_SAVE,
				 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				 GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
				 NULL);
  gtk_file_filter_set_name (filter, _("Slider Project Files"));
  gtk_file_filter_add_pattern (filter, "*.sliw");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  if (last_folder != NULL)
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
					   last_folder);
    }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      gboolean result;
      gchar *filename =
	gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      unsigned filename_len = strlen (filename);
      g_free (last_folder);
      last_folder = gtk_file_chooser_get_current_folder
	                   (GTK_FILE_CHOOSER (dialog));
      gtk_widget_destroy (dialog);
      g_free(loaded_fname); loaded_fname = NULL;
      if (filename_len <= 5 ||
	  strcmp(&filename[filename_len-5], ".sliw"))
	{
	  filename = g_realloc (filename,
				sizeof(gchar) * (filename_len + 6));
	  strcpy(&filename[filename_len], ".sliw");
	}
      /* result = save_sliw_project (filename); */
      result = TRUE;
      if (!result)
	{
	  g_free (filename);
	  return FALSE;
	}
      file_modified = FALSE;
      loaded_fname = filename;
      return TRUE;
    }
  else
    gtk_widget_destroy (dialog);
  return FALSE;
}

/**
 * Open a file from the GUI.
 */
void
open_file (void)
{
  GtkFileFilter *filter = gtk_file_filter_new ();
  GtkWidget *dialog =
    gtk_file_chooser_dialog_new (_("Open File"),
				 GTK_WINDOW (main_window),
				 GTK_FILE_CHOOSER_ACTION_OPEN,
				 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				 GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				 NULL);
  gtk_file_filter_set_name (filter, "Slider Project Files");
  gtk_file_filter_add_pattern (filter, "*.sliw");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  if (last_folder != NULL)
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
					   last_folder);
    }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename =
	gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      g_free (last_folder);
      last_folder = gtk_file_chooser_get_current_folder
	                   (GTK_FILE_CHOOSER (dialog));
      gtk_widget_destroy (dialog);
      g_free(loaded_fname); loaded_fname = NULL;
      /* unselect_fund_freq (g_fund_set);
      free_wv_editors ();
      init_wv_editors (); */
      if (!TRUE /* load_sliw_project (filename) */)
	{
	  free_wv_editors ();
	  g_free (filename);
	  init_wv_editors ();
	  new_sliw_project ();
	}
      else
	loaded_fname = filename;
      /* select_fund_freq (g_fund_set); */
    }
  else
    gtk_widget_destroy (dialog);
}

gboolean
main_window_delete (GtkWidget * widget, gpointer user_data)
{
  if (check_save (TRUE))
    return FALSE;
  return TRUE;
}

/**
 * Signal handler for the "expose_event" event sent to ::wave_render.
 *
 * This signal handler renders the waveform within the drawable area.
 */
gboolean
wavrnd_expose (GtkWidget * widget, GdkEventExpose * event, gpointer user_data)
{
  unsigned i;
  gint /* last_pt.y, */ win_width, win_height;

  if (G_UNLIKELY (wr_gc == NULL))
    {
      wr_gc = gdk_gc_new (wave_render->window);
      gdk_gc_set_rgb_fg_color (wr_gc, &wr_foreground);
    }

  win_width = widget->allocation.width;
  win_height = widget->allocation.height;

  {
    GdkPoint v1, v2, v3, pc;
    int i;

    v1.x = p2.x - p1.x; v1.y = p2.y - p1.y;
    v2.x = p3.x - p1.x; v2.y = p3.y - p1.y;
    v3.x = p4.x - p3.x; v3.y = p4.y - p3.y;

    gdk_draw_line (widget->window, wr_gc, p1.x, p1.y, p2.x, p2.y); 
    /* for (i = 0; i < 128; i++) {
      /\* int u = rand () % 128; *\/
      int u = i;
      pc.x = p1.x + v1.x * u / 128; pc.y = p1.y + v1.y * u / 128;
      gdk_draw_point (widget->window, wr_gc, pc.x, pc.y);
    } */

    // gdk_draw_line (widget->window, wr_gc, p1.x, p1.y, p3.x, p3.y);
    /* for (i = 0; i < 128; i++) {
      /\* int u = rand () % 128; *\/
      int u = i;
      pc.x = p1.x + v2.x * u / 128; pc.y = p1.y + v2.y * u / 128;
      gdk_draw_point (widget->window, wr_gc, pc.x, pc.y);
    } */

    gdk_draw_line (widget->window, wr_gc, p2.x, p2.y, p3.x, p3.y);
    /* u + v == 1 */
    /* for (i = 0; i < 128; i++) {
      /\* int u = rand () % 128; *\/
      int u = i;
      int v = 128 - u;
      pc.x = p1.x + v1.x * u / 128 + v2.x * v / 128;
      pc.y = p1.y + v1.y * u / 128 + v2.y * v / 128;
      gdk_draw_point (widget->window, wr_gc, pc.x, pc.y);
    } */

    gdk_draw_line (widget->window, wr_gc, p3.x, p3.y, p4.x, p4.y);

    /* Now we're getting interesting.  Project p3 onto the line p1 -
       p2, through the shortest-distance perpendicular, and draw the
       line.  */
    {
      GdkRectangle pt_rect;
      GdkPoint vt = { -v1.y, v1.x }; /* Perpendicular */
      GdkPoint op; /* orthogonal projection */
      GdkPoint pp; /* projected perpendicular point on line */
      /* Lucky us, it turns out we don't need the regular distance, we
	 can just use the distance squared for our calculations.  No
	 square-root computation.  */
      int v1_length_2 = v1.x * v1.x + v1.y * v1.y;
      int vt_length_2 = v1_length_2;
      int v2_length_2 = v2.x * v2.x + v2.y * v2.y;
      /* int v3_length_2 = v3.x * v3.x + v3.y * v3.y; */
      int v3_dot_vt;
      int sfac;
      /* NOT normalized.  Now this is interesting because we don't
	 divide out to normalize until the very end.  At that point,
	 we divide by the distance squared of the basis vector,
	 i.e. the vector of line p1 - p2.

	 Why?  First, our dot product computation carries through a
	 spurious `v1_length'.  Second, we take the result and
	 multiply it with `v1' directly.  Now, if we were multiplying
	 by a unit vector, we would simply divide out `v1_length' once
	 to get the desired displacement: the component of `v2' that
	 runs the length of `v1'.  But since our direction vector also
	 has a length of `v1', we need to divide out that magnitude
	 twice.  Luckily, that spares us from needing to compute a
	 square root and can just divide by the distance squared.  */
      /* project on line */
      op.x = (v2.x * v1.x + v2.y * v1.y);
      /* project on perpendicular */
      op.y = v2.x * vt.x + v2.y * vt.y;
      /* Render point on line.  */
      pp.x = p1.x + op.x * v1.x / v1_length_2;
      pp.y = p1.y + op.x * v1.y / v1_length_2;
      /* Alternate method, use perpendicular, works just as well.
	 (See later discussion for subtracting.)  */
      /* pp.x = p3.x - op.y * vt.x / v1_length_2;
      pp.y = p3.y - op.y * vt.y / v1_length_2; */
      gdk_draw_line (widget->window, wr_gc, p3.x, p3.y, pp.x, pp.y);
      {
	/* Use our spiffy new subroutines to compute perpendiculars
	   and solutions.  */
	IVPoint2D_i32 n_p1 = {{ p1.x, p1.y }};
	IVPoint2D_i32 n_p3 = {{ p3.x, p3.y }};
	IVVec2D_i32 n_v1 = {{ v1.x, v1.y }};
	IVVec2D_i32 n_v2 = {{ v2.x, v2.y }};
	IVVec2D_i32 n_vt = {{ -v1.y, v1.x }};
	IVVec2D_i32 n_v3 = {{ v3.x, v3.y }};
	IVNLine_v2i32 n_plane = { n_vt, n_p1 };
	IVRay_v2i32 n_v3_ray = { n_v3, n_p3 };
	IVPoint2D_i32 n_pp;

	iv_proj3_p2i32_NLine_v2i32(&n_pp, &n_p3, &n_plane);
	pt_rect.x = n_pp.d[IX] - 5;
	pt_rect.y = n_pp.d[IY] - 5;
	pt_rect.width = 10;
	pt_rect.height = 10;
	gdk_draw_rectangle
	  (widget->window, wr_gc, TRUE,
	  pt_rect.x, pt_rect.y, pt_rect.width, pt_rect.height);

	iv_isect3_Ray_NLine_v2i32(&n_pp, &n_v3_ray, &n_plane);

	if (n_pp.d[IX] != IVINT32_MIN) {
	  pt_rect.x = n_pp.d[IX] - 5;
	  pt_rect.y = n_pp.d[IY] - 5;
	  pt_rect.width = 10;
	  pt_rect.height = 10;
	  gdk_draw_rectangle
	    (widget->window, wr_gc, TRUE,
	     pt_rect.x, pt_rect.y, pt_rect.width, pt_rect.height);
	}

	/* pt_rect.x = n_p3.d[IX] - 5;
	pt_rect.y = n_p3.d[IY] - 5;
	pt_rect.width = 10;
	pt_rect.height = 10;
	gdk_draw_rectangle
	  (widget->window, wr_gc, TRUE,
	   pt_rect.x, pt_rect.y, pt_rect.width, pt_rect.height); */

        /* pt_rect.x = n_p1.d[IX] + n_v2.d[IX] - 5;
	pt_rect.y = n_p1.d[IY] + n_v2.d[IY] - 5;
	pt_rect.width = 10;
	pt_rect.height = 10;
	gdk_draw_rectangle
	  (widget->window, wr_gc, TRUE,
	   pt_rect.x, pt_rect.y, pt_rect.width, pt_rect.height); */
      }
      /* Compute other point on perpendicular.  */
      /* pc.x = pp.x + op.y * vt.x / vt_length_2;
      pc.y = pp.y + op.y * vt.y / vt_length_2;
      pt_rect.x = pc.x - 5;
      pt_rect.y = pc.y - 5;
      pt_rect.width = 10;
      pt_rect.height = 10;
      gdk_draw_rectangle
	(widget->window, wr_gc, TRUE,
	 pt_rect.x, pt_rect.y, pt_rect.width, pt_rect.height); */
      /* Compute the scaling factor `s`.  */
      /* Note: `vt' points away from our "plane."  For the purposes of
	 this equation, we want a vector pointing toward it.  Hence,
	 we negate its terms.  */
      /* NOT normalized, again.  So, how do we luck out on avoiding
	 computational complexity on this one?  First of all, we state
	 the final quantity that we want to get: the component of `v3'
	 that runs the length of `vt'.  Second, we state the spurious
	 quantities we are carrying through that will need to be
	 canceled out later: the length of `vt'.  */
      v3_dot_vt = v3.x * (-vt.x) + v3.y * (-vt.y);
      if (v3_dot_vt == 0) /* invalid: no solution */
	v3_dot_vt = 1;
      /* Here, we've already computed the distance along the
	 perpendicular and stored it in `op.y', which carries through
	 a spurious `vt_length' due to being computed from the dot
	 product with non-normalized vector.  We can then reuse that
	 variable in our equation here, to determine the scaling
	 factor.  Happily, the two spurious factors cancel out.
	 Unfortunately, that means we need to add an arbitrary
	 multiplier so that we can have fractional quantities in
	 integer arithmetic, which we will then divide out later.  */
      /* Since this is a scaling factor on `v3', the principal
	 quantity we would want to use for the fractional portion
	 would be the length of `v3'.  Unfortunately, since we already
	 have `v3_length' multiplied in, multiplying by the square
	 results in a total multiplier of the cube, which results in
	 overflow for larger values.  */
      /* So, this is something to watch out for.  If you are squaring
	 a quantity and multiplying by a scalar one more time, you're
	 largest arbitrary integer value is the cube root of the
	 largest integer that can be represented.  In other words, no
	 more than one-third the total number of permissible
	 digits.

	 If you're really getting tight on bits of precision and you
	 don't care about the least significant bits, you can throw
	 the least significant away by shifting right, do your
	 division, and finally shift back left.  Sure, it's always a
	 compromise: multiplying and dividing large numbers, computing
	 a square root, or just throwing away less significant
	 bits.  */
      sfac = op.y * 1024 / v3_dot_vt;
      /* Now, we're at the end-all and be-all of proving.  Draw the
	 final projected point indicating the intersection of our
	 "line" vector with our "plane" vector.  */
      pc.x = p3.x + sfac * v3.x / 1024;
      pc.y = p3.y + sfac * v3.y / 1024;
      pt_rect.x = pc.x - 5;
      pt_rect.y = pc.y - 5;
      pt_rect.width = 10;
      pt_rect.height = 10;
      /* If the scaling constant is negative, that means we're facing
	 the wrong direction, so don't bother drawing the
	 solution.  */
      /* if (sfac >= 0)
	gdk_draw_rectangle
	  (widget->window, wr_gc, TRUE,
	   pt_rect.x, pt_rect.y, pt_rect.width, pt_rect.height); */
    }

    // u + v == parallelogram
    /* for (i = 0; i < 16384; i++) {
      int u = rand () % 128;
      int v = rand () % 128;
      pc.x = p1.x + v1.x * u / 128 + v2.x * v / 128;
      pc.y = p1.y + v1.y * u / 128 + v2.y * v / 128;
      gdk_draw_point (widget->window, wr_gc, pc.x, pc.y);
    } */

    // u + v <= 1 == triangle
    /* for (i = 0; i < 16384; i++) {
      int lim = rand () % 128;
      int u, v;
      if (lim == 0) lim = 1;
      u = rand () % lim;
      v = lim - u;
      pc.x = p1.x + v1.x * u / 128 + v2.x * v / 128;
      pc.y = p1.y + v1.y * u / 128 + v2.y * v / 128;
      gdk_draw_point (widget->window, wr_gc, pc.x, pc.y);
    } */
 }

  // gdk_draw_line (widget->window, wr_gc, i, last_pt.y, i, ypt);

  return TRUE;
}

/* Draw a rectangle on the screen */
void
draw_brush (GtkWidget *widget,
            gdouble    x,
            gdouble    y)
{
  /* Paint to the pixmap, where we store our state */
  gdk_draw_line (widget->window,
		  wr_gc,
		 last_pt.x, last_pt.y, x, y);
  last_pt.x = x;
  last_pt.y = y;
}

/* Draw a rectangle on the screen */
void
drag_point (GtkWidget *widget,
            gdouble    x,
            gdouble    y)
{
  GdkRectangle update_rect;

  update_rect.x = 0;
  update_rect.y = 0;
  update_rect.width = widget->allocation.width;
  update_rect.height = widget->allocation.height;

  last_pt.x = x;
  last_pt.y = y;

  { /* Find out which point we're selecting and moving based off of the
       closest match.  */
    GdkPoint diff;
    long p1_dist_2, p2_dist_2, p3_dist_2, p4_dist_2;
    diff.x = last_pt.x - p1.x; diff.y = last_pt.y - p1.y;
    p1_dist_2 = diff.x * diff.x + diff.y * diff.y;
    diff.x = last_pt.x - p2.x; diff.y = last_pt.y - p2.y;
    p2_dist_2 = diff.x * diff.x + diff.y * diff.y;
    diff.x = last_pt.x - p3.x; diff.y = last_pt.y - p3.y;
    p3_dist_2 = diff.x * diff.x + diff.y * diff.y;
    diff.x = last_pt.x - p4.x; diff.y = last_pt.y - p4.y;
    p4_dist_2 = diff.x * diff.x + diff.y * diff.y;
    if (p1_dist_2 < p2_dist_2 && p1_dist_2 < p3_dist_2 &&
	p1_dist_2 < p4_dist_2)
      {
	/* Select p1 */
	p1.x = last_pt.x; p1.y = last_pt.y;
      }
    else if (p2_dist_2 < p1_dist_2 && p2_dist_2 < p3_dist_2 &&
	     p2_dist_2 < p4_dist_2)
      {
	/* Select p2 */
	p2.x = last_pt.x; p2.y = last_pt.y;
      }
    else if (p3_dist_2 < p2_dist_2 && p3_dist_2 < p1_dist_2 &&
	     p3_dist_2 < p4_dist_2)
      {
	/* Select p3 */
	p3.x = last_pt.x; p3.y = last_pt.y;
      }
    else /* if (p4_dist_2 < p1_dist_2 && p4_dist_2 < p2_dist_2 &&
	     p4_dist_2 < p3_dist_2) */
      {
	/* Select p4 */
	p4.x = last_pt.x; p4.y = last_pt.y;
      }
    }

  /* Now invalidate the affected region of the drawing area. */
  gdk_window_invalidate_rect (widget->window,
                              &update_rect,
                              FALSE);
}

gboolean
scribble_button_press_event (GtkWidget      *widget,
                             GdkEventButton *event,
                             gpointer        data)
{

  if (event->button == 1)
    {
      last_pt.x = event->x;
      last_pt.y = event->y;
      draw_brush (widget, event->x, event->y);
    }
  else if (event->button == 3)
    {
      last_pt.x = event->x;
      last_pt.y = event->y;
      drag_point (widget, event->x, event->y);
    }

  /* We've handled the event, stop processing */
  return TRUE;
}

gboolean
scribble_motion_notify_event (GtkWidget      *widget,
                              GdkEventMotion *event,
                              gpointer        data)
{
  int x, y;
  GdkModifierType state;

  /* This call is very important; it requests the next motion event.
   * If you don't call gdk_window_get_pointer() you'll only get
   * a single motion event. The reason is that we specified
   * GDK_POINTER_MOTION_HINT_MASK to gtk_widget_set_events().
   * If we hadn't specified that, we could just use event->x, event->y
   * as the pointer location. But we'd also get deluged in events.
   * By requesting the next event as we handle the current one,
   * we avoid getting a huge number of events faster than we
   * can cope.
   */

  gdk_window_get_pointer (event->window, &x, &y, &state);

  if (state & GDK_BUTTON1_MASK)
    draw_brush (widget, x, y);
  else if (state & GDK_BUTTON3_MASK)
    drag_point (widget, x, y);

  /* We've handled it, stop processing */
  return TRUE;
}

/**
 * Signal handler sent when the main edit area changes.
 *
 * This signal handler approximately preserves the ratio between the
 * two divided panes when the main window size changes.  In GTK+,
 * widgets that aren't windows in the window system are not sent
 * configure events, but rather "size-request" and "size-allocate"
 * events.  The signal handler's weren't quite as well documented in
 * the GTK+ documentation as they could have been.
 */
void
wv_edit_div_allocate (GtkWidget * widget,
		      GtkAllocation * allocation, gpointer user_data)
{
  /* Set the divider position to preserve the previous ratio.
     If this is the first time, set the ratio to a predefined value.  */
  static gboolean div_set = FALSE;
  static gfloat div_ratio;
  static gint last_win_height;
  gint div_pos;

  if (G_UNLIKELY (!div_set))
    {
      div_ratio = 0.5f;
      div_set = TRUE;
    }
  else if (last_win_height == allocation->height)
    {
      div_pos = gtk_paned_get_position (GTK_PANED (widget));
      div_ratio = (gfloat) div_pos / last_win_height;
      return;
    }

  div_pos = (gint) ((gfloat) allocation->height * div_ratio);
  gtk_paned_set_position (GTK_PANED (widget), div_pos);
  last_win_height = allocation->height;
}

/**
 * Signal handler for when the user hits enter in the command entry
 * edit box.
 *
 * This function executes the entered command.
 */
void
cmd_entry_activate (GtkEntry * entry, gpointer user_data)
{
  const gchar *cmd_text = gtk_entry_get_text (entry);
  GtkTextBuffer *text_buffer =
    gtk_text_view_get_buffer (GTK_TEXT_VIEW (hist_text));
  GtkTextIter iter;
  gtk_text_buffer_get_end_iter (text_buffer, &iter);
  gtk_text_buffer_place_cursor (text_buffer, &iter);
  /* Do not insert an extra newline at the beginning or end so that we
     do not waste screen space.  */
  if (!gtk_text_iter_is_start (&iter))
    gtk_text_buffer_insert_at_cursor (text_buffer, "\n", 1);
  /* Display a user-input prompt.  */
  gtk_text_buffer_insert_at_cursor (text_buffer, "$ ", 2);
  gtk_text_buffer_insert_at_cursor
    (text_buffer, cmd_text, -1);
  gtk_entry_set_text (entry, "");
  /* Make sure we also scroll to the end of the history window when
     inserting so that we can follow user text inserts.  */
  gtk_adjustment_set_value (hist_vadj, hist_vadj->upper);

  gtk_widget_queue_draw (wave_render);
}

/**
 * Signal handler for when the user navigates away from the command
 * entry edit box.
 *
 * This function executes the entered command.
 */
gboolean
cmd_entry_focus_out (GtkEntry * entry,
		     GdkEventFocus * event, gpointer user_data)
{
  cmd_entry_activate (entry, user_data);
  return FALSE;
}
