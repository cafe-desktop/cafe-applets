/* timerapplet.c:
 *
 * Copyright (C) 2014 Stefano Karapetsas
 *
 * This file is part of CAFE Applets.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *      Stefano Karapetsas <stefano@karapetsas.com>
 */

#include <config.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <ctk/ctk.h>

#include <libnotify/notify.h>

#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>

/* Applet constants */
#define APPLET_ICON  "cafe-panel-clock"
#define STEP         100

/* GSettings constants */
#define TIMER_SCHEMA            "org.cafe.panel.applet.timer"
#define NAME_KEY                "name"
#define DURATION_KEY            "duration"
#define SHOW_NOTIFICATION_KEY   "show-notification"
#define SHOW_DIALOG_KEY         "show-dialog"

typedef struct
{
    CafePanelApplet   *applet;

    GSettings         *settings;

    CtkActionGroup    *action_group;
    CtkLabel          *label;
    CtkImage          *image;
    CtkImage          *pause_image;
    CtkBox            *box;

    CtkSpinButton     *hours;
    CtkSpinButton     *minutes;
    CtkSpinButton     *seconds;

    gboolean           active;
    gboolean           pause;
    gint               elapsed;

    guint              timeout_id;
} TimerApplet;

static void timer_start_callback (CtkAction *action, TimerApplet *applet);
static void timer_pause_callback (CtkAction *action, TimerApplet *applet);
static void timer_stop_callback (CtkAction *action, TimerApplet *applet);
static void timer_about_callback (CtkAction *action, TimerApplet *applet);
static void timer_reset_callback (CtkAction *action, TimerApplet *applet);
static void timer_preferences_callback (CtkAction *action, TimerApplet *applet);

static const CtkActionEntry applet_menu_actions [] = {
    { "Start", "media-playback-start", N_("_Start timer"), NULL, NULL, G_CALLBACK (timer_start_callback) },
    { "Pause", "media-playback-pause", N_("P_ause timer"), NULL, NULL, G_CALLBACK (timer_pause_callback) },
    { "Stop", "media-playback-stop", N_("S_top timer"), NULL, NULL, G_CALLBACK (timer_stop_callback) },
    { "Reset", "edit-undo", N_("R_eset"), NULL, NULL, G_CALLBACK (timer_reset_callback) },
    { "Preferences", "document-properties", N_("_Preferences"), NULL, NULL, G_CALLBACK (timer_preferences_callback) },
    { "About", "help-about", N_("_About"), NULL, NULL, G_CALLBACK (timer_about_callback) }
};

static char *ui = "<menuitem name='Item 1' action='Start' />"
                  "<menuitem name='Item 2' action='Pause' />"
                  "<menuitem name='Item 3' action='Stop' />"
                  "<menuitem name='Item 4' action='Reset' />"
                  "<menuitem name='Item 5' action='Preferences' />"
                  "<menuitem name='Item 6' action='About' />";

static void
timer_applet_destroy (CafePanelApplet *applet_widget, TimerApplet *applet)
{
    g_assert (applet);

    if (applet->timeout_id != 0)
    {
        g_source_remove(applet->timeout_id);
        applet->timeout_id = 0;
    }

    g_object_unref (applet->settings);

    notify_uninit ();
}

/* timer management */
static gboolean
timer_callback (TimerApplet *applet)
{
    gboolean retval = TRUE;
    gchar *label;
    gchar *name;
    gchar *tooltip;
    gint hours, minutes, seconds, duration, remaining;

    label = NULL;
    tooltip = NULL;

    name = g_settings_get_string (applet->settings, NAME_KEY);

    if (!applet->active)
    {
        applet->pause = FALSE;
        applet->elapsed = 0;

        ctk_label_set_text (applet->label, name);
        ctk_widget_set_tooltip_text (CTK_WIDGET (applet->label), "");
        ctk_widget_hide (CTK_WIDGET (applet->pause_image));
    }
    else
    {
        if (!applet->pause)
            applet->elapsed += STEP;

        duration = g_settings_get_int (applet->settings, DURATION_KEY);

        remaining = duration - (applet->elapsed / 1000);

        if (remaining <= 0)
        {
            applet->active = FALSE;
            applet->timeout_id = 0;

            label = g_strdup_printf ("Finished %s", name);
            ctk_label_set_text (applet->label, label);
            ctk_widget_set_tooltip_text (CTK_WIDGET (applet->label), name);
            ctk_widget_hide (CTK_WIDGET (applet->pause_image));

            if (g_settings_get_boolean (applet->settings, SHOW_NOTIFICATION_KEY))
            {
                NotifyNotification *n;
                n = notify_notification_new (name, _("Timer finished!"), APPLET_ICON);
                notify_notification_set_timeout (n, 30000);
                notify_notification_show (n, NULL);
                g_object_unref (G_OBJECT (n));
            }

            if (g_settings_get_boolean (applet->settings, SHOW_DIALOG_KEY))
            {
                CtkWidget *dialog = ctk_message_dialog_new_with_markup (NULL,
                                                                        CTK_DIALOG_MODAL,
                                                                        CTK_MESSAGE_INFO,
                                                                        CTK_BUTTONS_OK,
                                                                        "<b>%s</b>\n\n%s", name, _("Timer finished!"));
                ctk_dialog_run (CTK_DIALOG (dialog));
                ctk_widget_destroy (dialog);
            }

            /* stop further calls */
            retval = FALSE;
        }
        else
        {
            hours = remaining / 60 / 60;
            minutes = remaining / 60 % 60;
            seconds = remaining % 60;

            if (hours > 0)
                label = g_strdup_printf ("%02d:%02d:%02d", hours, minutes, seconds);
            else
                label = g_strdup_printf ("%02d:%02d", minutes, seconds);

            hours = duration / 60 / 60;
            minutes = duration / 60 % 60;
            seconds = duration % 60;

            if (hours > 0)
                tooltip = g_strdup_printf ("%s (%02d:%02d:%02d)", name, hours, minutes, seconds);
            else
                tooltip = g_strdup_printf ("%s (%02d:%02d)", name, minutes, seconds);

            ctk_label_set_text (applet->label, label);
            ctk_widget_set_tooltip_text (CTK_WIDGET (applet->label), tooltip);
            ctk_widget_set_visible (CTK_WIDGET (applet->pause_image), applet->pause);
        }

        g_free (label);
        g_free (tooltip);
    }

    /* update actions sensitiveness */
    ctk_action_set_sensitive (ctk_action_group_get_action (applet->action_group, "Start"), !applet->active || applet->pause);
    ctk_action_set_sensitive (ctk_action_group_get_action (applet->action_group, "Pause"), applet->active && !applet->pause);
    ctk_action_set_sensitive (ctk_action_group_get_action (applet->action_group, "Stop"), applet->active);
    ctk_action_set_sensitive (ctk_action_group_get_action (applet->action_group, "Reset"), !applet->active && !applet->pause && applet->elapsed);
    ctk_action_set_sensitive (ctk_action_group_get_action (applet->action_group, "Preferences"), !applet->active && !applet->pause);

    g_free (name);

    return retval;
}

/* start action */
static void
timer_start_callback (CtkAction *action, TimerApplet *applet)
{
    applet->active = TRUE;
    if (applet->pause)
        applet->pause = FALSE;
    else
        applet->elapsed = 0;
    applet->timeout_id = g_timeout_add (STEP, (GSourceFunc) timer_callback, applet);
}

/* pause action */
static void
timer_pause_callback (CtkAction *action, TimerApplet *applet)
{
    applet->pause = TRUE;
    if (applet->timeout_id != 0)
    {
        g_source_remove(applet->timeout_id);
        applet->timeout_id = 0;
    }
    timer_callback (applet);
}

/* stop action */
static void
timer_stop_callback (CtkAction *action, TimerApplet *applet)
{
    applet->active = FALSE;
    if (applet->timeout_id != 0)
    {
        g_source_remove(applet->timeout_id);
        applet->timeout_id = 0;
    }
    timer_callback (applet);
}

/* reset action */
static void
timer_reset_callback (CtkAction *action, TimerApplet *applet)
{
    applet->active = FALSE;
    applet->pause = FALSE;
    applet->elapsed = 0;
    timer_callback (applet);
}

/* Show the about dialog */
static void
timer_about_callback (CtkAction *action, TimerApplet *applet)
{
    const char* authors[] = { "Stefano Karapetsas <stefano@karapetsas.com>", NULL };

    ctk_show_about_dialog(NULL,
                          "title", _("About Timer Applet"),
                          "version", VERSION,
                          "copyright", _("Copyright \xc2\xa9 2014 Stefano Karapetsas\n"
                                         "Copyright \xc2\xa9 2015-2020 CAFE developers"),
                          "authors", authors,
                          "comments", _("Start a timer and receive a notification when it is finished"),
                          "translator-credits", _("translator-credits"),
                          "logo-icon-name", APPLET_ICON,
                          NULL);
}

/* calculate duration and save in GSettings */
static void
timer_spin_button_value_changed (CtkSpinButton *spinbutton, TimerApplet *applet)
{
    gint duration = 0;

    duration += ctk_spin_button_get_value (applet->hours) * 60 * 60;
    duration += ctk_spin_button_get_value (applet->minutes) * 60;
    duration += ctk_spin_button_get_value (applet->seconds);

    g_settings_set_int (applet->settings, DURATION_KEY, duration);
}

/* Show the preferences dialog */
static void
timer_preferences_callback (CtkAction *action, TimerApplet *applet)
{
    CtkDialog *dialog;
    CtkGrid *grid;
    CtkWidget *widget;
    gint duration, hours, minutes, seconds;

    duration = g_settings_get_int (applet->settings, DURATION_KEY);
    hours = duration / 60 / 60;
    minutes = duration / 60 % 60;
    seconds = duration % 60;

    dialog = CTK_DIALOG (ctk_dialog_new_with_buttons(_("Timer Applet Preferences"),
                                                     NULL,
                                                     CTK_DIALOG_MODAL,
                                                     "ctk-close",
                                                     CTK_RESPONSE_CLOSE,
                                                     NULL));
    grid = CTK_GRID (ctk_grid_new ());
    ctk_grid_set_row_spacing (grid, 12);
    ctk_grid_set_column_spacing (grid, 12);

    ctk_window_set_default_size (CTK_WINDOW (dialog), 350, 150);
    ctk_container_set_border_width (CTK_CONTAINER (dialog), 10);

    widget = ctk_label_new (_("Name:"));
    ctk_label_set_xalign (CTK_LABEL (widget), 1.0);
    ctk_label_set_yalign (CTK_LABEL (widget), 0.5);
    ctk_grid_attach (grid, widget, 1, 0, 1, 1);

    widget = ctk_entry_new ();
    ctk_grid_attach (grid, widget, 2, 0, 1, 1);
    g_settings_bind (applet->settings, NAME_KEY, widget, "text", G_SETTINGS_BIND_DEFAULT);

    widget = ctk_label_new (_("Hours:"));
    ctk_label_set_xalign (CTK_LABEL (widget), 1.0);
    ctk_label_set_yalign (CTK_LABEL (widget), 0.5);
    ctk_grid_attach (grid, widget, 1, 1, 1, 1);

    widget = ctk_spin_button_new_with_range (0.0, 100.0, 1.0);
    ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), hours);
    ctk_grid_attach (grid, widget, 2, 1, 1, 1);
    applet->hours = CTK_SPIN_BUTTON (widget);
    g_signal_connect (widget, "value-changed", G_CALLBACK (timer_spin_button_value_changed), applet);

    widget = ctk_label_new (_("Minutes:"));
    ctk_label_set_xalign (CTK_LABEL (widget), 1.0);
    ctk_label_set_yalign (CTK_LABEL (widget), 0.5);
    ctk_grid_attach (grid, widget, 1, 2, 1, 1);

    widget = ctk_spin_button_new_with_range (0.0, 59.0, 1.0);
    ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), minutes);
    ctk_grid_attach (grid, widget, 2, 2, 1, 1);;
    applet->minutes = CTK_SPIN_BUTTON (widget);
    g_signal_connect (widget, "value-changed", G_CALLBACK (timer_spin_button_value_changed), applet);

    widget = ctk_label_new (_("Seconds:"));
    ctk_label_set_xalign (CTK_LABEL (widget), 1.0);
    ctk_label_set_yalign (CTK_LABEL (widget), 0.5);
    ctk_grid_attach (grid, widget, 1, 3, 1, 1);

    widget = ctk_spin_button_new_with_range (0.0, 59.0, 1.0);
    ctk_spin_button_set_value (CTK_SPIN_BUTTON (widget), seconds);
    ctk_grid_attach (grid, widget, 2, 3, 1, 1);
    applet->seconds = CTK_SPIN_BUTTON (widget);
    g_signal_connect (widget, "value-changed", G_CALLBACK (timer_spin_button_value_changed), applet);

    widget = ctk_check_button_new_with_label (_("Show notification popup"));
    ctk_grid_attach (grid, widget, 2, 4, 1, 1);
    g_settings_bind (applet->settings, SHOW_NOTIFICATION_KEY, widget, "active", G_SETTINGS_BIND_DEFAULT);

    widget = ctk_check_button_new_with_label (_("Show dialog"));
    ctk_grid_attach (grid, widget, 2, 5, 1, 1);
    g_settings_bind (applet->settings, SHOW_DIALOG_KEY, widget, "active", G_SETTINGS_BIND_DEFAULT);

    ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (dialog)), CTK_WIDGET (grid), TRUE, TRUE, 0);

    g_signal_connect (dialog, "response", G_CALLBACK (ctk_widget_destroy), dialog);

    ctk_widget_show_all (CTK_WIDGET (dialog));
}

static gboolean
timer_applet_click (TimerApplet *applet)
{
    if (!applet->active && !applet->pause && applet->elapsed)
        timer_reset_callback (NULL, applet);
    else if (applet->active && !applet->pause)
        timer_pause_callback (NULL, applet);
    else if (!applet->active || applet->pause)
        timer_start_callback (NULL, applet);
    return FALSE;
}

static void
timer_settings_changed (GSettings *settings, gchar *key, TimerApplet *applet)
{
    timer_callback (applet);
}

static gboolean
timer_applet_fill (CafePanelApplet* applet_widget)
{
    TimerApplet *applet;

    g_set_application_name (_("Timer Applet"));
    ctk_window_set_default_icon_name (APPLET_ICON);

    g_object_set (ctk_settings_get_default (), "ctk-menu-images", TRUE, NULL);
    g_object_set (ctk_settings_get_default (), "ctk-button-images", TRUE, NULL);

    if (!notify_is_initted ())
        notify_init ("timer-applet");

    cafe_panel_applet_set_flags (applet_widget, CAFE_PANEL_APPLET_EXPAND_MINOR);
    cafe_panel_applet_set_background_widget (CAFE_PANEL_APPLET (applet_widget), CTK_WIDGET (applet_widget));

    applet = g_malloc0(sizeof(TimerApplet));
    applet->applet = applet_widget;
    applet->settings = cafe_panel_applet_settings_new (applet_widget,TIMER_SCHEMA);
    applet->timeout_id = 0;
    applet->active = FALSE;
    applet->pause = FALSE;

    applet->box = CTK_BOX (ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0));
    applet->image = CTK_IMAGE (ctk_image_new_from_icon_name (APPLET_ICON, CTK_ICON_SIZE_BUTTON));
    applet->pause_image = CTK_IMAGE (ctk_image_new_from_icon_name ("media-playback-pause", CTK_ICON_SIZE_BUTTON));
    applet->label = CTK_LABEL (ctk_label_new (""));

    /* we add the Ctk label into the applet */
    ctk_box_pack_start (applet->box,
                        CTK_WIDGET (applet->image),
                        TRUE, TRUE, 0);
    ctk_box_pack_start (applet->box,
                        CTK_WIDGET (applet->pause_image),
                        TRUE, TRUE, 0);
    ctk_box_pack_start (applet->box,
                        CTK_WIDGET (applet->label),
                        TRUE, TRUE, 3);

    ctk_container_add (CTK_CONTAINER (applet_widget),
                       CTK_WIDGET (applet->box));

    ctk_widget_show_all (CTK_WIDGET (applet->applet));
    ctk_widget_hide (CTK_WIDGET (applet->pause_image));

    g_signal_connect(G_OBJECT (applet->applet), "destroy",
                     G_CALLBACK (timer_applet_destroy),
                     applet);

    g_signal_connect_swapped(CTK_WIDGET (applet->applet), "button-release-event",
                             G_CALLBACK (timer_applet_click), applet);

    /* set up context menu */
    applet->action_group = ctk_action_group_new ("Timer Applet Actions");
    ctk_action_group_set_translation_domain (applet->action_group, GETTEXT_PACKAGE);
    ctk_action_group_add_actions (applet->action_group, applet_menu_actions,
                                  G_N_ELEMENTS (applet_menu_actions), applet);
    cafe_panel_applet_setup_menu (applet->applet, ui, applet->action_group);

    /* execute callback to set actions sensitiveness */
    timer_callback (applet);

    /* GSettings callback */
    g_signal_connect (G_OBJECT (applet->settings), "changed",
                      G_CALLBACK (timer_settings_changed), applet);

    return TRUE;
}

/* this function, called by cafe-panel, will create the applet */
static gboolean
timer_factory (CafePanelApplet* applet, const char* iid, gpointer data)
{
    gboolean retval = FALSE;

    if (!g_strcmp0 (iid, "TimerApplet"))
        retval = timer_applet_fill (applet);

    return retval;
}

/* needed by cafe-panel applet library */
CAFE_PANEL_APPLET_OUT_PROCESS_FACTORY("TimerAppletFactory",
                                      PANEL_TYPE_APPLET,
                                      "Timer applet",
                                      timer_factory,
                                      NULL)
