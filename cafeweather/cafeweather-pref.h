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

#ifndef __MATEWEATHER_PREF_H_
#define __MATEWEATHER_PREF_H_

#include <gtk/gtk.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "cafeweather.h"

G_BEGIN_DECLS

#define MATEWEATHER_TYPE_PREF		(cafeweather_pref_get_type ())
#define MATEWEATHER_PREF(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MATEWEATHER_TYPE_PREF, CafeWeatherPref))
#define MATEWEATHER_PREF_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MATEWEATHER_TYPE_PREF, CafeWeatherPrefClass))
#define MATEWEATHER_IS_PREF(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATEWEATHER_TYPE_PREF))
#define MATEWEATHER_IS_PREF_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MATEWEATHER_TYPE_PREF))
#define MATEWEATHER_PREF_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MATEWEATHER_TYPE_PREF, CafeWeatherPrefClass))

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

#endif /* __MATEWEATHER_PREF_H */

