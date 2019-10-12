/* Graphical user interface building functions.

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

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

/** Main application window.  */
GtkWidget *main_window = NULL;
/** Composite wave rendering area.  */
GtkWidget *wave_render;
/** The window that shows the command output history.  */
GtkWidget *hist_text;
/** The adjustment associated with the command output history's
    vertical scrollbar.

    The main reason for making this available is so that we can follow
    the latest text added to the command history.  */
GtkAdjustment *hist_vadj;
/** Foreground color of wave rendering area.  */
GdkColor wr_foreground;
/** Background color of wave rendering area.  */
GdkColor wr_background;
/** Graphics context for drawing in the wave rendering area.  */
GdkGC *wr_gc = NULL;

GtkWidget *
create_main_window (void)
{
  GtkWidget *graph_ctl_div;
  GtkWidget *scrolled_window;
  GtkWidget *cmd_vbox;
  PangoFontDescription *font_desc;
  GtkWidget *cmd_entry;

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  /* gtk_container_set_border_width (GTK_CONTAINER (main_window), 8); */
  gtk_window_set_title (GTK_WINDOW (main_window), _("Graphics Lab"));
  gtk_window_set_default_size (GTK_WINDOW (main_window), 600, 400);

  wave_render = gtk_drawing_area_new ();
  gtk_widget_show (wave_render);

  graph_ctl_div = gtk_vpaned_new ();
  gtk_widget_show (graph_ctl_div);
  gtk_paned_pack1 (GTK_PANED (graph_ctl_div), wave_render, FALSE, TRUE);

  cmd_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (cmd_vbox);
  gtk_paned_pack2 (GTK_PANED (graph_ctl_div), cmd_vbox, TRUE, TRUE);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled_window);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (cmd_vbox), scrolled_window, TRUE, TRUE, 0);

  hist_text = gtk_text_view_new ();
  gtk_widget_show (hist_text);
  // text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (hist_text));
  gtk_text_view_set_editable (GTK_TEXT_VIEW (hist_text), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (hist_text), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (hist_text), GTK_WRAP_CHAR);
  font_desc = pango_font_description_from_string ("monospace 10");
  gtk_widget_modify_font (hist_text, font_desc);
  // gtk_text_buffer_set_text (text_buffer, manual_text, -1);
  hist_vadj = gtk_scrolled_window_get_vadjustment
    (GTK_SCROLLED_WINDOW (scrolled_window));
  gtk_container_add (GTK_CONTAINER (scrolled_window), hist_text);

  cmd_entry = gtk_entry_new ();
  gtk_widget_show (cmd_entry);
  gtk_widget_modify_font (cmd_entry, font_desc);
  pango_font_description_free (font_desc);
  gtk_box_pack_start (GTK_BOX (cmd_vbox), cmd_entry, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (main_window), graph_ctl_div);

  g_signal_connect ((gpointer) main_window, "delete-event",
		    G_CALLBACK (main_window_delete), NULL);
  g_signal_connect ((gpointer) graph_ctl_div, "size_allocate",
		    G_CALLBACK (wv_edit_div_allocate), NULL);
  g_signal_connect ((gpointer) wave_render, "expose_event",
		    G_CALLBACK (wavrnd_expose), NULL);

  g_signal_connect ((gpointer) wave_render, "motion-notify-event",
		    G_CALLBACK (scribble_motion_notify_event), NULL);
  g_signal_connect ((gpointer) wave_render, "button-press-event",
		    G_CALLBACK (scribble_button_press_event), NULL);

  g_signal_connect ((gpointer) cmd_entry, "activate",
		    G_CALLBACK (cmd_entry_activate), NULL);
  /* g_signal_connect ((gpointer) cmd_entry, "focus-out-event",
     G_CALLBACK (cmd_entry_focus_out), NULL); */

  /* Ask to receive events the drawing area doesn't normally
   * subscribe to
   */
  gtk_widget_set_events ((gpointer) wave_render,
			 gtk_widget_get_events ((gpointer) wave_render)
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK);

  /* Store pointers to all widgets, for use by lookup_widget().  */
  GLADE_HOOKUP_OBJECT_NO_REF (main_window, main_window, "main_window");
  GLADE_HOOKUP_OBJECT (main_window, wave_render, "wave_render");

  return main_window;
}

/**
 * Sets the foreground and background colors used by the wave
 * rendering area.
 */
void
set_render_colors (GdkColor * foreground, GdkColor * background)
{
  memcpy (&wr_foreground, foreground, sizeof (GdkColor));
  memcpy (&wr_background, background, sizeof (GdkColor));
  gtk_widget_modify_bg (wave_render, GTK_STATE_NORMAL, &wr_background);
  if (G_LIKELY (wr_gc != NULL))
    gdk_gc_set_rgb_fg_color (wr_gc, &wr_foreground);
}

void
interface_shutdown (void)
{
  if (wr_gc != NULL)
    g_object_unref (wr_gc);
}
