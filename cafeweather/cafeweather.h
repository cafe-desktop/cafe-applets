#ifndef __CAFEWEATHER_H__
#define __CAFEWEATHER_H__

/*
 *  todd kulesza <fflewddur@dropline.net>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 * main header file
 *
 */
#include <glib/gi18n.h>

#include <gio/gio.h>

#include <cafe-panel-applet.h>

#include <libcafeweather/cafeweather-prefs.h>


/* Radar map on by default. */
#define RADARMAP

G_BEGIN_DECLS

typedef struct _CafeWeatherApplet {
	CafePanelApplet* applet;
	WeatherInfo* cafeweather_info;

	GSettings* settings;

	GtkWidget* container;
	GtkWidget* box;
	GtkWidget* label;
	GtkWidget* image;

	CafePanelAppletOrient orient;
	gint size;
	gint timeout_tag;
	gint suncalc_timeout_tag;

	/* preferences  */
	CafeWeatherPrefs cafeweather_pref;

	GtkWidget* pref_dialog;

	/* dialog stuff */
	GtkWidget* details_dialog;
} CafeWeatherApplet;

G_END_DECLS

#endif
