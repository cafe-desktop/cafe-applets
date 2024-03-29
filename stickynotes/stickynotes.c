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
#include <libxml/parser.h>
#include <X11/Xatom.h>
#include <cdk/cdkx.h>
#include <ctk/ctk.h>
#define VNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libvnck/libvnck.h>
#include <string.h>
#include <sys/stat.h>

#include <ctksourceview/ctksource.h>

#include "stickynotes.h"
#include "stickynotes_callbacks.h"
#include "util.h"
#include "stickynotes_applet.h"

/* Stop gcc complaining about xmlChar's signedness */
#define XML_CHAR(str) ((xmlChar *) (str))
#define STICKYNOTES_ICON_SIZE 8

static gboolean save_scheduled = FALSE;

static void response_cb (CtkWidget *dialog, gint id, gpointer data);

/* Based on a function found in vnck */
static void
set_icon_geometry (CdkWindow *window,
                   int        x,
                   int        y,
                   int        width,
                   int        height)
{
	gulong data[4];
	Display *dpy;

	dpy = cdk_x11_display_get_xdisplay (cdk_window_get_display (window));

	data[0] = x;
	data[1] = y;
	data[2] = width;
	data[3] = height;

	XChangeProperty (dpy,
	                 CDK_WINDOW_XID (window),
	                 cdk_x11_get_xatom_by_name_for_display (
	                     cdk_window_get_display (window),
	                     "_NET_WM_ICON_GEOMETRY"),
	                 XA_CARDINAL, 32, PropModeReplace,
	                 (guchar *)&data, 4);
}

/* Called when a timeout occurs.  */
static gboolean
timeout_happened (gpointer data)
{
	if (GPOINTER_TO_UINT (data) == stickynotes->last_timeout_data)
		stickynotes_save ();
	return FALSE;
}

/* Called when a text buffer is changed.  */
static void
buffer_changed (CtkTextBuffer *buffer, StickyNote *note)
{
	if ( (note->h + note->y) > stickynotes->max_height )
		ctk_scrolled_window_set_policy ( CTK_SCROLLED_WINDOW(note->w_scroller),
													CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);

	/* When a buffer is changed, we set a 10 second timer.  When
	   the timer triggers, we will save the buffer if there have
	   been no subsequent changes.  */
	++stickynotes->last_timeout_data;
	g_timeout_add_seconds (10, (GSourceFunc) timeout_happened,
		       GUINT_TO_POINTER (stickynotes->last_timeout_data));
}

/* Create a new (empty) Sticky Note at a specific position
   and with specific size */
static StickyNote *
stickynote_new_aux (CdkScreen *screen, gint x, gint y, gint w, gint h)
{
	StickyNote *note;
	CtkBuilder *builder;

	note = g_new (StickyNote, 1);

	builder = ctk_builder_new ();
	ctk_builder_add_from_resource (builder, GRESOURCE "sticky-notes-note.ui", NULL);
	ctk_builder_add_from_resource (builder, GRESOURCE "sticky-notes-properties.ui", NULL);

	note->w_window = CTK_WIDGET (ctk_builder_get_object (builder, "stickynote_window"));
	ctk_window_set_screen(CTK_WINDOW(note->w_window),screen);
	ctk_window_set_decorated (CTK_WINDOW (note->w_window), FALSE);
	ctk_window_set_skip_taskbar_hint (CTK_WINDOW (note->w_window), TRUE);
	ctk_window_set_skip_pager_hint (CTK_WINDOW (note->w_window), TRUE);
	ctk_widget_add_events (note->w_window, CDK_BUTTON_PRESS_MASK);

	note->w_title = CTK_WIDGET (ctk_builder_get_object (builder, "title_label"));
	note->w_body = CTK_WIDGET (ctk_builder_get_object (builder, "body_text"));
	note->w_scroller = CTK_WIDGET (ctk_builder_get_object (builder, "body_scroller"));
	note->w_lock = CTK_WIDGET (ctk_builder_get_object (builder, "lock_button"));
	ctk_widget_add_events (note->w_lock, CDK_BUTTON_PRESS_MASK);

	note->buffer = CTK_SOURCE_BUFFER(ctk_text_view_get_buffer(CTK_TEXT_VIEW(note->w_body)));

	note->w_close = CTK_WIDGET (ctk_builder_get_object (builder, "close_button"));
	ctk_widget_add_events (note->w_close, CDK_BUTTON_PRESS_MASK);
	note->w_resize_se = CTK_WIDGET (ctk_builder_get_object (builder, "resize_se_box"));
	ctk_widget_add_events (note->w_resize_se, CDK_BUTTON_PRESS_MASK);
	note->w_resize_sw = CTK_WIDGET (ctk_builder_get_object (builder, "resize_sw_box"));
	ctk_widget_add_events (note->w_resize_sw, CDK_BUTTON_PRESS_MASK);

	note->img_lock = CTK_IMAGE (ctk_builder_get_object (builder,
	                "lock_img"));
	note->img_close = CTK_IMAGE (ctk_builder_get_object (builder,
	                "close_img"));
	note->img_resize_se = CTK_IMAGE (ctk_builder_get_object (builder,
	                "resize_se_img"));
	note->img_resize_sw = CTK_IMAGE (ctk_builder_get_object (builder,
	                "resize_sw_img"));

	/* deal with RTL environments */
	ctk_widget_set_direction (CTK_WIDGET (ctk_builder_get_object (builder, "resize_bar")),
			CTK_TEXT_DIR_LTR);

	note->w_menu = CTK_WIDGET (ctk_builder_get_object (builder, "stickynote_menu"));
	note->ta_lock_toggle_item = CTK_TOGGLE_ACTION (ctk_builder_get_object (builder,
	        "popup_toggle_lock"));

	note->w_properties = CTK_WIDGET (ctk_builder_get_object (builder,
			"stickynote_properties"));
	ctk_window_set_screen (CTK_WINDOW (note->w_properties), screen);

	note->w_entry = CTK_WIDGET (ctk_builder_get_object (builder, "title_entry"));
	note->w_color = CTK_WIDGET (ctk_builder_get_object (builder, "note_color"));
	note->w_color_label = CTK_WIDGET (ctk_builder_get_object (builder, "color_label"));
	note->w_font_color = CTK_WIDGET (ctk_builder_get_object (builder, "font_color"));
	note->w_font_color_label = CTK_WIDGET (ctk_builder_get_object (builder,
			"font_color_label"));
	note->w_font = CTK_WIDGET (ctk_builder_get_object (builder, "note_font"));
	note->w_font_label = CTK_WIDGET (ctk_builder_get_object (builder, "font_label"));
	note->w_def_color = CTK_WIDGET (&CTK_CHECK_BUTTON (
				ctk_builder_get_object (builder,
					"def_color_check"))->toggle_button);
	note->w_def_font = CTK_WIDGET (&CTK_CHECK_BUTTON (
				ctk_builder_get_object (builder,
					"def_font_check"))->toggle_button);

	note->color = NULL;
	note->font_color = NULL;
	note->font = NULL;
	note->locked = FALSE;
	note->x = x;
	note->y = y;
	note->w = w;
	note->h = h;

	/* Customize the window */
	if (g_settings_get_boolean (stickynotes->settings, "sticky"))
		ctk_window_stick(CTK_WINDOW(note->w_window));

	if (w == 0 || h == 0)
		ctk_window_resize (CTK_WINDOW(note->w_window),
				g_settings_get_int (stickynotes->settings, "default-width"),
				g_settings_get_int (stickynotes->settings, "default-height"));
	else
		ctk_window_resize (CTK_WINDOW(note->w_window),
				note->w,
				note->h);

	if (x != -1 && y != -1)
		ctk_window_move (CTK_WINDOW(note->w_window),
				note->x,
				note->y);

	/* Set the button images */
	ctk_image_set_from_icon_name (note->img_close, STICKYNOTES_STOCK_CLOSE, CTK_ICON_SIZE_MENU);
	ctk_image_set_pixel_size (note->img_close, STICKYNOTES_ICON_SIZE);

	ctk_image_set_from_icon_name (note->img_resize_se, STICKYNOTES_STOCK_RESIZE_SE, CTK_ICON_SIZE_MENU);
	ctk_image_set_pixel_size (note->img_resize_se, STICKYNOTES_ICON_SIZE);

	ctk_image_set_from_icon_name (note->img_resize_sw, STICKYNOTES_STOCK_RESIZE_SW, CTK_ICON_SIZE_MENU);
	ctk_image_set_pixel_size (note->img_resize_sw, STICKYNOTES_ICON_SIZE);

	ctk_widget_show(note->w_lock);
	ctk_widget_show(note->w_close);
	ctk_widget_show(CTK_WIDGET (ctk_builder_get_object (builder, "resize_bar")));

	/* Customize the title and colors, hide and unlock */
	stickynote_set_title(note, NULL);
	stickynote_set_color(note, NULL, NULL, TRUE);
	stickynote_set_font(note, NULL, TRUE);
	stickynote_set_locked(note, FALSE);

	ctk_widget_realize (note->w_window);

	/* Connect a popup menu to all buttons and title */
	/* CtkBuilder holds and drops the references to all the widgets it
	 * creates for as long as it exist (CtkBuilder). Hence in our callback
	 * we would have an invalid CtkMenu. We need to ref it.
	 */
	g_object_ref (note->w_menu);
	g_signal_connect (G_OBJECT (note->w_window), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	g_signal_connect (G_OBJECT (note->w_lock), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	g_signal_connect (G_OBJECT (note->w_close), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	g_signal_connect (G_OBJECT (note->w_resize_se), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	g_signal_connect (G_OBJECT (note->w_resize_sw), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	/* Connect a properties dialog to the note */
	ctk_window_set_transient_for (CTK_WINDOW(note->w_properties),
			CTK_WINDOW(note->w_window));
	ctk_dialog_set_default_response (CTK_DIALOG(note->w_properties),
			CTK_RESPONSE_CLOSE);
	g_signal_connect (G_OBJECT (note->w_properties), "response",
			G_CALLBACK (response_cb), note);

	/* Connect signals to the sticky note */
	g_signal_connect (G_OBJECT (note->w_lock), "clicked",
			G_CALLBACK (stickynote_toggle_lock_cb), note);
	g_signal_connect (G_OBJECT (note->w_close), "clicked",
			G_CALLBACK (stickynote_close_cb), note);
	g_signal_connect (G_OBJECT (note->w_resize_se), "button-press-event",
			G_CALLBACK (stickynote_resize_cb), note);
	g_signal_connect (G_OBJECT (note->w_resize_sw), "button-press-event",
			G_CALLBACK (stickynote_resize_cb), note);

	g_signal_connect (G_OBJECT (note->w_window), "button-press-event",
			G_CALLBACK (stickynote_move_cb), note);
	g_signal_connect (G_OBJECT (note->w_window), "configure-event",
			G_CALLBACK (stickynote_configure_cb), note);
	g_signal_connect (G_OBJECT (note->w_window), "delete-event",
			G_CALLBACK (stickynote_delete_cb), note);

	g_signal_connect (ctk_builder_get_object (builder,
					"popup_create"), "activate",
			G_CALLBACK (popup_create_cb), note);
	g_signal_connect (ctk_builder_get_object (builder,
					"popup_destroy"), "activate",
			G_CALLBACK (popup_destroy_cb), note);
	g_signal_connect (ctk_builder_get_object (builder,
					"popup_toggle_lock"), "toggled",
			G_CALLBACK (popup_toggle_lock_cb), note);
	g_signal_connect (ctk_builder_get_object (builder,
					"popup_properties"), "activate",
			G_CALLBACK (popup_properties_cb), note);

	g_signal_connect_swapped (G_OBJECT (note->w_entry), "changed",
			G_CALLBACK (properties_apply_title_cb), note);
	g_signal_connect (G_OBJECT (note->w_color), "color-set",
			G_CALLBACK (properties_color_cb), note);
	g_signal_connect (G_OBJECT (note->w_font_color), "color-set",
			G_CALLBACK (properties_color_cb), note);
	g_signal_connect_swapped (G_OBJECT (note->w_def_color), "toggled",
			G_CALLBACK (properties_apply_color_cb), note);
	g_signal_connect (G_OBJECT (note->w_font), "font-set",
			G_CALLBACK (properties_font_cb), note);
	g_signal_connect_swapped (G_OBJECT (note->w_def_font), "toggled",
			G_CALLBACK (properties_apply_font_cb), note);
	g_signal_connect (G_OBJECT (note->w_entry), "activate",
			G_CALLBACK (properties_activate_cb), note);
	g_signal_connect (G_OBJECT (note->w_properties), "delete-event",
			G_CALLBACK (ctk_widget_hide), note);

	g_object_unref(builder);

	g_signal_connect_after (note->w_body, "button-press-event",
	                        G_CALLBACK (ctk_true), note);

	g_signal_connect (ctk_text_view_get_buffer(CTK_TEXT_VIEW(note->w_body)),
			  "changed",
			  G_CALLBACK (buffer_changed), note);

	return note;
}

/* Create a new (empty) Sticky Note */
StickyNote *
stickynote_new (CdkScreen *screen)
{
	return stickynote_new_aux (screen, -1, -1, 0, 0);
}

/* Destroy a Sticky Note */
void stickynote_free(StickyNote *note)
{
	ctk_widget_destroy(note->w_properties);
	ctk_widget_destroy(note->w_menu);
	ctk_widget_destroy(note->w_window);

	g_free(note->color);
	g_free(note->font_color);
	g_free(note->font);

	g_free(note);
}

/* Change the sticky note title and color */
void stickynote_change_properties (StickyNote *note)
{
	char *color_str = NULL;

	ctk_entry_set_text(CTK_ENTRY(note->w_entry),
			ctk_label_get_text (CTK_LABEL (note->w_title)));

	ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(note->w_def_color),
			note->color == NULL);

	if (note->color)
		color_str = g_strdup (note->color);
	else
	{
		color_str = g_settings_get_string (stickynotes->settings, "default-color");
	}

	if (color_str)
	{
		CdkRGBA color;
		cdk_rgba_parse (&color, color_str);
		ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (note->w_color), &color);
		g_free (color_str);
	}

	if (note->font_color)
		color_str = g_strdup (note->font_color);
	else
	{
		color_str = g_settings_get_string (stickynotes->settings, "default-font-color");
	}

	if (color_str)
	{
		CdkRGBA font_color;
		cdk_rgba_parse (&font_color, color_str);
		ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (note->w_font_color), &font_color);
		g_free (color_str);
	}

	ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(note->w_def_font),
			note->font == NULL);
	if (note->font)
		ctk_font_button_set_font_name (CTK_FONT_BUTTON (note->w_font),
				note->font);

	ctk_widget_show (note->w_properties);

	stickynotes_save();
}

static void
response_cb (CtkWidget *dialog, gint id, gpointer data)
{
        if (id == CTK_RESPONSE_HELP)
		ctk_show_uri_on_window (CTK_WINDOW (dialog),
		                        "help:cafe-stickynotes-applet/stickynotes-settings-individual",
		                        ctk_get_current_event_time (),
		                        NULL);
        else if (id == CTK_RESPONSE_CLOSE)
                ctk_widget_hide (dialog);
}

/* Check if a sticky note is empty */
gboolean stickynote_get_empty(const StickyNote *note)
{
	return ctk_text_buffer_get_char_count(ctk_text_view_get_buffer(CTK_TEXT_VIEW(note->w_body))) == 0;
}

/* Set the sticky note title */
void stickynote_set_title(StickyNote *note, const gchar *title)
{
	/* If title is NULL, use the current date as the title. */
	if (!title) {
		gchar *date_title, *tmp;
		gchar *date_format = g_settings_get_string (stickynotes->settings, "date-format");
		if (!date_format)
			date_format = g_strdup ("%x");
		tmp = get_current_date (date_format);
		date_title = g_locale_to_utf8 (tmp, -1, NULL, NULL, NULL);

		ctk_window_set_title(CTK_WINDOW(note->w_window), date_title);
		ctk_label_set_text(CTK_LABEL (note->w_title), date_title);

		g_free (tmp);
		g_free(date_title);
		g_free(date_format);
	}
	else {
		ctk_window_set_title(CTK_WINDOW(note->w_window), title);
		ctk_label_set_text(CTK_LABEL (note->w_title), title);
	}
}

/* Set the sticky note color */
void
stickynote_set_color (StickyNote  *note,
		      const gchar *color_str,
		      const gchar *font_color_str,
		      gboolean     save)
{
	char *color_str_actual, *font_color_str_actual;
	gboolean force_default, use_system_color;

	if (save) {
		if (note->color)
			g_free (note->color);
		if (note->font_color)
			g_free (note->font_color);

		note->color = color_str ?
			g_strdup (color_str) : NULL;
		note->font_color = font_color_str ?
			g_strdup (font_color_str) : NULL;

		ctk_widget_set_sensitive (note->w_color_label,
				note->color != NULL);
		ctk_widget_set_sensitive (note->w_font_color_label,
				note->font_color != NULL);
		ctk_widget_set_sensitive (note->w_color,
				note->color != NULL);
		ctk_widget_set_sensitive (note->w_font_color,
				note->color != NULL);
	}

	force_default = g_settings_get_boolean (stickynotes->settings, "force-default");
	use_system_color = g_settings_get_boolean (stickynotes->settings, "use-system-color");

	/* If "force_default" is enabled or color_str is NULL,
	 * then we use the default color instead of color_str. */
	if (!color_str || force_default)
	{
		if (use_system_color)
			color_str_actual = NULL;
		else
			color_str_actual = g_settings_get_string (stickynotes->settings, "default-color");
	}
	else
		color_str_actual = g_strdup (color_str);

	if (!font_color_str || force_default)
	{
		if (use_system_color)
			font_color_str_actual = NULL;
		else
			font_color_str_actual = g_settings_get_string (stickynotes->settings, "default-font-color");
	}
	else
		font_color_str_actual = g_strdup (font_color_str);

	/* Do not use custom colors if "use_system_color" is enabled */
	if (color_str_actual) {
		CdkRGBA colors[4];
		gint i;

		for (i = 0; i <= 3; i++)
		{
			cdk_rgba_parse (&colors[i], color_str_actual);
			colors[i].red = (colors[i].red * (10 - i)) / 10;
			colors[i].green = (colors[i].green * (10 - i)) / 10;
			colors[i].blue = (colors[i].blue * (10 - i)) / 10;
		}

		ctk_widget_override_background_color (note->w_window, CTK_STATE_NORMAL, &colors[0]);
		ctk_widget_override_background_color (note->w_body, CTK_STATE_NORMAL, &colors[0]);
		ctk_widget_override_background_color (note->w_lock, CTK_STATE_NORMAL, &colors[0]);
		ctk_widget_override_background_color (note->w_close, CTK_STATE_NORMAL, &colors[0]);
		ctk_widget_override_background_color (note->w_resize_se, CTK_STATE_NORMAL, &colors[0]);
		ctk_widget_override_background_color (note->w_resize_sw, CTK_STATE_NORMAL, &colors[0]);
	} else {
		ctk_widget_override_background_color (note->w_window, CTK_STATE_NORMAL, NULL);
		ctk_widget_override_background_color (note->w_body, CTK_STATE_NORMAL, NULL);
		ctk_widget_override_background_color (note->w_lock, CTK_STATE_NORMAL, NULL);
		ctk_widget_override_background_color (note->w_close, CTK_STATE_NORMAL, NULL);
		ctk_widget_override_background_color (note->w_resize_se, CTK_STATE_NORMAL, NULL);
		ctk_widget_override_background_color (note->w_resize_sw, CTK_STATE_NORMAL, NULL);
	}

	if (font_color_str_actual)
	{
		CdkRGBA color;

		cdk_rgba_parse (&color, font_color_str_actual);

		ctk_widget_override_color (note->w_window, CTK_STATE_NORMAL, &color);
		ctk_widget_override_color (note->w_body, CTK_STATE_NORMAL, &color);
	}
	else
	{
		ctk_widget_override_color (note->w_window, CTK_STATE_NORMAL, NULL);
		ctk_widget_override_color (note->w_body, CTK_STATE_NORMAL, NULL);
	}

	if (color_str_actual)
		g_free (color_str_actual);
	if (font_color_str_actual)
		g_free (font_color_str_actual);
}

/* Set the sticky note font */
void
stickynote_set_font (StickyNote *note, const gchar *font_str, gboolean save)
{
	PangoFontDescription *font_desc;
	gchar *font_str_actual;

	if (save) {
		g_free (note->font);
		note->font = font_str ? g_strdup (font_str) : NULL;

		ctk_widget_set_sensitive (note->w_font_label, note->font != NULL);
		ctk_widget_set_sensitive(note->w_font, note->font != NULL);
	}

	/* If "force_default" is enabled or font_str is NULL,
	 * then we use the default font instead of font_str. */
	if (!font_str || g_settings_get_boolean (stickynotes->settings, "force-default"))
	{
		if (g_settings_get_boolean (stickynotes->settings, "use-system-font"))
			font_str_actual = NULL;
		else
			font_str_actual = g_settings_get_string (stickynotes->settings, "default-font");
	}
	else
		font_str_actual = g_strdup (font_str);

	/* Do not use custom fonts if "use_system_font" is enabled */
	font_desc = font_str_actual ?
		pango_font_description_from_string (font_str_actual): NULL;

	/* Apply the style to the widgets */
	ctk_widget_override_font (note->w_window, font_desc);
	ctk_widget_override_font (note->w_body, font_desc);

	g_free (font_str_actual);
	pango_font_description_free (font_desc);
}

/* Lock/Unlock a sticky note from editing */
void stickynote_set_locked(StickyNote *note, gboolean locked)
{
	note->locked = locked;

	/* Set cursor visibility and editability */
	ctk_text_view_set_editable(CTK_TEXT_VIEW(note->w_body), !locked);
	ctk_text_view_set_cursor_visible(CTK_TEXT_VIEW(note->w_body), !locked);

	/* Show appropriate icon and tooltip */
	if (locked) {
		ctk_image_set_from_icon_name (note->img_lock, STICKYNOTES_STOCK_LOCKED, CTK_ICON_SIZE_MENU);
		ctk_widget_set_tooltip_text(note->w_lock, _("This note is locked."));
	}
	else {
		ctk_image_set_from_icon_name (note->img_lock, STICKYNOTES_STOCK_UNLOCKED, CTK_ICON_SIZE_MENU);
		ctk_widget_set_tooltip_text(note->w_lock, _("This note is unlocked."));
	}

	ctk_image_set_pixel_size (note->img_lock, STICKYNOTES_ICON_SIZE);

	ctk_toggle_action_set_active(note->ta_lock_toggle_item, locked);

	stickynotes_applet_update_menus();
}

/* Show/Hide a sticky note */
void
stickynote_set_visible (StickyNote *note, gboolean visible)
{
	if (visible)
	{
		ctk_window_present (CTK_WINDOW (note->w_window));

		if (note->x != -1 || note->y != -1)
			ctk_window_move (CTK_WINDOW (note->w_window),
					note->x, note->y);
		/* Put the note on all workspaces if necessary. */
		if (g_settings_get_boolean (stickynotes->settings, "sticky"))
			ctk_window_stick(CTK_WINDOW(note->w_window));
		else if (note->workspace > 0)
		{
#if 0
			VnckWorkspace *vnck_ws;
			gulong xid;
			VnckWindow *vnck_win;
			VnckScreen *vnck_screen;

			g_print ("set_visible(): workspace = %i\n",
					note->workspace);

			xid = CDK_WINDOW_XID (note->w_window->window);
			vnck_screen = vnck_screen_get_default ();
			vnck_win = vnck_window_get (xid);
			vnck_ws = vnck_screen_get_workspace (
					vnck_screen,
					note->workspace - 1);
			if (vnck_win && vnck_ws)
				vnck_window_move_to_workspace (
						vnck_win, vnck_ws);
			else
				g_print ("set_visible(): errr\n");
#endif
			xstuff_change_workspace (CTK_WINDOW (note->w_window),
					note->workspace - 1);
		}
	}
	else {
		/* Hide sticky note */
		int x, y, width, height;
		stickynotes_applet_panel_icon_get_geometry (&x, &y, &width, &height);
		set_icon_geometry (ctk_widget_get_window (CTK_WIDGET (note->w_window)),
				   x, y, width, height);
		ctk_window_iconify(CTK_WINDOW (note->w_window));
	}
}

/* Add a sticky note */
void stickynotes_add (CdkScreen *screen)
{
	StickyNote *note;

	note = stickynote_new (screen);

	stickynotes->notes = g_list_append(stickynotes->notes, note);
	stickynotes_applet_update_tooltips();
	stickynotes_save();
	stickynote_set_visible (note, TRUE);
}

/* Remove a sticky note with confirmation, if needed */
void stickynotes_remove(StickyNote *note)
{
	CtkBuilder *builder;
	CtkWidget *dialog;

	builder = ctk_builder_new ();
	ctk_builder_add_from_resource (builder, GRESOURCE "sticky-notes-delete.ui", NULL);

	dialog = CTK_WIDGET (ctk_builder_get_object (builder, "delete_dialog"));

	ctk_window_set_transient_for(CTK_WINDOW(dialog), CTK_WINDOW(note->w_window));

	if (stickynote_get_empty(note)
	    || !g_settings_get_boolean (stickynotes->settings, "confirm-deletion")
	    || ctk_dialog_run(CTK_DIALOG(dialog)) == CTK_RESPONSE_OK) {

		/* Remove the note from the linked-list of all notes */
		stickynotes->notes = g_list_remove_all (stickynotes->notes, note);

		/* Destroy the note */
		stickynote_free (note);

		/* Update tooltips */
		stickynotes_applet_update_tooltips();

		/* Save notes */
		stickynotes_save();
	}

	ctk_widget_destroy(dialog);
	g_object_unref(builder);
}

/* Save all sticky notes in an XML configuration file */
gboolean
stickynotes_save_now (void)
{
	VnckScreen *vnck_screen;
	const gchar *title;
	CtkTextBuffer *buffer;
	CtkTextIter start, end;
	gchar *body;

	gint i;

	/* Create a new XML document */
	xmlDocPtr doc = xmlNewDoc(XML_CHAR ("1.0"));
	xmlNodePtr root = xmlNewDocNode(doc, NULL, XML_CHAR ("stickynotes"), NULL);

	xmlDocSetRootElement(doc, root);
	xmlNewProp(root, XML_CHAR("version"), XML_CHAR (VERSION));

	vnck_screen = vnck_screen_get_default ();
	vnck_screen_force_update (vnck_screen);

	/* For all sticky notes */
	for (i = 0; i < g_list_length(stickynotes->notes); i++) {
		VnckWindow *vnck_win;
		gulong xid = 0;

		/* Access the current note in the list */
		StickyNote *note = g_list_nth_data(stickynotes->notes, i);

		/* Retrieve the window size of the note */
		gchar *w_str = g_strdup_printf("%d", note->w);
		gchar *h_str = g_strdup_printf("%d", note->h);

		/* Retrieve the window position of the note */
		gchar *x_str = g_strdup_printf("%d", note->x);
		gchar *y_str = g_strdup_printf("%d", note->y);

		xid = CDK_WINDOW_XID (ctk_widget_get_window (note->w_window));
		vnck_win = vnck_window_get (xid);

		if (!g_settings_get_boolean (stickynotes->settings, "sticky") &&
			vnck_win)
			note->workspace = 1 +
				vnck_workspace_get_number (
				vnck_window_get_workspace (vnck_win));
		else
			note->workspace = 0;

		/* Retrieve the title of the note */
		title = ctk_label_get_text(CTK_LABEL(note->w_title));

		/* Retrieve body contents of the note */
		buffer = ctk_text_view_get_buffer(CTK_TEXT_VIEW(note->w_body));

		ctk_text_buffer_get_bounds(buffer, &start, &end);
		body = ctk_text_iter_get_text(&start, &end);

		/* Save the note as a node in the XML document */
		{
			xmlNodePtr node = xmlNewTextChild(root, NULL, XML_CHAR ("note"),
					XML_CHAR (body));
			xmlNewProp(node, XML_CHAR ("title"), XML_CHAR (title));
			if (note->color)
				xmlNewProp (node, XML_CHAR ("color"), XML_CHAR (note->color));
			if (note->font_color)
				xmlNewProp (node, XML_CHAR ("font_color"),
						XML_CHAR (note->font_color));
			if (note->font)
				xmlNewProp (node, XML_CHAR ("font"), XML_CHAR (note->font));
			if (note->locked)
				xmlNewProp (node, XML_CHAR ("locked"), XML_CHAR ("true"));
			xmlNewProp (node, XML_CHAR ("x"), XML_CHAR (x_str));
			xmlNewProp (node, XML_CHAR ("y"), XML_CHAR (y_str));
			xmlNewProp (node, XML_CHAR ("w"), XML_CHAR (w_str));
			xmlNewProp (node, XML_CHAR ("h"), XML_CHAR (h_str));
			if (note->workspace > 0)
			{
				char *workspace_str;

				workspace_str = g_strdup_printf ("%i",
						note->workspace);
				xmlNewProp (node, XML_CHAR ("workspace"), XML_CHAR (workspace_str));
				g_free (workspace_str);
			}
		}

		/* Now that it has been saved, reset the modified flag */
		ctk_text_buffer_set_modified(buffer, FALSE);

		g_free(x_str);
		g_free(y_str);
		g_free(w_str);
		g_free(h_str);
		g_free(body);
	}

	/* The XML file is $HOME/.config/cafe/stickynotes-applet, most probably */
	{
		gchar* path = g_build_filename(g_get_user_config_dir(), "cafe", NULL);
		gchar* file = g_build_filename(path, "stickynotes-applet.xml", NULL);
		g_mkdir_with_parents(path, S_IRWXU);
		g_free(path);

		xmlSaveFormatFile(file, doc, 1);

		g_free(file);
	}

	xmlFreeDoc(doc);

	save_scheduled = FALSE;

	return FALSE;
}

void
stickynotes_save (void)
{
	/* If a save isn't already scheduled, save everything a minute from now. */
	if (!save_scheduled) {
		g_timeout_add_seconds (60, (GSourceFunc) stickynotes_save_now, NULL);
		save_scheduled = TRUE;
	}
}

/* Load all sticky notes from an XML configuration file */
void
stickynotes_load (CdkScreen *screen)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr root;
	xmlNodePtr node;
	/* VnckScreen *vnck_screen; */
	GList *new_notes, *tmp1;  /* Lists of StickyNote*'s */
	GList *new_nodes; /* Lists of xmlNodePtr's */
	int x, y, w, h;

	/* The XML file is $HOME/.config/cafe/stickynotes-applet, most probably */
	{
		gchar* file = g_build_filename(g_get_user_config_dir(), "cafe", "stickynotes-applet.xml", NULL);

		if (g_file_test(file, G_FILE_TEST_EXISTS))
		{
			/* load file */
		doc = xmlParseFile(file);
		}
		else
		{
			/* old one */
			g_free(file);

			file = g_build_filename(g_get_home_dir(), ".cafe2", "stickynotes_applet", NULL);

			if (g_file_test(file, G_FILE_TEST_EXISTS))
			{
				/* load file */
				doc = xmlParseFile(file);
			}
		}

		g_free(file);
	}

	/* If the XML file does not exist, create a blank one */
	if (!doc)
	{
		stickynotes_save();
		return;
	}

	/* If the XML file is corrupted/incorrect, create a blank one */
	root = xmlDocGetRootElement(doc);
	if (!root || xmlStrcmp(root->name, XML_CHAR ("stickynotes")))
	{
		xmlFreeDoc(doc);
		stickynotes_save();
		return;
	}

	node = root->xmlChildrenNode;

	/* For all children of the root node (ie all sticky notes) */
	new_notes = NULL;
	new_nodes = NULL;
	while (node) {
		if (!xmlStrcmp(node->name, (const xmlChar *) "note"))
		{
			StickyNote *note;

			/* Retrieve and set the window size of the note */
			{
				gchar *w_str = (gchar *)xmlGetProp (node, XML_CHAR ("w"));
				gchar *h_str = (gchar *)xmlGetProp (node, XML_CHAR ("h"));
				if (w_str && h_str)
				{
					w = atoi (w_str);
					h = atoi (h_str);
				}
				else
				{
					w = 0;
					h = 0;
				}

				g_free (w_str);
				g_free (h_str);
			}

			/* Retrieve and set the window position of the note */
			{
				gchar *x_str = (gchar *)xmlGetProp (node, XML_CHAR ("x"));
				gchar *y_str = (gchar *)xmlGetProp (node, XML_CHAR ("y"));

				if (x_str && y_str)
				{
					x = atoi (x_str);
					y = atoi (y_str);
				}
				else
				{
					x = -1;
					y = -1;
				}

				g_free (x_str);
				g_free (y_str);
			}

			/* Create a new note */
			note = stickynote_new_aux (screen, x, y, w, h);
			stickynotes->notes = g_list_append (stickynotes->notes,
					note);
			new_notes = g_list_append (new_notes, note);
			new_nodes = g_list_append (new_nodes, node);

			/* Retrieve and set title of the note */
			{
				gchar *title = (gchar *)xmlGetProp(node, XML_CHAR ("title"));
				if (title)
					stickynote_set_title (note, title);
				g_free (title);
			}

			/* Retrieve and set the color of the note */
			{
				gchar *color_str;
				gchar *font_color_str;

				color_str = (gchar *)xmlGetProp (node, XML_CHAR ("color"));
				font_color_str = (gchar *)xmlGetProp (node, XML_CHAR ("font_color"));

				if (color_str || font_color_str)
					stickynote_set_color (note,
							color_str,
							font_color_str,
							TRUE);
				g_free (color_str);
				g_free (font_color_str);
			}

			/* Retrieve and set the font of the note */
			{
				gchar *font_str = (gchar *)xmlGetProp (node, XML_CHAR ("font"));
				if (font_str)
					stickynote_set_font (note, font_str,
							TRUE);
				g_free (font_str);
			}

			/* Retrieve the workspace */
			{
				char *workspace_str;

				workspace_str = (gchar *)xmlGetProp (node, XML_CHAR ("workspace"));
				if (workspace_str)
				{
					note->workspace = atoi (workspace_str);
					g_free (workspace_str);
				}
			}

			/* Retrieve and set (if any) the body contents of the
			 * note */
			{
				gchar *body = (gchar *)xmlNodeListGetString(doc,
						node->xmlChildrenNode, 1);
				if (body) {
					CtkTextBuffer *buffer;
					CtkTextIter start, end;

					buffer = ctk_text_view_get_buffer(
						CTK_TEXT_VIEW(note->w_body));
					ctk_text_buffer_get_bounds(
							buffer, &start, &end);
					ctk_text_buffer_insert(buffer,
							&start, body, -1);
				}
				g_free(body);
			}

			/* Retrieve and set the locked state of the note,
			 * by default unlocked */
			{
				gchar *locked = (gchar *)xmlGetProp(node, XML_CHAR ("locked"));
				if (locked)
					stickynote_set_locked(note,
						!strcmp(locked, "true"));
				g_free(locked);
			}
		}

		node = node->next;
	}

	tmp1 = new_notes;
	/*
	vnck_screen = vnck_screen_get_default ();
	vnck_screen_force_update (vnck_screen);
	*/

	while (tmp1)
	{
		StickyNote *note = tmp1->data;

		stickynote_set_visible (note, stickynotes->visible);
		tmp1 = tmp1->next;
	}

	g_list_free (new_notes);
	g_list_free (new_nodes);

	xmlFreeDoc(doc);
}
