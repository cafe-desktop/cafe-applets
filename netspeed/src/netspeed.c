 /*  netspeed.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Netspeed Applet was writen by Jörgen Scheibengruber <mfcn@gmx.de>
 *
 *  CAFE Netspeed Applet migrated by Stefano Karapetsas <stefano@karapetsas.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <ctk/ctk.h>
#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "backend.h"

 /* Icons for the interfaces */
static const char* const dev_type_icon[DEV_UNKNOWN + 1] = {
	/* FIXME: Need an actual loopback icon... */
	"reload",                    /* DEV_LO */
	"network-wired",             /* DEV_ETHERNET */
	"network-wireless",          /* DEV_WIRELESS */
	"modem",                     /* DEV_PPP */
	"cafe-netspeed-plip",        /* DEV_PLIP */
	"cafe-netspeed-plip",        /* DEV_SLIP */
	"network-workgroup",         /* DEV_UNKNOWN */
};

static const char* wireless_quality_icon[] = {
	"cafe-netspeed-wireless-25",
	"cafe-netspeed-wireless-50",
	"cafe-netspeed-wireless-75",
	"cafe-netspeed-wireless-100"
};

static const char IN_ICON[] = "go-down";
static const char OUT_ICON[] = "go-up";
static const char ERROR_ICON[] = "ctk-dialog-error";
static const char LOGO_ICON[] = "cafe-netspeed-applet";

/* How many old in out values do we store?
 * The value actually shown in the applet is the average
 * of these values -> prevents the value from
 * "jumping around like crazy"
 */
#define OLD_VALUES 5
#define GRAPH_VALUES 180
#define GRAPH_LINES 4

/* A struct containing all the "global" data of the
 * applet
 */
typedef struct
{
	CafePanelApplet *applet;
	CtkWidget *box, *pix_box, *speed_box,
	*in_box, *in_label, *in_pix,
	*out_box, *out_label, *out_pix,
	*sum_box, *sum_label, *dev_pix, *qual_pix;
	cairo_surface_t *qual_surfaces[4];

	CtkWidget *signalbar;

	gboolean labels_dont_shrink;

	DevInfo devinfo;
	gboolean device_has_changed;

	guint timeout_id;
	int refresh_time;
	char *up_cmd, *down_cmd;
	gboolean show_sum, show_bits;
	gboolean change_icon, auto_change_device;
	gboolean show_icon, short_unit;
	gboolean show_quality_icon;
	CdkRGBA         in_color;
	CdkRGBA         out_color;
	int width;

	CtkWidget *inbytes_text, *outbytes_text;
	CtkDialog *details, *settings;
	CtkDrawingArea *drawingarea;
	CtkWidget *network_device_combo;

	guint index_old;
	guint64 in_old[OLD_VALUES], out_old[OLD_VALUES];
	double max_graph, in_graph[GRAPH_VALUES], out_graph[GRAPH_VALUES];
	int index_graph;

	CtkWidget *connect_dialog;

	gboolean show_tooltip;

	GSettings *gsettings;
} CafeNetspeedApplet;

static void
update_tooltip(CafeNetspeedApplet* applet);

static void
device_change_cb(CtkComboBox *combo, CafeNetspeedApplet *applet);

/* Adds a Pango markup "size" to a bytestring
 */
static void
add_markup_size(char **string, int size)
{
	char *tmp = *string;
	*string = g_strdup_printf("<span size=\"%d\">%s</span>", size * 1000, tmp);
	g_free(tmp);
}

/* Adds a Pango markup "foreground" to a bytestring
 */
static void
add_markup_fgcolor(char **string, const char *color)
{
	char *tmp = *string;
	*string = g_strdup_printf("<span foreground=\"%s\">%s</span>", color, tmp);
	g_free(tmp);
}

/* Change the icons according to the selected device
 */
static void
change_icons(CafeNetspeedApplet *applet)
{
	cairo_surface_t *dev, *down;
	cairo_surface_t *in_arrow, *out_arrow;
	CtkIconTheme *icon_theme;
	gint icon_scale;

	gint icon_size = cafe_panel_applet_get_size (CAFE_PANEL_APPLET (applet->applet)) - 8;
	/* FIXME: Not all network icons include a high enough resolution, so to make them all
	 * consistent, we cap them at 48px.*/
	icon_size = CLAMP (icon_size, 16, 48);

	icon_theme = ctk_icon_theme_get_default();
	icon_scale = ctk_widget_get_scale_factor (CTK_WIDGET (applet->applet));

	/* If the user wants a different icon than current, we load it */
	if (applet->show_icon && applet->change_icon) {
		dev = ctk_icon_theme_load_surface(icon_theme,
			dev_type_icon[applet->devinfo.type],
			icon_size, icon_scale, NULL, 0, NULL);
	} else {
			dev = ctk_icon_theme_load_surface(icon_theme,
			dev_type_icon[DEV_UNKNOWN],
			icon_size, icon_scale, NULL, 0, NULL);
	}

	/* We need a fallback */
	if (dev == NULL)
		dev = ctk_icon_theme_load_surface(icon_theme,
			dev_type_icon[DEV_UNKNOWN],
			icon_size, icon_scale, NULL, 0, NULL);

	in_arrow = ctk_icon_theme_load_surface(icon_theme, IN_ICON, 16, icon_scale, NULL, 0, NULL);
	out_arrow = ctk_icon_theme_load_surface(icon_theme, OUT_ICON, 16, icon_scale, NULL, 0, NULL);

	/* Set the windowmanager icon for the applet */
	ctk_window_set_default_icon_name(LOGO_ICON);

	ctk_image_set_from_surface(CTK_IMAGE(applet->out_pix), out_arrow);
	ctk_image_set_from_surface(CTK_IMAGE(applet->in_pix), in_arrow);
	cairo_surface_destroy(in_arrow);
	cairo_surface_destroy(out_arrow);

	if (applet->devinfo.running) {
		ctk_widget_show(applet->in_box);
		ctk_widget_show(applet->out_box);
	} else {
		cairo_t *cr;
		cairo_surface_t *copy;
		gint down_coords;

		ctk_widget_hide(applet->in_box);
		ctk_widget_hide(applet->out_box);

		/* We're not allowed to modify "dev" */
		copy = cairo_surface_create_similar (dev,
			                             cairo_surface_get_content (dev),
			                             cairo_image_surface_get_width (dev) / icon_scale,
			                             cairo_image_surface_get_height (dev) / icon_scale);
		cr = cairo_create (copy);
		cairo_set_source_surface (cr, dev, 0, 0);
		cairo_paint (cr);

		down = ctk_icon_theme_load_surface(icon_theme, ERROR_ICON, icon_size, icon_scale, NULL, 0, NULL);
		down_coords = cairo_image_surface_get_width (copy) / icon_scale;
		cairo_scale (cr, 0.5, 0.5);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface (cr, down, down_coords, down_coords);
		cairo_paint (cr);

		cairo_surface_destroy(down);
		cairo_surface_destroy(dev);
		dev = copy;
	}

	if (applet->show_icon) {
		ctk_widget_show(applet->dev_pix);
		ctk_image_set_from_surface(CTK_IMAGE(applet->dev_pix), dev);
	} else {
		ctk_widget_hide(applet->dev_pix);
	}
	cairo_surface_destroy(dev);
}

/* Here some rearangement of the icons and the labels occurs
 * according to the panelsize and wether we show in and out
 * or just the sum
 */
static void
applet_change_size_or_orient(CafePanelApplet *applet_widget, int arg1, CafeNetspeedApplet *applet)
{
	int size;
	CafePanelAppletOrient orient;

	g_assert(applet);

	size = cafe_panel_applet_get_size(applet_widget);
	orient = cafe_panel_applet_get_orient(applet_widget);

	g_object_ref(applet->pix_box);
	g_object_ref(applet->in_pix);
	g_object_ref(applet->in_label);
	g_object_ref(applet->out_pix);
	g_object_ref(applet->out_label);
	g_object_ref(applet->sum_label);

	if (applet->in_box) {
		ctk_container_remove(CTK_CONTAINER(applet->in_box), applet->in_label);
		ctk_container_remove(CTK_CONTAINER(applet->in_box), applet->in_pix);
		ctk_widget_destroy(applet->in_box);
	}
	if (applet->out_box) {
		ctk_container_remove(CTK_CONTAINER(applet->out_box), applet->out_label);
		ctk_container_remove(CTK_CONTAINER(applet->out_box), applet->out_pix);
		ctk_widget_destroy(applet->out_box);
	}
	if (applet->sum_box) {
		ctk_container_remove(CTK_CONTAINER(applet->sum_box), applet->sum_label);
		ctk_widget_destroy(applet->sum_box);
	}
	if (applet->box) {
		ctk_container_remove(CTK_CONTAINER(applet->box), applet->pix_box);
		ctk_widget_destroy(applet->box);
	}

	if (orient == CAFE_PANEL_APPLET_ORIENT_LEFT || orient == CAFE_PANEL_APPLET_ORIENT_RIGHT) {
		applet->box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
		applet->speed_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
		if (size > 64) {
			applet->sum_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
			applet->in_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 1);
			applet->out_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 1);
		} else {
			applet->sum_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
			applet->in_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
			applet->out_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
		}
		applet->labels_dont_shrink = FALSE;
	} else {
		applet->in_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 1);
		applet->out_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 1);
		applet->box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 1);
		applet->sum_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
		if (size < 48) {
			applet->speed_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 1);
			applet->labels_dont_shrink = TRUE;
		} else {
			applet->speed_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
			applet->labels_dont_shrink = !applet->show_sum;
		}
	}

	ctk_box_pack_start(CTK_BOX(applet->in_box), applet->in_pix, FALSE, FALSE, 0);
	ctk_box_pack_start(CTK_BOX(applet->in_box), applet->in_label, TRUE, TRUE, 0);
	ctk_box_pack_start(CTK_BOX(applet->out_box), applet->out_pix, FALSE, FALSE, 0);
	ctk_box_pack_start(CTK_BOX(applet->out_box), applet->out_label, TRUE, TRUE, 0);
	ctk_box_pack_start(CTK_BOX(applet->sum_box), applet->sum_label, TRUE, TRUE, 0);
	ctk_box_pack_start(CTK_BOX(applet->box), applet->pix_box, FALSE, FALSE, 0);

	g_object_unref(applet->pix_box);
	g_object_unref(applet->in_pix);
	g_object_unref(applet->in_label);
	g_object_unref(applet->out_pix);
	g_object_unref(applet->out_label);
	g_object_unref(applet->sum_label);

	if (applet->show_sum) {
		ctk_box_pack_start(CTK_BOX(applet->speed_box), applet->sum_box, TRUE, TRUE, 0);
	} else {
		ctk_box_pack_start(CTK_BOX(applet->speed_box), applet->in_box, TRUE, TRUE, 0);
		ctk_box_pack_start(CTK_BOX(applet->speed_box), applet->out_box, TRUE, TRUE, 0);
	}
	ctk_box_pack_start(CTK_BOX(applet->box), applet->speed_box, TRUE, TRUE, 0);

	ctk_widget_show_all(applet->box);
	if (!applet->show_icon) {
		ctk_widget_hide(applet->dev_pix);
	}
	ctk_container_add(CTK_CONTAINER(applet->applet), applet->box);

	change_icons (applet);
}

/* Change visibility of signal quality icon for wireless devices
 */
static void
change_quality_icon(CafeNetspeedApplet *applet)
{
	if (applet->devinfo.type == DEV_WIRELESS &&
		applet->devinfo.up && applet->show_quality_icon) {
		ctk_widget_show(applet->qual_pix);
	} else {
		ctk_widget_hide(applet->qual_pix);
	}
}

static void
update_quality_icon(CafeNetspeedApplet *applet)
{
	if (!applet->show_quality_icon) {
		return;
	}

	unsigned int q;

	q = (applet->devinfo.qual);
	q /= 25;
	q = CLAMP(q, 0, 3); /* q out of range would crash when accessing qual_surfaces[q] */
	ctk_image_set_from_surface (CTK_IMAGE(applet->qual_pix), applet->qual_surfaces[q]);
}

static void
init_quality_surfaces(CafeNetspeedApplet *applet)
{
	CtkIconTheme *icon_theme;
	int i;
	cairo_surface_t *surface;
	gint icon_scale;

	/* FIXME: Add larger icon files. */
	gint icon_size = 24;

	icon_theme = ctk_icon_theme_get_default();
	icon_scale = ctk_widget_get_scale_factor (CTK_WIDGET (applet->applet));

	for (i = 0; i < 4; i++) {
		if (applet->qual_surfaces[i])
			cairo_surface_destroy(applet->qual_surfaces[i]);
		surface = ctk_icon_theme_load_surface(icon_theme,
			wireless_quality_icon[i], icon_size, icon_scale, NULL, 0, NULL);
		if (surface) {
		  cairo_t *cr;
		  applet->qual_surfaces[i] = cairo_surface_create_similar (surface,
			                                                   cairo_surface_get_content (surface),
			                                                   cairo_image_surface_get_width (surface) / icon_scale,
			                                                   cairo_image_surface_get_height (surface) / icon_scale);
		  cr = cairo_create (applet->qual_surfaces[i]);
		  cairo_set_source_surface (cr, surface, 0, 0);
		  cairo_paint (cr);
		  cairo_surface_destroy(surface);
		}
		else {
		  applet->qual_surfaces[i] = NULL;
		}
	}
}


static void
icon_theme_changed_cb(CtkIconTheme *icon_theme, gpointer user_data)
{
    CafeNetspeedApplet *applet = (CafeNetspeedApplet*)user_data;

    init_quality_surfaces(user_data);
    if (applet->devinfo.type == DEV_WIRELESS && applet->devinfo.up)
        update_quality_icon(user_data);
    change_icons(user_data);
}

/* Converts a number of bytes into a human
 * readable string - in [M/k]bytes[/s]
 * The string has to be freed
 */
static char*
bytes_to_string(double bytes, gboolean per_sec, gboolean bits, gboolean shortened)
{
	const char *format;
	const char *unit;
	guint kilo; /* no really a kilo : a kilo or kibi */

	if (bits) {
		bytes *= 8;
		kilo = 1000;
	} else
		kilo = 1024;

	if (bytes < kilo) {

		format = "%.0f %s";

		if (per_sec)
			if (shortened) {
				unit = bits ? /* translators: bits (short) */ N_("b"): /* translators: Bytes (short) */ N_("B");
			} else {
				unit = bits ? N_("b/s") : N_("B/s");
			}
		else
			unit = bits ? N_("bits") : N_("bytes");

	} else if (bytes < (kilo * kilo)) {
		format = (bytes < (100 * kilo)) ? "%.1f %s" : "%.0f %s";
		bytes /= kilo;

		if (per_sec)
			if (shortened) {
				unit = bits ? /* translators: kilobits (short) */ N_("k") : /* translators: Kilobytes (short) */ N_("K");
			} else {
				unit = bits ? N_("kb/s") : N_("KiB/s");
			}
		else
			unit = bits ? N_("kb")   : N_("KiB");

	} else {

		format = "%.1f %s";

		bytes /= kilo * kilo;

		if (per_sec)
			if (shortened) {
				unit = bits ? /* translators: megabits (short) */ N_("m") : /* translators: Megabytes (short) */ N_("M");
			} else {
				unit = bits ? N_("Mb/s") : N_("MiB/s");
			}
		else
			unit = bits ? N_("Mb")   : N_("MiB");
	}

	return g_strdup_printf(format, bytes, gettext(unit));
}


/* Redraws the graph drawingarea
 * Some really black magic is going on in here ;-)
 */
static void
redraw_graph(CafeNetspeedApplet *applet, cairo_t *cr)
{
	CtkWidget *da = CTK_WIDGET(applet->drawingarea);
	CtkStyleContext *stylecontext = ctk_widget_get_style_context (da);
	CdkWindow *real_window = ctk_widget_get_window (da);
	CdkPoint in_points[GRAPH_VALUES], out_points[GRAPH_VALUES];
	PangoLayout *layout;
	PangoRectangle logical_rect;
	char *text;
	int i, offset, w, h;
	double max_val;
	double dash[2] = { 1.0, 2.0 };

	w = cdk_window_get_width (real_window);
	h = cdk_window_get_height (real_window);

	/* the graph hight should be: hight/2 <= applet->max_graph < hight */
	for (max_val = 1; max_val < applet->max_graph; max_val *= 2) ;

	/* calculate the polygons (CdkPoint[]) for the graphs */
	offset = 0;
	for (i = (applet->index_graph + 1) % GRAPH_VALUES; applet->in_graph[i] < 0; i = (i + 1) % GRAPH_VALUES)
		offset++;
	for (i = offset + 1; i < GRAPH_VALUES; i++)
	{
		int index = (applet->index_graph + i) % GRAPH_VALUES;
		out_points[i].x = in_points[i].x = ((w - 6) * i) / GRAPH_VALUES + 4;
		in_points[i].y = h - 6 - (int)((h - 8) * applet->in_graph[index] / max_val);
		out_points[i].y = h - 6 - (int)((h - 8) * applet->out_graph[index] / max_val);
	}
	in_points[offset].x = out_points[offset].x = ((w - 6) * offset) / GRAPH_VALUES + 4;
	in_points[offset].y = in_points[(offset + 1) % GRAPH_VALUES].y;
	out_points[offset].y = out_points[(offset + 1) % GRAPH_VALUES].y;

	/* draw the background */
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_rectangle (cr, 02, 2, w - 6, h - 6);
	cairo_fill (cr);

	cairo_set_line_width(cr, 1.0);
	cairo_set_dash (cr, dash, 2, 0);

	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_rectangle (cr, 2, 2, w - 6, h - 6);
	cairo_stroke (cr);

	for (i = 0; i < GRAPH_LINES; i++) {
		int y = 2 + ((h - 6) * i) / GRAPH_LINES;
		cairo_move_to (cr, 2, y);
		cairo_line_to (cr, w - 4, y);
	}
	cairo_stroke (cr);

	/* draw the polygons */
	cairo_set_dash (cr, dash, 0, 1);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

	cdk_cairo_set_source_rgba (cr, &applet->in_color);
	for (i = offset; i < GRAPH_VALUES; i++) {
		cairo_line_to (cr, in_points[i].x, in_points[i].y);
	}
	cairo_stroke (cr);

	cdk_cairo_set_source_rgba (cr, &applet->out_color);
	for (i = offset; i < GRAPH_VALUES; i++) {
		cairo_line_to (cr, out_points[i].x, out_points[i].y);
	}
	cairo_stroke (cr);

	text = bytes_to_string(max_val, TRUE, applet->show_bits, applet->short_unit);
	add_markup_fgcolor(&text, "black");
	layout = ctk_widget_create_pango_layout (da, NULL);
	pango_layout_set_markup(layout, text, -1);
	g_free (text);
	ctk_render_layout(stylecontext, cr, 3, 2, layout);
	g_object_unref(G_OBJECT(layout));

	text = bytes_to_string(0.0, TRUE, applet->show_bits, applet->short_unit);
	add_markup_fgcolor(&text, "black");
	layout = ctk_widget_create_pango_layout (da, NULL);
	pango_layout_set_markup(layout, text, -1);
	pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	g_free (text);
	ctk_render_layout(stylecontext, cr, 3, h - 4 - logical_rect.height, layout);
	g_object_unref(G_OBJECT(layout));
}

static gboolean
set_applet_devinfo(CafeNetspeedApplet* applet, const char* iface)
{
	DevInfo info;

	get_device_info(iface, &info);

	if (info.running) {
		free_device_info(&applet->devinfo);
		applet->devinfo = info;
		applet->device_has_changed = TRUE;
		return TRUE;
	}

	free_device_info(&info);
	return FALSE;
}

/* Find the first available device, that is running and != lo */
static void
search_for_up_if(CafeNetspeedApplet *applet)
{
	const gchar *default_route;
	GList *devices, *tmp;
	DevInfo info;

	default_route = get_default_route();

	if (default_route != NULL) {
		if (set_applet_devinfo(applet, default_route))
			return;
	}

	devices = get_available_devices();
	for (tmp = devices; tmp; tmp = g_list_next(tmp)) {
		if (is_dummy_device(tmp->data))
			continue;
		if (set_applet_devinfo(applet, tmp->data))
			break;
	}
	free_devices_list(devices);
}

/* Here happens the really interesting stuff */
static void
update_applet(CafeNetspeedApplet *applet)
{
	guint64 indiff, outdiff;
	double inrate, outrate;
	char *inbytes, *outbytes;
	int i;
	DevInfo oldinfo;

	if (!applet)	return;

	/* First we try to figure out if the device has changed */
	oldinfo = applet->devinfo;
	get_device_info(oldinfo.name, &applet->devinfo);
	if (compare_device_info(&applet->devinfo, &oldinfo))
		applet->device_has_changed = TRUE;
	free_device_info(&oldinfo);

	/* If the device has changed, reintialize stuff */
	if (applet->device_has_changed) {
		change_icons(applet);
		change_quality_icon(applet);
		for (i = 0; i < OLD_VALUES; i++)
		{
			applet->in_old[i] = applet->devinfo.rx;
			applet->out_old[i] = applet->devinfo.tx;
		}
		for (i = 0; i < GRAPH_VALUES; i++)
		{
			applet->in_graph[i] = -1;
			applet->out_graph[i] = -1;
		}
		applet->max_graph = 0;
		applet->index_graph = 0;
		applet->device_has_changed = FALSE;
	}

	/* create the strings for the labels and tooltips */
	if (applet->devinfo.running)
	{
		if (applet->devinfo.rx < applet->in_old[applet->index_old]) indiff = 0;
		else indiff = applet->devinfo.rx - applet->in_old[applet->index_old];
		if (applet->devinfo.tx < applet->out_old[applet->index_old]) outdiff = 0;
		else outdiff = applet->devinfo.tx - applet->out_old[applet->index_old];

		inrate = indiff * 1000.0;
		inrate /= (double)(applet->refresh_time * OLD_VALUES);
		outrate = outdiff * 1000.0;
		outrate /= (double)(applet->refresh_time * OLD_VALUES);

		applet->in_graph[applet->index_graph] = inrate;
		applet->out_graph[applet->index_graph] = outrate;
		applet->max_graph = MAX(inrate, applet->max_graph);
		applet->max_graph = MAX(outrate, applet->max_graph);

		applet->devinfo.rx_rate = bytes_to_string(inrate, TRUE, applet->show_bits, applet->short_unit);
		applet->devinfo.tx_rate = bytes_to_string(outrate, TRUE, applet->show_bits, applet->short_unit);
		applet->devinfo.sum_rate = bytes_to_string(inrate + outrate, TRUE, applet->show_bits, applet->short_unit);
	} else {
		applet->devinfo.rx_rate = g_strdup("");
		applet->devinfo.tx_rate = g_strdup("");
		applet->devinfo.sum_rate = g_strdup("");
		applet->in_graph[applet->index_graph] = 0;
		applet->out_graph[applet->index_graph] = 0;
	}

	if (applet->devinfo.type == DEV_WIRELESS) {
		if (applet->devinfo.up)
			update_quality_icon(applet);

		if (applet->signalbar) {
			float quality;
			char *text;

			quality = applet->devinfo.qual / 100.0f;
			if (quality > 1.0)
				quality = 1.0;

			text = g_strdup_printf ("%d %%", applet->devinfo.qual);
			ctk_progress_bar_set_fraction (CTK_PROGRESS_BAR (applet->signalbar), quality);
			ctk_progress_bar_set_text (CTK_PROGRESS_BAR (applet->signalbar), text);
			g_free(text);
		}
	}

	update_tooltip(applet);

	/* Refresh the text of the labels and tooltip */
	if (applet->show_sum) {
		ctk_label_set_markup(CTK_LABEL(applet->sum_label), applet->devinfo.sum_rate);
	} else {
		ctk_label_set_markup(CTK_LABEL(applet->in_label), applet->devinfo.rx_rate);
		ctk_label_set_markup(CTK_LABEL(applet->out_label), applet->devinfo.tx_rate);
	}

	/* Refresh the values of the Infodialog */
	if (applet->inbytes_text) {
		inbytes = bytes_to_string((double)applet->devinfo.rx, FALSE, FALSE, FALSE);
		ctk_label_set_text(CTK_LABEL(applet->inbytes_text), inbytes);
		g_free(inbytes);
	}
	if (applet->outbytes_text) {
		outbytes = bytes_to_string((double)applet->devinfo.tx, FALSE, FALSE, FALSE);
		ctk_label_set_text(CTK_LABEL(applet->outbytes_text), outbytes);
		g_free(outbytes);
	}
	/* Redraw the graph of the Infodialog */
	if (applet->drawingarea)
		ctk_widget_queue_draw (CTK_WIDGET (applet->drawingarea));

	/* Save old values... */
	applet->in_old[applet->index_old] = applet->devinfo.rx;
	applet->out_old[applet->index_old] = applet->devinfo.tx;
	applet->index_old = (applet->index_old + 1) % OLD_VALUES;

	/* Move the graphindex. Check if we can scale down again */
	applet->index_graph = (applet->index_graph + 1) % GRAPH_VALUES;
	if (applet->index_graph % 20 == 0)
	{
		double max = 0;
		for (i = 0; i < GRAPH_VALUES; i++)
		{
			max = MAX(max, applet->in_graph[i]);
			max = MAX(max, applet->out_graph[i]);
		}
		applet->max_graph = max;
	}

	/* Always follow the default route */
	if (applet->auto_change_device) {
		gboolean change_device_now = !applet->devinfo.running;
		if (!change_device_now) {
			const gchar *default_route;
			default_route = get_default_route();
			change_device_now = (default_route != NULL
						&& strcmp(default_route,
							applet->devinfo.name));
		}
		if (change_device_now) {
			search_for_up_if(applet);
		}
	}
}

static gboolean
timeout_function(CafeNetspeedApplet *applet)
{
	if (!applet)
		return FALSE;
	if (!applet->timeout_id)
		return FALSE;

	update_applet(applet);
	return TRUE;
}

/* Display a section of netspeed help
 */
static void
display_help (CtkWidget *dialog, const gchar *section)
{
	GError *error = NULL;
	gboolean ret;
	char *uri;

	if (section)
		uri = g_strdup_printf ("help:cafe-netspeed-applet/%s", section);
	else
		uri = g_strdup ("help:cafe-netspeed-applet");

	ret = ctk_show_uri_on_window (NULL,
	                              uri,
	                              ctk_get_current_event_time (),
	                              &error);
	g_free (uri);

	if (ret == FALSE) {
		CtkWidget *error_dialog = ctk_message_dialog_new (NULL,
								  CTK_DIALOG_MODAL,
								  CTK_MESSAGE_ERROR,
								  CTK_BUTTONS_OK,
								  _("There was an error displaying help:\n%s"),
								  error->message);
		g_signal_connect (error_dialog, "response",
				  G_CALLBACK (ctk_widget_destroy), NULL);

		ctk_window_set_resizable (CTK_WINDOW (error_dialog), FALSE);
		ctk_window_set_screen  (CTK_WINDOW (error_dialog), ctk_widget_get_screen (dialog));
		ctk_widget_show (error_dialog);
		g_error_free (error);
	}
}

/* Opens gnome help application
 */
static void
help_cb (CtkAction *action, CafeNetspeedApplet *ap)
{
	display_help (CTK_WIDGET (ap->applet), NULL);
}

/* Just the about window... If it's already open, just fokus it
 */
static void
about_cb(CtkAction *action, gpointer data)
{
	const char *authors[] =
	{
		"Jörgen Scheibengruber <mfcn@gmx.de>",
		"Dennis Cranston <dennis_cranston@yahoo.com>",
		"Pedro Villavicencio Garrido <pvillavi@gnome.org>",
		"Benoît Dejean <benoit@placenet.org>",
		"Stefano Karapetsas <stefano@karapetsas.com>",
		"Perberos <perberos@gmail.com>",
		"Pablo Barciela <scow@riseup.net>",
		NULL
	};

	ctk_show_about_dialog (NULL,
			       "title", _("About CAFE Netspeed"),
			       "version", VERSION,
			       "copyright", _("Copyright \xc2\xa9 2002-2003 Jörgen Scheibengruber\n"
			                      "Copyright \xc2\xa9 2011-2014 Stefano Karapetsas\n"
			                      "Copyright \xc2\xa9 2015-2020 MATE developers\n"
			                      "Copyright \xc2\xa9 2023-2024 Pablo Barciela"),
			       "comments", _("A little applet that displays some information on the traffic on the specified network device"),
			       "authors", authors,
			       "documenters", NULL,
			       "translator-credits", _("translator-credits"),
			       "website", "http://www.cafe-desktop.org/",
			       "logo-icon-name", LOGO_ICON,
			       NULL);

}

/* this basically just retrieves the new devicestring
 * and then calls applet_device_change() and change_icons()
 */
static void
device_change_cb(CtkComboBox *combo, CafeNetspeedApplet *applet)
{
	GList *devices;
	int i, active;

	g_assert(combo);
	devices = g_object_get_data(G_OBJECT(combo), "devices");
	active = ctk_combo_box_get_active(combo);
	g_assert(active > -1);

	if (0 == active) {
		if (applet->auto_change_device)
			return;
		applet->auto_change_device = TRUE;
	} else {
		applet->auto_change_device = FALSE;
		for (i = 1; i < active; i++) {
			devices = g_list_next(devices);
		}
		if (g_str_equal(devices->data, applet->devinfo.name))
			return;
		free_device_info(&applet->devinfo);
		get_device_info(devices->data, &applet->devinfo);
	}

	applet->device_has_changed = TRUE;
	update_applet(applet);
}


/* Handle preference dialog response event
 */
static void
pref_response_cb (CtkDialog *dialog, gint id, gpointer data)
{
    CafeNetspeedApplet *applet = data;

    if(id == CTK_RESPONSE_HELP){
        display_help (CTK_WIDGET (dialog), "netspeed_applet-settings");
	return;
    }
    g_settings_delay (applet->gsettings);
    g_settings_set_string (applet->gsettings, "device", applet->devinfo.name);
    g_settings_set_boolean (applet->gsettings, "show-sum", applet->show_sum);
    g_settings_set_boolean (applet->gsettings, "show-bits", applet->show_bits);
    g_settings_set_boolean (applet->gsettings, "short-unit", applet->short_unit);
    g_settings_set_boolean (applet->gsettings, "show-icon", applet->show_icon);
    g_settings_set_boolean (applet->gsettings, "show-quality-icon", applet->show_quality_icon);
    g_settings_set_boolean (applet->gsettings, "change-icon", applet->change_icon);
    g_settings_set_boolean (applet->gsettings, "auto-change-device", applet->auto_change_device);
    g_settings_apply (applet->gsettings);

    ctk_widget_destroy(CTK_WIDGET(applet->settings));
    applet->settings = NULL;
}

/* Called when the showsum checkbutton is toggled...
 */
static void
showsum_change_cb(CtkToggleButton *togglebutton, CafeNetspeedApplet *applet)
{
	applet->show_sum = ctk_toggle_button_get_active(togglebutton);
	applet_change_size_or_orient(applet->applet, -1, (gpointer)applet);
	change_icons(applet);
}

/* Called when the showbits checkbutton is toggled...
 */
static void
showbits_change_cb(CtkToggleButton *togglebutton, CafeNetspeedApplet *applet)
{
	applet->show_bits = ctk_toggle_button_get_active(togglebutton);
}

/* Called when the shortunit checkbutton is toggled...
 */
static void
shortunit_change_cb(CtkToggleButton *togglebutton, CafeNetspeedApplet *applet)
{
	applet->short_unit = ctk_toggle_button_get_active(togglebutton);
}

/* Called when the showicon checkbutton is toggled...
 */
static void
showicon_change_cb(CtkToggleButton *togglebutton, CafeNetspeedApplet *applet)
{
	applet->show_icon = ctk_toggle_button_get_active(togglebutton);
	change_icons(applet);
}

/* Called when the showqualityicon checkbutton is toggled...
 */
static void
showqualityicon_change_cb(CtkToggleButton *togglebutton, CafeNetspeedApplet *applet)
{
	applet->show_quality_icon = ctk_toggle_button_get_active(togglebutton);
	change_quality_icon(applet);
}

/* Called when the changeicon checkbutton is toggled...
 */
static void
changeicon_change_cb(CtkToggleButton *togglebutton, CafeNetspeedApplet *applet)
{
	applet->change_icon = ctk_toggle_button_get_active(togglebutton);
	change_icons(applet);
}

/* Creates the settings dialog
 * After its been closed, take the new values and store
 * them in the gsettings database
 */
static void
settings_cb(CtkAction *action, gpointer data)
{
	CafeNetspeedApplet *applet = (CafeNetspeedApplet*)data;
	CtkWidget *vbox;
	CtkWidget *hbox;
	CtkWidget *categories_vbox;
	CtkWidget *category_vbox;
	CtkWidget *controls_vbox;
	CtkWidget *category_header_label;
	CtkWidget *network_device_hbox;
	CtkWidget *network_device_label;
	CtkWidget *indent_label;
	CtkWidget *show_sum_checkbutton;
	CtkWidget *show_bits_checkbutton;
	CtkWidget *short_unit_checkbutton;
	CtkWidget *show_icon_checkbutton;
	CtkWidget *show_quality_icon_checkbutton;
	CtkWidget *change_icon_checkbutton;
	CtkSizeGroup *category_label_size_group;
  	gchar *header_str;
	GList *ptr, *devices;
	int i, active = -1;

	g_assert(applet);

	if (applet->settings)
	{
		ctk_window_present(CTK_WINDOW(applet->settings));
		return;
	}

	category_label_size_group = ctk_size_group_new(CTK_SIZE_GROUP_HORIZONTAL);

	applet->settings = CTK_DIALOG(ctk_dialog_new_with_buttons(_("CAFE Netspeed Preferences"),
								  NULL,
								  CTK_DIALOG_DESTROY_WITH_PARENT,
								  "ctk-help", CTK_RESPONSE_HELP,
								  "ctk-close", CTK_RESPONSE_ACCEPT,
								  NULL));

	ctk_window_set_resizable(CTK_WINDOW(applet->settings), FALSE);
	ctk_window_set_screen(CTK_WINDOW(applet->settings),
			      ctk_widget_get_screen(CTK_WIDGET(applet->settings)));

	ctk_dialog_set_default_response(CTK_DIALOG(applet->settings), CTK_RESPONSE_CLOSE);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
	ctk_container_set_border_width(CTK_CONTAINER(vbox), 12);

	categories_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 18);
	ctk_box_pack_start(CTK_BOX (vbox), categories_vbox, TRUE, TRUE, 0);

	category_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start(CTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);

	header_str = g_strconcat("<span weight=\"bold\">", _("General Settings"), "</span>", NULL);
	category_header_label = ctk_label_new(header_str);
	ctk_label_set_use_markup(CTK_LABEL(category_header_label), TRUE);
	ctk_label_set_justify(CTK_LABEL(category_header_label), CTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (CTK_LABEL (category_header_label), 0.0);
	ctk_label_set_yalign (CTK_LABEL (category_header_label), 0.5);
	ctk_box_pack_start(CTK_BOX (category_vbox), category_header_label, FALSE, FALSE, 0);
	g_free(header_str);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_pack_start(CTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);

	indent_label = ctk_label_new("    ");
	ctk_label_set_justify(CTK_LABEL (indent_label), CTK_JUSTIFY_LEFT);
	ctk_box_pack_start(CTK_BOX (hbox), indent_label, FALSE, FALSE, 0);

	controls_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
	ctk_box_pack_start(CTK_BOX(hbox), controls_vbox, TRUE, TRUE, 0);

	network_device_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
	ctk_box_pack_start(CTK_BOX(controls_vbox), network_device_hbox, TRUE, TRUE, 0);

	network_device_label = ctk_label_new_with_mnemonic(_("Network _device:"));
	ctk_label_set_justify(CTK_LABEL(network_device_label), CTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (CTK_LABEL (network_device_label), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (network_device_label), 0.5f);
	ctk_size_group_add_widget(category_label_size_group, network_device_label);
	ctk_box_pack_start(CTK_BOX (network_device_hbox), network_device_label, FALSE, FALSE, 0);

	applet->network_device_combo = ctk_combo_box_text_new();
	ctk_label_set_mnemonic_widget(CTK_LABEL(network_device_label), applet->network_device_combo);
	ctk_box_pack_start (CTK_BOX (network_device_hbox), applet->network_device_combo, TRUE, TRUE, 0);

	/* Default means device with default route set */
	ctk_combo_box_text_append_text(CTK_COMBO_BOX_TEXT(applet->network_device_combo), _("Default"));
	ptr = devices = get_available_devices();
	for (i = 0; ptr; ptr = g_list_next(ptr)) {
		ctk_combo_box_text_append_text(CTK_COMBO_BOX_TEXT(applet->network_device_combo), ptr->data);
		if (g_str_equal(ptr->data, applet->devinfo.name)) active = (i + 1);
		++i;
	}
	if (active < 0 || applet->auto_change_device) {
		active = 0;
	}
	ctk_combo_box_set_active(CTK_COMBO_BOX(applet->network_device_combo), active);
	g_object_set_data_full(G_OBJECT(applet->network_device_combo), "devices", devices, (GDestroyNotify)free_devices_list);

	show_sum_checkbutton = ctk_check_button_new_with_mnemonic(_("Show _sum instead of in & out"));
	ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(show_sum_checkbutton), applet->show_sum);
	ctk_box_pack_start(CTK_BOX(controls_vbox), show_sum_checkbutton, FALSE, FALSE, 0);

	show_bits_checkbutton = ctk_check_button_new_with_mnemonic(_("Show _bits instead of bytes"));
	ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(show_bits_checkbutton), applet->show_bits);
	ctk_box_pack_start(CTK_BOX(controls_vbox), show_bits_checkbutton, FALSE, FALSE, 0);

	short_unit_checkbutton = ctk_check_button_new_with_mnemonic(_("Shorten _unit legend"));
	ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(short_unit_checkbutton), applet->short_unit);
	ctk_box_pack_start(CTK_BOX(controls_vbox), short_unit_checkbutton, FALSE, FALSE, 0);

	change_icon_checkbutton = ctk_check_button_new_with_mnemonic(_("_Change icon according to the selected device"));
	ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(change_icon_checkbutton), applet->change_icon);
	ctk_box_pack_start(CTK_BOX(controls_vbox), change_icon_checkbutton, FALSE, FALSE, 0);

	show_icon_checkbutton = ctk_check_button_new_with_mnemonic(_("Show _icon"));
	ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(show_icon_checkbutton), applet->show_icon);
	ctk_box_pack_start(CTK_BOX(controls_vbox), show_icon_checkbutton, FALSE, FALSE, 0);

	show_quality_icon_checkbutton = ctk_check_button_new_with_mnemonic(_("Show signal _quality icon for wireless devices"));
	ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(show_quality_icon_checkbutton), applet->show_quality_icon);
	ctk_box_pack_start(CTK_BOX(controls_vbox), show_quality_icon_checkbutton, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT (applet->network_device_combo), "changed",
			 G_CALLBACK(device_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (show_sum_checkbutton), "toggled",
			 G_CALLBACK(showsum_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (show_bits_checkbutton), "toggled",
			 G_CALLBACK(showbits_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (short_unit_checkbutton), "toggled",
			 G_CALLBACK(shortunit_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (show_icon_checkbutton), "toggled",
			 G_CALLBACK(showicon_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (show_quality_icon_checkbutton), "toggled",
			 G_CALLBACK(showqualityicon_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (change_icon_checkbutton), "toggled",
			 G_CALLBACK(changeicon_change_cb), (gpointer)applet);

	g_signal_connect(G_OBJECT (applet->settings), "response",
			 G_CALLBACK(pref_response_cb), (gpointer)applet);

	ctk_container_add(CTK_CONTAINER(ctk_dialog_get_content_area (applet->settings)), vbox);

	ctk_widget_show_all(CTK_WIDGET(applet->settings));
}

static gboolean
da_draw(CtkWidget *widget, cairo_t *cr, gpointer data)
{
	CafeNetspeedApplet *applet = (CafeNetspeedApplet*)data;

	redraw_graph(applet, cr);

	return FALSE;
}

static void
incolor_changed_cb (CtkColorChooser *button, gpointer data)
{
	CafeNetspeedApplet *applet = (CafeNetspeedApplet*)data;
	CdkRGBA color;
	gchar *string;

	ctk_color_chooser_get_rgba (CTK_COLOR_CHOOSER (button), &color);
	applet->in_color = color;

	string = cdk_rgba_to_string (&color);
	g_settings_set_string (applet->gsettings, "in-color", string);
	g_free (string);
}

static void
outcolor_changed_cb (CtkColorChooser *button, gpointer data)
{
	CafeNetspeedApplet *applet = (CafeNetspeedApplet*)data;
	CdkRGBA color;
	gchar *string;

	ctk_color_chooser_get_rgba (CTK_COLOR_CHOOSER (button), &color);
	applet->out_color = color;

	string = cdk_rgba_to_string (&color);
	g_settings_set_string (applet->gsettings, "out-color", string);
	g_free (string);
}

/* Handle info dialog response event
 */
static void
info_response_cb (CtkDialog *dialog, gint id, CafeNetspeedApplet *applet)
{

	if(id == CTK_RESPONSE_HELP){
		display_help (CTK_WIDGET (dialog), "netspeed_applet-details");
		return;
	}

	ctk_widget_destroy(CTK_WIDGET(applet->details));

	applet->details = NULL;
	applet->inbytes_text = NULL;
	applet->outbytes_text = NULL;
	applet->drawingarea = NULL;
	applet->signalbar = NULL;
}

/* Creates the details dialog
 */
static void
showinfo_cb(CtkAction *action, gpointer data)
{
	CafeNetspeedApplet *applet = (CafeNetspeedApplet*)data;
	CtkWidget *box, *hbox;
	CtkWidget *grid, *da_frame;
	CtkWidget *ip_label, *netmask_label;
	CtkWidget *hwaddr_label, *ptpip_label;
	CtkWidget *ip_text, *netmask_text;
	CtkWidget *hwaddr_text, *ptpip_text;
	CtkWidget *inbytes_label, *outbytes_label;
	CtkWidget *incolor_sel, *incolor_label;
	CtkWidget *outcolor_sel, *outcolor_label;
	char *title;

	g_assert(applet);

	if (applet->details)
	{
		ctk_window_present(CTK_WINDOW(applet->details));
		return;
	}

	title = g_strdup_printf(_("Device Details for %s"), applet->devinfo.name);
	applet->details = CTK_DIALOG(ctk_dialog_new_with_buttons(title,
		NULL,
		CTK_DIALOG_DESTROY_WITH_PARENT,
		"ctk-close", CTK_RESPONSE_ACCEPT,
		"ctk-help", CTK_RESPONSE_HELP,
		NULL));
	g_free(title);

	ctk_dialog_set_default_response(CTK_DIALOG(applet->details), CTK_RESPONSE_CLOSE);

	box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 10);
	ctk_container_set_border_width(CTK_CONTAINER(box), 12);

	grid = ctk_grid_new ();
	ctk_grid_set_row_spacing (CTK_GRID(grid), 10);
	ctk_grid_set_column_spacing (CTK_GRID(grid), 15);

	da_frame = ctk_frame_new(NULL);
	ctk_frame_set_shadow_type(CTK_FRAME(da_frame), CTK_SHADOW_NONE);
	applet->drawingarea = CTK_DRAWING_AREA(ctk_drawing_area_new());
	ctk_widget_set_size_request(CTK_WIDGET(applet->drawingarea), -1, 180);
	ctk_container_add(CTK_CONTAINER(da_frame), CTK_WIDGET(applet->drawingarea));

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 5);
	incolor_label = ctk_label_new_with_mnemonic(_("_In graph color"));
	outcolor_label = ctk_label_new_with_mnemonic(_("_Out graph color"));

	incolor_sel = ctk_color_button_new ();
	outcolor_sel = ctk_color_button_new ();

	ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (incolor_sel),  &applet->in_color);
	ctk_color_chooser_set_rgba (CTK_COLOR_CHOOSER (outcolor_sel),  &applet->out_color);

	ctk_label_set_mnemonic_widget(CTK_LABEL(incolor_label), incolor_sel);
	ctk_label_set_mnemonic_widget(CTK_LABEL(outcolor_label), outcolor_sel);

	ctk_box_pack_start(CTK_BOX(hbox), incolor_sel, FALSE, FALSE, 0);
	ctk_box_pack_start(CTK_BOX(hbox), incolor_label, FALSE, FALSE, 0);
	ctk_box_pack_start(CTK_BOX(hbox), outcolor_sel, FALSE, FALSE, 0);
	ctk_box_pack_start(CTK_BOX(hbox), outcolor_label, FALSE, FALSE, 0);

	ip_label = ctk_label_new(_("Internet Address:"));
	netmask_label = ctk_label_new(_("Netmask:"));
	hwaddr_label = ctk_label_new(_("Hardware Address:"));
	ptpip_label = ctk_label_new(_("P-t-P Address:"));
	inbytes_label = ctk_label_new(_("Bytes in:"));
	outbytes_label = ctk_label_new(_("Bytes out:"));

	ip_text = ctk_label_new(applet->devinfo.ip ? applet->devinfo.ip : _("none"));
	netmask_text = ctk_label_new(applet->devinfo.netmask ? applet->devinfo.netmask : _("none"));
	hwaddr_text = ctk_label_new(applet->devinfo.hwaddr ? applet->devinfo.hwaddr : _("none"));
	ptpip_text = ctk_label_new(applet->devinfo.ptpip ? applet->devinfo.ptpip : _("none"));
	applet->inbytes_text = ctk_label_new("0 byte");
	applet->outbytes_text = ctk_label_new("0 byte");

	ctk_label_set_selectable(CTK_LABEL(ip_text), TRUE);
	ctk_label_set_selectable(CTK_LABEL(netmask_text), TRUE);
	ctk_label_set_selectable(CTK_LABEL(hwaddr_text), TRUE);
	ctk_label_set_selectable(CTK_LABEL(ptpip_text), TRUE);

	ctk_label_set_xalign (CTK_LABEL (ip_label), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (ip_label), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (ip_text), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (ip_text), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (netmask_label), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (netmask_label), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (netmask_text), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (netmask_text), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (hwaddr_label), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (hwaddr_label), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (hwaddr_text), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (hwaddr_text), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (ptpip_label), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (ptpip_label), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (ptpip_text), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (ptpip_text), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (inbytes_label), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (inbytes_label), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (applet->inbytes_text), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (applet->inbytes_text), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (outbytes_label), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (outbytes_label), 0.5f);
	ctk_label_set_xalign (CTK_LABEL (applet->outbytes_text), 0.0f);
	ctk_label_set_yalign (CTK_LABEL (applet->outbytes_text), 0.5f);

	ctk_grid_attach(CTK_GRID(grid), ip_label, 0, 0, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), ip_text, 1, 0, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), netmask_label, 2, 0, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), netmask_text, 3, 0, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), hwaddr_label, 0, 1, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), hwaddr_text, 1, 1, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), ptpip_label, 2, 1, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), ptpip_text, 3, 1, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), inbytes_label, 0, 2, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), applet->inbytes_text, 1, 2, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), outbytes_label, 2, 2, 1, 1);
	ctk_grid_attach(CTK_GRID(grid), applet->outbytes_text, 3, 2, 1, 1);

	/* check if we got an ipv6 address */
	if (applet->devinfo.ipv6 && (strlen (applet->devinfo.ipv6) > 2)) {
		CtkWidget *ipv6_label, *ipv6_text;

		ipv6_label = ctk_label_new (_("IPV6 Address:"));
		ipv6_text = ctk_label_new (applet->devinfo.ipv6);

		ctk_label_set_selectable (CTK_LABEL (ipv6_text), TRUE);

		ctk_label_set_xalign (CTK_LABEL (ipv6_label), 0.0f);
		ctk_label_set_yalign (CTK_LABEL (ipv6_label), 0.5f);
		ctk_label_set_xalign (CTK_LABEL (ipv6_text), 0.0f);
		ctk_label_set_yalign (CTK_LABEL (ipv6_text), 0.5f);
		ctk_grid_attach (CTK_GRID (grid), ipv6_label, 0, 3, 1, 1);
		ctk_grid_attach (CTK_GRID (grid), ipv6_text, 1, 3, 1, 1);
	}

	if (applet->devinfo.type == DEV_WIRELESS) {
		CtkWidget *signal_label;
		CtkWidget *essid_label;
		CtkWidget *essid_text;
		float quality;
		char *text;

		/* _maybe_ we can add the encrypted icon between the essid and the signal bar. */

		applet->signalbar = ctk_progress_bar_new ();

		quality = applet->devinfo.qual / 100.0f;
		if (quality > 1.0)
		quality = 1.0;

		text = g_strdup_printf ("%d %%", applet->devinfo.qual);
		ctk_progress_bar_set_fraction (CTK_PROGRESS_BAR (applet->signalbar), quality);
		ctk_progress_bar_set_text (CTK_PROGRESS_BAR (applet->signalbar), text);
		g_free(text);

		signal_label = ctk_label_new (_("Signal Strength:"));
		essid_label = ctk_label_new (_("ESSID:"));
		essid_text = ctk_label_new (applet->devinfo.essid);

		ctk_label_set_xalign (CTK_LABEL (signal_label), 0.0f);
		ctk_label_set_yalign (CTK_LABEL (signal_label), 0.5f);
		ctk_label_set_xalign (CTK_LABEL (essid_label), 0.0f);
		ctk_label_set_yalign (CTK_LABEL (essid_label), 0.5f);;
		ctk_label_set_xalign (CTK_LABEL (essid_text), 0.0f);
		ctk_label_set_yalign (CTK_LABEL (essid_text), 0.5f);
		ctk_label_set_selectable (CTK_LABEL (essid_text), TRUE);

		ctk_grid_attach (CTK_GRID (grid), signal_label, 2, 4, 1, 1);
		ctk_grid_attach (CTK_GRID (grid), CTK_WIDGET (applet->signalbar), 3, 4, 1, 1);
		ctk_grid_attach (CTK_GRID (grid), essid_label, 0, 4, 3, 1);
		ctk_grid_attach (CTK_GRID (grid), essid_text, 1, 4, 3, 1);
	}

	g_signal_connect(G_OBJECT(applet->drawingarea), "draw",
			 G_CALLBACK(da_draw),
			 (gpointer)applet);

	g_signal_connect(G_OBJECT(incolor_sel), "color_set",
			 G_CALLBACK(incolor_changed_cb),
			 (gpointer)applet);

	g_signal_connect(G_OBJECT(outcolor_sel), "color_set",
			 G_CALLBACK(outcolor_changed_cb),
			 (gpointer)applet);

	g_signal_connect(G_OBJECT(applet->details), "response",
			 G_CALLBACK(info_response_cb), (gpointer)applet);

	ctk_box_pack_start(CTK_BOX(box), da_frame, TRUE, TRUE, 0);
	ctk_box_pack_start(CTK_BOX(box), hbox, FALSE, FALSE, 0);
	ctk_box_pack_start(CTK_BOX(box), grid, FALSE, FALSE, 0);

	ctk_container_add(CTK_CONTAINER(ctk_dialog_get_content_area (applet->details)), box);
	ctk_widget_show_all(CTK_WIDGET(applet->details));
}

static const CtkActionEntry cafe_netspeed_applet_menu_actions [] = {
		{ "CafeNetspeedAppletDetails", "dialog-information", N_("Device _Details"),
		  NULL, NULL, G_CALLBACK (showinfo_cb) },
		{ "CafeNetspeedAppletProperties", "document-properties", N_("Preferences..."),
		  NULL, NULL, G_CALLBACK (settings_cb) },
		{ "CafeNetspeedAppletHelp", "help-browser", N_("Help"),
		  NULL, NULL, G_CALLBACK (help_cb) },
		{ "CafeNetspeedAppletAbout", "help-about", N_("About..."),
		  NULL, NULL, G_CALLBACK (about_cb) }
};

/* Block the size_request signal emit by the label if the
 * text changes. Only if the label wants to grow, we give
 * permission. This will eventually result in the maximal
 * size of the applet and prevents the icons and labels from
 * "jumping around" in the cafe_panel which looks uggly
 */
static void
label_size_allocate_cb(CtkWidget *widget, CtkAllocation *allocation, CafeNetspeedApplet *applet)
{
	if (applet->labels_dont_shrink) {
		if (allocation->width <= applet->width)
			allocation->width = applet->width;
		else
			applet->width = allocation->width;
	}
}

static gboolean
applet_button_press(CtkWidget *widget, CdkEventButton *event, CafeNetspeedApplet *applet)
{
	if (event->button == 1)
	{
		GError *error = NULL;

		if (applet->connect_dialog)
		{
			ctk_window_present(CTK_WINDOW(applet->connect_dialog));
			return FALSE;
		}

		if (applet->up_cmd && applet->down_cmd)
		{
			const char *question;
			int response;

			if (applet->devinfo.up)
			{
				question = _("Do you want to disconnect %s now?");
			}
			else
			{
				question = _("Do you want to connect %s now?");
			}

			applet->connect_dialog = ctk_message_dialog_new(NULL,
					CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
					CTK_MESSAGE_QUESTION, CTK_BUTTONS_YES_NO,
					question,
					applet->devinfo.name);
			response = ctk_dialog_run(CTK_DIALOG(applet->connect_dialog));
			ctk_widget_destroy (applet->connect_dialog);
			applet->connect_dialog = NULL;

			if (response == CTK_RESPONSE_YES)
			{
				CtkWidget *dialog;
				char *command;

				command = g_strdup_printf("%s %s",
					applet->devinfo.up ? applet->down_cmd : applet->up_cmd,
					applet->devinfo.name);

				if (!g_spawn_command_line_async(command, &error))
				{

					dialog = ctk_message_dialog_new_with_markup(NULL,
							CTK_DIALOG_MODAL | CTK_DIALOG_DESTROY_WITH_PARENT,
							CTK_MESSAGE_ERROR, CTK_BUTTONS_OK,
							_("<b>Running command %s failed</b>\n%s"),
							command,
							error->message);
					ctk_dialog_run (CTK_DIALOG (dialog));
					ctk_widget_destroy (dialog);
					g_error_free (error);
				}
				g_free(command);
			}
		}
	}

	return FALSE;
}

/* Frees the applet and all the data it contains
 * Removes the timeout_cb
 */
static void
applet_destroy(CafePanelApplet *applet_widget, CafeNetspeedApplet *applet)
{
	CtkIconTheme *icon_theme;

	g_assert(applet);

	icon_theme = ctk_icon_theme_get_default();
	g_object_disconnect(G_OBJECT(icon_theme), "changed",
			    G_CALLBACK(icon_theme_changed_cb), (gpointer)applet,
			    NULL);

	g_source_remove(applet->timeout_id);
	applet->timeout_id = 0;

	if (applet->up_cmd)
		g_free(applet->up_cmd);
	if (applet->down_cmd)
		g_free(applet->down_cmd);
	if (applet->gsettings)
		g_object_unref (applet->gsettings);

	/* Should never be NULL */
	free_device_info(&applet->devinfo);
	g_free(applet);
	return;
}



static void
update_tooltip(CafeNetspeedApplet* applet)
{
  GString* tooltip;

  if (!applet->show_tooltip)
    return;

  tooltip = g_string_new("");

  if (!applet->devinfo.running)
    g_string_printf(tooltip, _("%s is down"), applet->devinfo.name);
  else {
    if (applet->show_sum) {
      g_string_printf(
		      tooltip,
		      _("%s: %s\nin: %s out: %s"),
		      applet->devinfo.name,
		      applet->devinfo.ip ? applet->devinfo.ip : _("has no ip"),
		      applet->devinfo.rx_rate,
		      applet->devinfo.tx_rate
		      );
    } else {
      g_string_printf(
		      tooltip,
		      _("%s: %s\nsum: %s"),
		      applet->devinfo.name,
		      applet->devinfo.ip ? applet->devinfo.ip : _("has no ip"),
		      applet->devinfo.sum_rate
		      );
    }
    if (applet->devinfo.type == DEV_WIRELESS)
      g_string_append_printf(
			     tooltip,
			     _("\nESSID: %s\nStrength: %d %%"),
			     applet->devinfo.essid ? applet->devinfo.essid : _("unknown"),
			     applet->devinfo.qual
			     );

  }

  ctk_widget_set_tooltip_text(CTK_WIDGET(applet->applet), tooltip->str);
  ctk_widget_trigger_tooltip_query(CTK_WIDGET(applet->applet));
  g_string_free(tooltip, TRUE);
}


static gboolean
cafe_netspeed_enter_cb(CtkWidget *widget, CdkEventCrossing *event, gpointer data)
{
	CafeNetspeedApplet *applet = data;

	applet->show_tooltip = TRUE;
	update_tooltip(applet);

	return TRUE;
}

static gboolean
cafe_netspeed_leave_cb(CtkWidget *widget, CdkEventCrossing *event, gpointer data)
{
	CafeNetspeedApplet *applet = data;

	applet->show_tooltip = FALSE;
	return TRUE;
}

/* The "main" function of the applet
 */
static gboolean
cafe_netspeed_applet_factory(CafePanelApplet *applet_widget, const gchar *iid, gpointer data)
{
	CafeNetspeedApplet *applet;
	int i;
	char* menu_string;
	CtkIconTheme *icon_theme;
	CtkWidget *spacer, *spacer_box;

	/* Have our background automatically painted. */
	cafe_panel_applet_set_background_widget(CAFE_PANEL_APPLET(applet_widget),
		CTK_WIDGET(applet_widget));

	if (strcmp (iid, "NetspeedApplet"))
		return FALSE;

	glibtop_init();
	g_set_application_name (_("CAFE Netspeed"));

	g_object_set (ctk_settings_get_default (), "ctk-menu-images", TRUE, NULL);
	g_object_set (ctk_settings_get_default (), "ctk-button-images", TRUE, NULL);

	icon_theme = ctk_icon_theme_get_default();

	/* Alloc the applet. The "NULL-setting" is really redudant
 	 * but aren't we paranoid?
	 */
	applet = g_malloc0(sizeof(CafeNetspeedApplet));
	applet->applet = applet_widget;
	memset(&applet->devinfo, 0, sizeof(DevInfo));
	applet->refresh_time = 1000.0;
	applet->show_sum = FALSE;
	applet->show_bits = FALSE;
	applet->short_unit = FALSE;
	applet->show_icon = TRUE;
	applet->show_quality_icon = TRUE;
	applet->change_icon = TRUE;
	applet->auto_change_device = TRUE;

	/* Set the default colors of the graph
	*/
	applet->in_color.red = 0xdf00;
	applet->in_color.green = 0x2800;
	applet->in_color.blue = 0x4700;
	applet->out_color.red = 0x3700;
	applet->out_color.green = 0x2800;
	applet->out_color.blue = 0xdf00;

	for (i = 0; i < GRAPH_VALUES; i++)
	{
		applet->in_graph[i] = -1;
		applet->out_graph[i] = -1;
	}

	applet->gsettings = cafe_panel_applet_settings_new (applet_widget, "org.cafe.panel.applet.netspeed");

	/* Get stored settings from gsettings
	 */
	char *tmp = NULL;

	applet->show_sum = g_settings_get_boolean (applet->gsettings, "show-sum");
	applet->show_bits = g_settings_get_boolean (applet->gsettings, "show-bits");
	applet->short_unit = g_settings_get_boolean (applet->gsettings, "short-unit");
	applet->show_icon = g_settings_get_boolean (applet->gsettings, "show-icon");
	applet->show_quality_icon = g_settings_get_boolean (applet->gsettings, "show-quality-icon");
	applet->change_icon = g_settings_get_boolean (applet->gsettings, "change-icon");
	applet->auto_change_device = g_settings_get_boolean (applet->gsettings, "auto-change-device");

	tmp = g_settings_get_string (applet->gsettings, "device");
	if (tmp && strcmp(tmp, ""))
	{
		get_device_info(tmp, &applet->devinfo);
		g_free(tmp);
	}
	tmp = g_settings_get_string (applet->gsettings, "up-command");
	if (tmp && strcmp(tmp, ""))
	{
		applet->up_cmd = g_strdup(tmp);
		g_free(tmp);
	}
	tmp = g_settings_get_string (applet->gsettings, "down-command");
	if (tmp && strcmp(tmp, ""))
	{
		applet->down_cmd = g_strdup(tmp);
		g_free(tmp);
	}

	tmp = g_settings_get_string (applet->gsettings, "in-color");
	if (tmp)
	{
		cdk_rgba_parse (&applet->in_color, tmp);
		g_free(tmp);
	}
	tmp = g_settings_get_string (applet->gsettings, "out-color");
	if (tmp)
	{
		cdk_rgba_parse (&applet->out_color, tmp);
		g_free(tmp);
	}

	if (!applet->devinfo.name) {
		GList *ptr, *devices = get_available_devices();
		ptr = devices;
		while (ptr) {
			if (!g_str_equal(ptr->data, "lo")) {
				get_device_info(ptr->data, &applet->devinfo);
				break;
			}
			ptr = g_list_next(ptr);
		}
		free_devices_list(devices);
	}
	if (!applet->devinfo.name)
		get_device_info("lo", &applet->devinfo);
	applet->device_has_changed = TRUE;

	applet->in_label = ctk_label_new("");
	applet->out_label = ctk_label_new("");
	applet->sum_label = ctk_label_new("");

	applet->in_pix = ctk_image_new();
	applet->out_pix = ctk_image_new();
	applet->dev_pix = ctk_image_new();
	applet->qual_pix = ctk_image_new();

	applet->pix_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	spacer = ctk_label_new("");
	ctk_box_pack_start(CTK_BOX(applet->pix_box), spacer, TRUE, TRUE, 0);
	spacer = ctk_label_new("");
	ctk_box_pack_end(CTK_BOX(applet->pix_box), spacer, TRUE, TRUE, 0);

	spacer_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
	ctk_box_pack_start(CTK_BOX(applet->pix_box), spacer_box, FALSE, FALSE, 0);
	ctk_box_pack_start(CTK_BOX(spacer_box), applet->qual_pix, FALSE, FALSE, 0);
	ctk_box_pack_start(CTK_BOX(spacer_box), applet->dev_pix, FALSE, FALSE, 0);

	init_quality_surfaces(applet);

	applet_change_size_or_orient(applet_widget, -1, (gpointer)applet);
	ctk_widget_show_all(CTK_WIDGET(applet_widget));
	update_applet(applet);

	cafe_panel_applet_set_flags(applet_widget, CAFE_PANEL_APPLET_EXPAND_MINOR);

	applet->timeout_id = g_timeout_add(applet->refresh_time,
                           (GSourceFunc)timeout_function,
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet_widget), "change_size",
                           G_CALLBACK(applet_change_size_or_orient),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(icon_theme), "changed",
                           G_CALLBACK(icon_theme_changed_cb),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet_widget), "change_orient",
                           G_CALLBACK(applet_change_size_or_orient),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet->in_label), "size_allocate",
                           G_CALLBACK(label_size_allocate_cb),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet->out_label), "size_allocate",
                           G_CALLBACK(label_size_allocate_cb),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet->sum_label), "size_allocate",
                           G_CALLBACK(label_size_allocate_cb),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet_widget), "destroy",
                           G_CALLBACK(applet_destroy),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet_widget), "button-press-event",
                           G_CALLBACK(applet_button_press),
                           (gpointer)applet);

	g_signal_connect(G_OBJECT(applet_widget), "leave_notify_event",
			 G_CALLBACK(cafe_netspeed_leave_cb),
			 (gpointer)applet);

	g_signal_connect(G_OBJECT(applet_widget), "enter_notify_event",
			 G_CALLBACK(cafe_netspeed_enter_cb),
			 (gpointer)applet);

	CtkActionGroup *action_group;
	gchar *ui_path;
	action_group = ctk_action_group_new ("Netspeed Applet Actions");
	ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	ctk_action_group_add_actions (action_group,
                                  cafe_netspeed_applet_menu_actions,
                                  G_N_ELEMENTS (cafe_netspeed_applet_menu_actions),
                                  applet);
	ui_path = g_build_filename (NETSPEED_MENU_UI_DIR, "netspeed-menu.xml", NULL);
	cafe_panel_applet_setup_menu_from_file (CAFE_PANEL_APPLET (applet->applet), ui_path, action_group);
	g_free (ui_path);
	g_object_unref (action_group);

	return TRUE;
}

CAFE_PANEL_APPLET_OUT_PROCESS_FACTORY("NetspeedAppletFactory",
									  PANEL_TYPE_APPLET,
									  "NetspeedApplet",
									  cafe_netspeed_applet_factory,
									  NULL)
