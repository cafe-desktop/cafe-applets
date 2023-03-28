/* CAFE cpuload/memload panel applet
 * (C) 2002 The Free Software Foundation
 *
 * Authors:
 *		  Todd Kulesza
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <ctk/ctk.h>

#include <gio/gio.h>
#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>

#include "global.h"

#define PROP_CPU		0
#define PROP_MEM		1
#define PROP_NET		2
#define PROP_SWAP		3
#define PROP_AVG		4
#define PROP_DISK		5

#define PROP_SPEED		6
#define PROP_SIZE		7
#define PROP_NET_THRESHOLD1   8
#define PROP_NET_THRESHOLD2   9
#define PROP_NET_THRESHOLD3   10
#define HIG_IDENTATION		"    "
#define NEVER_SENSITIVE		"never_sensitive"

/* set sensitive and setup NEVER_SENSITIVE appropriately */
static void
hard_set_sensitive (CtkWidget *w, gboolean sensitivity)
{
	ctk_widget_set_sensitive (w, sensitivity);
	g_object_set_data (G_OBJECT (w), NEVER_SENSITIVE,
			   GINT_TO_POINTER ( ! sensitivity));
}


/* set sensitive, but always insensitive if NEVER_SENSITIVE is set */
static void
soft_set_sensitive (CtkWidget *w, gboolean sensitivity)
{
	if (g_object_get_data (G_OBJECT (w), NEVER_SENSITIVE))
		ctk_widget_set_sensitive (w, FALSE);
	else
		ctk_widget_set_sensitive (w, sensitivity);
}

static void
properties_set_insensitive(MultiloadApplet *ma)
{
	gint i, total_graphs, last_graph;

	total_graphs = 0;
	last_graph = 0;

	for (i = 0; i < NGRAPHS; i++)
		if (ma->graphs[i]->visible)
		{
			last_graph = i;
			total_graphs++;
		}

	if (total_graphs < 2)
		soft_set_sensitive(ma->check_boxes[last_graph], FALSE);

	return;
}

static void
properties_close_cb (CtkWidget *widget, gint arg, MultiloadApplet *ma)
{
	GError *error = NULL;

	switch (arg)
	{
		case GTK_RESPONSE_HELP:

			ctk_show_uri_on_window (NULL,
			                        "help:cafe-multiload/multiload-prefs",
			                        ctk_get_current_event_time (),
			                        &error);

			if (error) { /* FIXME: the user needs to see this */
				g_warning ("help error: %s\n", error->message);
				g_error_free (error);
				error = NULL;
			}
			break;

		case GTK_RESPONSE_CLOSE:
		default:
			ctk_widget_destroy (widget);
			ma->prop_dialog = NULL;
	}
}

static void
property_toggled_cb(CtkWidget *widget, gpointer name)
{
	MultiloadApplet *ma;
	gint prop_type, i;
	gboolean active = ctk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	ma = g_object_get_data(G_OBJECT(widget), "MultiloadApplet");
	prop_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "prop_type"));

	/* FIXME: the first toggle button to be checked/dechecked does not work, but after that everything is cool.  what gives? */
	/* FIXME: check if this is still valid for gsettings */
	g_settings_set_boolean (ma->settings, (gchar *)name, active);
	g_settings_set_boolean (ma->settings, (gchar *)name, active);

	if (active)
	{
		for (i = 0; i < NGRAPHS; i++)
			soft_set_sensitive(ma->check_boxes[i], TRUE);
		ctk_widget_show_all (ma->graphs[prop_type]->main_widget);
		ma->graphs[prop_type]->visible = TRUE;
		load_graph_start(ma->graphs[prop_type]);
	}
	else
	{
		load_graph_stop(ma->graphs[prop_type]);
		ctk_widget_hide (ma->graphs[prop_type]->main_widget);
		ma->graphs[prop_type]->visible = FALSE;
		properties_set_insensitive(ma);
	}

	return;
}

static void
spin_button_changed_cb(CtkWidget *widget, gpointer name)
{
  MultiloadApplet *ma;
  gint value;
  gint prop_type, i;

  ma = g_object_get_data(G_OBJECT(widget), "MultiloadApplet");
  prop_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "prop_type"));
  value = ctk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

  switch(prop_type)
  {
    case PROP_SPEED:
      g_settings_set_int (ma->settings, (gchar *)name, value);
      for (i = 0; i < NGRAPHS; i++)
      {
        load_graph_stop(ma->graphs[i]);
        ma->graphs[i]->speed = value;
        if (ma->graphs[i]->visible)
          load_graph_start(ma->graphs[i]);
      }

      break;

    case PROP_SIZE:
      for (i = 0; i < NGRAPHS; i++)
      {
        g_settings_set_int (ma->settings, (gchar *)name, value);
        ma->graphs[i]->size = value ;

        if (ma->graphs[i]->orient)
          ctk_widget_set_size_request (
            ma->graphs[i]->main_widget,
            ma->graphs[i]->pixel_size,
            ma->graphs[i]->size);
        else
          ctk_widget_set_size_request (
            ma->graphs[i]->main_widget,
            ma->graphs[i]->size,
            ma->graphs[i]->pixel_size);
      }
			break;

    case PROP_NET_THRESHOLD1:
      g_settings_set_uint (ma->settings, (gchar *)name, value);
      if (value >= ma->graphs[2]->net_threshold2)
      {
        ctk_spin_button_set_value(GTK_SPIN_BUTTON(widget),
            (gdouble)g_settings_get_uint (ma->settings, "netthreshold2") - 1);
        ma->graphs[2]->net_threshold1 = g_settings_get_uint (ma->settings, "netthreshold2") - 1;
      }
      else
        ma->graphs[2]->net_threshold1 = value;
  		break;

    case PROP_NET_THRESHOLD2:
      g_settings_set_uint (ma->settings, (gchar *)name, value);
      if (value >= ma->graphs[2]->net_threshold3)
      {
        ctk_spin_button_set_value(GTK_SPIN_BUTTON(widget),
            (gdouble)g_settings_get_uint (ma->settings, "netthreshold3") - 1);
        ma->graphs[2]->net_threshold2 = g_settings_get_uint (ma->settings, "netthreshold3") - 1;
      }
      else if (value <= ma->graphs[2]->net_threshold1)
      {
        ctk_spin_button_set_value(GTK_SPIN_BUTTON(widget),
            (gdouble)g_settings_get_uint (ma->settings, "netthreshold1") + 1);
        ma->graphs[2]->net_threshold2 = g_settings_get_uint (ma->settings, "netthreshold1") + 1;
      }
      else
        ma->graphs[2]->net_threshold2 = value;

  		break;

    case PROP_NET_THRESHOLD3:
      g_settings_set_uint (ma->settings, (gchar *)name, value);
      if (value <= ma->graphs[2]->net_threshold2)
      {
        ctk_spin_button_set_value(GTK_SPIN_BUTTON(widget),
            (gdouble)g_settings_get_uint (ma->settings, "netthreshold2") + 1);
        ma->graphs[2]->net_threshold3 = g_settings_get_uint (ma->settings, "netthreshold2") + 1;
      }
      else
        ma->graphs[2]->net_threshold3 = value;
      break;
    default:
      g_assert_not_reached();
  }

	return;
}

/* create a new page in the notebook widget, add it, and return a pointer to it */
static CtkWidget *
add_page(CtkWidget *notebook, gchar *label)
{
	CtkWidget *page;
	CtkWidget *page_label;

	page = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_set_homogeneous (GTK_BOX (page), TRUE);
	page_label = ctk_label_new(label);
	ctk_container_set_border_width(GTK_CONTAINER(page), 6);

	ctk_notebook_append_page(GTK_NOTEBOOK(notebook), page, page_label);

	return page;
}

/* save the selected color to gsettings and apply it on the applet */
static void
color_picker_set_cb(CtkColorChooser *color_picker, gchar *key)
{
	gchar *color_string;
	guint8 prop_type;
	GdkRGBA color;
	MultiloadApplet *ma;

	ma = g_object_get_data (G_OBJECT (color_picker), "MultiloadApplet");

	if (strstr(key, "cpuload"))
		prop_type = PROP_CPU;
	else if (strstr(key, "memload"))
		prop_type = PROP_MEM;
	else if (strstr(key, "netload2"))
		prop_type = PROP_NET;
	else if (strstr(key, "swapload"))
		prop_type = PROP_SWAP;
	else if (strstr(key, "loadavg"))
		prop_type = PROP_AVG;
	else if (strstr(key, "diskload"))
		prop_type = PROP_DISK;
	else
		g_assert_not_reached();

	ctk_color_chooser_get_rgba(color_picker, &color);

	color_string = gdk_rgba_to_string (&color);
	g_settings_set_string(ma->settings, key, color_string);

	gdk_rgba_parse(&(ma->graphs[prop_type]->colors[g_ascii_digit_value(key[strlen(key) - 1]) ]),
					color_string);

	return;
}

/* create a color selector */
static void
add_color_selector(CtkWidget *page, gchar *name, gchar *key, MultiloadApplet *ma)
{
	CtkWidget *vbox;
	CtkWidget *label;
	CtkWidget *color_picker;
	GdkRGBA color;
	gchar *color_string;

	color_string = g_settings_get_string (ma->settings, key);
	if (!color_string)
		color_string = g_strdup ("#000000");
	gdk_rgba_parse (&color, color_string);
	g_free (color_string);

	vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	label = ctk_label_new_with_mnemonic(name);
	color_picker = ctk_color_button_new();
	ctk_label_set_mnemonic_widget (GTK_LABEL (label), color_picker);

	ctk_box_pack_start(GTK_BOX(vbox), color_picker, FALSE, FALSE, 0);
	ctk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	ctk_box_pack_start(GTK_BOX(page), vbox, FALSE, FALSE, 0);

	g_object_set_data (G_OBJECT (color_picker), "MultiloadApplet", ma);

	ctk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color_picker), &color);

	g_signal_connect(G_OBJECT(color_picker), "color_set", G_CALLBACK(color_picker_set_cb), key);

	if ( ! g_settings_is_writable (ma->settings, key))
		hard_set_sensitive (vbox, FALSE);

	return;
}

/* save the checkbox option to gsettings and apply it on the applet */
static void
checkbox_toggled_cb(CtkCheckButton *checkbox, gchar *key)
{
	MultiloadApplet *ma;
	gboolean option;

	ma = g_object_get_data (G_OBJECT (checkbox), "MultiloadApplet");

	option = g_settings_get_boolean(ma->settings, key);
	g_settings_set_boolean(ma->settings, key, !option);

	return;
}

/* adds checkbox option */
static void
add_checkbox(CtkWidget *page, gchar *name, gchar *key, MultiloadApplet *ma)
{
	CtkWidget *vbox;
	CtkWidget *checkbox;
	gboolean option;

	option = g_settings_get_boolean (ma->settings, key);

	vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	checkbox = ctk_check_button_new_with_mnemonic (name);
	ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkbox), option);

	ctk_box_pack_start(GTK_BOX(vbox), checkbox, FALSE, FALSE, 0);

	ctk_box_pack_start(GTK_BOX(page), vbox, FALSE, FALSE, 0);

	g_object_set_data (G_OBJECT (checkbox), "MultiloadApplet", ma);

	g_signal_connect(G_OBJECT(checkbox), "toggled", G_CALLBACK(checkbox_toggled_cb), key);

	if ( ! g_settings_is_writable (ma->settings, key))
		hard_set_sensitive (vbox, FALSE);

	return;
}

/* creates the properties dialog using up-to-the-minute info from gsettings */
static void
fill_properties(CtkWidget *dialog, MultiloadApplet *ma)
{
	CtkWidget *page;
	CtkWidget *hbox, *vbox;
	CtkWidget *categories_vbox;
	CtkWidget *category_vbox;
	CtkWidget *control_vbox;
	CtkWidget *control_hbox;
	CtkWidget *check_box;
	CtkWidget *indent;
	CtkWidget *spin_button;
	CtkWidget *label;
	CafePanelAppletOrient orient;
	CtkSizeGroup *label_size;
	CtkSizeGroup *spin_size;
	gchar *label_text;
	gchar *title;

	vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	ctk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	ctk_widget_show (vbox);

	ctk_box_pack_start (GTK_BOX (ctk_dialog_get_content_area (GTK_DIALOG (dialog))), vbox,
			    TRUE, TRUE, 0);

	categories_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 18);
	ctk_box_pack_start (GTK_BOX (vbox), categories_vbox, TRUE, TRUE, 0);
	ctk_widget_show (categories_vbox);

	category_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (GTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);
	ctk_widget_show (category_vbox);

	title = g_strconcat ("<span weight=\"bold\">", _("Monitored Resources"), "</span>", NULL);
	label = ctk_label_new_with_mnemonic (_(title));
	ctk_label_set_use_markup (GTK_LABEL (label), TRUE);
	ctk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (GTK_LABEL (label), 0.0);
	ctk_box_pack_start (GTK_BOX (category_vbox), label, FALSE, FALSE, 0);
	g_free (title);

	hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_pack_start (GTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	indent = ctk_label_new (HIG_IDENTATION);
	ctk_label_set_justify (GTK_LABEL (indent), GTK_JUSTIFY_LEFT);
	ctk_box_pack_start (GTK_BOX (hbox), indent, FALSE, FALSE, 0);
	ctk_widget_show (indent);

	control_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (GTK_BOX (hbox), control_vbox, TRUE, TRUE, 0);
	ctk_widget_show (control_vbox);

	control_hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	ctk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	ctk_widget_show (control_hbox);

	check_box = ctk_check_button_new_with_mnemonic(_("_Processor"));
	ma->check_boxes[0] = check_box;
	ctk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				g_settings_get_boolean (ma->settings, "view-cpuload"));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_CPU));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view-cpuload");
	ctk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! g_settings_is_writable (ma->settings, "view-cpuload"))
		hard_set_sensitive (check_box, FALSE);

	check_box = ctk_check_button_new_with_mnemonic(_("_Memory"));
	ma->check_boxes[1] = check_box;
	ctk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				g_settings_get_boolean (ma->settings, "view-memload"));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_MEM));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view-memload");
	ctk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! g_settings_is_writable (ma->settings, "view-memload"))
		hard_set_sensitive (check_box, FALSE);

	check_box = ctk_check_button_new_with_mnemonic(_("_Network"));
	ma->check_boxes[2] = check_box;
	ctk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				g_settings_get_boolean (ma->settings, "view-netload"));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_NET));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view-netload");
	ctk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! g_settings_is_writable (ma->settings, "view-netload"))
		hard_set_sensitive (check_box, FALSE);

	check_box = ctk_check_button_new_with_mnemonic (_("S_wap Space"));
	ma->check_boxes[3] = check_box;
	ctk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				g_settings_get_boolean (ma->settings, "view-swapload"));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_SWAP));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view-swapload");
	ctk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! g_settings_is_writable (ma->settings, "view-swapload"))
		hard_set_sensitive (check_box, FALSE);

	check_box = ctk_check_button_new_with_mnemonic(_("_Load"));
	ma->check_boxes[4] = check_box;
	ctk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				g_settings_get_boolean (ma->settings, "view-loadavg"));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_AVG));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view-loadavg");
	ctk_box_pack_start(GTK_BOX(control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! g_settings_is_writable (ma->settings, "view-loadavg"))
		hard_set_sensitive (check_box, FALSE);

	check_box = ctk_check_button_new_with_mnemonic(_("_Harddisk"));
	ma->check_boxes[5] = check_box;
	ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_box),
			g_settings_get_boolean (ma->settings, "view-diskload"));
	g_object_set_data (G_OBJECT (check_box), "MultiloadApplet", ma);
	g_object_set_data (G_OBJECT (check_box), "prop_type",
			GINT_TO_POINTER (PROP_DISK));
	g_signal_connect (G_OBJECT (check_box), "toggled",
			G_CALLBACK (property_toggled_cb), "view-diskload");
	ctk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	category_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (GTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);
	ctk_widget_show (category_vbox);

	title = g_strconcat ("<span weight=\"bold\">", _("Options"), "</span>", NULL);
	label = ctk_label_new (title);
	ctk_label_set_use_markup (GTK_LABEL (label), TRUE);
	ctk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (GTK_LABEL (label), 0.0);
	ctk_box_pack_start (GTK_BOX (category_vbox), label, FALSE, FALSE, 0);
	ctk_widget_show (label);
	g_free (title);

	hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_pack_start (GTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	indent = ctk_label_new (HIG_IDENTATION);
	ctk_label_set_justify (GTK_LABEL (indent), GTK_JUSTIFY_LEFT);
	ctk_box_pack_start (GTK_BOX (hbox), indent, FALSE, FALSE, 0);
	ctk_widget_show (indent);

	control_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (GTK_BOX (hbox), control_vbox, TRUE, TRUE, 0);
	ctk_widget_show (control_vbox);

	control_hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	ctk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	ctk_widget_show (control_hbox);

	label_size = ctk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	orient = cafe_panel_applet_get_orient(ma->applet);
	if ( (orient == CAFE_PANEL_APPLET_ORIENT_UP) || (orient == CAFE_PANEL_APPLET_ORIENT_DOWN) )
		label_text = g_strdup(_("System m_onitor width: "));
	else
		label_text = g_strdup(_("System m_onitor height: "));

	label = ctk_label_new_with_mnemonic(label_text);
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_size_group_add_widget (label_size, label);
        ctk_box_pack_start (GTK_BOX (control_hbox), label, FALSE, FALSE, 0);

	hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	ctk_box_pack_start (GTK_BOX (control_hbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	spin_size = ctk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	spin_button = ctk_spin_button_new_with_range(10, 1000, 5);
	ctk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	g_object_set_data(G_OBJECT(spin_button), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(spin_button), "prop_type",
				GINT_TO_POINTER(PROP_SIZE));
	ctk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button),
				(gdouble)g_settings_get_int(ma->settings, "size"));
	g_signal_connect(G_OBJECT(spin_button), "value_changed",
				G_CALLBACK(spin_button_changed_cb), "size");

	if ( ! g_settings_is_writable (ma->settings, "size")) {
		hard_set_sensitive (label, FALSE);
		hard_set_sensitive (hbox, FALSE);
	}

	ctk_size_group_add_widget (spin_size, spin_button);
	ctk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);

	label = ctk_label_new (_("pixels"));
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	control_hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	ctk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	ctk_widget_show (control_hbox);

	label = ctk_label_new_with_mnemonic(_("Sys_tem monitor update interval: "));
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_size_group_add_widget (label_size, label);
	ctk_box_pack_start (GTK_BOX (control_hbox), label, FALSE, FALSE, 0);

	hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	ctk_box_pack_start (GTK_BOX (control_hbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	spin_button = ctk_spin_button_new_with_range(50, 10000, 50);
	ctk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	g_object_set_data(G_OBJECT(spin_button), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(spin_button), "prop_type",
				GINT_TO_POINTER(PROP_SPEED));
	ctk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button),
				(gdouble)g_settings_get_int (ma->settings, "speed"));
	g_signal_connect(G_OBJECT(spin_button), "value_changed",
				G_CALLBACK(spin_button_changed_cb), "speed");
	ctk_size_group_add_widget (spin_size, spin_button);
	ctk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);

	if ( ! g_settings_is_writable (ma->settings, "speed")) {
		hard_set_sensitive (label, FALSE);
		hard_set_sensitive (hbox, FALSE);
	}

	label = ctk_label_new(_("milliseconds"));
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	g_free(label_text);


	category_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (GTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);
	ctk_widget_show (category_vbox);

	title = g_strconcat ("<span weight=\"bold\">", _("Colors"), "</span>", NULL);
	label = ctk_label_new (title);
	ctk_label_set_use_markup (GTK_LABEL (label), TRUE);
	ctk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (GTK_LABEL (label), 0.0);
	ctk_box_pack_start (GTK_BOX (category_vbox), label, FALSE, FALSE, 0);
	ctk_widget_show (label);
	g_free (title);

	hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_pack_start (GTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	indent = ctk_label_new (HIG_IDENTATION);
	ctk_label_set_justify (GTK_LABEL (indent), GTK_JUSTIFY_LEFT);
	ctk_box_pack_start (GTK_BOX (hbox), indent, FALSE, FALSE, 0);
	ctk_widget_show (indent);

	control_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (GTK_BOX (hbox), control_vbox, TRUE, TRUE, 0);
	ctk_widget_show (control_vbox);

	ma->notebook = ctk_notebook_new();
	ctk_container_add (GTK_CONTAINER (control_vbox), ma->notebook);

	page = add_page(ma->notebook,  _("Processor"));
	ctk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector(page, _("_User"), "cpuload-color0", ma);
	add_color_selector(page, _("S_ystem"), "cpuload-color1", ma);
	add_color_selector(page, _("N_ice"), "cpuload-color2", ma);
	add_color_selector(page, _("I_OWait"), "cpuload-color3", ma);
	add_color_selector(page, _("I_dle"), "cpuload-color4", ma);

	page = add_page(ma->notebook,  _("Memory"));
	ctk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector(page, _("_User"), "memload-color0", ma);
	add_color_selector(page, _("Sh_ared"), "memload-color1", ma);
	add_color_selector(page, _("_Buffers"), "memload-color2", ma);
	add_color_selector (page, _("Cach_ed"), "memload-color3", ma);
	add_color_selector(page, _("F_ree"), "memload-color4", ma);

	page = add_page(ma->notebook,  _("Network"));
	ctk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector (page, _("_In"), "netload2-color0", ma);
	add_color_selector(page, _("_Out"), "netload2-color1", ma);
	add_color_selector (page, _("_Local"), "netload2-color2", ma);
	add_color_selector(page, _("_Background"), "netload2-color3", ma);
	add_color_selector(page, _("_Gridline"), "netload2-color4", ma);
	add_color_selector(page, _("_Indicator"), "netload2-color5", ma);

	page = add_page(ma->notebook,  _("Swap Space"));
	ctk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector(page, _("_Used"), "swapload-color0", ma);
	add_color_selector(page, _("_Free"), "swapload-color1", ma);

	page = add_page(ma->notebook,  _("Load"));
	ctk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector(page, _("_Average"), "loadavg-color0", ma);
	add_color_selector(page, _("_Background"), "loadavg-color1", ma);
	add_color_selector(page, _("_Gridline"), "loadavg-color2", ma);

	page = add_page (ma->notebook, _("Harddisk"));
	ctk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector (page, _("_Read"), "diskload-color0", ma);
	add_color_selector (page, _("_Write"), "diskload-color1", ma);
	add_color_selector (page, _("_Background"), "diskload-color2", ma);
	add_checkbox(page, _("Use diskstats for NVMe"), "diskload-nvme-diskstats", ma);

	title = g_strconcat ("<span weight=\"bold\">", _("Network speed thresholds"), "</span>", NULL);
	label = ctk_label_new (title);
	ctk_label_set_use_markup (GTK_LABEL (label), TRUE);
	ctk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (GTK_LABEL (label), 0.0);
	ctk_box_pack_start (GTK_BOX (category_vbox), label, FALSE, FALSE, 0);
	ctk_widget_show (label);
	g_free (title);

	hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_pack_start (GTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	indent = ctk_label_new (HIG_IDENTATION);
	ctk_label_set_justify (GTK_LABEL (indent), GTK_JUSTIFY_LEFT);
	ctk_box_pack_start (GTK_BOX (hbox), indent, FALSE, FALSE, 0);
	ctk_widget_show (indent);

	control_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (GTK_BOX (hbox), control_vbox, TRUE, TRUE, 0);
	ctk_widget_show (control_vbox);

	control_hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	ctk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	ctk_widget_show (control_hbox);

	label_size = ctk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	label_text = g_strdup(_("Threshold 1: "));
	label = ctk_label_new_with_mnemonic(label_text);
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_size_group_add_widget (label_size, label);
	ctk_box_pack_start (GTK_BOX (control_hbox), label, FALSE, FALSE, 0);

	hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	ctk_box_pack_start (GTK_BOX (control_hbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	spin_size = ctk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	spin_button = ctk_spin_button_new_with_range(10, 1000000000, 5);
	ctk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	g_object_set_data(G_OBJECT(spin_button), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(spin_button), "prop_type",
				GUINT_TO_POINTER(PROP_NET_THRESHOLD1));
	ctk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button),
				(gdouble)g_settings_get_uint(ma->settings, "netthreshold1"));
	g_signal_connect(G_OBJECT(spin_button), "value_changed",
				G_CALLBACK(spin_button_changed_cb), "netthreshold1");

	if ( ! g_settings_is_writable (ma->settings, "netthreshold1"))
	{
		hard_set_sensitive (label, FALSE);
		hard_set_sensitive (hbox, FALSE);
	}

	ctk_size_group_add_widget (spin_size, spin_button);
	ctk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);

	label = ctk_label_new (_("bytes"));
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	control_hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	ctk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	ctk_widget_show (control_hbox);

	label = ctk_label_new_with_mnemonic(_("Threshold 2: "));
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_size_group_add_widget (label_size, label);
	ctk_box_pack_start (GTK_BOX (control_hbox), label, FALSE, FALSE, 0);

	hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	ctk_box_pack_start (GTK_BOX (control_hbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	spin_button = ctk_spin_button_new_with_range(10, 1000000000, 5);
	ctk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	g_object_set_data(G_OBJECT(spin_button), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(spin_button), "prop_type",
				GINT_TO_POINTER(PROP_NET_THRESHOLD2));
	ctk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button),
				(gdouble)g_settings_get_uint (ma->settings, "netthreshold2"));
	g_signal_connect(G_OBJECT(spin_button), "value_changed",
				G_CALLBACK(spin_button_changed_cb), "netthreshold2");
	ctk_size_group_add_widget (spin_size, spin_button);
	ctk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);

	if ( ! g_settings_is_writable (ma->settings, "netthreshold2"))
	{
		hard_set_sensitive (label, FALSE);
		hard_set_sensitive (hbox, FALSE);
	}

	label = ctk_label_new(_("bytes"));
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	control_hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	ctk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	ctk_widget_show (control_hbox);

	label = ctk_label_new_with_mnemonic(_("Threshold 3: "));
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_size_group_add_widget (label_size, label);
	ctk_box_pack_start (GTK_BOX (control_hbox), label, FALSE, FALSE, 0);

	hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	ctk_box_pack_start (GTK_BOX (control_hbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	spin_button = ctk_spin_button_new_with_range(10, 1000000000, 5);
	ctk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	g_object_set_data(G_OBJECT(spin_button), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(spin_button), "prop_type",
			GINT_TO_POINTER(PROP_NET_THRESHOLD3));
	ctk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button),
			(gdouble)g_settings_get_uint (ma->settings, "netthreshold3"));
	g_signal_connect(G_OBJECT(spin_button), "value_changed",
			G_CALLBACK(spin_button_changed_cb), "netthreshold3");
	ctk_size_group_add_widget (spin_size, spin_button);
	ctk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);

	if ( ! g_settings_is_writable (ma->settings, "netthreshold3"))
	{
	  hard_set_sensitive (label, FALSE);
	  hard_set_sensitive (hbox, FALSE);
	}

	label = ctk_label_new(_("bytes"));
	ctk_label_set_xalign (GTK_LABEL (label), 0.0f);
	ctk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	g_free(label_text);

	category_vbox = ctk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (GTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);

	control_hbox = ctk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
	ctk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	ctk_widget_show (control_hbox);

	ctk_widget_show (category_vbox);


	return;
}

/* show properties dialog */
void
multiload_properties_cb (CtkAction       *action,
			 MultiloadApplet *ma)
{
	CtkWidget *dialog = NULL;

	if (ma->prop_dialog) {
		dialog = ma->prop_dialog;

		ctk_window_set_screen (GTK_WINDOW (dialog),
				ctk_widget_get_screen (GTK_WIDGET (ma->applet)));

		ctk_notebook_set_current_page (GTK_NOTEBOOK (ma->notebook),
					       ma->last_clicked);
		ctk_window_present (GTK_WINDOW (dialog));
		return;
	}

	dialog = ctk_dialog_new_with_buttons (_("System Monitor Preferences"),
					      NULL, 0,
					   "ctk-help", GTK_RESPONSE_HELP,
					   "ctk-close", GTK_RESPONSE_CLOSE,
					      NULL);
	ctk_window_set_screen (GTK_WINDOW (dialog),
			       ctk_widget_get_screen (GTK_WIDGET (ma->applet)));
	ctk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
	ctk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	ctk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	ctk_box_set_spacing (GTK_BOX (ctk_dialog_get_content_area (GTK_DIALOG (dialog))), 2);

	fill_properties(dialog, ma);

	properties_set_insensitive(ma);

	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(properties_close_cb), ma);

	ma->prop_dialog = dialog;

	ctk_widget_show_all(dialog);

	ctk_notebook_set_current_page (GTK_NOTEBOOK (ma->notebook),
				       ma->last_clicked);
}
