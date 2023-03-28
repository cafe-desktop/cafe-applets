/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Main status dialog
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <gio/gio.h>

#define CAFEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "cafeweather.h"
#include "cafeweather-applet.h"
#include "cafeweather-pref.h"
#include "cafeweather-dialog.h"

struct _CafeWeatherDialogPrivate {
	CtkWidget* cond_location;
	CtkWidget* cond_update;
	CtkWidget* cond_cond;
	CtkWidget* cond_sky;
	CtkWidget* cond_temp;
	CtkWidget* cond_dew;
	CtkWidget* cond_humidity;
	CtkWidget* cond_wind;
	CtkWidget* cond_pressure;
	CtkWidget* cond_vis;
	CtkWidget* cond_apparent;
	CtkWidget* cond_sunrise;
	CtkWidget* cond_sunset;
	CtkWidget* cond_image;
	CtkWidget* forecast_text;
	CtkWidget* radar_image;

	CafeWeatherApplet* applet;
};

enum {
	PROP_0,
	PROP_CAFEWEATHER_APPLET,
};

G_DEFINE_TYPE_WITH_PRIVATE (CafeWeatherDialog, cafeweather_dialog, CTK_TYPE_DIALOG);

#define MONOSPACE_FONT_SCHEMA  "org.cafe.interface"
#define MONOSPACE_FONT_KEY     "monospace-font-name"

static void cafeweather_dialog_save_geometry(CafeWeatherDialog* dialog)
{
	GSettings* settings;
	int w, h;

	settings = dialog->priv->applet->settings;

	ctk_window_get_size(CTK_WINDOW(dialog), &w, &h);

#if 0
	/* FIXME those keys are not in org.cafe.weather! */
	g_settings_set_int (settings, "dialog-width", w);
	g_settings_set_int (settings, "dialog-height", h);
#endif
}

static void cafeweather_dialog_load_geometry(CafeWeatherDialog* dialog)
{
	GSettings* settings;
	int w, h;

	settings = dialog->priv->applet->settings;

#if 0
	/* FIXME those keys are not in org.cafe.weather! */
	w = g_settings_get_int (settings, "dialog-width");
	h = g_settings_get_int (settings, "dialog-height");

	if (w > 0 && h > 0)
	{
		ctk_window_resize(CTK_WINDOW(dialog), w, h);
	}
#endif
}

static void response_cb(CafeWeatherDialog* dialog, gint id, gpointer data)
{
    if (id == CTK_RESPONSE_OK) {
	cafeweather_update (dialog->priv->applet);

	cafeweather_dialog_update (dialog);
    } else {
        ctk_widget_destroy (CTK_WIDGET(dialog));
    }
}

static void link_cb(CtkButton* button, gpointer data)
{
    ctk_show_uri_on_window (NULL,
                            "http://www.weather.com/",
                            ctk_get_current_event_time (),
                            NULL);
}

static gchar* replace_multiple_new_lines(gchar* s)
{
	gchar *prev_s = s;
	gint count = 0;
	gint i;

	if (s == NULL) {
		return s;
	}

	for (;*s != '\0';s++) {

		count = 0;

		if (*s == '\n') {
			s++;
			while (*s == '\n' || *s == ' ') {
				count++;
				s++;
			}
		}
		for (i = count; i > 1; i--) {
			*(s - i) = ' ';
		}
	}
	return prev_s;
}

static void cafeweather_dialog_create(CafeWeatherDialog* dialog)
{
  CafeWeatherDialogPrivate *priv;
  CafeWeatherApplet *gw_applet;

  CtkWidget *weather_vbox;
  CtkWidget *weather_notebook;
  CtkWidget *cond_hbox;
  CtkWidget *cond_grid;
  CtkWidget *cond_location_lbl;
  CtkWidget *cond_update_lbl;
  CtkWidget *cond_temp_lbl;
  CtkWidget *cond_cond_lbl;
  CtkWidget *cond_sky_lbl;
  CtkWidget *cond_wind_lbl;
  CtkWidget *cond_humidity_lbl;
  CtkWidget *cond_pressure_lbl;
  CtkWidget *cond_vis_lbl;
  CtkWidget *cond_dew_lbl;
  CtkWidget *cond_apparent_lbl;
  CtkWidget *cond_sunrise_lbl;
  CtkWidget *cond_sunset_lbl;
  CtkWidget *cond_vbox;
  CtkWidget *current_note_lbl;
  CtkWidget *forecast_note_lbl;
  CtkWidget *radar_note_lbl;
  CtkWidget *radar_vbox;
  CtkWidget *radar_link_btn;
  CtkWidget *radar_link_box;
  CtkWidget *forecast_hbox;
  CtkWidget *ebox;
  CtkWidget *scrolled_window;
  CtkWidget *imagescroll_window;

  priv = dialog->priv;
  gw_applet = priv->applet;

  g_object_set (dialog, "destroy-with-parent", TRUE, NULL);
  ctk_window_set_title (CTK_WINDOW (dialog), _("Details"));
  ctk_dialog_add_buttons (CTK_DIALOG (dialog),
  			  _("_Update"), CTK_RESPONSE_OK,
  			  "ctk-close", CTK_RESPONSE_CLOSE,
			  NULL);
  ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_CLOSE);
  ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
  ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), 2);
  ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);

  if (gw_applet->cafeweather_pref.radar_enabled)
      ctk_window_set_default_size (CTK_WINDOW (dialog), 570,440);
  else
      ctk_window_set_default_size (CTK_WINDOW (dialog), 590, 340);

  ctk_window_set_screen (CTK_WINDOW (dialog),
			 ctk_widget_get_screen (CTK_WIDGET (gw_applet->applet)));
  cafeweather_dialog_load_geometry (dialog);

  /* Must come after load geometry, otherwise it will get reset. */
  ctk_window_set_resizable (CTK_WINDOW (dialog), TRUE);

  weather_vbox = ctk_dialog_get_content_area (CTK_DIALOG (dialog));
  ctk_widget_show (weather_vbox);

  weather_notebook = ctk_notebook_new ();
  ctk_container_set_border_width (CTK_CONTAINER (weather_notebook), 5);
  ctk_widget_show (weather_notebook);
  ctk_box_pack_start (CTK_BOX (weather_vbox), weather_notebook, TRUE, TRUE, 0);

  cond_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
  ctk_widget_show (cond_hbox);
  ctk_container_add (CTK_CONTAINER (weather_notebook), cond_hbox);
  ctk_container_set_border_width (CTK_CONTAINER (cond_hbox), 4);

  cond_grid = ctk_grid_new ();
  ctk_widget_show (cond_grid);
  ctk_box_pack_start (CTK_BOX (cond_hbox), cond_grid, TRUE, TRUE, 0);
  ctk_container_set_border_width (CTK_CONTAINER (cond_grid), 12);
  ctk_grid_set_row_spacing (CTK_GRID (cond_grid), 6);
  ctk_grid_set_column_spacing (CTK_GRID (cond_grid), 12);

  cond_location_lbl = ctk_label_new (_("City:"));
  ctk_widget_show (cond_location_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_location_lbl, 0, 0, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_location_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_location_lbl), 0.0);

  cond_update_lbl = ctk_label_new (_("Last update:"));
  ctk_widget_show (cond_update_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_update_lbl, 0, 1, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_update_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_update_lbl), 0.0);

  cond_cond_lbl = ctk_label_new (_("Conditions:"));
  ctk_widget_show (cond_cond_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_cond_lbl, 0, 2, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_cond_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_cond_lbl), 0.0);

  cond_sky_lbl = ctk_label_new (_("Sky:"));
  ctk_widget_show (cond_sky_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_sky_lbl, 0, 3, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_sky_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_sky_lbl), 0.0);

  cond_temp_lbl = ctk_label_new (_("Temperature:"));
  ctk_widget_show (cond_temp_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_temp_lbl, 0, 4, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_temp_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_temp_lbl), 0.0);

  cond_apparent_lbl = ctk_label_new (_("Feels like:"));
  ctk_widget_show (cond_apparent_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_apparent_lbl, 0, 5, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_apparent_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_apparent_lbl), 0.0);

  cond_dew_lbl = ctk_label_new (_("Dew point:"));
  ctk_widget_show (cond_dew_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_dew_lbl, 0, 6, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_dew_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_dew_lbl), 0.0);

  cond_humidity_lbl = ctk_label_new (_("Relative humidity:"));
  ctk_widget_show (cond_humidity_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_humidity_lbl, 0, 7, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_humidity_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_humidity_lbl), 0.0);

  cond_wind_lbl = ctk_label_new (_("Wind:"));
  ctk_widget_show (cond_wind_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_wind_lbl, 0, 8, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_wind_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_wind_lbl), 0.0);

  cond_pressure_lbl = ctk_label_new (_("Pressure:"));
  ctk_widget_show (cond_pressure_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_pressure_lbl, 0, 9, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_pressure_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_pressure_lbl), 0.0);

  cond_vis_lbl = ctk_label_new (_("Visibility:"));
  ctk_widget_show (cond_vis_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_vis_lbl, 0, 10, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_vis_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_vis_lbl), 0.0);

  cond_sunrise_lbl = ctk_label_new (_("Sunrise:"));
  ctk_widget_show (cond_sunrise_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_sunrise_lbl, 0, 11, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_sunrise_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_sunrise_lbl), 0.0);

  cond_sunset_lbl = ctk_label_new (_("Sunset:"));
  ctk_widget_show (cond_sunset_lbl);
  ctk_grid_attach (CTK_GRID (cond_grid), cond_sunset_lbl, 0, 12, 1, 1);
  ctk_label_set_justify (CTK_LABEL (cond_sunset_lbl), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (cond_sunset_lbl), 0.0);

  priv->cond_location = ctk_label_new ("");
  ctk_widget_show (priv->cond_location);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_location, 1, 0, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_location), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_location), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_location), 0.0);

  priv->cond_update = ctk_label_new ("");
  ctk_widget_show (priv->cond_update);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_update, 1, 1, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_update), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_update), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_update), 0.0);

  priv->cond_cond = ctk_label_new ("");
  ctk_widget_show (priv->cond_cond);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_cond, 1, 2, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_cond), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_cond), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_cond), 0.0);

  priv->cond_sky = ctk_label_new ("");
  ctk_widget_show (priv->cond_sky);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_sky, 1, 3, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_sky), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_sky), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_sky), 0.0);

  priv->cond_temp = ctk_label_new ("");
  ctk_widget_show (priv->cond_temp);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_temp, 1, 4, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_temp), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_temp), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_temp), 0.0);

  priv->cond_apparent = ctk_label_new ("");
  ctk_widget_show (priv->cond_apparent);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_apparent, 1, 5, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_apparent), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_apparent), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_apparent), 0.0);

  priv->cond_dew = ctk_label_new ("");
  ctk_widget_show (priv->cond_dew);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_dew, 1, 6, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_dew), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_dew), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_dew), 0.0);

  priv->cond_humidity = ctk_label_new ("");
  ctk_widget_show (priv->cond_humidity);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_humidity, 1, 7, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_humidity), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_humidity), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_humidity), 0.0);

  priv->cond_wind = ctk_label_new ("");
  ctk_widget_show (priv->cond_wind);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_wind, 1, 8, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_wind), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_wind), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_wind), 0.0);

  priv->cond_pressure = ctk_label_new ("");
  ctk_widget_show (priv->cond_pressure);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_pressure, 1, 9, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_pressure), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_pressure), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_pressure), 0.0);

  priv->cond_vis = ctk_label_new ("");
  ctk_widget_show (priv->cond_vis);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_vis, 1, 10, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_vis), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_vis), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_vis), 0.0);

  priv->cond_sunrise = ctk_label_new ("");
  ctk_widget_show (priv->cond_sunrise);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_sunrise, 1, 11, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_sunrise), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_sunrise), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_sunrise), 0.0);

  priv->cond_sunset = ctk_label_new ("");
  ctk_widget_show (priv->cond_sunset);
  ctk_grid_attach (CTK_GRID (cond_grid), priv->cond_sunset, 1, 12, 1, 1);
  ctk_label_set_selectable (CTK_LABEL (priv->cond_sunset), TRUE);
  ctk_label_set_justify (CTK_LABEL (priv->cond_sunset), CTK_JUSTIFY_LEFT);
  ctk_label_set_xalign (CTK_LABEL (priv->cond_sunset), 0.0);

  cond_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
  ctk_widget_set_valign (cond_vbox, CTK_ALIGN_START);
  ctk_widget_set_vexpand (cond_vbox, TRUE);
  ctk_widget_show (cond_vbox);
  ctk_box_pack_end (CTK_BOX (cond_hbox), cond_vbox, FALSE, FALSE, 0);
  ctk_container_set_border_width (CTK_CONTAINER (cond_vbox), 2);

  priv->cond_image = ctk_image_new_from_icon_name ("stock-unknown", CTK_ICON_SIZE_BUTTON);
  ctk_widget_show (priv->cond_image);
  ctk_container_add (CTK_CONTAINER (cond_vbox), priv->cond_image);

  current_note_lbl = ctk_label_new (_("Current Conditions"));
  ctk_widget_show (current_note_lbl);
  ctk_notebook_set_tab_label (CTK_NOTEBOOK (weather_notebook), ctk_notebook_get_nth_page (CTK_NOTEBOOK (weather_notebook), 0), current_note_lbl);

  if (gw_applet->cafeweather_pref.location->zone_valid) {

      forecast_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
      ctk_container_set_border_width (CTK_CONTAINER (forecast_hbox), 12);
      ctk_widget_show (forecast_hbox);

      scrolled_window = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
                                      CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window),
                                           CTK_SHADOW_ETCHED_IN);

      priv->forecast_text = ctk_text_view_new ();
      set_access_namedesc (priv->forecast_text, _("Forecast Report"), _("See the ForeCast Details"));
      ctk_container_add (CTK_CONTAINER (scrolled_window), priv->forecast_text);
      ctk_text_view_set_editable (CTK_TEXT_VIEW (priv->forecast_text), FALSE);
      ctk_text_view_set_left_margin (CTK_TEXT_VIEW (priv->forecast_text), 6);
      ctk_widget_show (priv->forecast_text);
      ctk_widget_show (scrolled_window);
      ctk_box_pack_start (CTK_BOX (forecast_hbox), scrolled_window, TRUE, TRUE, 0);

      ctk_container_add (CTK_CONTAINER (weather_notebook), forecast_hbox);

      forecast_note_lbl = ctk_label_new (_("Forecast"));
      ctk_widget_show (forecast_note_lbl);
      ctk_notebook_set_tab_label (CTK_NOTEBOOK (weather_notebook), ctk_notebook_get_nth_page (CTK_NOTEBOOK (weather_notebook), 1), forecast_note_lbl);

  }

  if (gw_applet->cafeweather_pref.radar_enabled) {

      radar_note_lbl = ctk_label_new_with_mnemonic (_("Radar Map"));
      ctk_widget_show (radar_note_lbl);

      radar_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
      ctk_widget_show (radar_vbox);
      ctk_notebook_append_page (CTK_NOTEBOOK (weather_notebook), radar_vbox, radar_note_lbl);
      ctk_container_set_border_width (CTK_CONTAINER (radar_vbox), 6);

      priv->radar_image = ctk_image_new ();

      imagescroll_window = ctk_scrolled_window_new (NULL, NULL);
      ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (imagescroll_window),
                                 CTK_POLICY_AUTOMATIC,
                                 CTK_POLICY_AUTOMATIC);
      ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (imagescroll_window),
                                      CTK_SHADOW_ETCHED_IN);

      ebox = ctk_event_box_new ();
      ctk_widget_show (ebox);

      ctk_container_add (CTK_CONTAINER (imagescroll_window),ebox);
      ctk_box_pack_start (CTK_BOX (radar_vbox), imagescroll_window, TRUE, TRUE, 0);
      ctk_widget_show (priv->radar_image);
      ctk_widget_show (imagescroll_window);

      ctk_container_add (CTK_CONTAINER (ebox), priv->radar_image);

      radar_link_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
      ctk_widget_set_halign (radar_link_box, CTK_ALIGN_CENTER);
      ctk_widget_set_hexpand (radar_link_box, TRUE);
      ctk_widget_show (radar_link_box);
      ctk_box_pack_start (CTK_BOX (radar_vbox), radar_link_box, FALSE, FALSE, 0);

      radar_link_btn = ctk_button_new_with_mnemonic (_("_Visit Weather.com"));
      set_access_namedesc (radar_link_btn, _("Visit Weather.com"), _("Click to Enter Weather.com"));
      ctk_widget_set_size_request (radar_link_btn, 450, -2);
      ctk_widget_show (radar_link_btn);
      if (!g_settings_get_boolean (gw_applet->settings, "use-custom-radar-url"))
          ctk_container_add (CTK_CONTAINER (radar_link_box), radar_link_btn);

      g_signal_connect (G_OBJECT (radar_link_btn), "clicked",
                        G_CALLBACK (link_cb), NULL);

  }

  g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (response_cb), NULL);

}

static PangoFontDescription* get_system_monospace_font(void)
{
    PangoFontDescription *desc = NULL;
    GSettings *settings;
    char *name;

    settings = g_settings_new (MONOSPACE_FONT_SCHEMA);
    name = g_settings_get_string (settings, MONOSPACE_FONT_KEY);

    if (name) {
    	desc = pango_font_description_from_string (name);
    	g_free (name);
    }

    g_object_unref (settings);

    return desc;
}

static void
override_widget_font (CtkWidget            *widget,
                      PangoFontDescription *font)
{
    static gboolean provider_added = FALSE;
    static CtkCssProvider *provider;
    gchar          *css;
    gchar          *family;
    gchar          *weight;
    const gchar    *style;
    gchar          *size;

    family = g_strdup_printf ("font-family: %s;", pango_font_description_get_family (font));

    weight = g_strdup_printf ("font-weight: %d;", pango_font_description_get_weight (font));

    if (pango_font_description_get_style (font) == PANGO_STYLE_NORMAL)
        style = "font-style: normal;";
    else if (pango_font_description_get_style (font) == PANGO_STYLE_ITALIC)
        style = "font-style: italic;";
    else
        style = "font-style: oblique;";

    size = g_strdup_printf ("font-size: %d%s;",
                            pango_font_description_get_size (font) / PANGO_SCALE,
                            pango_font_description_get_size_is_absolute (font) ? "px" : "pt");
    if (!provider_added)
        provider = ctk_css_provider_new ();

    ctk_widget_set_name(CTK_WIDGET(widget), "CafeWeatherAppletTextView");
    css = g_strdup_printf ("#CafeWeatherAppletTextView { %s %s %s %s }", family, weight, style, size);
    ctk_css_provider_load_from_data (provider, css, -1, NULL);

    if (!provider_added) {
        ctk_style_context_add_provider_for_screen (ctk_widget_get_screen (widget),
                                                   CTK_STYLE_PROVIDER (provider),
                                                   CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        provider_added = TRUE;
    }

    g_free (css);
    g_free (family);
    g_free (weight);
    g_free (size);
}

void cafeweather_dialog_update(CafeWeatherDialog* dialog)
{
    CafeWeatherDialogPrivate *priv;
    CafeWeatherApplet *gw_applet;
    gchar *forecast;
    CtkTextBuffer *buffer;
    PangoFontDescription *font_desc;
    const gchar *icon_name;

    priv = dialog->priv;
    gw_applet = priv->applet;

    /* Check for parallel network update in progress */
    if(gw_applet->cafeweather_info == NULL)
    	return;

    /* Update icon */
    icon_name = weather_info_get_icon_name (gw_applet->cafeweather_info);
    ctk_image_set_from_icon_name (CTK_IMAGE (priv->cond_image),
                                  icon_name, CTK_ICON_SIZE_DIALOG);

    /* Update current condition fields and forecast */
    ctk_label_set_text(CTK_LABEL(priv->cond_location), weather_info_get_location_name(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_update), weather_info_get_update(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_cond), weather_info_get_conditions(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_sky), weather_info_get_sky(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_temp), weather_info_get_temp(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_apparent), weather_info_get_apparent(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_dew), weather_info_get_dew(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_humidity), weather_info_get_humidity(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_wind), weather_info_get_wind(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_pressure), weather_info_get_pressure(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_vis), weather_info_get_visibility(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_sunrise), weather_info_get_sunrise(gw_applet->cafeweather_info));
    ctk_label_set_text(CTK_LABEL(priv->cond_sunset), weather_info_get_sunset(gw_applet->cafeweather_info));

    /* Update forecast */
    if (gw_applet->cafeweather_pref.location->zone_valid) {
	font_desc = get_system_monospace_font ();
	if (font_desc) {
            override_widget_font (priv->forecast_text, font_desc);
            pango_font_description_free (font_desc);
	}

        buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (priv->forecast_text));
        forecast = g_strdup(weather_info_get_forecast(gw_applet->cafeweather_info));
        if (forecast) {
            forecast = g_strstrip(replace_multiple_new_lines(forecast));
            ctk_text_buffer_set_text(buffer, forecast, -1);
            g_free(forecast);
        } else {
            ctk_text_buffer_set_text(buffer, _("Forecast not currently available for this location."), -1);
        }
    }

    /* Update radar map */
    if (gw_applet->cafeweather_pref.radar_enabled) {
        GdkPixbufAnimation *radar;

	radar = weather_info_get_radar (gw_applet->cafeweather_info);
        if (radar) {
            ctk_image_set_from_animation (CTK_IMAGE (priv->radar_image), radar);
        }
    }
}

static void cafeweather_dialog_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    CafeWeatherDialog *dialog = CAFEWEATHER_DIALOG (object);

    switch (prop_id) {
	case PROP_CAFEWEATHER_APPLET:
	    dialog->priv->applet = g_value_get_pointer (value);
	    break;
    }
}

static void cafeweather_dialog_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    CafeWeatherDialog *dialog = CAFEWEATHER_DIALOG (object);

    switch (prop_id) {
	case PROP_CAFEWEATHER_APPLET:
	    g_value_set_pointer (value, dialog->priv->applet);
	    break;
    }
}

static void cafeweather_dialog_init(CafeWeatherDialog* self)
{
    self->priv = cafeweather_dialog_get_instance_private (self);
}

static GObject* cafeweather_dialog_constructor(GType type, guint n_construct_params, GObjectConstructParam* construct_params)
{
    GObject *object;
    CafeWeatherDialog *self;

    object = G_OBJECT_CLASS (cafeweather_dialog_parent_class)->
        constructor (type, n_construct_params, construct_params);
    self = CAFEWEATHER_DIALOG (object);

    cafeweather_dialog_create (self);
    cafeweather_dialog_update (self);

    return object;
}

CtkWidget* cafeweather_dialog_new(CafeWeatherApplet* applet)
{
	return g_object_new(CAFEWEATHER_TYPE_DIALOG,
		"cafeweather-applet", applet,
		NULL);
}

static void cafeweather_dialog_unrealize(CtkWidget* widget)
{
    CafeWeatherDialog* self = CAFEWEATHER_DIALOG(widget);

    cafeweather_dialog_save_geometry(self);

    CTK_WIDGET_CLASS(cafeweather_dialog_parent_class)->unrealize(widget);
}

static void cafeweather_dialog_class_init(CafeWeatherDialogClass* klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

    cafeweather_dialog_parent_class = g_type_class_peek_parent (klass);

    object_class->set_property = cafeweather_dialog_set_property;
    object_class->get_property = cafeweather_dialog_get_property;
    object_class->constructor = cafeweather_dialog_constructor;
    widget_class->unrealize = cafeweather_dialog_unrealize;

    /* This becomes an OBJECT property when CafeWeatherApplet is redone */
    g_object_class_install_property(object_class, PROP_CAFEWEATHER_APPLET, g_param_spec_pointer ("cafeweather-applet", "CafeWeather Applet", "The CafeWeather Applet", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
