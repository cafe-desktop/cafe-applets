/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Main applet widget
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <gio/gio.h>
#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>

#include <gdk/gdkkeysyms.h>

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#define CAFEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "cafeweather.h"
#include "cafeweather-about.h"
#include "cafeweather-pref.h"
#include "cafeweather-dialog.h"
#include "cafeweather-applet.h"

#define MAX_CONSECUTIVE_FAULTS (3)

static void about_cb (GtkAction      *action,
		      CafeWeatherApplet *gw_applet)
{

    cafeweather_about_run (gw_applet);
}

static void help_cb (GtkAction      *action,
		     CafeWeatherApplet *gw_applet)
{
    GError *error = NULL;

    ctk_show_uri_on_window (NULL,
                            "help:cafeweather",
                            ctk_get_current_event_time (),
                            &error);

    if (error) { 
	GtkWidget *dialog = ctk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						    _("There was an error displaying help: %s"), error->message);
	g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (ctk_widget_destroy), NULL);
	ctk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	ctk_window_set_screen (GTK_WINDOW (dialog), ctk_widget_get_screen (GTK_WIDGET (gw_applet->applet)));
	ctk_widget_show (dialog);
        g_error_free (error);
        error = NULL;
    }
}

static void pref_cb (GtkAction      *action,
		     CafeWeatherApplet *gw_applet)
{
   if (gw_applet->pref_dialog) {
	ctk_window_present (GTK_WINDOW (gw_applet->pref_dialog));
   } else {
	gw_applet->pref_dialog = cafeweather_pref_new(gw_applet);
	g_object_add_weak_pointer(G_OBJECT(gw_applet->pref_dialog),
				  (gpointer *)&(gw_applet->pref_dialog));
	ctk_widget_show_all (gw_applet->pref_dialog);
   }
}

static void details_cb (GtkAction      *action,
			CafeWeatherApplet *gw_applet)
{
   if (gw_applet->details_dialog) {
	ctk_window_present (GTK_WINDOW (gw_applet->details_dialog));
   } else {
	gw_applet->details_dialog = cafeweather_dialog_new(gw_applet);
	g_object_add_weak_pointer(G_OBJECT(gw_applet->details_dialog),
				  (gpointer *)&(gw_applet->details_dialog));
	cafeweather_dialog_update (CAFEWEATHER_DIALOG (gw_applet->details_dialog));
	ctk_widget_show (gw_applet->details_dialog);
   }
}

static void update_cb (GtkAction      *action,
		       CafeWeatherApplet *gw_applet)
{
    cafeweather_update (gw_applet);
}


static const GtkActionEntry weather_applet_menu_actions [] = {
	{ "Details", NULL, N_("_Details"),
	  NULL, NULL,
	  G_CALLBACK (details_cb) },
	{ "Update", "view-refresh", N_("_Update"),
	  NULL, NULL,
	  G_CALLBACK (update_cb) },
	{ "Props", "document-properties", N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (pref_cb) },
	{ "Help", "help-browser", N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (help_cb) },
	{ "About", "help-about", N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (about_cb) }
};

static void place_widgets (CafeWeatherApplet *gw_applet)
{
    GtkRequisition req;
    int total_size = 0;
    gboolean horizontal = FALSE;
    int panel_size = gw_applet->size;
    const gchar *temp;   
    const gchar *icon_name;
	
    switch (gw_applet->orient) {
	case CAFE_PANEL_APPLET_ORIENT_LEFT:
	case CAFE_PANEL_APPLET_ORIENT_RIGHT:
	    horizontal = FALSE;
	    break;
	case CAFE_PANEL_APPLET_ORIENT_UP:
	case CAFE_PANEL_APPLET_ORIENT_DOWN:
	    horizontal = TRUE;
	    break;
    }

    /* Create the weather icon */
    icon_name = weather_info_get_icon_name (gw_applet->cafeweather_info);
    gw_applet->image = ctk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON); 

    if (icon_name != NULL) {
        ctk_widget_show (gw_applet->image);
        ctk_widget_get_preferred_size (gw_applet->image, &req, NULL);
        if (horizontal)
            total_size += req.height;
        else
            total_size += req.width;
    }

    /* Create the temperature label */
    gw_applet->label = ctk_label_new("0\302\260F");
    
    /* Update temperature text */
    temp = weather_info_get_temp_summary(gw_applet->cafeweather_info);
    if (temp) 
        ctk_label_set_text(GTK_LABEL(gw_applet->label), temp);

    /* Check the label size to determine box layout */
    ctk_widget_show (gw_applet->label);
    ctk_widget_get_preferred_size (gw_applet->label, &req, NULL);
    if (horizontal)
        total_size += req.height;
    else
        total_size += req.width;

    /* Pack the box */
    if (gw_applet->box)
        ctk_widget_destroy (gw_applet->box);
    
    if (horizontal && (total_size <= panel_size))
        gw_applet->box = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    else if (horizontal && (total_size > panel_size))
        gw_applet->box = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    else if (!horizontal && (total_size <= panel_size))
        gw_applet->box = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    else 
        gw_applet->box = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    /* better for vertical panels */
    if (horizontal)
        ctk_widget_set_valign (gw_applet->box, GTK_ALIGN_CENTER);
    else
        ctk_widget_set_halign (gw_applet->box, GTK_ALIGN_CENTER);

    /* Rebuild the applet it's visual area */
    ctk_container_add (GTK_CONTAINER (gw_applet->applet), gw_applet->box);
    ctk_box_pack_start (GTK_BOX (gw_applet->box), gw_applet->image, TRUE, TRUE, 0);
    ctk_box_pack_start (GTK_BOX (gw_applet->box), gw_applet->label, TRUE, TRUE, 0);

    ctk_widget_show_all (GTK_WIDGET (gw_applet->applet));
}

static void change_orient_cb (CafePanelApplet *w, CafePanelAppletOrient o, gpointer data)
{
    CafeWeatherApplet *gw_applet = (CafeWeatherApplet *)data;
	
    gw_applet->orient = o;
    place_widgets(gw_applet);
    return;
}

static void size_allocate_cb(CafePanelApplet *w, GtkAllocation *allocation, gpointer data)
{
    CafeWeatherApplet *gw_applet = (CafeWeatherApplet *)data;
	
    if ((gw_applet->orient == CAFE_PANEL_APPLET_ORIENT_LEFT) || (gw_applet->orient == CAFE_PANEL_APPLET_ORIENT_RIGHT)) {
      if (gw_applet->size == allocation->width)
	return;
      gw_applet->size = allocation->width;
    } else {
      if (gw_applet->size == allocation->height)
	return;
      gw_applet->size = allocation->height;
    }
	
    place_widgets(gw_applet);
    return;
}

static gboolean clicked_cb (GtkWidget *widget, GdkEventButton *ev, gpointer data)
{
    CafeWeatherApplet *gw_applet = data;

    if ((ev == NULL) || (ev->button != 1))
        return FALSE;

    if (ev->type == GDK_BUTTON_PRESS) {
	if (!gw_applet->details_dialog)
		details_cb (NULL, gw_applet);
	else
		ctk_widget_destroy (GTK_WIDGET (gw_applet->details_dialog));
	
	return TRUE;
    }
    
    return FALSE;
}

static gboolean 
key_press_cb (GtkWidget *widget, GdkEventKey *event, CafeWeatherApplet *gw_applet)
{
	switch (event->keyval) {	
	case GDK_KEY_u:
		if (event->state == GDK_CONTROL_MASK) {
			cafeweather_update (gw_applet);
			return TRUE;
		}
		break;
	case GDK_KEY_d:
		if (event->state == GDK_CONTROL_MASK) {
			details_cb (NULL, gw_applet);
			return TRUE;
		}
		break;		
	case GDK_KEY_KP_Enter:
	case GDK_KEY_ISO_Enter:
	case GDK_KEY_3270_Enter:
	case GDK_KEY_Return:
	case GDK_KEY_space:
	case GDK_KEY_KP_Space:
		details_cb (NULL, gw_applet);
		return TRUE;
	default:
		break;
	}

	return FALSE;

}

static void
network_changed (GNetworkMonitor *monitor, gboolean available, CafeWeatherApplet *gw_applet)
{
    if (available) {
        cafeweather_update (gw_applet);
    }
}


static void
applet_destroy (GtkWidget *widget, CafeWeatherApplet *gw_applet)
{
    GNetworkMonitor *monitor;

    if (gw_applet->pref_dialog)
       ctk_widget_destroy (gw_applet->pref_dialog);

    if (gw_applet->details_dialog)
       ctk_widget_destroy (gw_applet->details_dialog);

    if (gw_applet->timeout_tag > 0) {
       g_source_remove(gw_applet->timeout_tag);
       gw_applet->timeout_tag = 0;
    }
	
    if (gw_applet->suncalc_timeout_tag > 0) {
       g_source_remove(gw_applet->suncalc_timeout_tag);
       gw_applet->suncalc_timeout_tag = 0;
    }
	
    if (gw_applet->settings) {
       g_object_unref (gw_applet->settings);
       gw_applet->settings = NULL;
    }

    monitor = g_network_monitor_get_default ();
    g_signal_handlers_disconnect_by_func (monitor,
                                          G_CALLBACK (network_changed),
                                          gw_applet);

    weather_info_abort (gw_applet->cafeweather_info);
}

void cafeweather_applet_create (CafeWeatherApplet *gw_applet)
{
    GtkActionGroup *action_group;
    gchar          *ui_path;
    AtkObject      *atk_obj;
    GNetworkMonitor*monitor;

    gw_applet->cafeweather_pref.location = NULL;
    gw_applet->cafeweather_pref.show_notifications = FALSE;
    gw_applet->cafeweather_pref.update_interval = 1800;
    gw_applet->cafeweather_pref.update_enabled = TRUE;
    gw_applet->cafeweather_pref.detailed = FALSE;
    gw_applet->cafeweather_pref.radar_enabled = TRUE;
    gw_applet->cafeweather_pref.temperature_unit = TEMP_UNIT_INVALID;
    gw_applet->cafeweather_pref.speed_unit = SPEED_UNIT_INVALID;
    gw_applet->cafeweather_pref.pressure_unit = PRESSURE_UNIT_INVALID;
    gw_applet->cafeweather_pref.distance_unit = DISTANCE_UNIT_INVALID;
    
    cafe_panel_applet_set_flags (gw_applet->applet, CAFE_PANEL_APPLET_EXPAND_MINOR);

    cafe_panel_applet_set_background_widget(gw_applet->applet,
                                       GTK_WIDGET(gw_applet->applet));

    g_set_application_name (_("Weather Report"));

    ctk_window_set_default_icon_name ("weather-storm");

    g_signal_connect (G_OBJECT(gw_applet->applet), "change_orient",
                       G_CALLBACK(change_orient_cb), gw_applet);
    g_signal_connect (G_OBJECT(gw_applet->applet), "size_allocate",
                       G_CALLBACK(size_allocate_cb), gw_applet);
    g_signal_connect (G_OBJECT(gw_applet->applet), "destroy", 
                       G_CALLBACK (applet_destroy), gw_applet);
    g_signal_connect (G_OBJECT(gw_applet->applet), "button_press_event",
                       G_CALLBACK(clicked_cb), gw_applet);
    g_signal_connect (G_OBJECT(gw_applet->applet), "key_press_event",           
			G_CALLBACK(key_press_cb), gw_applet);
                     
    ctk_widget_set_tooltip_text (GTK_WIDGET(gw_applet->applet), _("CAFE Weather"));

    atk_obj = ctk_widget_get_accessible (GTK_WIDGET (gw_applet->applet));
    if (GTK_IS_ACCESSIBLE (atk_obj))
	   atk_object_set_name (atk_obj, _("CAFE Weather"));

    gw_applet->size = cafe_panel_applet_get_size (gw_applet->applet);

    gw_applet->orient = cafe_panel_applet_get_orient (gw_applet->applet);

    action_group = ctk_action_group_new ("CafeWeather Applet Actions");
    ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    ctk_action_group_add_actions (action_group,
				  weather_applet_menu_actions,
				  G_N_ELEMENTS (weather_applet_menu_actions),
				  gw_applet);
    ui_path = g_build_filename (CAFEWEATHER_MENU_UI_DIR, "cafeweather-applet-menu.xml", NULL);
    cafe_panel_applet_setup_menu_from_file (gw_applet->applet,
				       ui_path, action_group);
    g_free (ui_path);

    if (cafe_panel_applet_get_locked_down (gw_applet->applet)) {
	    GtkAction *action;

	    action = ctk_action_group_get_action (action_group, "Props");
	    ctk_action_set_visible (action, FALSE);
    }
    g_object_unref (action_group);
	
    place_widgets(gw_applet);        

    monitor = g_network_monitor_get_default();
    g_signal_connect (monitor, "network-changed",
                      G_CALLBACK (network_changed), gw_applet);}

gint timeout_cb (gpointer data)
{
    CafeWeatherApplet *gw_applet = (CafeWeatherApplet *)data;
	
    cafeweather_update(gw_applet);
    return 0;  /* Do not repeat timeout (will be reset by cafeweather_update) */
}

static void
update_finish (WeatherInfo *info, gpointer data)
{
    static int gw_fault_counter = 0;
#ifdef HAVE_LIBNOTIFY
    char *message, *detail;
#endif
    char *s;
    CafeWeatherApplet *gw_applet = (CafeWeatherApplet *)data;
    gint nxtSunEvent;
    const gchar *icon_name;

    /* Update timer */
    if (gw_applet->timeout_tag > 0)
        g_source_remove(gw_applet->timeout_tag);
    if (gw_applet->cafeweather_pref.update_enabled)
    {
	gw_applet->timeout_tag =
		g_timeout_add_seconds (
                       gw_applet->cafeweather_pref.update_interval,
                        timeout_cb, gw_applet);

        nxtSunEvent = weather_info_next_sun_event(gw_applet->cafeweather_info);
        if (nxtSunEvent >= 0)
            gw_applet->suncalc_timeout_tag =
                        g_timeout_add_seconds (nxtSunEvent,
                                suncalc_timeout_cb, gw_applet);
    }

    if ((TRUE == weather_info_is_valid (info)) ||
	     (gw_fault_counter >= MAX_CONSECUTIVE_FAULTS))
    {
	    gw_fault_counter = 0;
            icon_name = weather_info_get_icon_name (gw_applet->cafeweather_info);
            ctk_image_set_from_icon_name (GTK_IMAGE(gw_applet->image), 
                                          icon_name, GTK_ICON_SIZE_BUTTON);
	      
	    ctk_label_set_text (GTK_LABEL (gw_applet->label), 
	        		weather_info_get_temp_summary(
					gw_applet->cafeweather_info));
	    
	    s = weather_info_get_weather_summary (gw_applet->cafeweather_info);
	    ctk_widget_set_tooltip_text (GTK_WIDGET (gw_applet->applet), s);
	    g_free (s);

	    /* Update dialog -- if one is present */
	    if (gw_applet->details_dialog) {
	    	cafeweather_dialog_update (CAFEWEATHER_DIALOG (gw_applet->details_dialog));
	    }

	    /* update applet */
	    place_widgets(gw_applet);

#ifdef HAVE_LIBNOTIFY
        if (gw_applet->cafeweather_pref.show_notifications)
        {
		    NotifyNotification *n;
	            
		    /* Show notifications if possible */
	            if (!notify_is_initted ())
	                notify_init (_("Weather Forecast"));

		    if (notify_is_initted ())
		    {
			 GError *error = NULL;
                         const char *icon;
			 
	           	 /* Show notification */
	           	 message = g_strdup_printf ("%s: %s",
					 weather_info_get_location_name (info),
					 weather_info_get_sky (info));
	           	 detail = g_strdup_printf (
					 _("City: %s\nSky: %s\nTemperature: %s"),
					 weather_info_get_location_name (info),
					 weather_info_get_sky (info),
					 weather_info_get_temp_summary (info));

			 icon = weather_info_get_icon_name (gw_applet->cafeweather_info);
			 if (icon == NULL)
				 icon = "stock-unknown";
	           	 
			 n = notify_notification_new (message, detail, icon);
	
		   	 notify_notification_show (n, &error);
			 if (error)
			 {
				 g_warning ("%s", error->message);
				 g_error_free (error);
			 }
		   	     
		   	 g_free (message);
		   	 g_free (detail);
		    }
        }
#endif
    }
    else
    {
	    /* there has been an error during retrival
	     * just update the fault counter
	     */
	    gw_fault_counter++;
    }
}

gint suncalc_timeout_cb (gpointer data)
{
    WeatherInfo *info = ((CafeWeatherApplet *)data)->cafeweather_info;
    update_finish(info, data);
    return 0;  /* Do not repeat timeout (will be reset by update_finish) */
}


void cafeweather_update (CafeWeatherApplet *gw_applet)
{
    WeatherPrefs prefs;
    const gchar *icon_name;

    icon_name = weather_info_get_icon_name(gw_applet->cafeweather_info);
    ctk_image_set_from_icon_name (GTK_IMAGE (gw_applet->image), 
    			          icon_name, GTK_ICON_SIZE_BUTTON); 
    ctk_widget_set_tooltip_text (GTK_WIDGET(gw_applet->applet),  _("Updating..."));

    /* Set preferred forecast type */
    prefs.type = gw_applet->cafeweather_pref.detailed ? FORECAST_ZONE : FORECAST_STATE;

    /* Set radar map retrieval option */
    prefs.radar = gw_applet->cafeweather_pref.radar_enabled;
    prefs.radar_custom_url = (gw_applet->cafeweather_pref.use_custom_radar_url &&
    				gw_applet->cafeweather_pref.radar) ?
				gw_applet->cafeweather_pref.radar : NULL;

    /* Set the units */
    prefs.temperature_unit = gw_applet->cafeweather_pref.temperature_unit;
    prefs.speed_unit = gw_applet->cafeweather_pref.speed_unit;
    prefs.pressure_unit = gw_applet->cafeweather_pref.pressure_unit;
    prefs.distance_unit = gw_applet->cafeweather_pref.distance_unit;

    /* Update current conditions */
    if (gw_applet->cafeweather_info && 
    	weather_location_equal(weather_info_get_location(gw_applet->cafeweather_info),
    			       gw_applet->cafeweather_pref.location)) {
	weather_info_update(gw_applet->cafeweather_info, &prefs,
			    update_finish, gw_applet);
    } else {
        weather_info_free(gw_applet->cafeweather_info);
        gw_applet->cafeweather_info = weather_info_new(gw_applet->cafeweather_pref.location,
						    &prefs,
						    update_finish, gw_applet);
    }
}
