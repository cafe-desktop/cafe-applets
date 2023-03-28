#ifndef __CAFEWEATHER_DIALOG_H_
#define __CAFEWEATHER_DIALOG_H_

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

#include <ctk/ctk.h>

#define CAFEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "cafeweather.h"

G_BEGIN_DECLS

#define CAFEWEATHER_TYPE_DIALOG		(cafeweather_dialog_get_type ())
#define CAFEWEATHER_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), CAFEWEATHER_TYPE_DIALOG, CafeWeatherDialog))
#define CAFEWEATHER_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), CAFEWEATHER_TYPE_DIALOG, CafeWeatherDialogClass))
#define CAFEWEATHER_IS_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CAFEWEATHER_TYPE_DIALOG))
#define CAFEWEATHER_IS_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), CAFEWEATHER_TYPE_DIALOG))
#define CAFEWEATHER_DIALOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), CAFEWEATHER_TYPE_DIALOG, CafeWeatherDialogClass))

typedef struct _CafeWeatherDialog CafeWeatherDialog;
typedef struct _CafeWeatherDialogPrivate CafeWeatherDialogPrivate;
typedef struct _CafeWeatherDialogClass CafeWeatherDialogClass;

struct _CafeWeatherDialog
{
	GtkDialog parent;

	/* private */
	CafeWeatherDialogPrivate *priv;
};


struct _CafeWeatherDialogClass
{
	GtkDialogClass parent_class;
};

GType		 cafeweather_dialog_get_type	(void);
GtkWidget	*cafeweather_dialog_new		(CafeWeatherApplet *applet);
void		 cafeweather_dialog_update		(CafeWeatherDialog *dialog);

G_END_DECLS

#endif /* __CAFEWEATHER_DIALOG_H_ */

