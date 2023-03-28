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
#include <ctk/ctk.h>
#include <gio/gio.h>
#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>

#define CAFEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include <libcafeweather/cafeweather-prefs.h>

#include "cafeweather.h"
#include "cafeweather-pref.h"
#include "cafeweather-dialog.h"
#include "cafeweather-applet.h"


static gboolean cafeweather_applet_new(CafePanelApplet* applet, const gchar* iid, gpointer data)
{
	CafeWeatherApplet* gw_applet;

	gw_applet = g_new0(CafeWeatherApplet, 1);

	gw_applet->applet = applet;
	gw_applet->cafeweather_info = NULL;
	gw_applet->settings = cafe_panel_applet_settings_new (applet, "org.cafe.weather");

	cafeweather_applet_create(gw_applet);

	cafeweather_prefs_load(&gw_applet->cafeweather_pref, gw_applet->settings);

	cafeweather_update(gw_applet);

	return TRUE;
}

static gboolean cafeweather_applet_factory(CafePanelApplet* applet, const gchar* iid, gpointer data)
{
	gboolean retval = FALSE;

	retval = cafeweather_applet_new(applet, iid, data);

	return retval;
}

CAFE_PANEL_APPLET_OUT_PROCESS_FACTORY("CafeWeatherAppletFactory", PANEL_TYPE_APPLET, "cafeweather", cafeweather_applet_factory, NULL)
