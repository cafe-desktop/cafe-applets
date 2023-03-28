/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Preferences dialog
 *
 */

#ifndef __CAFEWEATHER_PREF_H_
#define __CAFEWEATHER_PREF_H_

#include <gtk/gtk.h>

#define CAFEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "cafeweather.h"

G_BEGIN_DECLS

#define CAFEWEATHER_TYPE_PREF		(cafeweather_pref_get_type ())
#define CAFEWEATHER_PREF(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFEWEATHER_TYPE_PREF, CafeWeatherPref))
#define CAFEWEATHER_PREF_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CAFEWEATHER_TYPE_PREF, CafeWeatherPrefClass))
#define CAFEWEATHER_IS_PREF(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFEWEATHER_TYPE_PREF))
#define CAFEWEATHER_IS_PREF_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CAFEWEATHER_TYPE_PREF))
#define CAFEWEATHER_PREF_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CAFEWEATHER_TYPE_PREF, CafeWeatherPrefClass))

typedef struct _CafeWeatherPref CafeWeatherPref;
typedef struct _CafeWeatherPrefPrivate CafeWeatherPrefPrivate;
typedef struct _CafeWeatherPrefClass CafeWeatherPrefClass;

struct _CafeWeatherPref
{
	GtkDialog parent;

	/* private */
	CafeWeatherPrefPrivate *priv;
};


struct _CafeWeatherPrefClass
{
	GtkDialogClass parent_class;
};

GType		 cafeweather_pref_get_type	(void);
GtkWidget	*cafeweather_pref_new	(CafeWeatherApplet *applet);


void set_access_namedesc (GtkWidget *widget, const gchar *name, const gchar *desc);


G_END_DECLS

#endif /* __CAFEWEATHER_PREF_H */

