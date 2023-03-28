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

	CtkWidget* container;
	CtkWidget* box;
	CtkWidget* label;
	CtkWidget* image;

	CafePanelAppletOrient orient;
	gint size;
	gint timeout_tag;
	gint suncalc_timeout_tag;

	/* preferences  */
	CafeWeatherPrefs cafeweather_pref;

	CtkWidget* pref_dialog;

	/* dialog stuff */
	CtkWidget* details_dialog;
} CafeWeatherApplet;

G_END_DECLS

#endif
