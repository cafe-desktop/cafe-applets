#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <ctk/ctk.h>
#include <gio/gio.h>
#include <cafe-panel-applet.h>

G_BEGIN_DECLS

#define NCPUSTATES 5
#define NGRAPHS 6

typedef struct _MultiloadApplet MultiloadApplet;
typedef struct _LoadGraph LoadGraph;
typedef void (*LoadGraphDataFunc) (int, int [], LoadGraph *);

#include "netspeed.h"

struct _LoadGraph {
    MultiloadApplet *multiload;

    guint n, id;
    guint speed, size;
    guint orient, pixel_size;
    guint draw_width, draw_height;
    LoadGraphDataFunc get_data;

    guint allocated;

    GdkRGBA *colors;
    gint **data;
    guint data_size;
    guint *pos;

    CtkWidget *main_widget;
    CtkWidget *frame, *box, *disp;
    cairo_surface_t *surface;
    int timer_index;

    gint show_frame;

    long cpu_time [NCPUSTATES];
    long cpu_last [NCPUSTATES];
    int cpu_initialized;

    double loadavg1;
    NetSpeed *netspeed_in;
    NetSpeed *netspeed_out;
    guint net_threshold;
    guint net_threshold1;
    guint net_threshold2;
    guint net_threshold3;

    gboolean visible;
    gboolean tooltip_update;
    const gchar *name;
};

struct _MultiloadApplet
{
	CafePanelApplet *applet;

	GSettings *settings;

	LoadGraph *graphs[NGRAPHS];

	CtkWidget *box;

	gboolean view_cpuload;
	gboolean view_memload;
	gboolean view_netload;
	gboolean view_swapload;
	gboolean view_loadavg;
	gboolean view_diskload;

	CtkWidget *about_dialog;
	CtkWidget *check_boxes[NGRAPHS];
	CtkWidget *prop_dialog;
	CtkWidget *notebook;
	int last_clicked;
};

#include "load-graph.h"
#include "linux-proc.h"

/* show properties dialog */
G_GNUC_INTERNAL void
multiload_properties_cb (CtkAction       *action,
			 MultiloadApplet *ma);

/* remove the old graphs and rebuild them */
G_GNUC_INTERNAL void
multiload_applet_refresh(MultiloadApplet *ma);

/* update the tooltip to the graph's current "used" percentage */
G_GNUC_INTERNAL void
multiload_applet_tooltip_update(LoadGraph *g);

G_END_DECLS

#endif
