/* Sticky Notes
 * Copyright (C) 2002-2003 Loban A Rahman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>
#include "util.h"

#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cdk/cdk.h>
#include <cdk/cdkx.h>
#include <ctk/ctk.h>

/* Returns the current date in a customizable form, the default
 * looks like this: "Nov 30, '78" */
gchar * get_current_date(const gchar *format)
{
  	time_t clock = time(NULL);
  	struct tm *current = localtime(&clock);

	gint date_length = 10;
  	gchar *date = g_new(gchar, date_length);
  	
	do
	{
		date_length += 5;
		date = (gchar *) g_renew(gchar, date, date_length);
	}
  	while(strftime(date, date_length, format, current) == 0);
	
  	return date;
}

static Atom
xstuff_atom_get (const char *atom_name)
{
	static GHashTable *atom_hash;
	Display           *xdisplay;
	Atom               retval;

	g_return_val_if_fail (atom_name != NULL, None);

	xdisplay = GDK_DISPLAY_XDISPLAY (cdk_display_get_default ());

	if (!atom_hash)
		atom_hash = g_hash_table_new_full (
				g_str_hash, g_str_equal, g_free, NULL);

	retval = GPOINTER_TO_UINT (g_hash_table_lookup (atom_hash, atom_name));
	if (!retval) {
		retval = XInternAtom (xdisplay, atom_name, FALSE);

		if (retval != None)
			g_hash_table_insert (atom_hash, g_strdup (atom_name),
					     GUINT_TO_POINTER (retval));
	}

	return retval;
}

int
xstuff_get_current_workspace (CtkWindow *window)
{
	Window  root_window;
	Atom    type = None;
	gulong  nitems;
	gulong  bytes_after;
	gulong *num;
	int     format;
	int     result;
	int     retval;
	GdkDisplay *cdk_display;
	Display *xdisplay;

	root_window = GDK_WINDOW_XID (ctk_widget_get_window (CTK_WIDGET (window)));
    cdk_display = cdk_display_get_default ();
	xdisplay = GDK_DISPLAY_XDISPLAY (cdk_display);

	cdk_x11_display_error_trap_push (cdk_display);
	result = XGetWindowProperty (xdisplay,
				     root_window,
				     xstuff_atom_get ("_NET_CURRENT_DESKTOP"),
				     0, G_MAXLONG,
				     False, XA_CARDINAL,
				     &type, &format, &nitems,
				     &bytes_after, (gpointer) &num);
	if (cdk_x11_display_error_trap_pop (cdk_display) || result != Success)
		return -1;
 
	if (type != XA_CARDINAL) {
		XFree (num);
		return -1;
	}

	retval = *num;
 
	XFree (num);

	return retval;
}
void
xstuff_change_workspace (CtkWindow *window,
                         int        new_space)
{
  XEvent xev;
  Window xwindow;
  Display *cdk_display;
  Screen *screen;

  cdk_display = GDK_DISPLAY_XDISPLAY (cdk_display_get_default ());
  xwindow = GDK_WINDOW_XID (GDK_WINDOW (ctk_widget_get_window (CTK_WIDGET (window))));
  screen = GDK_SCREEN_XSCREEN (ctk_widget_get_screen (CTK_WIDGET (window)));
  
  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.display = cdk_display;
  xev.xclient.window = xwindow;
  xev.xclient.message_type = xstuff_atom_get ("_NET_WM_DESKTOP");
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = new_space;
  xev.xclient.data.l[1] = 0;
  xev.xclient.data.l[2] = 0;

  XSendEvent (cdk_display,
	      RootWindowOfScreen (screen),
              False,
	      SubstructureRedirectMask | SubstructureNotifyMask,
	      &xev);
}
