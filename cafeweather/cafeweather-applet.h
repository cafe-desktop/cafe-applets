#ifndef __CAFEWEATHER_APPLET_H_
#define __CAFEWEATHER_APPLET_H_

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

#define CAFEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "cafeweather.h"

G_BEGIN_DECLS

extern void cafeweather_applet_create(CafeWeatherApplet *gw_applet);
extern gint timeout_cb (gpointer data);
extern gint suncalc_timeout_cb (gpointer data);
extern void cafeweather_update (CafeWeatherApplet *applet);

G_END_DECLS

#endif /* __CAFEWEATHER_APPLET_H_ */

