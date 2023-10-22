/*
 * CAFE CPUFreq Applet
 * Copyright (C) 2004 Carlos Garcia Campos <carlosgc@gnome.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors : Carlos García Campos <carlosgc@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctk/ctk.h>
#include <cdk/cdkx.h>
#include <cdk/cdkkeysyms.h>
#include <gio/gio.h>
#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#include "cpufreq-applet.h"
#include "cpufreq-prefs.h"
#include "cpufreq-popup.h"
#include "cpufreq-monitor.h"
#include "cpufreq-monitor-factory.h"
#include "cpufreq-utils.h"

struct _CPUFreqApplet {
        CafePanelApplet       base;

        /* Visibility */
	CPUFreqShowMode   show_mode;
	CPUFreqShowTextMode show_text_mode;
        gboolean          show_freq;
        gboolean          show_perc;
        gboolean          show_unit;
        gboolean          show_icon;

        CPUFreqMonitor   *monitor;

        CafePanelAppletOrient orient;
        gint              size;

        CtkWidget        *label;
        CtkWidget        *unit_label;
        CtkWidget        *icon;
        CtkWidget        *box;
	CtkWidget        *labels_box;
        CtkWidget        *container;
        cairo_surface_t  *surfaces[5];

	gboolean          need_refresh;

        CPUFreqPrefs     *prefs;
        CPUFreqPopup     *popup;
};

struct _CPUFreqAppletClass {
        CafePanelAppletClass parent_class;
};

static void     cpufreq_applet_preferences_cb    (CtkAction          *action,
                                                  CPUFreqApplet      *applet);
static void     cpufreq_applet_help_cb           (CtkAction          *action,
                                                  CPUFreqApplet      *applet);
static void     cpufreq_applet_about_cb          (CtkAction          *action,
                                                  CPUFreqApplet      *applet);

static void     cpufreq_applet_pixmap_set_image  (CPUFreqApplet      *applet,
                                                  gint                perc);

static void     cpufreq_applet_setup             (CPUFreqApplet      *applet);
static void     cpufreq_applet_update            (CPUFreqApplet      *applet,
                                                  CPUFreqMonitor     *monitor);
static void     cpufreq_applet_refresh           (CPUFreqApplet      *applet);

static void     cpufreq_applet_dispose           (GObject            *widget);
static gboolean cpufreq_applet_button_press      (CtkWidget          *widget,
                                                  CdkEventButton     *event);
static gboolean cpufreq_applet_key_press         (CtkWidget          *widget,
                                                  CdkEventKey        *event);
static void     cpufreq_applet_size_allocate     (CtkWidget          *widget,
                                                  CtkAllocation      *allocation);
static void     cpufreq_applet_change_orient     (CafePanelApplet        *pa,
                                                  CafePanelAppletOrient   orient);
static void     cpufreq_applet_style_updated     (CtkWidget *widget);
static gboolean cpufreq_applet_factory           (CPUFreqApplet      *applet,
                                                  const gchar        *iid,
                                                  gpointer            gdata);

static const gchar* const cpufreq_icons[] = {
	CAFE_PIXMAPSDIR "/cafe-cpufreq-applet/cpufreq-25.png",
	CAFE_PIXMAPSDIR "/cafe-cpufreq-applet/cpufreq-50.png",
	CAFE_PIXMAPSDIR "/cafe-cpufreq-applet/cpufreq-75.png",
	CAFE_PIXMAPSDIR "/cafe-cpufreq-applet/cpufreq-100.png",
	CAFE_PIXMAPSDIR "/cafe-cpufreq-applet/cpufreq-na.png",
	NULL
};

static const CtkActionEntry cpufreq_applet_menu_actions[] = {
	{ "CPUFreqAppletPreferences", "document-properties", N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (cpufreq_applet_preferences_cb) },
	{ "CPUFreqAppletHelp", "help-browser", N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (cpufreq_applet_help_cb) },
	{ "CPUFreqAppletAbout", "help-about", N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (cpufreq_applet_about_cb) }
};

G_DEFINE_TYPE (CPUFreqApplet, cpufreq_applet, PANEL_TYPE_APPLET)

/* Enum Types */
GType
cpufreq_applet_show_mode_get_type (void)
{
        static GType etype = 0;

        if (etype == 0) {
                static const GEnumValue values[] = {
                        { CPUFREQ_MODE_GRAPHIC, "CPUFREQ_MODE_GRAPHIC", "mode-graphic" },
                        { CPUFREQ_MODE_TEXT,    "CPUFREQ_MODE_TEXT",    "mode-text" },
                        { CPUFREQ_MODE_BOTH,    "CPUFREQ_MODE_BOTH",    "mode-both" },
                        { 0, NULL, NULL }
                };

                etype = g_enum_register_static ("CPUFreqShowMode", values);
        }

        return etype;
}

GType
cpufreq_applet_show_text_mode_get_type (void)
{
        static GType etype = 0;

        if (etype == 0) {
                static const GEnumValue values[] = {
                        { CPUFREQ_MODE_TEXT_FREQUENCY,      "CPUFREQ_MODE_TEXT_FREQUENCY",      "mode-text-frequency" },
                        { CPUFREQ_MODE_TEXT_FREQUENCY_UNIT, "CPUFREQ_MODE_TEXT_FREQUENCY_UNIT", "mode-text-frequency-unit" },
                        { CPUFREQ_MODE_TEXT_PERCENTAGE,     "CPUFREQ_MODE_TEXT_PERCENTAGE",     "mode-text-percentage" },
                        { 0, NULL, NULL }
                };

                etype = g_enum_register_static ("CPUFreqShowTextMode", values);
        }

        return etype;
}

static void
cpufreq_applet_init (CPUFreqApplet *applet)
{
        applet->prefs = NULL;
        applet->popup = NULL;
        applet->monitor = NULL;

        applet->label = ctk_label_new (NULL);
        applet->unit_label = ctk_label_new (NULL);
        applet->icon = ctk_image_new ();
        applet->box = NULL;

	applet->show_mode = CPUFREQ_MODE_BOTH;
	applet->show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY_UNIT;

	applet->need_refresh = TRUE;

        cafe_panel_applet_set_flags (CAFE_PANEL_APPLET (applet), CAFE_PANEL_APPLET_EXPAND_MINOR);
	cafe_panel_applet_set_background_widget (CAFE_PANEL_APPLET (applet), CTK_WIDGET (applet));

        applet->size = cafe_panel_applet_get_size (CAFE_PANEL_APPLET (applet));
        applet->orient = cafe_panel_applet_get_orient (CAFE_PANEL_APPLET (applet));

	switch (applet->orient) {
	case CAFE_PANEL_APPLET_ORIENT_LEFT:
	case CAFE_PANEL_APPLET_ORIENT_RIGHT:
		applet->container = ctk_alignment_new (0.5, 0.5, 0, 0);
		break;
	case CAFE_PANEL_APPLET_ORIENT_UP:
	case CAFE_PANEL_APPLET_ORIENT_DOWN:
		applet->container = ctk_alignment_new (0, 0.5, 0, 0);
		break;
	}

	ctk_container_add (CTK_CONTAINER (applet), applet->container);
	ctk_widget_show (applet->container);
}

static void
cpufreq_applet_class_init (CPUFreqAppletClass *klass)
{
        CafePanelAppletClass *applet_class = CAFE_PANEL_APPLET_CLASS (klass);
        GObjectClass     *gobject_class = G_OBJECT_CLASS (klass);
        CtkWidgetClass   *widget_class = CTK_WIDGET_CLASS (klass);

        gobject_class->dispose = cpufreq_applet_dispose;

        widget_class->size_allocate = cpufreq_applet_size_allocate;
        widget_class->style_updated = cpufreq_applet_style_updated;
        widget_class->button_press_event = cpufreq_applet_button_press;
        widget_class->key_press_event = cpufreq_applet_key_press;

        applet_class->change_orient = cpufreq_applet_change_orient;
}

static void
cpufreq_applet_dispose (GObject *widget)
{
        CPUFreqApplet *applet;
        gint           i;

        applet = CPUFREQ_APPLET (widget);

        if (applet->monitor) {
                g_object_unref (G_OBJECT (applet->monitor));
                applet->monitor = NULL;
        }

        for (i = 0; i <= 3; i++) {
                if (applet->surfaces[i]) {
                        cairo_surface_destroy (applet->surfaces[i]);
                        applet->surfaces[i] = NULL;
                }
        }

        if (applet->prefs) {
                g_object_unref (applet->prefs);
                applet->prefs = NULL;
        }

        if (applet->popup) {
                g_object_unref (applet->popup);
                applet->popup = NULL;
        }

        G_OBJECT_CLASS (cpufreq_applet_parent_class)->dispose (widget);
}

static void
cpufreq_applet_size_allocate (CtkWidget *widget, CtkAllocation *allocation)
{
        CPUFreqApplet *applet;
        gint           size = 0;

        applet = CPUFREQ_APPLET (widget);

        switch (applet->orient) {
        case CAFE_PANEL_APPLET_ORIENT_LEFT:
        case CAFE_PANEL_APPLET_ORIENT_RIGHT:
                size = allocation->width;
                break;
        case CAFE_PANEL_APPLET_ORIENT_UP:
        case CAFE_PANEL_APPLET_ORIENT_DOWN:
                size = allocation->height;
                break;
        }

        if (size != applet->size) {
                applet->size = size;
                cpufreq_applet_refresh (applet);
        }

        CTK_WIDGET_CLASS (cpufreq_applet_parent_class)->size_allocate (widget, allocation);
}

static gint
get_max_text_width (CtkWidget *widget,
                    const char *text)
{
	PangoContext *context;
	PangoLayout *layout;
	PangoRectangle logical_rect;

	context = ctk_widget_get_pango_context (widget);
	layout = pango_layout_new (context);
	pango_layout_set_text (layout, text, -1);
	pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

	g_object_unref (layout);

	return logical_rect.width;
}

static void
cpufreq_applet_menu_popup (CPUFreqApplet *applet,
                           guint32        time)
{
        CtkWidget *menu;

        if (!cpufreq_utils_selector_is_available ())
                return;

        if (!applet->popup) {
                applet->popup = cpufreq_popup_new ();
                cpufreq_popup_set_monitor (applet->popup, applet->monitor);
		cpufreq_popup_set_parent (applet->popup, CTK_WIDGET (applet));
        }

        menu = cpufreq_popup_get_menu (applet->popup);

        if (!menu)
                return;
                
        /*Set up theme and transparency support*/
        CtkWidget *toplevel = ctk_widget_get_toplevel (menu);
        /* Fix any failures of compiz/other wm's to communicate with ctk for transparency */
        CdkScreen *screen = ctk_widget_get_screen(CTK_WIDGET(toplevel));
        CdkVisual *visual = cdk_screen_get_rgba_visual(screen);
        ctk_widget_set_visual(CTK_WIDGET(toplevel), visual);
        /* Set menu and it's toplevel window to follow panel theme */
        CtkStyleContext *context;
        context = ctk_widget_get_style_context (CTK_WIDGET(toplevel));
        ctk_style_context_add_class(context,"gnome-panel-menu-bar");
        ctk_style_context_add_class(context,"cafe-panel-menu-bar");

        ctk_menu_popup_at_widget (CTK_MENU (menu),
                                  CTK_WIDGET (applet),
                                  CDK_GRAVITY_SOUTH_WEST,
                                  CDK_GRAVITY_NORTH_WEST,
                                  NULL);
}

static gboolean
cpufreq_applet_button_press (CtkWidget *widget, CdkEventButton *event)
{
        CPUFreqApplet *applet;

        applet = CPUFREQ_APPLET (widget);

        if (event->button == 2)
                return FALSE;

        if (event->button == 1 &&
            event->type != CDK_2BUTTON_PRESS &&
            event->type != CDK_3BUTTON_PRESS) {
                cpufreq_applet_menu_popup (applet, event->time);

                return TRUE;
        }

        return CTK_WIDGET_CLASS (cpufreq_applet_parent_class)->button_press_event (widget, event);
}

static gboolean
cpufreq_applet_key_press (CtkWidget *widget, CdkEventKey *event)
{
        CPUFreqApplet *applet;

        applet = CPUFREQ_APPLET (widget);

        switch (event->keyval) {
        case CDK_KEY_KP_Enter:
        case CDK_KEY_ISO_Enter:
        case CDK_KEY_3270_Enter:
        case CDK_KEY_Return:
        case CDK_KEY_space:
        case CDK_KEY_KP_Space:
                cpufreq_applet_menu_popup (applet, event->time);

                return TRUE;
        default:
                break;
        }

        return FALSE;
}

static void
cpufreq_applet_change_orient (CafePanelApplet *pa, CafePanelAppletOrient orient)
{
        CPUFreqApplet *applet;
        CtkAllocation  allocation;
        gint           size;

        applet = CPUFREQ_APPLET (pa);

        applet->orient = orient;

        ctk_widget_get_allocation (CTK_WIDGET (applet), &allocation);

        if ((orient == CAFE_PANEL_APPLET_ORIENT_LEFT) ||
            (orient == CAFE_PANEL_APPLET_ORIENT_RIGHT)) {
                size = allocation.width;
		ctk_alignment_set (CTK_ALIGNMENT (applet->container),
				   0.5, 0.5, 0, 0);
        } else {
                size = allocation.height;
		ctk_alignment_set (CTK_ALIGNMENT (applet->container),
				   0, 0.5, 0, 0);
        }

        if (size != applet->size) {
                applet->size = size;
                cpufreq_applet_refresh (applet);
        }
}

static void
cpufreq_applet_style_updated (CtkWidget *widget)
{
        CPUFreqApplet *applet;

        applet = CPUFREQ_APPLET (widget);

        cpufreq_applet_refresh (applet);

        /*Reset label sizes to zero that have been held to maximum reached width*/
        ctk_widget_set_size_request (CTK_WIDGET (applet->label), 0, 0);
        ctk_widget_set_size_request (CTK_WIDGET (applet->unit_label), 0, 0);
}

static void
cpufreq_applet_preferences_cb (CtkAction     *action,
                               CPUFreqApplet *applet)
{
        cpufreq_preferences_dialog_run (applet->prefs,
                                        ctk_widget_get_screen (CTK_WIDGET (applet)));
}

static void
cpufreq_applet_help_cb (CtkAction     *action,
                        CPUFreqApplet *applet)
{
        GError *error = NULL;

	ctk_show_uri_on_window (NULL,
	                        "help:cafe-cpufreq-applet",
	                        ctk_get_current_event_time (),
	                        &error);

        if (error) {
                cpufreq_utils_display_error (_("Could not open help document"),
                                             error->message);
                g_error_free (error);
        }
}

static void
cpufreq_applet_about_cb (CtkAction     *action,
                         CPUFreqApplet *applet)
{
        static const gchar *const authors[] = {
                "Carlos Garcia Campos <carlosgc@gnome.org>",
                NULL
        };
        static const gchar* documenters[] = {
                "Carlos Garcia Campos <carlosgc@gnome.org>",
                "Davyd Madeley <davyd@madeley.id.au>",
                N_("CAFE Documentation Team"),
                NULL
        };
        static const gchar *const artists[] = {
                "Pablo Arroyo Loma <zzioma@yahoo.es>",
                NULL
        };

#ifdef ENABLE_NLS
	const char **p;
	for (p = documenters; *p; ++p)
		*p = _(*p);
#endif

        ctk_show_about_dialog (NULL,
                               "title",         _("About CPU Frequency Scaling Monitor"),
                               "version",       VERSION,
                               "copyright",     _("Copyright \xC2\xA9 2004 Carlos Garcia Campos\n"
                                                  "Copyright \xc2\xa9 2012-2020 CAFE developers"),
                               "comments",      _("This utility shows the current CPU "
                                                  "Frequency Scaling."),
                               "authors",       authors,
                               "documenters",   documenters,
                               "artists",       artists,
                               "translator-credits",    _("translator-credits"),
			       "logo-icon-name",        "cafe-cpu-frequency-applet",
                               NULL);
}

static void
cpufreq_applet_pixmap_set_image (CPUFreqApplet *applet, gint perc)
{
        gint image;
        gint scale;
        gint size = 24; /* FIXME */

        /* 0-29   -> 25%
         * 30-69  -> 50%
         * 70-89  -> 75%
         * 90-100 -> 100%
         */
        if (perc < 30)
                image = 0;
        else if ((perc >= 30) && (perc < 70))
                image = 1;
        else if ((perc >= 70) && (perc < 90))
                image = 2;
        else if ((perc >= 90) && (perc <= 100))
                image = 3;
        else
                image = 4;

        scale = ctk_widget_get_scale_factor (CTK_WIDGET (applet->icon));

        if (applet->surfaces[image] == NULL) {
                GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale (cpufreq_icons[image],
                                                                       size * scale,
                                                                       size * scale,
                                                                       TRUE,
                                                                       NULL);
                applet->surfaces[image] = cdk_cairo_surface_create_from_pixbuf (pixbuf, scale, NULL);
        }

        ctk_image_set_from_surface (CTK_IMAGE (applet->icon), applet->surfaces[image]);
}

static gboolean
refresh_cb (CPUFreqApplet *applet)
{
	cpufreq_applet_refresh (applet);

	return FALSE;
}

static void
cpufreq_applet_update_visibility (CPUFreqApplet *applet)
{
        CPUFreqShowMode     show_mode;
        CPUFreqShowTextMode show_text_mode;
        gboolean            show_freq = FALSE;
        gboolean            show_perc = FALSE;
        gboolean            show_unit = FALSE;
        gboolean            show_icon = FALSE;
        gboolean            changed = FALSE;
        gboolean            need_update = FALSE;

        show_mode = cpufreq_prefs_get_show_mode (applet->prefs);
        show_text_mode = cpufreq_prefs_get_show_text_mode (applet->prefs);

        if (show_mode != CPUFREQ_MODE_GRAPHIC) {
                show_icon = (show_mode == CPUFREQ_MODE_BOTH);

                switch (show_text_mode) {
                case CPUFREQ_MODE_TEXT_FREQUENCY:
                        show_freq = TRUE;
                        break;
                case CPUFREQ_MODE_TEXT_PERCENTAGE:
                        show_perc = TRUE;
                        break;
                case CPUFREQ_MODE_TEXT_FREQUENCY_UNIT:
                        show_freq = TRUE;
                        show_unit = TRUE;
                        break;
                }
        } else {
                show_icon = TRUE;
        }

	if (applet->show_mode != show_mode) {
		applet->show_mode = show_mode;
		need_update = TRUE;
	}

	if (applet->show_text_mode != show_text_mode) {
		applet->show_text_mode = show_text_mode;
		need_update = TRUE;
	}

        if (show_freq != applet->show_freq) {
                applet->show_freq = show_freq;
                changed = TRUE;
        }

        if (show_perc != applet->show_perc) {
                applet->show_perc = show_perc;
                changed = TRUE;
        }

        if (changed) {
                g_object_set (G_OBJECT (applet->label),
                              "visible",
                              applet->show_freq || applet->show_perc,
                              NULL);
        }

        if (show_unit != applet->show_unit) {
                applet->show_unit = show_unit;
                changed = TRUE;

                g_object_set (G_OBJECT (applet->unit_label),
                              "visible", applet->show_unit,
                              NULL);
        }

        if (show_icon != applet->show_icon) {
                applet->show_icon = show_icon;
                changed = TRUE;

                g_object_set (G_OBJECT (applet->icon),
                              "visible", applet->show_icon,
                              NULL);
        }

        if (changed)
		g_idle_add ((GSourceFunc)refresh_cb, applet);

        if (need_update)
                cpufreq_applet_update (applet, applet->monitor);
}

static void
cpufreq_applet_update (CPUFreqApplet *applet, CPUFreqMonitor *monitor)
{
        gchar       *text_mode = NULL;
        gchar       *freq_label, *unit_label;
        gint         freq;
        gint         perc;
        guint        cpu;
        CtkRequisition  req;
        const gchar *governor;

        cpu = cpufreq_monitor_get_cpu (monitor);
        freq = cpufreq_monitor_get_frequency (monitor);
        perc = cpufreq_monitor_get_percentage (monitor);
        governor = cpufreq_monitor_get_governor (monitor);

        freq_label = cpufreq_utils_get_frequency_label (freq);
        unit_label = cpufreq_utils_get_frequency_unit (freq);

        if (applet->show_freq) {
                /*Force the label to render if frequencies are not found right away*/
                if (freq_label == NULL){
                        ctk_label_set_text (CTK_LABEL (applet->label),"---");
                }
                else{
                        ctk_label_set_text (CTK_LABEL (applet->label), freq_label);
                }
                /*Hold the largest size set by any jumping text */
                ctk_widget_get_preferred_size (CTK_WIDGET (applet->label),&req, NULL);
                ctk_widget_set_size_request (CTK_WIDGET (applet->label),req.width, req.height);
        }

        if (applet->show_perc) {
                gchar *text_perc;

                text_perc = g_strdup_printf ("%d%%", perc);
                ctk_label_set_text (CTK_LABEL (applet->label), text_perc);
                g_free (text_perc);
        }

        if (applet->show_unit) {
                ctk_label_set_text (CTK_LABEL (applet->unit_label), unit_label);
                /*Hold the largest size set by MHZ or GHZ to prevent jumping */
                ctk_widget_get_preferred_size (CTK_WIDGET (applet->unit_label),&req, NULL);
                ctk_widget_set_size_request (CTK_WIDGET (applet->unit_label),req.width, req.height);
        }

        if (applet->show_icon) {
                cpufreq_applet_pixmap_set_image (applet, perc);
        }

	if (governor) {
		gchar *gov_text;

		gov_text = g_strdup (governor);
		gov_text[0] = g_ascii_toupper (gov_text[0]);
		text_mode = g_strdup_printf ("%s\n%s %s (%d%%)",
					     gov_text, freq_label,
					     unit_label, perc);
		g_free (gov_text);
	}

        g_free (freq_label);
        g_free (unit_label);

	if (text_mode) {
		gchar *text_tip;

		text_tip = cpufreq_utils_get_n_cpus () == 1 ?
			g_strdup_printf ("%s", text_mode) :
			g_strdup_printf ("CPU %u - %s", cpu, text_mode);
		g_free (text_mode);

		ctk_widget_set_tooltip_text (CTK_WIDGET (applet), text_tip);
		g_free (text_tip);
	}

        /* Call refresh only the first time */
        if (applet->need_refresh) {
                cpufreq_applet_refresh (applet);
		applet->need_refresh = FALSE;
        }
}

static void
cpufreq_applet_refresh (CPUFreqApplet *applet)
{
    gint      total_size = 0;
    gint      panel_size, label_size;
    gint      unit_label_size, pixmap_size;
    gint      size_step = 12;
    gboolean  horizontal;
    gboolean  do_unref = FALSE;

    panel_size = applet->size - 1; /* 1 pixel margin */

    horizontal = (applet->orient == CAFE_PANEL_APPLET_ORIENT_UP ||
             applet->orient == CAFE_PANEL_APPLET_ORIENT_DOWN);


       /* We want a fixed label size, the biggest */

    ctk_widget_get_preferred_width(CTK_WIDGET(applet->label), &label_size, NULL);
    total_size += label_size;

    ctk_widget_get_preferred_width(CTK_WIDGET(applet->unit_label), &unit_label_size, NULL);
    total_size += unit_label_size;

    ctk_widget_get_preferred_width(CTK_WIDGET(applet->icon), &pixmap_size, NULL);
    total_size += pixmap_size;

        if (applet->box) {
                do_unref = TRUE;
                g_object_ref (applet->icon);
                ctk_container_remove (CTK_CONTAINER (applet->box), applet->icon);
        if (applet->labels_box) {
                        g_object_ref (applet->label);
                        ctk_container_remove (CTK_CONTAINER (applet->labels_box), applet->label);
                        g_object_ref (applet->unit_label);
                        ctk_container_remove (CTK_CONTAINER (applet->labels_box), applet->unit_label);
                }
                ctk_widget_destroy (applet->box);
        }

    if (horizontal) {
        applet->labels_box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
        if ((label_size + pixmap_size) <= panel_size)
            applet->box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
        else
            applet->box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
    } else {
                if (total_size <= panel_size) {
                        applet->box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
                        applet->labels_box  = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
                } else if ((label_size + unit_label_size) <= (panel_size - size_step)) {
                        applet->box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
                        applet->labels_box  = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 2);
                } else {
                        applet->box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
                        applet->labels_box  = ctk_box_new (CTK_ORIENTATION_VERTICAL, 2);
                }
	}

        ctk_box_pack_start (CTK_BOX (applet->labels_box), applet->label, FALSE, FALSE, 0);
        ctk_box_pack_start (CTK_BOX (applet->labels_box), applet->unit_label, FALSE, FALSE, 0);
        ctk_box_pack_start (CTK_BOX (applet->box), applet->icon, FALSE, FALSE, 0);

        ctk_box_pack_start (CTK_BOX (applet->box), applet->labels_box, FALSE, FALSE, 0);
        ctk_widget_show (applet->labels_box);

        ctk_container_add (CTK_CONTAINER (applet->container), applet->box);
        ctk_widget_show (applet->box);

        if (do_unref) {
                g_object_unref (applet->label);
                g_object_unref (applet->unit_label);
                g_object_unref (applet->icon);
        }
}

/* Preferences callbacks */
static void
cpufreq_applet_prefs_cpu_changed (CPUFreqPrefs  *prefs,
				  GParamSpec    *arg1,
				  CPUFreqApplet *applet)
{
	cpufreq_monitor_set_cpu (applet->monitor,
				 cpufreq_prefs_get_cpu (applet->prefs));
}

static void
cpufreq_applet_prefs_show_mode_changed (CPUFreqPrefs  *prefs,
                                        GParamSpec    *arg1,
                                        CPUFreqApplet *applet)
{
        cpufreq_applet_update_visibility (applet);
}

static void
cpufreq_applet_setup (CPUFreqApplet *applet)
{
	CtkActionGroup *action_group;
	gchar          *ui_path;
        AtkObject      *atk_obj;
        gchar          *prefs_key;
	GSettings      *settings;

	g_set_application_name  (_("CPU Frequency Scaling Monitor"));

	ctk_window_set_default_icon_name ("cafe-cpu-frequency-applet");

	g_object_set (ctk_settings_get_default (), "ctk-menu-images", TRUE, NULL);
	g_object_set (ctk_settings_get_default (), "ctk-button-images", TRUE, NULL);

        /* Preferences */
        if (applet->prefs)
                g_object_unref (applet->prefs);

        settings = cafe_panel_applet_settings_new (CAFE_PANEL_APPLET (applet), "org.cafe.panel.applet.cpufreq");
        applet->prefs = cpufreq_prefs_new (settings);

	g_signal_connect (G_OBJECT (applet->prefs),
			  "notify::cpu",
			  G_CALLBACK (cpufreq_applet_prefs_cpu_changed),
			  (gpointer) applet);
        g_signal_connect (G_OBJECT (applet->prefs),
                          "notify::show-mode",
                          G_CALLBACK (cpufreq_applet_prefs_show_mode_changed),
                          (gpointer) applet);
        g_signal_connect (G_OBJECT (applet->prefs),
                          "notify::show-text-mode",
                          G_CALLBACK (cpufreq_applet_prefs_show_mode_changed),
                          (gpointer) applet);

        /* Monitor */
        applet->monitor = cpufreq_monitor_factory_create_monitor (
                cpufreq_prefs_get_cpu (applet->prefs));
        cpufreq_monitor_run (applet->monitor);
        g_signal_connect_swapped (G_OBJECT (applet->monitor), "changed",
                                  G_CALLBACK (cpufreq_applet_update),
                                  (gpointer) applet);

        /* Setup the menus */
	action_group = ctk_action_group_new ("CPUFreq Applet Actions");
	ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	ctk_action_group_add_actions (action_group,
				      cpufreq_applet_menu_actions,
				      G_N_ELEMENTS (cpufreq_applet_menu_actions),
				      applet);
	ui_path = g_build_filename (CPUFREQ_MENU_UI_DIR, "cpufreq-applet-menu.xml", NULL);
        cafe_panel_applet_setup_menu_from_file (CAFE_PANEL_APPLET (applet),
					   ui_path, action_group);
	g_free (ui_path);

        if (cafe_panel_applet_get_locked_down (CAFE_PANEL_APPLET (applet))) {
		CtkAction *action;

		action = ctk_action_group_get_action (action_group, "CPUFreqPreferences");
		ctk_action_set_visible (action, FALSE);
        }
	g_object_unref (action_group);

        atk_obj = ctk_widget_get_accessible (CTK_WIDGET (applet));

        if (CTK_IS_ACCESSIBLE (atk_obj)) {
                atk_object_set_name (atk_obj, _("CPU Frequency Scaling Monitor"));
                atk_object_set_description (atk_obj, _("This utility shows the current CPU Frequency"));
        }

	cpufreq_applet_update_visibility (applet);

	ctk_widget_show (CTK_WIDGET (applet));
}

static gboolean
cpufreq_applet_factory (CPUFreqApplet *applet, const gchar *iid, gpointer gdata)
{
        gboolean retval = FALSE;

        if (!strcmp (iid, "CPUFreqApplet")) {
                cpufreq_applet_setup (applet);

                retval = TRUE;
        }

        return retval;
}

CAFE_PANEL_APPLET_OUT_PROCESS_FACTORY ("CPUFreqAppletFactory",
				  CPUFREQ_TYPE_APPLET,
				  "cpufreq-applet",
				  (CafePanelAppletFactoryCallback) cpufreq_applet_factory,
				  NULL)
