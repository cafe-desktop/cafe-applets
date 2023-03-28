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
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "cpufreq-prefs.h"
#include "cpufreq-utils.h"

enum {
	PROP_0,
	PROP_CPU,
	PROP_SHOW_MODE,
	PROP_SHOW_TEXT_MODE,
};

struct _CPUFreqPrefsPrivate {
	GSettings          *settings;

	guint               cpu;
	CPUFreqShowMode     show_mode;
	CPUFreqShowTextMode show_text_mode;

	/* Preferences dialog */
	CtkWidget *dialog;
	CtkWidget *show_freq;
	CtkWidget *show_unit;
	CtkWidget *show_perc;
	CtkWidget *cpu_combo;
	CtkWidget *monitor_settings_box;
	CtkWidget *show_mode_combo;
};

static void cpufreq_prefs_finalize                  (GObject           *object);

static void cpufreq_prefs_set_property              (GObject           *object,
						     guint              prop_id,
						     const GValue      *value,
						     GParamSpec        *pspec);
static void cpufreq_prefs_get_property              (GObject           *object,
						     guint              prop_id,
						     GValue            *value,
						     GParamSpec        *pspec);

static void cpufreq_prefs_dialog_update_sensitivity (CPUFreqPrefs      *prefs);


G_DEFINE_TYPE_WITH_PRIVATE (CPUFreqPrefs, cpufreq_prefs, G_TYPE_OBJECT)

static void
cpufreq_prefs_init (CPUFreqPrefs *prefs)
{
	prefs->priv = cpufreq_prefs_get_instance_private (prefs);

	prefs->priv->settings = NULL;

	prefs->priv->cpu = 0;
}

static void
cpufreq_prefs_class_init (CPUFreqPrefsClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->set_property = cpufreq_prefs_set_property;
	g_object_class->get_property = cpufreq_prefs_get_property;

	/* Properties */
	g_object_class_install_property (g_object_class,
					 PROP_CPU,
					 g_param_spec_uint ("cpu",
							    "CPU",
							    "The monitored cpu",
							    0,
							    G_MAXUINT,
							    0,
							    G_PARAM_READWRITE));
	g_object_class_install_property (g_object_class,
					 PROP_SHOW_MODE,
					 g_param_spec_enum ("show-mode",
							    "ShowMode",
							    "The applet show mode",
							    CPUFREQ_TYPE_SHOW_MODE,
							    CPUFREQ_MODE_BOTH,
							    G_PARAM_READWRITE));
	g_object_class_install_property (g_object_class,
					 PROP_SHOW_TEXT_MODE,
					 g_param_spec_enum ("show-text-mode",
							    "ShowTextMode",
							    "The applet show text mode",
							    CPUFREQ_TYPE_SHOW_TEXT_MODE,
							    CPUFREQ_MODE_TEXT_FREQUENCY_UNIT,
							    G_PARAM_READWRITE));

	g_object_class->finalize = cpufreq_prefs_finalize;
}

static void
cpufreq_prefs_finalize (GObject *object)
{
	CPUFreqPrefs *prefs = CPUFREQ_PREFS (object);

	if (prefs->priv->settings) {
		g_object_unref (prefs->priv->settings);
		prefs->priv->settings = NULL;
	}

	if (prefs->priv->dialog) {
		ctk_widget_destroy (prefs->priv->dialog);
		prefs->priv->dialog = NULL;
	}

	G_OBJECT_CLASS (cpufreq_prefs_parent_class)->finalize (object);
}

static void
cpufreq_prefs_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
	CPUFreqPrefs *prefs = CPUFREQ_PREFS (object);
	gboolean      update_sensitivity = FALSE;

	switch (prop_id) {
	case PROP_CPU: {
		guint cpu;

		cpu = g_value_get_uint (value);
		if (prefs->priv->cpu != cpu) {
			prefs->priv->cpu = cpu;
			g_settings_set_int (prefs->priv->settings,
					      "cpu", cpu);
		}
	}
		break;
	case PROP_SHOW_MODE: {
		CPUFreqShowMode mode;

		mode = g_value_get_enum (value);
		if (prefs->priv->show_mode != mode) {
			update_sensitivity = TRUE;
			prefs->priv->show_mode = mode;
			g_settings_set_int (prefs->priv->settings,
					      "show-mode", mode);
		}
	}
		break;
	case PROP_SHOW_TEXT_MODE: {
		CPUFreqShowTextMode mode;

		mode = g_value_get_enum (value);
		if (prefs->priv->show_text_mode != mode) {
			update_sensitivity = TRUE;
			prefs->priv->show_text_mode = mode;
			g_settings_set_int (prefs->priv->settings,
					      "show-text-mode", mode);
		}
	}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}

	if (prefs->priv->dialog && update_sensitivity)
		cpufreq_prefs_dialog_update_sensitivity (prefs);
}

static void
cpufreq_prefs_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
	CPUFreqPrefs *prefs = CPUFREQ_PREFS (object);

	switch (prop_id) {
	case PROP_CPU:
		g_value_set_uint (value, prefs->priv->cpu);
		break;
	case PROP_SHOW_MODE:
		g_value_set_enum (value, prefs->priv->show_mode);
		break;
	case PROP_SHOW_TEXT_MODE:
		g_value_set_enum (value, prefs->priv->show_text_mode);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
cpufreq_prefs_setup (CPUFreqPrefs *prefs)
{
	g_assert (G_IS_SETTINGS (prefs->priv->settings));

	prefs->priv->cpu = g_settings_get_int (prefs->priv->settings, "cpu");
	prefs->priv->show_mode = g_settings_get_int (prefs->priv->settings, "show-mode");
	prefs->priv->show_text_mode = g_settings_get_int (prefs->priv->settings, "show-text-mode");
}

CPUFreqPrefs *
cpufreq_prefs_new (GSettings *settings)
{
	CPUFreqPrefs *prefs;

	g_return_val_if_fail (settings != NULL, NULL);

	prefs = CPUFREQ_PREFS (g_object_new (CPUFREQ_TYPE_PREFS, NULL));
	prefs->priv->settings = g_object_ref (settings);

	cpufreq_prefs_setup (prefs);

	return prefs;
}

/* Public Methods */
guint
cpufreq_prefs_get_cpu (CPUFreqPrefs *prefs)
{
	g_return_val_if_fail (CPUFREQ_IS_PREFS (prefs), 0);
	
	return MIN (prefs->priv->cpu, cpufreq_utils_get_n_cpus () - 1);
}

CPUFreqShowMode
cpufreq_prefs_get_show_mode (CPUFreqPrefs *prefs)
{
	g_return_val_if_fail (CPUFREQ_IS_PREFS (prefs),
			      CPUFREQ_MODE_BOTH);

	return prefs->priv->show_mode;
}

CPUFreqShowTextMode
cpufreq_prefs_get_show_text_mode (CPUFreqPrefs *prefs)
{
	g_return_val_if_fail (CPUFREQ_IS_PREFS (prefs),
			      CPUFREQ_MODE_TEXT_FREQUENCY_UNIT);

	return prefs->priv->show_text_mode;
}

/* Preferences Dialog */
static void
cpufreq_prefs_dialog_show_freq_toggled (CtkWidget *show_freq, CPUFreqPrefs *prefs)
{
	CPUFreqShowTextMode show_text_mode;
           
        if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (show_freq))) {
		CtkWidget *show_unit = prefs->priv->show_unit;

                if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (show_unit)))
                        show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY_UNIT;
                else
                        show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY;

		g_object_set (G_OBJECT (prefs),
			      "show-text-mode", show_text_mode,
			      NULL);
	}
}	

static void
cpufreq_prefs_dialog_show_unit_toggled (CtkWidget *show_unit, CPUFreqPrefs *prefs)
{
	CPUFreqShowTextMode show_text_mode;
           
        if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (show_unit))) {
                show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY_UNIT;
        } else {
                show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY;
        }

	g_object_set (G_OBJECT (prefs),
		      "show-text-mode", show_text_mode,
		      NULL);
}

static void
cpufreq_prefs_dialog_show_perc_toggled (CtkWidget *show_perc, CPUFreqPrefs *prefs)
{

	CPUFreqShowTextMode show_text_mode;
	
        if (ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (show_perc))) {
                /* Show cpu usage in percentage */
                show_text_mode = CPUFREQ_MODE_TEXT_PERCENTAGE;

		g_object_set (G_OBJECT (prefs),
			      "show-text-mode", show_text_mode,
			      NULL);
        }
}

static void
cpufreq_prefs_dialog_cpu_number_changed (CtkWidget *cpu_combo, CPUFreqPrefs *prefs)
{
        gint cpu;
	
        cpu = ctk_combo_box_get_active (CTK_COMBO_BOX (prefs->priv->cpu_combo));

        if (cpu >= 0) {
		g_object_set (G_OBJECT (prefs),
			      "cpu", cpu,
			      NULL);
        }
}

static void
cpufreq_prefs_dialog_show_mode_changed (CtkWidget *show_mode_combo, CPUFreqPrefs *prefs)
{
	CPUFreqShowMode show_mode;
           
        show_mode = ctk_combo_box_get_active (CTK_COMBO_BOX (show_mode_combo));
	g_object_set (G_OBJECT (prefs),
		      "show-mode", show_mode,
		      NULL);
}

static void
cpufreq_prefs_dialog_response_cb (CPUFreqPrefs *prefs,
				  gint          response,
				  CtkDialog    *dialog)
{
        GError *error = NULL;

        if (response == CTK_RESPONSE_HELP) {
		ctk_show_uri_on_window (CTK_WINDOW (prefs->priv->dialog),
	                            "help:cafe-cpufreq-applet/cpufreq-applet-prefs",
	                            ctk_get_current_event_time (),
	                            &error);

                if (error) {
                        cpufreq_utils_display_error (_("Could not open help document"),
						     error->message);
                        g_error_free (error);
		}
        } else {
                ctk_widget_destroy (prefs->priv->dialog);
                prefs->priv->dialog = NULL;
        }
}

static void
cpufreq_prefs_dialog_update_visibility (CPUFreqPrefs *prefs)
{
	if (cpufreq_utils_get_n_cpus () > 1)
		ctk_widget_show (prefs->priv->monitor_settings_box);
	else
		ctk_widget_hide (prefs->priv->monitor_settings_box);
}

static void
cpufreq_prefs_dialog_update_sensitivity (CPUFreqPrefs *prefs)
{
	ctk_widget_set_sensitive (prefs->priv->show_mode_combo,
				  g_settings_is_writable (prefs->priv->settings, "show-mode"));
	
	if (prefs->priv->show_mode != CPUFREQ_MODE_GRAPHIC) {
		gboolean key_writable;
		
		key_writable = g_settings_is_writable (prefs->priv->settings, "show-text-mode");
		
		ctk_widget_set_sensitive (prefs->priv->show_freq,
					  (TRUE && key_writable));
		ctk_widget_set_sensitive (prefs->priv->show_perc,
					  (TRUE && key_writable));
		
		if (prefs->priv->show_text_mode == CPUFREQ_MODE_TEXT_PERCENTAGE)
			ctk_widget_set_sensitive (prefs->priv->show_unit,
						  FALSE);
		else
			ctk_widget_set_sensitive (prefs->priv->show_unit,
						  (TRUE && key_writable));
	} else {
		ctk_widget_set_sensitive (prefs->priv->show_freq, FALSE);
		ctk_widget_set_sensitive (prefs->priv->show_unit, FALSE);
		ctk_widget_set_sensitive (prefs->priv->show_perc, FALSE);
	}
}

static void
cpufreq_prefs_dialog_update (CPUFreqPrefs *prefs)
{
	if (cpufreq_utils_get_n_cpus () > 1) {
		ctk_combo_box_set_active (CTK_COMBO_BOX (prefs->priv->cpu_combo),
					  MIN (prefs->priv->cpu, cpufreq_utils_get_n_cpus () - 1));
	}

	ctk_combo_box_set_active (CTK_COMBO_BOX (prefs->priv->show_mode_combo),
				  prefs->priv->show_mode);
	
	switch (prefs->priv->show_text_mode) {
	case CPUFREQ_MODE_TEXT_FREQUENCY:
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (prefs->priv->show_freq),
					      TRUE);
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (prefs->priv->show_unit),
					      FALSE);

		break;
	case CPUFREQ_MODE_TEXT_FREQUENCY_UNIT:
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (prefs->priv->show_freq),
					      TRUE);
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (prefs->priv->show_unit),
					      TRUE);

		break;
	case CPUFREQ_MODE_TEXT_PERCENTAGE:
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (prefs->priv->show_perc),
					      TRUE);

		break;
	}
}

static void
cpufreq_prefs_dialog_cpu_combo_setup (CPUFreqPrefs *prefs)
{
	CtkListStore    *model;
	CtkTreeIter      iter;
	CtkCellRenderer *renderer;
	guint            i;
	guint            n_cpus;

	model = ctk_list_store_new (1, G_TYPE_STRING);
	ctk_combo_box_set_model (CTK_COMBO_BOX (prefs->priv->cpu_combo),
				 CTK_TREE_MODEL (model));

	n_cpus = cpufreq_utils_get_n_cpus ();
	
	for (i = 0; i < n_cpus; i++) {
		gchar *text_label;
		
		text_label = g_strdup_printf ("CPU %u", i);

		ctk_list_store_append (model, &iter);
		ctk_list_store_set (model, &iter,
				    0, text_label,
				    -1);

		g_free (text_label);
	}

	g_object_unref (model);

	renderer = ctk_cell_renderer_text_new ();
	ctk_cell_layout_clear (CTK_CELL_LAYOUT (prefs->priv->cpu_combo));
	ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (prefs->priv->cpu_combo),
				    renderer, TRUE);
	ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (prefs->priv->cpu_combo),
					renderer,
					"text", 0,
					NULL);
}

static void
cpufreq_prefs_dialog_show_mode_combo_setup (CPUFreqPrefs *prefs)
{
	CtkListStore    *model;
	CtkTreeIter      iter;
	CtkCellRenderer *renderer;

	model = ctk_list_store_new (1, G_TYPE_STRING);
	ctk_combo_box_set_model (CTK_COMBO_BOX (prefs->priv->show_mode_combo),
				 CTK_TREE_MODEL (model));

	ctk_list_store_append (model, &iter);
	ctk_list_store_set (model, &iter,
			    0, _("Graphic"),
			    -1);

	ctk_list_store_append (model, &iter);
	ctk_list_store_set (model, &iter,
			    0, _("Text"),
			    -1);

	ctk_list_store_append (model, &iter);
	ctk_list_store_set (model, &iter,
			    0, _("Graphic and Text"),
			    -1);

	g_object_unref (model);

	renderer = ctk_cell_renderer_text_new ();
	ctk_cell_layout_clear (CTK_CELL_LAYOUT (prefs->priv->show_mode_combo));
	ctk_cell_layout_pack_start (CTK_CELL_LAYOUT (prefs->priv->show_mode_combo),
				    renderer, TRUE);
	ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (prefs->priv->show_mode_combo),
					renderer,
					"text", 0,
					NULL);
}

static void
cpufreq_prefs_dialog_create (CPUFreqPrefs *prefs)
{
	CtkBuilder *builder;

	builder = ctk_builder_new ();
	ctk_builder_add_from_file (builder, CTK_BUILDERDIR "/cpufreq-preferences.ui", NULL);

	prefs->priv->dialog = CTK_WIDGET (ctk_builder_get_object (builder, "prefs_dialog"));

	prefs->priv->cpu_combo = CTK_WIDGET (ctk_builder_get_object (builder, "prefs_cpu_number"));
	
	prefs->priv->show_mode_combo = CTK_WIDGET (ctk_builder_get_object (builder, "prefs_show_mode"));
	
	prefs->priv->show_freq = CTK_WIDGET (ctk_builder_get_object (builder, "prefs_show_freq"));
	prefs->priv->show_unit = CTK_WIDGET (ctk_builder_get_object (builder, "prefs_show_unit"));
	prefs->priv->show_perc = CTK_WIDGET (ctk_builder_get_object (builder, "prefs_show_perc"));

	prefs->priv->monitor_settings_box = CTK_WIDGET (ctk_builder_get_object (builder, "monitor_settings_box"));

	g_object_unref (builder);

	cpufreq_prefs_dialog_show_mode_combo_setup (prefs);
	
	if (cpufreq_utils_get_n_cpus () > 1)
		cpufreq_prefs_dialog_cpu_combo_setup (prefs);
		
	g_signal_connect_swapped (G_OBJECT (prefs->priv->dialog), "response",
				  G_CALLBACK (cpufreq_prefs_dialog_response_cb),
				  (gpointer) prefs);
	
	g_signal_connect (G_OBJECT (prefs->priv->show_freq), "toggled",
			  G_CALLBACK (cpufreq_prefs_dialog_show_freq_toggled),
			  (gpointer) prefs);
	g_signal_connect (G_OBJECT (prefs->priv->show_unit), "toggled",
			  G_CALLBACK (cpufreq_prefs_dialog_show_unit_toggled),
			  (gpointer) prefs);
	g_signal_connect (G_OBJECT (prefs->priv->show_perc), "toggled",
			  G_CALLBACK (cpufreq_prefs_dialog_show_perc_toggled),
			  (gpointer) prefs);
	g_signal_connect (G_OBJECT (prefs->priv->cpu_combo), "changed",
			  G_CALLBACK (cpufreq_prefs_dialog_cpu_number_changed),
			  (gpointer) prefs);
	g_signal_connect (G_OBJECT (prefs->priv->show_mode_combo), "changed",
			  G_CALLBACK (cpufreq_prefs_dialog_show_mode_changed),
			  (gpointer) prefs);
}

void 
cpufreq_preferences_dialog_run (CPUFreqPrefs *prefs, GdkScreen *screen)
{
        g_return_if_fail (CPUFREQ_IS_PREFS (prefs));

        if (prefs->priv->dialog) {
                /* Dialog already exist, only show it */
                ctk_window_present (CTK_WINDOW (prefs->priv->dialog));
                return;
        }

	cpufreq_prefs_dialog_create (prefs);
        ctk_window_set_screen (CTK_WINDOW (prefs->priv->dialog), screen);

	cpufreq_prefs_dialog_update_sensitivity (prefs);
	cpufreq_prefs_dialog_update_visibility (prefs);
	cpufreq_prefs_dialog_update (prefs);

	ctk_widget_show (prefs->priv->dialog);
}
