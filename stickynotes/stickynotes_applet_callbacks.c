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
#include <string.h>
#include "stickynotes_applet_callbacks.h"
#include "stickynotes.h"
#include <cdk/cdkkeysyms.h>
#include <X11/Xatom.h>
#include <cdk/cdkx.h>

static gboolean get_desktop_window (Window *window)
{
	Window *desktop_window;
	CdkWindow *root_window;
	CdkAtom type_returned;
	int format_returned;
	int length_returned;

	root_window = cdk_screen_get_root_window (
				cdk_screen_get_default ());

	if (cdk_property_get (root_window,
			      cdk_atom_intern ("BAUL_DESKTOP_WINDOW_ID", FALSE),
			      cdk_x11_xatom_to_atom (XA_WINDOW),
			      0, 4, FALSE,
			      &type_returned,
			      &format_returned,
			      &length_returned,
			      (guchar**) &desktop_window)) {
		*window = *desktop_window;
		g_free (desktop_window);
		return TRUE;
	}
	else {
		*window = 0;
		return FALSE;
	}
}

static void
popup_add_note (StickyNotesApplet *applet, CtkWidget *item)
{
	stickynotes_add (ctk_widget_get_screen (applet->w_applet));
}

static void
stickynote_show_notes (gboolean visible)
{
	StickyNote *note;
	GList *l;

    if (stickynotes->visible == visible) return;
    stickynotes->visible = visible;

	for (l = stickynotes->notes; l; l = l->next)
	{
		note = l->data;
		stickynote_set_visible (note, visible);
	}
}

static void
stickynote_toggle_notes_visible ()
{
    stickynote_show_notes(!stickynotes->visible);
}

/* Applet Callback : Mouse button press on the applet. */
gboolean
applet_button_cb (CtkWidget         *widget,
                  CdkEventButton    *event,
                  StickyNotesApplet *applet)
{
	if (event->type == CDK_2BUTTON_PRESS)
	{
		popup_add_note (applet, NULL);
		return TRUE;
	}
	else if (event->button == 1)
	{
		stickynote_toggle_notes_visible ();
		return TRUE;
	}

	return FALSE;
}

/* Applet Callback : Keypress on the applet. */
gboolean
applet_key_cb (CtkWidget         *widget,
               CdkEventKey       *event,
               StickyNotesApplet *applet)
{
	switch (event->keyval)
	{
		case CDK_KEY_KP_Space:
		case CDK_KEY_space:
		case CDK_KEY_KP_Enter:
		case CDK_KEY_Return:
			stickynote_show_notes (TRUE);
			return TRUE;
	}
 	return FALSE;
}

/* Applet Callback : Cross (enter or leave) the applet. */
gboolean applet_cross_cb(CtkWidget *widget, CdkEventCrossing *event, StickyNotesApplet *applet)
{
	applet->prelighted = event->type == CDK_ENTER_NOTIFY || ctk_widget_has_focus(widget);

	stickynotes_applet_update_icon(applet);

	return FALSE;
}

/* Applet Callback : On focus (in or out) of the applet. */
gboolean applet_focus_cb(CtkWidget *widget, CdkEventFocus *event, StickyNotesApplet *applet)
{
	stickynotes_applet_update_icon(applet);

	return FALSE;
}

static CdkFilterReturn desktop_window_event_filter (CdkXEvent *xevent,
						    CdkEvent  *event,
						    gpointer   data)
{
	gboolean desktop_hide = g_settings_get_boolean (stickynotes->settings, "desktop-hide");
	if (desktop_hide  &&
	    (((XEvent*)xevent)->xany.type == PropertyNotify) &&
	    (((XEvent*)xevent)->xproperty.atom == cdk_x11_get_xatom_by_name ("_NET_WM_USER_TIME"))) {
		stickynote_show_notes (FALSE);
	}
	return CDK_FILTER_CONTINUE;
}

void install_check_click_on_desktop (void)
{
	Window desktop_window;
	CdkWindow *window;
	Atom user_time_window;
	Atom user_time;

	if (!get_desktop_window (&desktop_window)) {
		return;
	}

	/* Access the desktop window. desktop_window is the root window for the
	 * default screen, so we know using cdk_display_get_default() is correct. */
	window = cdk_x11_window_foreign_new_for_display (cdk_display_get_default (),
	                                                 desktop_window);

	/* It may contain an atom to tell us which other window to monitor */
	user_time_window = cdk_x11_get_xatom_by_name ("_NET_WM_USER_TIME_WINDOW");
	user_time = cdk_x11_get_xatom_by_name ("_NET_WM_USER_TIME");
	if (user_time != None  &&  user_time_window != None)
	{
		/* Looks like the atoms are there */
		Atom actual_type;
		Window *data;
		int actual_format;
		gulong nitems;
		gulong bytes;

		/* We only use this extra property if the actual user-time property's missing */
		XGetWindowProperty(CDK_DISPLAY_XDISPLAY(cdk_window_get_display(window)), desktop_window, user_time,
					0, 4, False, AnyPropertyType, &actual_type, &actual_format,
					&nitems, &bytes, (unsigned char **)&data );
		if (actual_type == None)
		{
			/* No user-time property, so look for the user-time-window */
			XGetWindowProperty(CDK_DISPLAY_XDISPLAY(cdk_window_get_display(window)), desktop_window, user_time_window,
					0, 4, False, AnyPropertyType, &actual_type, &actual_format,
					&nitems, &bytes, (unsigned char **)&data );
			if (actual_type != None)
			{
				/* We have another window to monitor */
				desktop_window = *data;
				window = cdk_x11_window_foreign_new_for_display (cdk_window_get_display (window),
				                                                 desktop_window);
			}
		}
	}

	cdk_window_set_events (window, CDK_PROPERTY_CHANGE_MASK);
	cdk_window_add_filter (window, desktop_window_event_filter, NULL);
}

/* Applet Callback : Change the panel orientation. */
void applet_change_orient_cb(CafePanelApplet *cafe_panel_applet, CafePanelAppletOrient orient, StickyNotesApplet *applet)
{
	applet->panel_orient = orient;
}

/* Applet Callback : Resize the applet. */
void applet_size_allocate_cb(CtkWidget *widget, CtkAllocation *allocation, StickyNotesApplet *applet)
{
	if ((applet->panel_orient == CAFE_PANEL_APPLET_ORIENT_UP) || (applet->panel_orient == CAFE_PANEL_APPLET_ORIENT_DOWN)) {
	  if (applet->panel_size == allocation->height)
	    return;
	  applet->panel_size = allocation->height;
	} else {
	  if (applet->panel_size == allocation->width)
	    return;
	  applet->panel_size = allocation->width;
	}

	stickynotes_applet_update_icon(applet);

	return;
}

/* Applet Callback : Deletes the applet. */
void applet_destroy_cb (CafePanelApplet *cafe_panel_applet, StickyNotesApplet *applet)
{
	GList *notes;

	stickynotes_save_now ();

	if (applet->destroy_all_dialog != NULL)
		ctk_widget_destroy (applet->destroy_all_dialog);

	if (applet->action_group)
		g_object_unref (applet->action_group);

	if (stickynotes->applets != NULL)
		stickynotes->applets = g_list_remove (stickynotes->applets, applet);

	if (stickynotes->applets == NULL) {
		notes = stickynotes->notes;
		while (notes) {
			StickyNote *note = notes->data;
			stickynote_free (note);
			notes = g_list_next (notes);
		}
	}
}

/* Destroy all response Callback: Callback for the destroy all dialog */
static void
destroy_all_response_cb (CtkDialog *dialog, gint id, StickyNotesApplet *applet)
{
	if (id == CTK_RESPONSE_OK) {
		while (g_list_length(stickynotes->notes) > 0) {
			StickyNote *note = g_list_nth_data(stickynotes->notes, 0);
			stickynote_free(note);
			stickynotes->notes = g_list_remove(stickynotes->notes, note);
		}
	}

	stickynotes_applet_update_tooltips();
	stickynotes_save();

	ctk_widget_destroy (CTK_WIDGET (dialog));
	applet->destroy_all_dialog = NULL;
}

/* Menu Callback : New Note */
void menu_new_note_cb(CtkAction *action, StickyNotesApplet *applet)
{
	popup_add_note (applet, NULL);
}

/* Menu Callback : Hide Notes */
void menu_hide_notes_cb(CtkAction *action, StickyNotesApplet *applet)
{
	stickynote_show_notes (FALSE);
}

/* Menu Callback : Destroy all sticky notes */
void menu_destroy_all_cb(CtkAction *action, StickyNotesApplet *applet)
{
	CtkBuilder *builder;

	builder = ctk_builder_new ();
  	ctk_builder_add_from_file (builder, BUILDER_PATH, NULL);

	if (applet->destroy_all_dialog != NULL) {
		ctk_window_set_screen (CTK_WINDOW (applet->destroy_all_dialog),
		                       ctk_widget_get_screen (CTK_WIDGET (applet->w_applet)));

		ctk_window_present (CTK_WINDOW (applet->destroy_all_dialog));
		return;
	}

	applet->destroy_all_dialog = CTK_WIDGET (ctk_builder_get_object (builder, "delete_all_dialog"));

	g_object_unref (builder);

	g_signal_connect (applet->destroy_all_dialog, "response",
	                  G_CALLBACK (destroy_all_response_cb),
	                  applet);

	ctk_window_set_screen (CTK_WINDOW (applet->destroy_all_dialog),
	                       ctk_widget_get_screen (applet->w_applet));

	ctk_widget_show_all (applet->destroy_all_dialog);
}

/* Menu Callback: Lock/Unlock sticky notes */
void menu_toggle_lock_cb(CtkAction *action, StickyNotesApplet *applet)
{
	gboolean locked = ctk_toggle_action_get_active (CTK_TOGGLE_ACTION (action));

	if (g_settings_is_writable (stickynotes->settings, "locked"))
		g_settings_set_boolean (stickynotes->settings, "locked", locked);
}

/* Menu Callback : Configure preferences */
void menu_preferences_cb(CtkAction *action, StickyNotesApplet *applet)
{
	stickynotes_applet_update_prefs();
	ctk_window_set_screen(CTK_WINDOW(stickynotes->w_prefs), ctk_widget_get_screen(applet->w_applet));
	ctk_window_present(CTK_WINDOW(stickynotes->w_prefs));
}

/* Menu Callback : Show help */
void menu_help_cb(CtkAction *action, StickyNotesApplet *applet)
{
	GError *error = NULL;

	ctk_show_uri_on_window (NULL,
	                        "help:cafe-stickynotes-applet",
	                        ctk_get_current_event_time (),
	                        &error);
	if (error) {
		CtkWidget *dialog = ctk_message_dialog_new(NULL, CTK_DIALOG_MODAL, CTK_MESSAGE_ERROR, CTK_BUTTONS_CLOSE,
							   _("There was an error displaying help: %s"), error->message);
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(ctk_widget_destroy), NULL);
		ctk_window_set_resizable(CTK_WINDOW(dialog), FALSE);
		ctk_window_set_screen (CTK_WINDOW(dialog), ctk_widget_get_screen(applet->w_applet));
		ctk_widget_show(dialog);
		g_error_free(error);
	}
}

/* Menu Callback : Display About window */
void
menu_about_cb (CtkAction *action,
	       StickyNotesApplet *applet)
{
	static const gchar *authors[] = {
		"Loban A Rahman <loban@earthling.net>",
		"Davyd Madeley <davyd@madeley.id.au>",
		NULL
	};

	static const gchar *documenters[] = {
		"Loban A Rahman <loban@earthling.net>",
		N_("Sun GNOME Documentation Team <gdocteam@sun.com>"),
		N_("CAFE Documentation Team"),
		NULL
	};

#ifdef ENABLE_NLS
	const char **p;
	for (p = documenters; *p; ++p)
		*p = _(*p);
#endif

	ctk_show_about_dialog (NULL,
		"title",        _("About Sticky Notes"),
		"version",	VERSION,
		"copyright",	_("Copyright \xc2\xa9 2002-2003 Loban A Rahman\n"
				  "Copyright \xc2\xa9 2005 Davyd Madeley\n"
				  "Copyright \xc2\xa9 2012-2020 CAFE developers"),
		"comments",	_("Sticky Notes for the "
				  "CAFE Desktop Environment"),
		"authors",	authors,
		"documenters",	documenters,
		"translator-credits",	_("translator-credits"),
		"logo-icon-name",	"cafe-sticky-notes-applet",
		NULL);
}

/* Preferences Callback : Save. */
void
preferences_save_cb (gpointer data)
{
	gint width = ctk_adjustment_get_value (stickynotes->w_prefs_width);
	gint height = ctk_adjustment_get_value (stickynotes->w_prefs_height);
	gboolean sys_color = ctk_toggle_button_get_active (
			CTK_TOGGLE_BUTTON (stickynotes->w_prefs_sys_color));
	gboolean sys_font = ctk_toggle_button_get_active (
			CTK_TOGGLE_BUTTON (stickynotes->w_prefs_sys_font));
	gboolean sticky = ctk_toggle_button_get_active (
			CTK_TOGGLE_BUTTON (stickynotes->w_prefs_sticky));
	gboolean force_default = ctk_toggle_button_get_active (
			CTK_TOGGLE_BUTTON (stickynotes->w_prefs_force));
	gboolean desktop_hide = ctk_toggle_button_get_active (
			CTK_TOGGLE_BUTTON (stickynotes->w_prefs_desktop));

	if (g_settings_is_writable (stickynotes->settings,"default-width"))
		g_settings_set_int (stickynotes->settings,"default-width", width);
	if (g_settings_is_writable (stickynotes->settings,"default-height"))
		g_settings_set_int (stickynotes->settings,"default-height", height);
	if (g_settings_is_writable (stickynotes->settings,"use-system-color"))
		g_settings_set_boolean (stickynotes->settings,"use-system-color", sys_color);
	if (g_settings_is_writable (stickynotes->settings,"use-system-font"))
		g_settings_set_boolean (stickynotes->settings,"use-system-font", sys_font);
	if (g_settings_is_writable (stickynotes->settings,"sticky"))
		g_settings_set_boolean (stickynotes->settings,"sticky", sticky);
	if (g_settings_is_writable (stickynotes->settings,"force-default"))
		g_settings_set_boolean (stickynotes->settings,"force-default", force_default);
	if (g_settings_is_writable (stickynotes->settings,"desktop-hide"))
		g_settings_set_boolean (stickynotes->settings,"desktop-hide", desktop_hide);
}

/* Preferences Callback : Change color. */
void
preferences_color_cb (CtkWidget *button, gpointer data)
{
	char *color_str, *font_color_str;

	CdkRGBA color, font_color;

	ctk_color_chooser_get_rgba (CTK_COLOR_CHOOSER (stickynotes->w_prefs_color), &color);
	ctk_color_chooser_get_rgba (CTK_COLOR_CHOOSER (stickynotes->w_prefs_font_color), &font_color);

	color_str = cdk_rgba_to_string (&color);
	font_color_str = cdk_rgba_to_string (&font_color);

	g_settings_set_string (stickynotes->settings, "default-color", color_str);
	g_settings_set_string (stickynotes->settings, "default-font-color", font_color_str);

	g_free (color_str);
	g_free (font_color_str);
}

/* Preferences Callback : Change font. */
void preferences_font_cb (CtkWidget *button, gpointer data)
{
	const char *font_str;

	font_str = ctk_font_button_get_font_name (CTK_FONT_BUTTON (button));
	g_settings_set_string (stickynotes->settings, "default-font", font_str);
}

/* Preferences Callback : Apply to existing notes. */
void preferences_apply_cb(GSettings *settings, gchar *key, gpointer data)
{
	GList *l;
	StickyNote *note;

	if (!strcmp (key, "sticky"))
	{
		if (g_settings_get_boolean (settings, key))
			for (l = stickynotes->notes; l; l = l->next)
			{
				note = l->data;
				ctk_window_stick (CTK_WINDOW (note->w_window));
			}
		else
			for (l = stickynotes->notes; l; l = l->next)
			{
				note = l->data;
				ctk_window_unstick (CTK_WINDOW (
							note->w_window));
			}
	}

	else if (!strcmp (key, "locked"))
	{
		for (l = stickynotes->notes; l; l = l->next)
		{
			note = l->data;
			stickynote_set_locked (note,
					g_settings_get_boolean (settings, key));
		}
		stickynotes_save();
	}

	else if (!strcmp (key, "use-system-color") ||
		 !strcmp (key, "default-color"))
	{
		for (l = stickynotes->notes; l; l = l->next)
		{
			note = l->data;
			stickynote_set_color (note,
					note->color, note->font_color,
					FALSE);
		}
	}

	else if (!strcmp (key, "use-system-font") ||
		 !strcmp (key, "default-font"))
	{
		for (l = stickynotes->notes; l; l = l->next)
		{
			note = l->data;
			stickynote_set_font (note, note->font, FALSE);
		}
	}

	else if (!strcmp (key, "force-default"))
	{
		for (l = stickynotes->notes; l; l = l->next)
		{
			note = l->data;
			stickynote_set_color(note,
					note->color, note->font_color,
					FALSE);
			stickynote_set_font(note, note->font, FALSE);
		}
	}

	stickynotes_applet_update_prefs();
	stickynotes_applet_update_menus();
}

/* Preferences Callback : Response. */
void preferences_response_cb(CtkWidget *dialog, gint response, gpointer data)
{
	if (response == CTK_RESPONSE_HELP) {
		GError *error = NULL;

		ctk_show_uri_on_window (CTK_WINDOW (dialog),
		                        "help:cafe-stickynotes-applet/stickynotes-advanced-settings",
		                        ctk_get_current_event_time (),
		                        &error);
		if (error) {
			dialog = ctk_message_dialog_new(NULL, CTK_DIALOG_MODAL, CTK_MESSAGE_ERROR, CTK_BUTTONS_CLOSE,
								   _("There was an error displaying help: %s"), error->message);
			g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(ctk_widget_destroy), NULL);
			ctk_window_set_resizable(CTK_WINDOW(dialog), FALSE);
			ctk_window_set_screen (CTK_WINDOW(dialog), ctk_widget_get_screen(CTK_WIDGET(dialog)));
			ctk_widget_show(dialog);
			g_error_free(error);
		}
	}

	else if (response == CTK_RESPONSE_CLOSE)
	        ctk_widget_hide(CTK_WIDGET(dialog));
}

/* Preferences Callback : Delete */
gboolean preferences_delete_cb(CtkWidget *widget, CdkEvent *event, gpointer data)
{
	ctk_widget_hide(widget);

	return TRUE;
}
