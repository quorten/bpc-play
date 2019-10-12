/* Slider Wave Editor main startup file.

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
 * Slider Wave Editor main startup file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <gtk/gtk.h>

#include "binreloc.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"

gchar *package_prefix = PACKAGE_PREFIX;
gchar *package_data_dir = PACKAGE_DATA_DIR;
gchar *package_locale_dir = PACKAGE_LOCALE_DIR;

int
main (int argc, char *argv[])
{
  /* Initialize packge paths.  */
#ifdef G_OS_WIN32
  package_prefix = g_win32_get_package_installation_directory (NULL, NULL);
  package_data_dir = g_build_filename (package_prefix, "share", NULL);
  package_locale_dir =
    g_build_filename (package_prefix, "share", "locale", NULL);
#else
  br_init (NULL);
  package_prefix = br_find_prefix (package_prefix);
  package_data_dir = br_find_data_dir (package_data_dir);
  package_locale_dir = br_find_locale_dir (package_locale_dir);
#endif

  /* Initialize NLS.  */
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, package_locale_dir);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  /* Initialize GTK+.  */
  gtk_set_locale ();
  gtk_init (&argc, &argv);
  {
    gchar *pixmap_dir = g_build_filename (package_data_dir, PACKAGE,
					  "pixmaps", NULL);
    add_pixmap_directory (pixmap_dir);
    g_free (pixmap_dir);
  }
  {
    GList *icon_list = NULL;
    icon_list = g_list_prepend (icon_list, create_pixbuf ("icon48.png"));
    icon_list = g_list_prepend (icon_list, create_pixbuf ("icon32.png"));
    icon_list = g_list_prepend (icon_list, create_pixbuf ("icon16.png"));
    icon_list = g_list_prepend (icon_list, create_pixbuf ("icon256.png"));
    icon_list = g_list_reverse (icon_list);
    gtk_window_set_default_icon_list (icon_list);
    /* A copy of this list has been made, so it can be freed.  */
    g_list_free (icon_list);
  }

  { /* Start everything up.  */
    GdkColor foreground, background;
    foreground.red   = 0x0000; background.red   = 0x0000;
    foreground.green = 0xFFFF; background.green = 0x0000;
    foreground.blue  = 0x0000; background.blue  = 0x0000;
    create_main_window ();
    set_render_colors (&foreground, &background);
    gtk_window_set_default_size (GTK_WINDOW (main_window), 600, 540);
    gtk_widget_show (main_window);
    g_signal_connect ((gpointer) main_window, "destroy",
		      G_CALLBACK (gtk_main_quit), NULL);
  }
  gtk_main ();

  /* Shutdown.  */
  interface_shutdown ();
#ifdef G_OS_WIN32
  g_free (package_prefix);
  g_free (package_data_dir);
  g_free (package_locale_dir);
#else
  free (package_prefix);
  free (package_data_dir);
  free (package_locale_dir);
#endif
  return 0;
}

#ifdef _MSC_VER
#include <windows.h>

int WINAPI
WinMain (HINSTANCE hInstance,
	 HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  return main (__argc, __argv);
}
#endif
