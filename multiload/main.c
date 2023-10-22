/* CAFE multiload panel applet
 * (C) 1997 The Free Software Foundation
 *
 * Authors: Tim P. Gerla
 *          Martin Baulig
 *          Todd Kulesza
 *
 * With code from wmload.c, v0.9.2, apparently by Ryan Land, rland@bc1.com.
 *
 */

#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

#include <glibtop.h>
#include <ctk/ctk.h>
#include <cdk/cdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>

#include "global.h"

static void
about_cb (CtkAction       *action,
	  MultiloadApplet *ma)
{
    const gchar * const authors[] =
    {
		"Martin Baulig <martin@home-of-linux.org>",
		"Todd Kulesza <fflewddur@dropline.net>",
		"Benoît Dejean <TazForEver@dlfp.org>",
		"Davyd Madeley <davyd@madeley.id.au>",
		NULL
    };

    const gchar* documenters[] =
    {
		"Chee Bin HOH <cbhoh@gnome.org>",
		N_("Sun GNOME Documentation Team <gdocteam@sun.com>"),
		N_("CAFE Documentation Team"),
		NULL
    };

#ifdef ENABLE_NLS
    const char **p;
    for (p = documenters; *p; ++p)
        *p = _(*p);
#endif

    ctk_show_about_dialog (NULL,
	"title",        _("About System Monitor"),
	"version",	VERSION,
	"copyright",	_("Copyright \xc2\xa9 1999-2005 Free Software Foundation and others\n"
	                  "Copyright \xc2\xa9 2012-2020 CAFE developers"),
	"comments",	_("A system load monitor capable of displaying graphs "
			"for CPU, ram, and swap space use, plus network "
			"traffic."),
	"authors",	authors,
	"documenters",	documenters,
	"translator-credits",	_("translator-credits"),
	"logo-icon-name",	"utilities-system-monitor",
	NULL);
}

static void
help_cb (CtkAction       *action,
	 MultiloadApplet *ma)
{

 	GError *error = NULL;

	ctk_show_uri_on_window (NULL,
	                        "help:cafe-multiload",
	                        ctk_get_current_event_time (),
	                        &error);

    	if (error) { /* FIXME: the user needs to see this */
        	g_warning ("help error: %s\n", error->message);
        	g_error_free (error);
        	error = NULL;
    	}


}

/* run the full-scale system process monitor */

static void
start_procman (MultiloadApplet *ma)
{
	GError *error = NULL;
	GDesktopAppInfo *appinfo;
	gchar *monitor;
	CdkAppLaunchContext *launch_context;
	CdkDisplay *display;
	GAppInfo *app_info;
	CdkScreen *screen;

	g_return_if_fail (ma != NULL);

	monitor = g_settings_get_string (ma->settings, "system-monitor");
	if (monitor == NULL)
	        monitor = g_strdup ("cafe-system-monitor.desktop");

	screen = ctk_widget_get_screen (CTK_WIDGET (ma->applet));
	appinfo = g_desktop_app_info_new (monitor);
	if (appinfo) {
		CdkScreen *screen;
		CdkAppLaunchContext *context;
		screen = ctk_widget_get_screen (CTK_WIDGET (ma->applet));
		display = cdk_screen_get_display (screen);
		context = cdk_display_get_app_launch_context (display);
		cdk_app_launch_context_set_screen (context, screen);
		g_app_info_launch (G_APP_INFO (appinfo), NULL, G_APP_LAUNCH_CONTEXT (context), &error);
		g_object_unref (context);
		g_object_unref (appinfo);
	}
	else {
		app_info = g_app_info_create_from_commandline ("cafe-system-monitor",
							      _("Start system-monitor"),
							      G_APP_INFO_CREATE_NONE,
							      &error);

		if (!error) {
			display = cdk_screen_get_display (screen);
			launch_context = cdk_display_get_app_launch_context (display);
			cdk_app_launch_context_set_screen (launch_context, screen);
			g_app_info_launch (app_info, NULL, G_APP_LAUNCH_CONTEXT (launch_context), &error);

			g_object_unref (launch_context);
		}
	}
	g_free (monitor);

	if (error) {
		CtkWidget *dialog;

		dialog = ctk_message_dialog_new (NULL,
						 CTK_DIALOG_DESTROY_WITH_PARENT,
						 CTK_MESSAGE_ERROR,
						 CTK_BUTTONS_OK,
						 _("There was an error executing '%s': %s"),
						 "cafe-system-monitor",
						 error->message);

		g_signal_connect (dialog, "response",
				  G_CALLBACK (ctk_widget_destroy),
				  NULL);

		ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);
		ctk_window_set_screen (CTK_WINDOW (dialog),
				       ctk_widget_get_screen (CTK_WIDGET (ma->applet)));

		ctk_widget_show (dialog);

		g_error_free (error);
	}
}

static void
start_procman_cb (CtkAction       *action,
		  MultiloadApplet *ma)
{
	start_procman (ma);
}

static void
multiload_change_size_cb(CafePanelApplet *applet, gint size, gpointer data)
{
	MultiloadApplet *ma = (MultiloadApplet *)data;

	multiload_applet_refresh(ma);

	return;
}

static void
multiload_change_orient_cb(CafePanelApplet *applet, gint arg1, gpointer data)
{
	MultiloadApplet *ma = data;
	multiload_applet_refresh((MultiloadApplet *)data);
	ctk_widget_show (CTK_WIDGET (ma->applet));
	return;
}

static void
multiload_destroy_cb(CtkWidget *widget, gpointer data)
{
	gint i;
	MultiloadApplet *ma = data;

	for (i = 0; i < NGRAPHS; i++)
	{
		load_graph_stop(ma->graphs[i]);
		if (ma->graphs[i]->colors)
		{
			g_free (ma->graphs[i]->colors);
			ma->graphs[i]->colors = NULL;
		}
		ctk_widget_destroy(ma->graphs[i]->main_widget);

		load_graph_unalloc(ma->graphs[i]);
		g_free(ma->graphs[i]);
	}

	if (ma->about_dialog)
		ctk_widget_destroy (ma->about_dialog);

	if (ma->prop_dialog)
		ctk_widget_destroy (ma->prop_dialog);

	ctk_widget_destroy(CTK_WIDGET(ma->applet));

	g_free (ma);

	return;
}


static gboolean
multiload_button_press_event_cb (CtkWidget *widget, CdkEventButton *event, MultiloadApplet *ma)
{
	g_return_val_if_fail (event != NULL, FALSE);
	g_return_val_if_fail (ma != NULL, FALSE);

	if (event->button == 1 && event->type == CDK_BUTTON_PRESS) {
		start_procman (ma);
		return TRUE;
	}
	return FALSE;
}

static gboolean
multiload_key_press_event_cb (CtkWidget *widget, CdkEventKey *event, MultiloadApplet *ma)
{
	g_return_val_if_fail (event != NULL, FALSE);
	g_return_val_if_fail (ma != NULL, FALSE);

	switch (event->keyval) {
	/* this list of keyvals taken from mixer applet, which seemed to have
		a good list of keys to use */
	case CDK_KEY_KP_Enter:
	case CDK_KEY_ISO_Enter:
	case CDK_KEY_3270_Enter:
	case CDK_KEY_Return:
	case CDK_KEY_space:
	case CDK_KEY_KP_Space:
		/* activate */
		start_procman (ma);
		return TRUE;

	default:
		break;
	}

	return FALSE;
}

/* update the tooltip to the graph's current "used" percentage */
void
multiload_applet_tooltip_update(LoadGraph *g)
{
	gchar *tooltip_text, *name;

	g_assert(g);
	g_assert(g->name);

	/* label the tooltip intuitively */
	if (!strncmp(g->name, "cpuload", strlen("cpuload")))
		name = g_strdup(_("Processor"));
	else if (!strncmp(g->name, "memload", strlen("memload")))
		name = g_strdup(_("Memory"));
	else if (!strncmp(g->name, "netload2", strlen("netload2")))
		name = g_strdup(_("Network"));
	else if (!strncmp(g->name, "swapload", strlen("swapload")))
		name = g_strdup(_("Swap Space"));
	else if (!strncmp(g->name, "loadavg", strlen("loadavg")))
		name = g_strdup(_("Load Average"));
	else if (!strncmp (g->name, "diskload", strlen("diskload")))
		name = g_strdup(_("Disk"));
	else
		g_assert_not_reached();

	if (!strncmp(g->name, "memload", strlen("memload"))) {
		guint mem_user, mem_cache, user_percent, cache_percent;
		mem_user  = g->data[1][0];
		mem_cache = g->data[1][1] + g->data[1][2] + g->data[1][3];
		user_percent = 100.0f * mem_user / g->draw_height;
		cache_percent = 100.0f * mem_cache / g->draw_height;
		user_percent = MIN(user_percent, 100);
		cache_percent = MIN(cache_percent, 100);

		/* xgettext: use and cache are > 1 most of the time,
		   please assume that they always are.
		 */
		tooltip_text = g_strdup_printf(_("%s:\n"
						 "%u%% in use by programs\n"
						 "%u%% in use as cache"),
					       name,
					       user_percent,
					       cache_percent);
	} else if (!strcmp(g->name, "loadavg")) {

		tooltip_text = g_strdup_printf(_("The system load average is %0.02f"),
					       g->loadavg1);

	} else if (!strcmp(g->name, "netload2")) {
		char *tx_in, *tx_out;
		tx_in = netspeed_get(g->netspeed_in);
		tx_out = netspeed_get(g->netspeed_out);
		/* xgettext: same as in graphic tab of g-s-m */
		tooltip_text = g_strdup_printf(_("%s:\n"
						 "Receiving %s\n"
						 "Sending %s"),
					       name, tx_in, tx_out);
		g_free(tx_in);
		g_free(tx_out);
	} else {
		const char *msg;
		guint i, total_used, percent;

		for (i = 0, total_used = 0; i < (g->n - 1); i++)
			total_used += g->data[1][i];

		percent = 100.0f * total_used / g->draw_height;
		percent = MIN(percent, 100);

		msg = ngettext("%s:\n"
			       "%u%% in use",
			       "%s:\n"
			       "%u%% in use",
			       percent);

		tooltip_text = g_strdup_printf(msg,
					       name,
					       percent);
	}

	ctk_widget_set_tooltip_text(g->disp, tooltip_text);

	g_free(tooltip_text);
	g_free(name);
}

static void
multiload_create_graphs(MultiloadApplet *ma)
{
	struct { const char *label;
		 const char *name;
		 int num_colours;
		 LoadGraphDataFunc callback;
	       } graph_types[] = {
			{ _("CPU Load"),     "cpuload",  5, GetLoad },
			{ _("Memory Load"),  "memload",  5, GetMemory },
			{ _("Net Load"),     "netload2",  6, GetNet },
			{ _("Swap Load"),    "swapload", 2, GetSwap },
			{ _("Load Average"), "loadavg",  3, GetLoadAvg },
			{ _("Disk Load"),    "diskload", 3, GetDiskLoad }
		};

	gint speed, size;
	guint net_threshold1;
	guint net_threshold2;
	guint net_threshold3;
	gint i;

	speed = g_settings_get_int (ma->settings, "speed");
	size = g_settings_get_int (ma->settings, "size");
	net_threshold1  = g_settings_get_uint (ma->settings, "netthreshold1");
	net_threshold2  = g_settings_get_uint (ma->settings, "netthreshold2");
	net_threshold3  = g_settings_get_uint (ma->settings, "netthreshold3");
	if (net_threshold1 >= net_threshold2)
	{
	   net_threshold1 = net_threshold2 - 1;
	}
	if (net_threshold2 >= net_threshold3)
	{
	   net_threshold3 = net_threshold2 + 1;
	}
	speed = MAX (speed, 50);
	size = CLAMP (size, 10, 400);

	for (i = 0; i < G_N_ELEMENTS (graph_types); i++)
	{
		gboolean visible;
		char *key;

		/* This is a special case to handle migration from an
		 * older version of netload to a newer one in the
		 * 2.25.1 release. */
		if (g_strcmp0 ("netload2", graph_types[i].name) == 0) {
		  key = g_strdup ("view-netload");
		} else {
		  key = g_strdup_printf ("view-%s", graph_types[i].name);
		}
		visible = g_settings_get_boolean (ma->settings, key);
		g_free (key);

		ma->graphs[i] = load_graph_new (ma,
				                graph_types[i].num_colours,
						graph_types[i].label,
                                                i,
						speed,
						size,
						visible,
						graph_types[i].name,
						graph_types[i].callback);
	}
	/* for Network graph, colors[4] is grid line color, it should not be used in loop in load-graph.c */
	/* for Network graph, colors[5] is indicator color, it should not be used in loop in load-graph.c */
	ma->graphs[2]->n = 4;
	ma->graphs[2]->net_threshold1 = net_threshold1;
	ma->graphs[2]->net_threshold2 = net_threshold2;
	ma->graphs[2]->net_threshold3 = net_threshold3;
	/* for Load graph, colors[2] is grid line color, it should not be used in loop in load-graph.c */
	ma->graphs[4]->n = 2;
}

/* remove the old graphs and rebuild them */
void
multiload_applet_refresh(MultiloadApplet *ma)
{
	gint i;
	CafePanelAppletOrient orientation;

	/* stop and free the old graphs */
	for (i = 0; i < NGRAPHS; i++)
	{
		if (!ma->graphs[i])
			continue;

		load_graph_stop(ma->graphs[i]);
		ctk_widget_destroy(ma->graphs[i]->main_widget);

		load_graph_unalloc(ma->graphs[i]);
		g_free(ma->graphs[i]);
	}

	if (ma->box)
		ctk_widget_destroy(ma->box);

	orientation = cafe_panel_applet_get_orient(ma->applet);

	if ( (orientation == CAFE_PANEL_APPLET_ORIENT_UP) ||
	     (orientation == CAFE_PANEL_APPLET_ORIENT_DOWN) ) {
		ma->box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	}
	else
		ma->box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

	ctk_container_add(CTK_CONTAINER(ma->applet), ma->box);

	/* create the NGRAPHS graphs, passing in their user-configurable properties with gsettings. */
	multiload_create_graphs (ma);

	/* only start and display the graphs the user has turned on */

	for (i = 0; i < NGRAPHS; i++) {
	    ctk_box_pack_start(CTK_BOX(ma->box),
			       ma->graphs[i]->main_widget,
			       TRUE, TRUE, 1);
	    if (ma->graphs[i]->visible) {
	    	ctk_widget_show_all (ma->graphs[i]->main_widget);
		load_graph_start(ma->graphs[i]);
	    }
	}
	ctk_widget_show (ma->box);

	return;
}

static const CtkActionEntry multiload_menu_actions [] = {
	{ "MultiLoadProperties", "document-properties", N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (multiload_properties_cb) },
	{ "MultiLoadRunProcman", "system-run", N_("_Open System Monitor"),
	  NULL, NULL,
	  G_CALLBACK (start_procman_cb) },
	{ "MultiLoadHelp", "help-browser", N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (help_cb) },
	{ "MultiLoadAbout", "help-about", N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (about_cb) }
};

/* create a box and stuff the load graphs inside of it */
static gboolean
multiload_applet_new(CafePanelApplet *applet, const gchar *iid, gpointer data)
{
	CtkStyleContext *context;
	MultiloadApplet *ma;
	GSettings *lockdown_settings;
	CtkActionGroup *action_group;
	gchar *ui_path;

	context = ctk_widget_get_style_context (CTK_WIDGET (applet));
	ctk_style_context_add_class (context, "multiload-applet");

	ma = g_new0(MultiloadApplet, 1);

	ma->applet = applet;

	ma->about_dialog = NULL;
	ma->prop_dialog = NULL;
        ma->last_clicked = 0;

	g_set_application_name (_("System Monitor"));

	ctk_window_set_default_icon_name ("utilities-system-monitor");

	g_object_set (ctk_settings_get_default (), "ctk-menu-images", TRUE, NULL);
	g_object_set (ctk_settings_get_default (), "ctk-button-images", TRUE, NULL);

	cafe_panel_applet_set_background_widget (applet, CTK_WIDGET(applet));

	ma->settings = cafe_panel_applet_settings_new (applet, "org.cafe.panel.applet.multiload");
	cafe_panel_applet_set_flags (applet, CAFE_PANEL_APPLET_EXPAND_MINOR);

	action_group = ctk_action_group_new ("Multiload Applet Actions");
	ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	ctk_action_group_add_actions (action_group,
				      multiload_menu_actions,
				      G_N_ELEMENTS (multiload_menu_actions),
				      ma);
	ui_path = g_build_filename (MULTILOAD_MENU_UI_DIR, "multiload-applet-menu.xml", NULL);
	cafe_panel_applet_setup_menu_from_file (applet, ui_path, action_group);
	g_free (ui_path);


	if (cafe_panel_applet_get_locked_down (applet)) {
		CtkAction *action;

		action = ctk_action_group_get_action (action_group, "MultiLoadProperties");
		ctk_action_set_visible (action, FALSE);
	}

	lockdown_settings = g_settings_new ("org.cafe.lockdown");
	if (g_settings_get_boolean (lockdown_settings, "disable-command-line") ||
	    cafe_panel_applet_get_locked_down (applet)) {
		CtkAction *action;

		/* When the panel is locked down or when the command line is inhibited,
		   it seems very likely that running the procman would be at least harmful */
		action = ctk_action_group_get_action (action_group, "MultiLoadRunProcman");
		ctk_action_set_visible (action, FALSE);
	}
	g_object_unref (lockdown_settings);

	g_object_unref (action_group);

	g_signal_connect(G_OBJECT(applet), "change_size",
				G_CALLBACK(multiload_change_size_cb), ma);
	g_signal_connect(G_OBJECT(applet), "change_orient",
				G_CALLBACK(multiload_change_orient_cb), ma);
	g_signal_connect(G_OBJECT(applet), "destroy",
				G_CALLBACK(multiload_destroy_cb), ma);
	g_signal_connect(G_OBJECT(applet), "button_press_event",
				G_CALLBACK(multiload_button_press_event_cb), ma);
	g_signal_connect(G_OBJECT(applet), "key_press_event",
				G_CALLBACK(multiload_key_press_event_cb), ma);

	multiload_applet_refresh (ma);

	ctk_widget_show(CTK_WIDGET(applet));

	return TRUE;
}

static gboolean
multiload_factory (CafePanelApplet *applet,
				const gchar *iid,
				gpointer data)
{
	gboolean retval = FALSE;

	glibtop_init();

	retval = multiload_applet_new(applet, iid, data);

	return retval;
}

CAFE_PANEL_APPLET_OUT_PROCESS_FACTORY ("MultiLoadAppletFactory",
				  PANEL_TYPE_APPLET,
				  "multiload",
				  multiload_factory,
				  NULL)
