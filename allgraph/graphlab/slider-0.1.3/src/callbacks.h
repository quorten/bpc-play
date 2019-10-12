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

/**
 * @file
 * GTK+ widget signal handlers.
 *
 * Most of the functions in this file are not deserving of very heavy
 * documentation.  All of the signal handlers are connected to their
 * signals in interface.h, and the function calling form of each
 * signal handler is dictated by the signal that it is connected to.
 * Therefore, it is mostly unnecessary to document the signal
 * handlers, as most of their behavior is explained in the GTK+
 * documentation.  However, the parameter @a user_data is documented
 * whenever it is expected to be a non-NULL value in any of the signal
 * handlers.
 */

#ifndef CALLBACKS_H
#define CALLBACKS_H

gboolean main_window_delete (GtkWidget * widget, gpointer user_data);
gboolean
wavrnd_expose (GtkWidget * widget,
	       GdkEventExpose * event, gpointer user_data);

void
draw_brush (GtkWidget *widget,
            gdouble    x,
            gdouble    y);
void
drag_point (GtkWidget *widget,
            gdouble    x,
            gdouble    y);
gboolean
scribble_button_press_event (GtkWidget      *widget,
                             GdkEventButton *event,
                             gpointer        data);
gboolean
scribble_motion_notify_event (GtkWidget      *widget,
                              GdkEventMotion *event,
                              gpointer        data);

void
wv_edit_div_allocate (GtkWidget * widget,
		      GtkAllocation * allocation, gpointer user_data);
void
cmd_entry_activate (GtkEntry * entry, gpointer user_data);
gboolean
cmd_entry_focus_out (GtkEntry * entry,
		     GdkEventFocus * event, gpointer user_data);

#endif /* not CALLBACKS_H */
