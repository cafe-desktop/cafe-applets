/* -*- Mode: C; c-basic-offset: 4 -*-
 * geyes.c - A cheap xeyes ripoff.
 * Copyright (C) 1999 Dave Camp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>
#include <math.h>
#include <stdlib.h>
#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>
#include "geyes.h"

#define UPDATE_TIMEOUT 100

static gfloat
ctk_align_to_gfloat (CtkAlign align)
{
	switch (align) {
		case CTK_ALIGN_START:
			return 0.0;
		case CTK_ALIGN_END:
			return 1.0;
		case CTK_ALIGN_CENTER:
		case CTK_ALIGN_FILL:
			return 0.5;
		default:
			return 0.0;
	}
}

/* TODO - Optimize this a bit */
static void 
calculate_pupil_xy (EyesApplet *eyes_applet,
		    gint x, gint y,
		    gint *pupil_x, gint *pupil_y, CtkWidget* widget)
{
        CtkAllocation allocation;
        double sina;
        double cosa;
        double h;
        double temp;
 	 double nx, ny;

	 gfloat xalign, yalign;
	 gint width, height;

	 ctk_widget_get_allocation (CTK_WIDGET(widget), &allocation);
	 width = allocation.width;
	 height = allocation.height;
	 xalign = ctk_align_to_gfloat (ctk_widget_get_halign (widget));
	 yalign = ctk_align_to_gfloat (ctk_widget_get_valign (widget));

	 nx = x - MAX(width - eyes_applet->eye_width, 0) * xalign - eyes_applet->eye_width / 2;
	 ny = y - MAX(height- eyes_applet->eye_height, 0) * yalign - eyes_applet->eye_height / 2;
  
	 h = hypot (nx, ny);
        if (h < 0.5 || fabs (h)
            < (fabs (hypot (eyes_applet->eye_height / 2, eyes_applet->eye_width / 2)) - eyes_applet->wall_thickness - eyes_applet->pupil_height)) {
                *pupil_x = nx + eyes_applet->eye_width / 2;
                *pupil_y = ny + eyes_applet->eye_height / 2;
                return;
        }
        
	 sina = nx / h; 
	 cosa = ny / h;
	
        temp = hypot ((eyes_applet->eye_width / 2) * sina, (eyes_applet->eye_height / 2) * cosa);
        temp -= hypot ((eyes_applet->pupil_width / 2) * sina, (eyes_applet->pupil_height / 2) * cosa);
        temp -= hypot ((eyes_applet->wall_thickness / 2) * sina, (eyes_applet->wall_thickness / 2) * cosa);

        *pupil_x = temp * sina + (eyes_applet->eye_width / 2);
        *pupil_y = temp * cosa + (eyes_applet->eye_height / 2);
}

static void 
draw_eye (EyesApplet *eyes_applet,
	  gint eye_num, 
          gint pupil_x, 
          gint pupil_y)
{
	GdkPixbuf *pixbuf;
	CdkRectangle rect, r1, r2;

	pixbuf = gdk_pixbuf_copy (eyes_applet->eye_image);
	r1.x = pupil_x - eyes_applet->pupil_width / 2;
	r1.y = pupil_y - eyes_applet->pupil_height / 2;
	r1.width = eyes_applet->pupil_width;
	r1.height = eyes_applet->pupil_height;
	r2.x = 0;
	r2.y = 0;
	r2.width = eyes_applet->eye_width;
	r2.height = eyes_applet->eye_height;
	cdk_rectangle_intersect (&r1, &r2, &rect);
	gdk_pixbuf_composite (eyes_applet->pupil_image, pixbuf, 
					   rect.x,
					   rect.y,
					   rect.width,
				      	   rect.height,
				      	   pupil_x - eyes_applet->pupil_width / 2,
					   pupil_y - eyes_applet->pupil_height / 2, 1.0, 1.0,
				      	   GDK_INTERP_BILINEAR,
				           255);
	ctk_image_set_from_pixbuf (CTK_IMAGE (eyes_applet->eyes[eye_num]),
						  pixbuf);
	g_object_unref (pixbuf);

}

static gint 
timer_cb (EyesApplet *eyes_applet)
{
        CdkDisplay *display;
        CdkSeat *seat;
        gint x, y;
        gint pupil_x, pupil_y;
        gint i;

        display = ctk_widget_get_display (CTK_WIDGET (eyes_applet->applet));
        seat = cdk_display_get_default_seat (display);

        for (i = 0; i < eyes_applet->num_eyes; i++) {
		if (ctk_widget_get_realized (eyes_applet->eyes[i])) {
            cdk_window_get_device_position (ctk_widget_get_window (eyes_applet->eyes[i]),
                                            cdk_seat_get_pointer (seat),
                                            &x, &y, NULL);

			if ((x != eyes_applet->pointer_last_x[i]) || (y != eyes_applet->pointer_last_y[i])) { 

				calculate_pupil_xy (eyes_applet, x, y, &pupil_x, &pupil_y, eyes_applet->eyes[i]);
				draw_eye (eyes_applet, i, pupil_x, pupil_y);
	    	        
			        eyes_applet->pointer_last_x[i] = x;
			        eyes_applet->pointer_last_y[i] = y;
			}
		}
        }
        return TRUE;
}

static void
about_cb (CtkAction   *action,
	  EyesApplet  *eyes_applet)
{
    static const gchar *authors [] = {
                "Dave Camp <campd@oit.edu>",
                "Pablo Barciela <scow@riseup.net>",
                NULL
	};

	const gchar *documenters[] = {
                "Arjan Scherpenisse <acscherp@wins.uva.nl>",
                "Telsa Gwynne <hobbit@aloss.ukuu.org.uk>",
                N_("Sun GNOME Documentation Team <gdocteam@sun.com>"),
                N_("MATE Documentation Team"),
                N_("CAFE Documentation Team"),
		NULL
	};

#ifdef ENABLE_NLS
	const char **p;
	for (p = documenters; *p; ++p)
		*p = _(*p);
#endif

	ctk_show_about_dialog (NULL,
		"title",              _("About Eyes"),
		"version",            VERSION,
		"comments",           _("A goofy set of eyes for the CAFE "
		                      "panel. They follow your mouse."),
		"copyright",          _("Copyright \xC2\xA9 1999 Dave Camp\n"
		                        "Copyright \xc2\xa9 2012-2020 MATE developers\n"
		                        "Copyright \xc2\xa9 2023-2024 Pablo Barciela"),
		"authors",            authors,
		"documenters",        documenters,
		"translator-credits", _("translator-credits"),
		"logo-icon-name",     "cafe-eyes-applet",
		NULL);
}

static int
properties_load (EyesApplet *eyes_applet)
{
        gchar *theme_path = NULL;

	theme_path = g_settings_get_string (eyes_applet->settings, "theme-path");

	if (theme_path == NULL)
		theme_path = g_strdup (GEYES_THEMES_DIR "Default-tiny");
	
        if (load_theme (eyes_applet, theme_path) == FALSE) {
		g_free (theme_path);

		return FALSE;
	}

        g_free (theme_path);
	
	return TRUE;
}

void
setup_eyes (EyesApplet *eyes_applet) 
{
	int i;

        eyes_applet->hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
        ctk_box_pack_start (CTK_BOX (eyes_applet->vbox), eyes_applet->hbox, TRUE, TRUE, 0);

	eyes_applet->eyes = g_new0 (CtkWidget *, eyes_applet->num_eyes);
	eyes_applet->pointer_last_x = g_new0 (gint, eyes_applet->num_eyes);
	eyes_applet->pointer_last_y = g_new0 (gint, eyes_applet->num_eyes);

        for (i = 0; i < eyes_applet->num_eyes; i++) {
                eyes_applet->eyes[i] = ctk_image_new ();
                if (eyes_applet->eyes[i] == NULL)
                        g_error ("Error creating geyes\n");
               
		ctk_widget_set_size_request (CTK_WIDGET (eyes_applet->eyes[i]),
					     eyes_applet->eye_width,
					     eyes_applet->eye_height);
 
                ctk_widget_show (eyes_applet->eyes[i]);
                
		ctk_box_pack_start (CTK_BOX (eyes_applet->hbox), 
                                    eyes_applet->eyes [i],
                                    TRUE,
                                    TRUE,
                                    0);
                
		if ((eyes_applet->num_eyes != 1) && (i == 0)) {
			ctk_widget_set_halign (eyes_applet->eyes[i], CTK_ALIGN_END);
			ctk_widget_set_valign (eyes_applet->eyes[i], CTK_ALIGN_CENTER);
		}
		else if ((eyes_applet->num_eyes != 1) && (i == eyes_applet->num_eyes - 1)) {
			ctk_widget_set_halign (eyes_applet->eyes[i], CTK_ALIGN_START);
			ctk_widget_set_valign (eyes_applet->eyes[i], CTK_ALIGN_CENTER);
		}
		else {
			ctk_widget_set_halign (eyes_applet->eyes[i], CTK_ALIGN_CENTER);
			ctk_widget_set_valign (eyes_applet->eyes[i], CTK_ALIGN_CENTER);
		}
		
                ctk_widget_realize (eyes_applet->eyes[i]);
		
		eyes_applet->pointer_last_x[i] = G_MAXINT;
		eyes_applet->pointer_last_y[i] = G_MAXINT;
		
		draw_eye (eyes_applet, i,
			  eyes_applet->eye_width / 2,
                          eyes_applet->eye_height / 2);
                
        }
        ctk_widget_show (eyes_applet->hbox);
}

void
destroy_eyes (EyesApplet *eyes_applet)
{
	ctk_widget_destroy (eyes_applet->hbox);
	eyes_applet->hbox = NULL;

	g_free (eyes_applet->eyes);
	g_free (eyes_applet->pointer_last_x);
	g_free (eyes_applet->pointer_last_y);
}

static EyesApplet *
create_eyes (CafePanelApplet *applet)
{
	EyesApplet *eyes_applet = g_new0 (EyesApplet, 1);

        eyes_applet->applet = applet;
        eyes_applet->vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
	eyes_applet->settings = 
		cafe_panel_applet_settings_new (applet, "org.cafe.panel.applet.geyes");

	ctk_container_add (CTK_CONTAINER (applet), eyes_applet->vbox);

	return eyes_applet;
}

static void
dispose_cb (GObject *object, EyesApplet *eyes_applet)
{
	g_return_if_fail (eyes_applet);

	g_source_remove (eyes_applet->timeout_id);
	if (eyes_applet->hbox)
		destroy_eyes (eyes_applet);
	eyes_applet->timeout_id = 0;
	if (eyes_applet->eye_image)
		g_object_unref (eyes_applet->eye_image);
	eyes_applet->eye_image = NULL;
	if (eyes_applet->pupil_image)
		g_object_unref (eyes_applet->pupil_image);
	eyes_applet->pupil_image = NULL;
	if (eyes_applet->theme_dir)
		g_free (eyes_applet->theme_dir);
	eyes_applet->theme_dir = NULL;
	if (eyes_applet->theme_name)
		g_free (eyes_applet->theme_name);
	eyes_applet->theme_name = NULL;
	if (eyes_applet->eye_filename)
		g_free (eyes_applet->eye_filename);
	eyes_applet->eye_filename = NULL;
	if (eyes_applet->pupil_filename)
		g_free (eyes_applet->pupil_filename);
	eyes_applet->pupil_filename = NULL;
	
	if (eyes_applet->prop_box.pbox)
		ctk_widget_destroy (eyes_applet->prop_box.pbox);

	if (eyes_applet->settings)
		g_object_unref (eyes_applet->settings);
	eyes_applet->settings = NULL;

	g_free (eyes_applet);
}

static void
help_cb (CtkAction  *action,
	 EyesApplet *eyes_applet)
{
	GError *error = NULL;

	ctk_show_uri_on_window (NULL,
	                        "help:cafe-geyes",
	                        ctk_get_current_event_time (),
	                        &error);

	if (error) {
		CtkWidget *dialog = ctk_message_dialog_new (NULL, CTK_DIALOG_MODAL, CTK_MESSAGE_ERROR, CTK_BUTTONS_CLOSE, 
							    _("There was an error displaying help: %s"), error->message);
		g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (ctk_widget_destroy), NULL);
		ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);
		ctk_window_set_screen (CTK_WINDOW (dialog), ctk_widget_get_screen (CTK_WIDGET (eyes_applet->applet)));
		ctk_widget_show (dialog);
		g_error_free (error);
		error = NULL;
	}
}


static const CtkActionEntry geyes_applet_menu_actions [] = {
	{ "Props", "document-properties", N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (properties_cb) },
	{ "Help", "help-browser", N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (help_cb) },
	{ "About", "help-about", N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (about_cb) }
};

static void
set_atk_name_description (CtkWidget *widget, const gchar *name,
    const gchar *description)
{
	AtkObject *aobj;
   
	aobj = ctk_widget_get_accessible (widget);
	/* Check if gail is loaded */
	if (CTK_IS_ACCESSIBLE (aobj) == FALSE)
		return;

	atk_object_set_name (aobj, name);
	atk_object_set_description (aobj, description);
}

static gboolean
geyes_applet_fill (CafePanelApplet *applet)
{
	EyesApplet *eyes_applet;
	CtkActionGroup *action_group;
	gchar *ui_path;

	g_set_application_name (_("Eyes"));
	
	ctk_window_set_default_icon_name ("cafe-eyes-applet");

	g_object_set (ctk_settings_get_default (), "ctk-menu-images", TRUE, NULL);
	g_object_set (ctk_settings_get_default (), "ctk-button-images", TRUE, NULL);

	cafe_panel_applet_set_flags (applet, CAFE_PANEL_APPLET_EXPAND_MINOR);
	cafe_panel_applet_set_background_widget (applet, CTK_WIDGET (applet));
	
        eyes_applet = create_eyes (applet);

        eyes_applet->timeout_id = g_timeout_add (
		UPDATE_TIMEOUT, (GSourceFunc) timer_cb, eyes_applet);

	action_group = ctk_action_group_new ("Geyes Applet Actions");
	ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	ctk_action_group_add_actions (action_group,
				      geyes_applet_menu_actions,
				      G_N_ELEMENTS (geyes_applet_menu_actions),
				      eyes_applet);
	ui_path = g_build_filename (GEYES_MENU_UI_DIR, "geyes-applet-menu.xml", NULL);
	cafe_panel_applet_setup_menu_from_file (eyes_applet->applet,
					   ui_path, action_group);
	g_free (ui_path);

	if (cafe_panel_applet_get_locked_down (eyes_applet->applet)) {
		CtkAction *action;

		action = ctk_action_group_get_action (action_group, "Props");
		ctk_action_set_visible (action, FALSE);
	}
	g_object_unref (action_group);

	ctk_widget_set_tooltip_text (CTK_WIDGET (eyes_applet->applet), _("Eyes"));

	set_atk_name_description (CTK_WIDGET (eyes_applet->applet), _("Eyes"), 
			_("The eyes look in the direction of the mouse pointer"));

	g_signal_connect (eyes_applet->vbox,
			  "dispose",
			  G_CALLBACK (dispose_cb),
			  eyes_applet);

	ctk_widget_show_all (CTK_WIDGET (eyes_applet->applet));

	/* setup here and not in create eyes so the destroy signal is set so 
	 * that when there is an error within loading the theme
	 * we can emit this signal */
        if (properties_load (eyes_applet) == FALSE)
		return FALSE;

	setup_eyes (eyes_applet);

	return TRUE;
}

static gboolean
geyes_applet_factory (CafePanelApplet *applet,
		      const gchar *iid,
		      gpointer     data)
{
	gboolean retval = FALSE;

	theme_dirs_create ();

	if (!strcmp (iid, "GeyesApplet"))
		retval = geyes_applet_fill (applet); 
   
	if (retval == FALSE) {
		exit (-1);
	}

	return retval;
}

CAFE_PANEL_APPLET_OUT_PROCESS_FACTORY ("GeyesAppletFactory",
				  PANEL_TYPE_APPLET,
				  "geyes",
				  geyes_applet_factory,
				  NULL)
