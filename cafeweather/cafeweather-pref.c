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

/* Radar map on by default. */
#define RADARMAP

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <locale.h>

#include <cafe-panel-applet.h>
#include <gio/gio.h>

#define CAFEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include <libcafeweather/cafeweather-xml.h>
#include "cafeweather.h"
#include "cafeweather-pref.h"
#include "cafeweather-applet.h"
#include "cafeweather-dialog.h"

#define NEVER_SENSITIVE "never_sensitive"

struct _CafeWeatherPrefPrivate {
	CtkWidget* notebook;

	CtkWidget* basic_temp_combo;
	CtkWidget* basic_speed_combo;
	CtkWidget* basic_dist_combo;
	CtkWidget* basic_pres_combo;
	CtkWidget* find_entry;
	CtkWidget* find_next_btn;

	#ifdef RADARMAP
		CtkWidget* basic_radar_btn;
		CtkWidget* basic_radar_url_btn;
		CtkWidget* basic_radar_url_hbox;
		CtkWidget* basic_radar_url_entry;
	#endif /* RADARMAP */

    #ifdef HAVE_LIBNOTIFY
        CtkWidget* basic_show_notifications_btn;
    #endif

	CtkWidget* basic_update_spin;
	CtkWidget* basic_update_btn;
	CtkWidget* tree;

	CtkTreeModel* model;

	CafeWeatherApplet* applet;
};

enum {
	PROP_0,
	PROP_CAFEWEATHER_APPLET,
};

G_DEFINE_TYPE_WITH_PRIVATE (CafeWeatherPref, cafeweather_pref, CTK_TYPE_DIALOG);

/* set sensitive and setup NEVER_SENSITIVE appropriately */
static void hard_set_sensitive(CtkWidget* w, gboolean sensitivity)
{
	ctk_widget_set_sensitive(w, sensitivity);
	g_object_set_data(G_OBJECT(w), NEVER_SENSITIVE, GINT_TO_POINTER(!sensitivity));
}


/* set sensitive, but always insensitive if NEVER_SENSITIVE is set */
static void soft_set_sensitive(CtkWidget* w, gboolean sensitivity)
{
	if (g_object_get_data(G_OBJECT(w), NEVER_SENSITIVE))
	{
		ctk_widget_set_sensitive(w, FALSE);
	}
	else
	{
		ctk_widget_set_sensitive(w, sensitivity);
	}
}

/* sets up ATK Relation between the widgets */
static void add_atk_relation(CtkWidget* widget1, CtkWidget* widget2, AtkRelationType type)
{
	AtkObject* atk_obj1;
	AtkObject* atk_obj2;
	AtkRelationSet* relation_set;
	AtkRelation* relation;

	atk_obj1 = ctk_widget_get_accessible(widget1);

	if (!CTK_IS_ACCESSIBLE(atk_obj1))
	{
		return;
	}

	atk_obj2 = ctk_widget_get_accessible(widget2);

	relation_set = atk_object_ref_relation_set(atk_obj1);
	relation = atk_relation_new(&atk_obj2, 1, type);
	atk_relation_set_add(relation_set, relation);
	g_object_unref(G_OBJECT(relation));
}

/* sets accessible name and description */
void set_access_namedesc(CtkWidget* widget, const gchar* name, const gchar* desc)
{
	AtkObject* obj = ctk_widget_get_accessible(widget);

	if (!CTK_IS_ACCESSIBLE(obj))
	{
		return;
	}

	if (desc)
	{
		atk_object_set_description(obj, desc);
	}

	if (name)
	{
		atk_object_set_name(obj, name);
	}
}

/* sets accessible name, description, CONTROLLED_BY
 * and CONTROLLER_FOR relations for the components
 * in cafeweather preference dialog.
 */
static void cafeweather_pref_set_accessibility(CafeWeatherPref* pref)
{
    /* Relation between components in General page */
    add_atk_relation(pref->priv->basic_update_btn, pref->priv->basic_update_spin, ATK_RELATION_CONTROLLER_FOR);
    add_atk_relation(pref->priv->basic_radar_btn, pref->priv->basic_radar_url_btn, ATK_RELATION_CONTROLLER_FOR);
    add_atk_relation(pref->priv->basic_radar_btn, pref->priv->basic_radar_url_entry, ATK_RELATION_CONTROLLER_FOR);

    add_atk_relation(pref->priv->basic_update_spin, pref->priv->basic_update_btn, ATK_RELATION_CONTROLLED_BY);
    add_atk_relation(pref->priv->basic_radar_url_btn, pref->priv->basic_radar_btn, ATK_RELATION_CONTROLLED_BY);
    add_atk_relation(pref->priv->basic_radar_url_entry, pref->priv->basic_radar_btn, ATK_RELATION_CONTROLLED_BY);

    /* Accessible Name and Description for the components in Preference Dialog */
    set_access_namedesc(pref->priv->tree, _("Location view"), _("Select Location from the list"));
    set_access_namedesc(pref->priv->basic_update_spin, _("Update spin button"), _("Spinbutton for updating"));
    set_access_namedesc(pref->priv->basic_radar_url_entry, _("Address Entry"), _("Enter the URL"));
}


/* Update pref dialog from cafeweather_pref */
static gboolean update_dialog(CafeWeatherPref* pref)
{
    CafeWeatherApplet* gw_applet = pref->priv->applet;

    g_return_val_if_fail(gw_applet->cafeweather_pref.location != NULL, FALSE);

    ctk_spin_button_set_value(CTK_SPIN_BUTTON(pref->priv->basic_update_spin), gw_applet->cafeweather_pref.update_interval / 60);
    ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(pref->priv->basic_update_btn), gw_applet->cafeweather_pref.update_enabled);
    soft_set_sensitive(pref->priv->basic_update_spin, gw_applet->cafeweather_pref.update_enabled);

    ctk_combo_box_set_active(CTK_COMBO_BOX(pref->priv->basic_temp_combo), gw_applet->cafeweather_pref.temperature_unit - 2);

    ctk_combo_box_set_active(CTK_COMBO_BOX(pref->priv->basic_speed_combo), gw_applet->cafeweather_pref.speed_unit - 2);

    ctk_combo_box_set_active(CTK_COMBO_BOX(pref->priv->basic_pres_combo), gw_applet->cafeweather_pref.pressure_unit - 2);

    ctk_combo_box_set_active(CTK_COMBO_BOX(pref->priv->basic_dist_combo), gw_applet->cafeweather_pref.distance_unit - 2);

	#ifdef RADARMAP
		ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(pref->priv->basic_radar_btn), gw_applet->cafeweather_pref.radar_enabled);

		soft_set_sensitive(pref->priv->basic_radar_url_btn, gw_applet->cafeweather_pref.radar_enabled);
		ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(pref->priv->basic_radar_url_btn), gw_applet->cafeweather_pref.use_custom_radar_url);
		soft_set_sensitive(pref->priv->basic_radar_url_hbox, gw_applet->cafeweather_pref.radar_enabled);

		if (gw_applet->cafeweather_pref.radar)
		{
			ctk_entry_set_text(CTK_ENTRY(pref->priv->basic_radar_url_entry), gw_applet->cafeweather_pref.radar);
		}

	#endif /* RADARMAP */

	#ifdef HAVE_LIBNOTIFY
		ctk_toggle_button_set_active(CTK_TOGGLE_BUTTON(pref->priv->basic_show_notifications_btn), gw_applet->cafeweather_pref.show_notifications);
	#endif
    return TRUE;
}

static void row_selected_cb(CtkTreeSelection* selection, CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;
	CtkTreeModel* model;
	WeatherLocation* loc = NULL;
	CtkTreeIter iter;

	if (!ctk_tree_selection_get_selected(selection, &model, &iter))
		return;

	ctk_tree_model_get(model, &iter, CAFEWEATHER_XML_COL_POINTER, &loc, -1);

	if (!loc)
	{
		return;
	}

	g_settings_set_string (gw_applet->settings, "location1", loc->code);
	g_settings_set_string (gw_applet->settings, "location2", loc->zone);
	g_settings_set_string (gw_applet->settings, "location3", loc->radar);
	g_settings_set_string (gw_applet->settings, "location4", loc->name);
	g_settings_set_string (gw_applet->settings, "coordinates", loc->coordinates);

	if (gw_applet->cafeweather_pref.location)
	{
		weather_location_free(gw_applet->cafeweather_pref.location);
	}

	gw_applet->cafeweather_pref.location = 
		weather_location_new (loc->name, loc->code, loc->zone, loc->radar, loc->coordinates,
			NULL, NULL);

	cafeweather_update(gw_applet);
}

static gboolean compare_location(CtkTreeModel* model, CtkTreePath* path, CtkTreeIter* iter, gpointer user_data)
{
    CafeWeatherPref* pref = user_data;
    WeatherLocation* loc;
    CtkTreeView* view;

    ctk_tree_model_get(model, iter, CAFEWEATHER_XML_COL_POINTER, &loc, -1);

    if (!loc)
    {
		return FALSE;
	}

    if (!weather_location_equal(loc, pref->priv->applet->cafeweather_pref.location))
    {
		return FALSE;
	}

    view = CTK_TREE_VIEW(pref->priv->tree);
    ctk_tree_view_expand_to_path(view, path);
    ctk_tree_view_set_cursor(view, path, NULL, FALSE);
    ctk_tree_view_scroll_to_cell(view, path, NULL, TRUE, 0.5, 0.5);

    return TRUE;
}

static void load_locations(CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;
	CtkTreeView* tree = CTK_TREE_VIEW(pref->priv->tree);
	CtkTreeViewColumn* column;
	CtkCellRenderer* cell_renderer;
	WeatherLocation* current_location;

	/* Add a column for the locations */
	cell_renderer = ctk_cell_renderer_text_new();
	column = ctk_tree_view_column_new_with_attributes("not used", cell_renderer, "text", CAFEWEATHER_XML_COL_LOC, NULL);
	ctk_tree_view_append_column(tree, column);
	ctk_tree_view_set_expander_column(CTK_TREE_VIEW(tree), column);

	/* load locations from xml file */
	pref->priv->model = cafeweather_xml_load_locations();

	if (!pref->priv->model)
	{
		CtkWidget* d = ctk_message_dialog_new(NULL, 0, CTK_MESSAGE_ERROR, CTK_BUTTONS_OK, _("Failed to load the Locations XML database.  Please report this as a bug."));
		ctk_dialog_run(CTK_DIALOG(d));
		ctk_widget_destroy(d);
	}

	ctk_tree_view_set_model (tree, pref->priv->model);

	if (pref->priv->applet->cafeweather_pref.location)
	{
		/* Select the current (default) location */
		ctk_tree_model_foreach(CTK_TREE_MODEL(pref->priv->model), compare_location, pref);
	}
}

static void show_notifications_toggled(CtkToggleButton* button, CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;
	
	gboolean toggled = ctk_toggle_button_get_active(button);
	
	if (toggled != gw_applet->cafeweather_pref.show_notifications)
	{
		/* sync with cafeweather_pref struct */
		gw_applet->cafeweather_pref.show_notifications = toggled;
	    
		/* sync with gsettings */
		g_settings_set_boolean (gw_applet->settings, "show-notifications", toggled);
	}
}

static void auto_update_toggled(CtkToggleButton* button, CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;
	gboolean toggled;
	gint nxtSunEvent;

	toggled = ctk_toggle_button_get_active(button);
	gw_applet->cafeweather_pref.update_enabled = toggled;
	soft_set_sensitive(pref->priv->basic_update_spin, toggled);
	g_settings_set_boolean (gw_applet->settings, "auto-update", toggled);

	if (gw_applet->timeout_tag > 0)
	{
		g_source_remove(gw_applet->timeout_tag);
	}

	if (gw_applet->suncalc_timeout_tag > 0)
	{
		g_source_remove(gw_applet->suncalc_timeout_tag);
	}

	if (gw_applet->cafeweather_pref.update_enabled)
	{
		gw_applet->timeout_tag = g_timeout_add_seconds(gw_applet->cafeweather_pref.update_interval, timeout_cb, gw_applet);
		nxtSunEvent = weather_info_next_sun_event(gw_applet->cafeweather_info);

		if (nxtSunEvent >= 0)
		{
			gw_applet->suncalc_timeout_tag = g_timeout_add_seconds(nxtSunEvent, suncalc_timeout_cb, gw_applet);
		}
	}
}

static void temp_combo_changed_cb(CtkComboBox* combo, CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;
	TempUnit new_unit, old_unit;

	g_return_if_fail(gw_applet != NULL);

	new_unit = ctk_combo_box_get_active(combo) + 2;

	old_unit = gw_applet->cafeweather_pref.temperature_unit;

	if (new_unit == old_unit)
	{
		return;
	}

	gw_applet->cafeweather_pref.temperature_unit = new_unit;

	g_settings_set_enum (gw_applet->settings, "temperature-unit", new_unit);

	ctk_label_set_text(CTK_LABEL(gw_applet->label), weather_info_get_temp_summary(gw_applet->cafeweather_info));

	if (gw_applet->details_dialog)
	{
		cafeweather_dialog_update(CAFEWEATHER_DIALOG(gw_applet->details_dialog));
	}
}

static void speed_combo_changed_cb(CtkComboBox* combo, CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;
	SpeedUnit new_unit, old_unit;

	g_return_if_fail(gw_applet != NULL);

	new_unit = ctk_combo_box_get_active(combo) + 2;

	old_unit = gw_applet->cafeweather_pref.speed_unit;

	if (new_unit == old_unit)
	{
		return;
	}

	gw_applet->cafeweather_pref.speed_unit = new_unit;

	g_settings_set_enum (gw_applet->settings, "speed-unit", new_unit);

	if (gw_applet->details_dialog)
	{
		cafeweather_dialog_update(CAFEWEATHER_DIALOG(gw_applet->details_dialog));
	}
}

static void pres_combo_changed_cb(CtkComboBox* combo, CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;
	PressureUnit new_unit, old_unit;

	g_return_if_fail(gw_applet != NULL);

	new_unit = ctk_combo_box_get_active(combo) + 2;

	old_unit = gw_applet->cafeweather_pref.pressure_unit;

	if (new_unit == old_unit)
	{
		return;
	}

	gw_applet->cafeweather_pref.pressure_unit = new_unit;

	g_settings_set_enum (gw_applet->settings, "pressure-unit", new_unit);

	if (gw_applet->details_dialog)
	{
		cafeweather_dialog_update(CAFEWEATHER_DIALOG(gw_applet->details_dialog));
	}
}

static void dist_combo_changed_cb(CtkComboBox* combo, CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;
	DistanceUnit new_unit, old_unit;

	g_return_if_fail(gw_applet != NULL);

	new_unit = ctk_combo_box_get_active(combo) + 2;

	old_unit = gw_applet->cafeweather_pref.distance_unit;

	if (new_unit == old_unit)
	{
		return;
	}

	gw_applet->cafeweather_pref.distance_unit = new_unit;

	g_settings_set_enum (gw_applet->settings, "distance-unit", new_unit);

	if (gw_applet->details_dialog)
	{
		cafeweather_dialog_update(CAFEWEATHER_DIALOG(gw_applet->details_dialog));
	}
}

static void radar_toggled(CtkToggleButton* button, CafeWeatherPref* pref)
{
    CafeWeatherApplet* gw_applet = pref->priv->applet;
    gboolean toggled;

    toggled = ctk_toggle_button_get_active(button);
    gw_applet->cafeweather_pref.radar_enabled = toggled;
    g_settings_set_boolean (gw_applet->settings, "enable-radar-map", toggled);
    soft_set_sensitive(pref->priv->basic_radar_url_btn, toggled);

    if (toggled == FALSE || ctk_toggle_button_get_active(CTK_TOGGLE_BUTTON (pref->priv->basic_radar_url_btn)) == TRUE)
    {
		soft_set_sensitive (pref->priv->basic_radar_url_hbox, toggled);
	}
}

static void use_radar_url_toggled(CtkToggleButton* button, CafeWeatherPref* pref)
{
    CafeWeatherApplet* gw_applet = pref->priv->applet;
    gboolean toggled;

    toggled = ctk_toggle_button_get_active(button);
    gw_applet->cafeweather_pref.use_custom_radar_url = toggled;
    g_settings_set_boolean (gw_applet->settings, "use-custom-radar-url", toggled);
    soft_set_sensitive(pref->priv->basic_radar_url_hbox, toggled);
}

static gboolean radar_url_changed(CtkWidget* widget, GdkEventFocus* event, CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;
	gchar *text;

	text = ctk_editable_get_chars(CTK_EDITABLE(widget), 0, -1);

	if (gw_applet->cafeweather_pref.radar)
	{
		g_free(gw_applet->cafeweather_pref.radar);
	}

	if (text)
	{
		gw_applet->cafeweather_pref.radar = g_strdup(text);
		g_free (text);
	}
	else
	{
		gw_applet->cafeweather_pref.radar = NULL;
	}

	g_settings_set_string (gw_applet->settings, "radar", gw_applet->cafeweather_pref.radar);

	return FALSE;
}

static void update_interval_changed(CtkSpinButton* button, CafeWeatherPref* pref)
{
	CafeWeatherApplet* gw_applet = pref->priv->applet;

	gw_applet->cafeweather_pref.update_interval = ctk_spin_button_get_value_as_int(button)*60;
	g_settings_set_int (gw_applet->settings, "auto-update-interval", gw_applet->cafeweather_pref.update_interval);

	if (gw_applet->timeout_tag > 0)
	{
		g_source_remove(gw_applet->timeout_tag);
	}

	if (gw_applet->cafeweather_pref.update_enabled)
	{
		gw_applet->timeout_tag = g_timeout_add_seconds(gw_applet->cafeweather_pref.update_interval, timeout_cb, gw_applet);
	}
}

static gboolean free_data(CtkTreeModel* model, CtkTreePath* path, CtkTreeIter* iter, gpointer data)
{
	WeatherLocation* location;

	ctk_tree_model_get(model, iter, CAFEWEATHER_XML_COL_POINTER, &location, -1);

	if (!location)
	{
	   return FALSE;
	}

	weather_location_free(location);

	return FALSE;
}


static CtkWidget* create_hig_category(CtkWidget* main_box, gchar* title)
{
	CtkWidget* vbox;
	CtkWidget* vbox2;
	CtkWidget* hbox;
	CtkWidget*label;
	gchar* tmp;

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (CTK_BOX (main_box), vbox, FALSE, FALSE, 0);

	tmp = g_strdup_printf ("<b>%s</b>", title);
	label = ctk_label_new (NULL);
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);
	ctk_label_set_markup (CTK_LABEL (label), tmp);
	g_free (tmp);
	ctk_box_pack_start (CTK_BOX (vbox), label, TRUE, FALSE, 0);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	label = ctk_label_new ("    ");
	ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);

	vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (CTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

	return vbox2;
}

static gboolean find_location(CtkTreeModel* model, CtkTreeIter* iter, const gchar* location, gboolean go_parent)
{
	CtkTreeIter iter_child;
	CtkTreeIter iter_parent;
	gchar* aux_loc;
	gboolean valid;
	int len;

	len = strlen (location);

	if (len <= 0)
	{
		return FALSE;
	}

	do {

		ctk_tree_model_get (model, iter, CAFEWEATHER_XML_COL_LOC, &aux_loc, -1);

		if (g_ascii_strncasecmp (aux_loc, location, len) == 0)
		{
			g_free (aux_loc);
			return TRUE;
		}

		if (ctk_tree_model_iter_has_child(model, iter))
		{
			ctk_tree_model_iter_nth_child(model, &iter_child, iter, 0);

			if (find_location (model, &iter_child, location, FALSE))
			{
				/* Manual copying of the iter */
				iter->stamp = iter_child.stamp;
				iter->user_data = iter_child.user_data;
				iter->user_data2 = iter_child.user_data2;
				iter->user_data3 = iter_child.user_data3;

				g_free (aux_loc);

				return TRUE;
			}
		}

		g_free (aux_loc);

		valid = ctk_tree_model_iter_next(model, iter);

	} while (valid);

	if (go_parent)
	{
		iter_parent = *iter;

		while (ctk_tree_model_iter_parent (model, iter, &iter_parent))
		{
			if (ctk_tree_model_iter_next (model, iter))
			{
				return find_location (model, iter, location, TRUE);
			}

			iter_parent = *iter;
		}
	}

	return FALSE;
}

static void find_next_clicked(CtkButton* button, CafeWeatherPref* pref)
{
	CtkTreeView *tree;
	CtkTreeModel *model;
	CtkEntry *entry;
	CtkTreeSelection *selection;
	CtkTreeIter iter;
	CtkTreeIter iter_parent;
	CtkTreePath *path;
	const gchar *location;

	tree = CTK_TREE_VIEW (pref->priv->tree);
	model = ctk_tree_view_get_model (tree);
	entry = CTK_ENTRY (pref->priv->find_entry);

	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (tree));

	if (ctk_tree_selection_count_selected_rows (selection) >= 1)
	{
		ctk_tree_selection_get_selected (selection, &model, &iter);
		/* Select next or select parent */
		if (!ctk_tree_model_iter_next (model, &iter))
		{
			iter_parent = iter;

			if (!ctk_tree_model_iter_parent (model, &iter, &iter_parent) || !ctk_tree_model_iter_next (model, &iter))
			{
				ctk_tree_model_get_iter_first (model, &iter);
			}
		}

	}
	else
	{
		ctk_tree_model_get_iter_first (model, &iter);
	}

	location = ctk_entry_get_text (entry);

	if (find_location (model, &iter, location, TRUE))
	{
		ctk_widget_set_sensitive (pref->priv->find_next_btn, TRUE);

		path = ctk_tree_model_get_path (model, &iter);
		ctk_tree_view_expand_to_path (tree, path);
		ctk_tree_selection_select_path (selection, path);
		ctk_tree_view_scroll_to_cell (tree, path, NULL, TRUE, 0.5, 0);

		ctk_tree_path_free (path);
	}
	else
	{
		ctk_widget_set_sensitive (pref->priv->find_next_btn, FALSE);
	}
}

static void find_entry_changed(CtkEditable* entry, CafeWeatherPref* pref)
{
	CtkTreeView *tree;
	CtkTreeModel *model;
	CtkTreeSelection *selection;
	CtkTreeIter iter;
	CtkTreePath *path;
	const gchar *location;

	tree = CTK_TREE_VIEW (pref->priv->tree);
	model = ctk_tree_view_get_model (tree);

	g_return_if_fail (model != NULL);

	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (tree));
	ctk_tree_model_get_iter_first (model, &iter);

	location = ctk_entry_get_text (CTK_ENTRY (entry));

	if (find_location (model, &iter, location, TRUE))
	{
		ctk_widget_set_sensitive (pref->priv->find_next_btn, TRUE);

		path = ctk_tree_model_get_path (model, &iter);
		ctk_tree_view_expand_to_path (tree, path);
		ctk_tree_selection_select_iter (selection, &iter);
		ctk_tree_view_scroll_to_cell (tree, path, NULL, TRUE, 0.5, 0);

		ctk_tree_path_free (path);
	}
	else
	{
		ctk_widget_set_sensitive (pref->priv->find_next_btn, FALSE);
	}
}


static void help_cb(CtkDialog* dialog, CafeWeatherPref* pref)
{
	gint current_page;
	gchar *uri;
	GError* error = NULL;

	current_page = ctk_notebook_get_current_page (CTK_NOTEBOOK (pref->priv->notebook));
	uri = g_strdup_printf ("help:cafeweather/cafeweather-prefs#cafeweather-%s", (current_page == 0) ? "metric" : "change-location");
	ctk_show_uri_on_window (CTK_WINDOW (dialog),
	                        uri,
	                        ctk_get_current_event_time (),
	                        &error);
	g_free (uri);

	if (error)
	{
		CtkWidget* error_dialog = ctk_message_dialog_new (NULL, CTK_DIALOG_MODAL, CTK_MESSAGE_ERROR, CTK_BUTTONS_CLOSE, _("There was an error displaying help: %s"), error->message);
		g_signal_connect (G_OBJECT (error_dialog), "response", G_CALLBACK (ctk_widget_destroy), NULL);
		ctk_window_set_resizable (CTK_WINDOW (error_dialog), FALSE);
		ctk_window_set_screen (CTK_WINDOW (error_dialog), ctk_widget_get_screen (CTK_WIDGET (dialog)));
		ctk_widget_show (error_dialog);
		g_error_free (error);
		error = NULL;
	}
}


static void response_cb(CtkDialog* dialog, gint id, CafeWeatherPref* pref)
{
	if (id == CTK_RESPONSE_HELP)
	{
		help_cb(dialog, pref);
	}
	else
	{
		ctk_widget_destroy(CTK_WIDGET(dialog));
	}
}


static void cafeweather_pref_create(CafeWeatherPref* pref)
{
	CtkWidget* pref_vbox;
	#ifdef RADARMAP
		CtkWidget* radar_toggle_hbox;
	#endif /* RADARMAP */
	CtkWidget* pref_basic_update_lbl;
	CtkWidget* pref_basic_update_hbox;
	CtkAdjustment* pref_basic_update_spin_adj;
	CtkWidget* pref_basic_update_sec_lbl;
	CtkWidget* pref_basic_note_lbl;
	CtkWidget* pref_loc_hbox;
	CtkWidget* pref_loc_note_lbl;
	CtkWidget* scrolled_window;
	CtkWidget* label;
	CtkWidget* value_hbox;
	CtkWidget* tree_label;
	CtkTreeSelection *selection;
	CtkWidget* pref_basic_vbox;
	CtkWidget* vbox;
	CtkWidget* frame;
	CtkWidget* temp_label;
	CtkWidget* temp_combo;
	CtkWidget* speed_label;
	CtkWidget* speed_combo;
	CtkWidget* pres_label;
	CtkWidget* pres_combo;
	CtkWidget* dist_label;
	CtkWidget* dist_combo;
	CtkWidget* unit_grid;
	CtkWidget* pref_find_label;
	CtkWidget* pref_find_hbox;
	CtkWidget* image;


	g_object_set (pref, "destroy-with-parent", TRUE, NULL);
	ctk_window_set_title (CTK_WINDOW (pref), _("Weather Preferences"));
	ctk_dialog_add_buttons (CTK_DIALOG (pref), "ctk-close", CTK_RESPONSE_CLOSE, "ctk-help", CTK_RESPONSE_HELP, NULL);
	ctk_dialog_set_default_response (CTK_DIALOG (pref), CTK_RESPONSE_CLOSE);
	ctk_container_set_border_width (CTK_CONTAINER (pref), 5);
	ctk_window_set_resizable (CTK_WINDOW (pref), TRUE);
	ctk_window_set_screen (CTK_WINDOW (pref), ctk_widget_get_screen (CTK_WIDGET (pref->priv->applet->applet)));

	pref_vbox = ctk_dialog_get_content_area (CTK_DIALOG (pref));
	ctk_box_set_spacing (CTK_BOX (pref_vbox), 2);
	ctk_widget_show (pref_vbox);

	pref->priv->notebook = ctk_notebook_new ();
	ctk_container_set_border_width (CTK_CONTAINER (pref->priv->notebook), 5);
	ctk_widget_show (pref->priv->notebook);
	ctk_box_pack_start (CTK_BOX (pref_vbox), pref->priv->notebook, TRUE, TRUE, 0);

  /*
   * General settings page.
   */

	pref_basic_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 18);
	ctk_container_set_border_width (CTK_CONTAINER (pref_basic_vbox), 12);
	ctk_container_add (CTK_CONTAINER (pref->priv->notebook), pref_basic_vbox);

	pref->priv->basic_update_btn = ctk_check_button_new_with_mnemonic (_("_Automatically update every:"));
	ctk_widget_set_halign (pref->priv->basic_update_btn, CTK_ALIGN_START);
	ctk_widget_set_vexpand (pref->priv->basic_update_btn, TRUE);
	ctk_widget_show (pref->priv->basic_update_btn);
	g_signal_connect (G_OBJECT (pref->priv->basic_update_btn), "toggled", G_CALLBACK (auto_update_toggled), pref);

	if (!g_settings_is_writable (pref->priv->applet->settings, "auto-update"))
	{
		hard_set_sensitive (pref->priv->basic_update_btn, FALSE);
	}

	/*
	 * Units settings page.
	 */

	/* Temperature Unit */
	temp_label = ctk_label_new_with_mnemonic (_("_Temperature unit:"));
	ctk_label_set_use_markup (CTK_LABEL (temp_label), TRUE);
	ctk_label_set_justify (CTK_LABEL (temp_label), CTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (CTK_LABEL (temp_label), 0.0);
	ctk_widget_show (temp_label);

	temp_combo = ctk_combo_box_text_new ();
	pref->priv->basic_temp_combo = temp_combo;
	ctk_label_set_mnemonic_widget (CTK_LABEL (temp_label), temp_combo);
	//ctk_combo_box_append_text (CTK_COMBO_BOX (temp_combo), _("Default"));
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (temp_combo), _("Kelvin"));
	/* TRANSLATORS: Celsius is sometimes referred Centigrade */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (temp_combo), _("Celsius"));
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (temp_combo), _("Fahrenheit"));
	ctk_widget_show (temp_combo);

	if ( ! g_settings_is_writable (pref->priv->applet->settings, "temperature-unit"))
	{
		hard_set_sensitive (pref->priv->basic_temp_combo, FALSE);
	}

	/* Speed Unit */
	speed_label = ctk_label_new_with_mnemonic (_("_Wind speed unit:"));
	ctk_label_set_use_markup (CTK_LABEL (speed_label), TRUE);
	ctk_label_set_justify (CTK_LABEL (speed_label), CTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (CTK_LABEL (speed_label), 0.0);
	ctk_widget_show (speed_label);

	speed_combo = ctk_combo_box_text_new ();
	pref->priv->basic_speed_combo = speed_combo;
	ctk_label_set_mnemonic_widget (CTK_LABEL (speed_label), speed_combo);
	//ctk_combo_box_append_text (CTK_COMBO_BOX (speed_combo), _("Default"));
	/* TRANSLATOR: The wind speed unit "meters per second" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (speed_combo), _("m/s"));
	/* TRANSLATOR: The wind speed unit "kilometers per hour" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (speed_combo), _("km/h"));
	/* TRANSLATOR: The wind speed unit "miles per hour" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (speed_combo), _("mph"));
	/* TRANSLATOR: The wind speed unit "knots" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (speed_combo), _("knots"));
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (speed_combo), _("Beaufort scale"));
	ctk_widget_show (speed_combo);

	if (!g_settings_is_writable (pref->priv->applet->settings, "speed-unit"))
	{
		hard_set_sensitive (pref->priv->basic_speed_combo, FALSE);
	}

	/* Pressure Unit */
	pres_label = ctk_label_new_with_mnemonic (_("_Pressure unit:"));
	ctk_label_set_use_markup (CTK_LABEL (pres_label), TRUE);
	ctk_label_set_justify (CTK_LABEL (pres_label), CTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (CTK_LABEL (pres_label), 0.0);
	ctk_widget_show (pres_label);

	pres_combo = ctk_combo_box_text_new ();
	pref->priv->basic_pres_combo = pres_combo;
	ctk_label_set_mnemonic_widget (CTK_LABEL (pres_label), pres_combo);
	//ctk_combo_box_append_text (CTK_COMBO_BOX (pres_combo), _("Default"));
	/* TRANSLATOR: The pressure unit "kiloPascals" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (pres_combo), _("kPa"));
	/* TRANSLATOR: The pressure unit "hectoPascals" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (pres_combo), _("hPa"));
	/* TRANSLATOR: The pressure unit "millibars" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (pres_combo), _("mb"));
	/* TRANSLATOR: The pressure unit "millibars of mercury" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (pres_combo), _("mmHg"));
	/* TRANSLATOR: The pressure unit "inches of mercury" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (pres_combo), _("inHg"));
	/* TRANSLATOR: The pressure unit "atmospheres" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (pres_combo), _("atm"));
	ctk_widget_show (pres_combo);

	if (!g_settings_is_writable(pref->priv->applet->settings, "pressure-unit"))
	{
		hard_set_sensitive(pref->priv->basic_pres_combo, FALSE);
	}

	/* Distance Unit */
	dist_label = ctk_label_new_with_mnemonic (_("_Visibility unit:"));
	ctk_label_set_use_markup (CTK_LABEL (dist_label), TRUE);
	ctk_label_set_justify (CTK_LABEL (dist_label), CTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (CTK_LABEL (dist_label), 0.0);
	ctk_widget_show (dist_label);

	dist_combo = ctk_combo_box_text_new ();
	pref->priv->basic_dist_combo = dist_combo;
	ctk_label_set_mnemonic_widget (CTK_LABEL (dist_label), dist_combo);
	//ctk_combo_box_append_text (CTK_COMBO_BOX (dist_combo), _("Default"));
	/* TRANSLATOR: The distance unit "meters" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (dist_combo), _("meters"));
	/* TRANSLATOR: The distance unit "kilometers" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (dist_combo), _("km"));
	/* TRANSLATOR: The distance unit "miles" */
	ctk_combo_box_text_append_text (CTK_COMBO_BOX_TEXT (dist_combo), _("miles"));
	ctk_widget_show (dist_combo);

	if ( ! g_settings_is_writable (pref->priv->applet->settings, "distance-unit"))
		hard_set_sensitive (pref->priv->basic_dist_combo, FALSE);

	unit_grid = ctk_grid_new ();
	ctk_grid_set_row_spacing(CTK_GRID(unit_grid), 6);
	ctk_grid_set_column_spacing(CTK_GRID(unit_grid), 12);
	ctk_widget_set_halign (temp_label, CTK_ALIGN_START);
	ctk_grid_attach(CTK_GRID(unit_grid), temp_label, 0, 0, 1, 1);
	ctk_grid_attach(CTK_GRID(unit_grid), temp_combo,  1, 0, 1, 1);
	ctk_widget_set_halign (speed_label, CTK_ALIGN_START);
	ctk_grid_attach(CTK_GRID(unit_grid), speed_label, 0, 1, 1, 1);
	ctk_grid_attach(CTK_GRID(unit_grid), speed_combo, 1, 1, 1, 1);
	ctk_widget_set_halign (pres_label, CTK_ALIGN_START);
	ctk_grid_attach(CTK_GRID(unit_grid), pres_label, 0, 2, 1, 1);
	ctk_grid_attach(CTK_GRID(unit_grid), pres_combo,  1, 2, 1, 1);
	ctk_widget_set_halign (dist_label, CTK_ALIGN_START);
	ctk_grid_attach(CTK_GRID(unit_grid), dist_label, 0, 3, 1, 1);
	ctk_grid_attach(CTK_GRID(unit_grid), dist_combo,  1, 3, 1, 1);
	ctk_widget_show(unit_grid);

	g_signal_connect (temp_combo, "changed", G_CALLBACK (temp_combo_changed_cb), pref);
	g_signal_connect (speed_combo, "changed", G_CALLBACK (speed_combo_changed_cb), pref);
	g_signal_connect (dist_combo, "changed", G_CALLBACK (dist_combo_changed_cb), pref);
	g_signal_connect (pres_combo, "changed", G_CALLBACK (pres_combo_changed_cb), pref);


	#ifdef RADARMAP
		pref->priv->basic_radar_btn = ctk_check_button_new_with_mnemonic (_("Enable _radar map"));
		ctk_widget_show (pref->priv->basic_radar_btn);
		g_signal_connect (G_OBJECT (pref->priv->basic_radar_btn), "toggled", G_CALLBACK (radar_toggled), pref);

		if (!g_settings_is_writable (pref->priv->applet->settings, "enable-radar-map"))
		{
			hard_set_sensitive(pref->priv->basic_radar_btn, FALSE);
		}

		radar_toggle_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
		ctk_widget_show(radar_toggle_hbox);

		label = ctk_label_new ("    ");
		ctk_widget_show (label);
		ctk_box_pack_start (CTK_BOX (radar_toggle_hbox), label, FALSE, FALSE, 0);

		pref->priv->basic_radar_url_btn = ctk_check_button_new_with_mnemonic (_("Use _custom address for radar map"));
		ctk_widget_show (pref->priv->basic_radar_url_btn);
		ctk_box_pack_start (CTK_BOX (radar_toggle_hbox), pref->priv->basic_radar_url_btn, FALSE, FALSE, 0);

		g_signal_connect (G_OBJECT (pref->priv->basic_radar_url_btn), "toggled", G_CALLBACK (use_radar_url_toggled), pref);

		if ( ! g_settings_is_writable (pref->priv->applet->settings, "use-custom-radar-url"))
		{
			hard_set_sensitive (pref->priv->basic_radar_url_btn, FALSE);
		}

		pref->priv->basic_radar_url_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
		ctk_widget_show (pref->priv->basic_radar_url_hbox);

		label = ctk_label_new ("    ");
		ctk_widget_show (label);
		ctk_box_pack_start (CTK_BOX (pref->priv->basic_radar_url_hbox), label, FALSE, FALSE, 0);

		label = ctk_label_new_with_mnemonic (_("A_ddress:"));
		ctk_widget_show (label);
		ctk_box_pack_start (CTK_BOX (pref->priv->basic_radar_url_hbox), label, FALSE, FALSE, 0);
		pref->priv->basic_radar_url_entry = ctk_entry_new ();
		ctk_widget_show (pref->priv->basic_radar_url_entry);
		ctk_box_pack_start (CTK_BOX (pref->priv->basic_radar_url_hbox), pref->priv->basic_radar_url_entry, TRUE, TRUE, 0);
		g_signal_connect (G_OBJECT (pref->priv->basic_radar_url_entry), "focus_out_event", G_CALLBACK (radar_url_changed), pref);
		if ( ! g_settings_is_writable (pref->priv->applet->settings, "radar"))
		{
			hard_set_sensitive (pref->priv->basic_radar_url_entry, FALSE);
		}
	#endif /* RADARMAP */

    #ifdef HAVE_LIBNOTIFY
		/* setup show-notifications button */
		pref->priv->basic_show_notifications_btn = ctk_check_button_new_with_mnemonic (_("Show _notifications"));

		if (!g_settings_is_writable (pref->priv->applet->settings, "show-notifications"))
		{
			hard_set_sensitive (pref->priv->basic_show_notifications_btn, FALSE);
		}

		g_signal_connect (G_OBJECT (pref->priv->basic_show_notifications_btn), "toggled", G_CALLBACK (show_notifications_toggled), pref);
    #endif

	frame = create_hig_category (pref_basic_vbox, _("Update"));

	pref_basic_update_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);

	pref_basic_update_lbl = ctk_label_new_with_mnemonic (_("_Automatically update every:"));
	ctk_widget_show (pref_basic_update_lbl);

	ctk_widget_show (pref_basic_update_hbox);

	pref_basic_update_spin_adj = ctk_adjustment_new (30, 1, 3600, 5, 25, 1);
	pref->priv->basic_update_spin = ctk_spin_button_new (pref_basic_update_spin_adj, 1, 0);
	ctk_widget_show (pref->priv->basic_update_spin);

	ctk_spin_button_set_numeric (CTK_SPIN_BUTTON (pref->priv->basic_update_spin), TRUE);
	ctk_spin_button_set_update_policy (CTK_SPIN_BUTTON (pref->priv->basic_update_spin), CTK_UPDATE_IF_VALID);
	g_signal_connect (G_OBJECT (pref->priv->basic_update_spin), "value_changed", G_CALLBACK (update_interval_changed), pref);

	pref_basic_update_sec_lbl = ctk_label_new (_("minutes"));
	ctk_widget_show (pref_basic_update_sec_lbl);

	if ( ! g_settings_is_writable (pref->priv->applet->settings, "auto-update-interval"))
	{
		hard_set_sensitive (pref->priv->basic_update_spin, FALSE);
		hard_set_sensitive (pref_basic_update_sec_lbl, FALSE);
	}

	value_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);

	ctk_box_pack_start (CTK_BOX (pref_basic_update_hbox), pref->priv->basic_update_btn, FALSE, TRUE, 0);
	ctk_box_pack_start (CTK_BOX (pref_basic_update_hbox), value_hbox, FALSE, FALSE, 0);
	ctk_box_pack_start (CTK_BOX (value_hbox), pref->priv->basic_update_spin, FALSE, FALSE, 0);
	ctk_box_pack_start (CTK_BOX (value_hbox), pref_basic_update_sec_lbl, FALSE, FALSE, 0);

	ctk_container_add (CTK_CONTAINER (frame), pref_basic_update_hbox);

	frame = create_hig_category (pref_basic_vbox, _("Display"));

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);

	ctk_box_pack_start (CTK_BOX (vbox), unit_grid, TRUE, TRUE, 0);

	#ifdef RADARMAP
		ctk_box_pack_start (CTK_BOX (vbox), pref->priv->basic_radar_btn, TRUE, TRUE, 0);
		ctk_box_pack_start (CTK_BOX (vbox), radar_toggle_hbox, TRUE, TRUE, 0);
		ctk_box_pack_start (CTK_BOX (vbox), pref->priv->basic_radar_url_hbox, TRUE, TRUE, 0);
	#endif /* RADARMAP */
	
	#ifdef HAVE_LIBNOTIFY
		/* add the show-notification toggle button to the vbox of the display section */
		ctk_box_pack_start (CTK_BOX (vbox), pref->priv->basic_show_notifications_btn, TRUE, TRUE, 0);
	#endif

	ctk_container_add (CTK_CONTAINER (frame), vbox);

	pref_basic_note_lbl = ctk_label_new (_("General"));
	ctk_widget_show (pref_basic_note_lbl);
	ctk_notebook_set_tab_label (CTK_NOTEBOOK (pref->priv->notebook), ctk_notebook_get_nth_page (CTK_NOTEBOOK (pref->priv->notebook), 0), pref_basic_note_lbl);

  /*
   * Location page.
   */
	pref_loc_hbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_container_set_border_width (CTK_CONTAINER (pref_loc_hbox), 12);
	ctk_container_add (CTK_CONTAINER (pref->priv->notebook), pref_loc_hbox);

	tree_label = ctk_label_new_with_mnemonic (_("_Select a location:"));
	ctk_label_set_xalign (CTK_LABEL (tree_label), 0.0);
	ctk_box_pack_start (CTK_BOX (pref_loc_hbox), tree_label, FALSE, FALSE, 0);

	scrolled_window = ctk_scrolled_window_new (NULL, NULL);
	ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled_window), CTK_SHADOW_IN);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window), CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);

	pref->priv->tree = ctk_tree_view_new ();
	ctk_label_set_mnemonic_widget (CTK_LABEL (tree_label), CTK_WIDGET (pref->priv->tree));
	ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (pref->priv->tree), FALSE);

	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (pref->priv->tree));
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (row_selected_cb), pref);

	ctk_container_add (CTK_CONTAINER (scrolled_window), pref->priv->tree);
	ctk_widget_show (pref->priv->tree);
	ctk_widget_show (scrolled_window);
	ctk_box_pack_start (CTK_BOX (pref_loc_hbox), scrolled_window, TRUE, TRUE, 0);
	load_locations(pref);

	pref_find_hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 6);
	pref_find_label = ctk_label_new (_("_Find:"));
	ctk_label_set_use_underline (CTK_LABEL (pref_find_label), TRUE);

	pref->priv->find_entry = ctk_entry_new ();
	ctk_label_set_mnemonic_widget (CTK_LABEL (pref_find_label), pref->priv->find_entry);

	pref->priv->find_next_btn = ctk_button_new_with_mnemonic (_("Find _Next"));
	ctk_widget_set_sensitive (pref->priv->find_next_btn, FALSE);

	image = ctk_image_new_from_icon_name ("edit-find", CTK_ICON_SIZE_BUTTON);
	ctk_button_set_image (CTK_BUTTON (pref->priv->find_next_btn), image);

	g_signal_connect (G_OBJECT (pref->priv->find_next_btn), "clicked", G_CALLBACK (find_next_clicked), pref);
	g_signal_connect (G_OBJECT (pref->priv->find_entry), "changed", G_CALLBACK (find_entry_changed), pref);

	ctk_container_set_border_width (CTK_CONTAINER (pref_find_hbox), 0);
	ctk_box_pack_start (CTK_BOX (pref_find_hbox), pref_find_label, FALSE, FALSE, 0);
	ctk_box_pack_start (CTK_BOX (pref_find_hbox), pref->priv->find_entry, TRUE, TRUE, 0);
	ctk_box_pack_start (CTK_BOX (pref_find_hbox), pref->priv->find_next_btn, FALSE, FALSE, 0);

	ctk_box_pack_start (CTK_BOX (pref_loc_hbox), pref_find_hbox, FALSE, FALSE, 0);

	if ( ! g_settings_is_writable (pref->priv->applet->settings, "location0"))
	{
		hard_set_sensitive (scrolled_window, FALSE);
	}

	pref_loc_note_lbl = ctk_label_new (_("Location"));
	ctk_widget_show (pref_loc_note_lbl);
	ctk_notebook_set_tab_label (CTK_NOTEBOOK (pref->priv->notebook), ctk_notebook_get_nth_page (CTK_NOTEBOOK (pref->priv->notebook), 1), pref_loc_note_lbl);


	g_signal_connect (G_OBJECT (pref), "response", G_CALLBACK (response_cb), pref);

	cafeweather_pref_set_accessibility (pref);
	ctk_label_set_mnemonic_widget (CTK_LABEL (pref_basic_update_sec_lbl), pref->priv->basic_update_spin);
	ctk_label_set_mnemonic_widget (CTK_LABEL (label), pref->priv->basic_radar_url_entry);
}


static void cafeweather_pref_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec *pspec)
{
    CafeWeatherPref* pref = CAFEWEATHER_PREF(object);

    switch (prop_id)
    {
		case PROP_CAFEWEATHER_APPLET:
			pref->priv->applet = g_value_get_pointer(value);
			break;
    }
}


static void cafeweather_pref_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    CafeWeatherPref* pref = CAFEWEATHER_PREF(object);

    switch (prop_id)
    {
		case PROP_CAFEWEATHER_APPLET:
			g_value_set_pointer(value, pref->priv->applet);
			break;
    }
}


static void cafeweather_pref_init(CafeWeatherPref* self)
{
	self->priv = cafeweather_pref_get_instance_private(self);
}


static GObject* cafeweather_pref_constructor(GType type, guint n_construct_params, GObjectConstructParam* construct_params)
{
    GObject* object;
    CafeWeatherPref* self;

    object = G_OBJECT_CLASS(cafeweather_pref_parent_class)->constructor(type, n_construct_params, construct_params);
    self = CAFEWEATHER_PREF(object);

    cafeweather_pref_create(self);
    update_dialog(self);

    return object;
}


CtkWidget* cafeweather_pref_new(CafeWeatherApplet* applet)
{
    return g_object_new(CAFEWEATHER_TYPE_PREF, "cafeweather-applet", applet, NULL);
}


static void cafeweather_pref_finalize(GObject* object)
{
	CafeWeatherPref* self = CAFEWEATHER_PREF(object);

	ctk_tree_model_foreach(self->priv->model, free_data, NULL);
	g_object_unref(G_OBJECT(self->priv->model));

	G_OBJECT_CLASS(cafeweather_pref_parent_class)->finalize(object);
}


static void cafeweather_pref_class_init(CafeWeatherPrefClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    cafeweather_pref_parent_class = g_type_class_peek_parent(klass);

    object_class->set_property = cafeweather_pref_set_property;
    object_class->get_property = cafeweather_pref_get_property;
    object_class->constructor = cafeweather_pref_constructor;
    object_class->finalize = cafeweather_pref_finalize;

    /* This becomes an OBJECT property when CafeWeatherApplet is redone */
    g_object_class_install_property(object_class, PROP_CAFEWEATHER_APPLET, g_param_spec_pointer("cafeweather-applet", "CafeWeather Applet", "The CafeWeather Applet", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


