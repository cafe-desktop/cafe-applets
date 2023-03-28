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

#include <glib.h>
#include <config.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include <libcafeweather/cafeweather-prefs.h>

#include "cafeweather.h"
#include "cafeweather-pref.h"
#include "cafeweather-dialog.h"
#include "cafeweather-applet.h"


static gboolean cafeweather_applet_new(MatePanelApplet* applet, const gchar* iid, gpointer data)
{
	MateWeatherApplet* gw_applet;

	gw_applet = g_new0(MateWeatherApplet, 1);

	gw_applet->applet = applet;
	gw_applet->cafeweather_info = NULL;
	gw_applet->settings = cafe_panel_applet_settings_new (applet, "org.cafe.weather");

	cafeweather_applet_create(gw_applet);

	cafeweather_prefs_load(&gw_applet->cafeweather_pref, gw_applet->settings);

	cafeweather_update(gw_applet);

	return TRUE;
}

static gboolean cafeweather_applet_factory(MatePanelApplet* applet, const gchar* iid, gpointer data)
{
	gboolean retval = FALSE;

	retval = cafeweather_applet_new(applet, iid, data);

	return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY("MateWeatherAppletFactory", PANEL_TYPE_APPLET, "cafeweather", cafeweather_applet_factory, NULL)
