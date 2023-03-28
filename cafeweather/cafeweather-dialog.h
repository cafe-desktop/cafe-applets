#ifndef __MATEWEATHER_DIALOG_H_
#define __MATEWEATHER_DIALOG_H_

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

#include <gtk/gtk.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "cafeweather.h"

G_BEGIN_DECLS

#define MATEWEATHER_TYPE_DIALOG		(cafeweather_dialog_get_type ())
#define MATEWEATHER_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MATEWEATHER_TYPE_DIALOG, CafeWeatherDialog))
#define MATEWEATHER_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MATEWEATHER_TYPE_DIALOG, CafeWeatherDialogClass))
#define MATEWEATHER_IS_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATEWEATHER_TYPE_DIALOG))
#define MATEWEATHER_IS_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MATEWEATHER_TYPE_DIALOG))
#define MATEWEATHER_DIALOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MATEWEATHER_TYPE_DIALOG, CafeWeatherDialogClass))

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

#endif /* __MATEWEATHER_DIALOG_H_ */

