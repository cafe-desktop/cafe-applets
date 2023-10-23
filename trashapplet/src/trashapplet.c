/* -*- mode: c; c-basic-offset: 8 -*-
 * trashapplet.c
 *
 * Copyright (c) 2004  Michiel Sikkes <michiel@eyesopened.nl>,
 *               2004  Emmanuele Bassi <ebassi@gmail.com>
 *               2008  Ryan Lortie <desrt@desrt.ca>
 *                     Matthias Clasen <mclasen@redhat.com>
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <ctk/ctk.h>
#include <cdk/cdkkeysyms.h>
#include <gio/gio.h>
#include <cafe-panel-applet.h>

#include "trash-empty.h"
#include "xstuff.h"

typedef CafePanelAppletClass TrashAppletClass;

typedef struct
{
  CafePanelApplet applet;

  GFileMonitor *trash_monitor;
  GFile *trash;

  CtkImage *image;
  GIcon *icon;
  gint items;
} TrashApplet;

G_DEFINE_TYPE (TrashApplet, trash_applet, PANEL_TYPE_APPLET);
#define TRASH_TYPE_APPLET (trash_applet_get_type ())
#define TRASH_APPLET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                           TRASH_TYPE_APPLET, TrashApplet))

static void trash_applet_do_empty    (CtkAction   *action,
                                      TrashApplet *applet);
static void trash_applet_show_about  (CtkAction   *action,
                                      TrashApplet *applet);
static void trash_applet_open_folder (CtkAction   *action,
                                      TrashApplet *applet);
static void trash_applet_show_help   (CtkAction   *action,
                                      TrashApplet *applet);

static const CtkActionEntry trash_applet_menu_actions [] = {
	{ "EmptyTrash", "edit-clear", N_("_Empty Trash"),
	  NULL, NULL,
	  G_CALLBACK (trash_applet_do_empty) },
	{ "OpenTrash", "document-open", N_("_Open Trash"),
	  NULL, NULL,
	  G_CALLBACK (trash_applet_open_folder) },
	{ "HelpTrash", "help-browser", N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (trash_applet_show_help) },
	{ "AboutTrash", "help-about", N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (trash_applet_show_about) }
};

static void
trash_applet_monitor_changed (TrashApplet *applet)
{
  GError *error = NULL;
  GFileInfo *info;
  GIcon *icon;
  gint items;

  info = g_file_query_info (applet->trash,
                            G_FILE_ATTRIBUTE_STANDARD_ICON","
                            G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT,
                            0, NULL, &error);

  if (!info)
    {
      g_critical ("could not query trash:/: '%s'", error->message);
      g_error_free (error);

      return;
    }

  icon = g_file_info_get_icon (info);
  items = g_file_info_get_attribute_uint32 (info,
                                            G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT);

  if (!g_icon_equal (icon, applet->icon))
    {
      /* note: the size is meaningless here,
       * since we do set_pixel_size() later
       */
      ctk_image_set_from_gicon (CTK_IMAGE (applet->image),
                                icon, CTK_ICON_SIZE_MENU);

      if (applet->icon)
        g_object_unref (applet->icon);

      applet->icon = g_object_ref (icon);
    }

  if (items != applet->items)
    {
      if (items)
        {
          char *text;

          text = g_strdup_printf (ngettext ("%d Item in Trash",
                                            "%d Items in Trash",
                                            items), items);
          ctk_widget_set_tooltip_text (CTK_WIDGET (applet), text);
          g_free (text);
        }
      else
        ctk_widget_set_tooltip_text (CTK_WIDGET (applet),
                                     _("No Items in Trash"));

      applet->items = items;
    }

  g_object_unref (info);
}

static void
trash_applet_set_icon_size (TrashApplet *applet,
                            gint         size)
{
  /* copied from button-widget.c in the panel */
  if (size < 22)
    size = 16;
  else if (size < 24)
    size = 22;
  else if (size < 32)
    size = 24;
  else if (size < 48)
    size = 32;
  else
    size = 48;

  /* CtkImage already contains a check to do nothing if it's the same */
  ctk_image_set_pixel_size (applet->image, size);
}

static void
trash_applet_size_allocate (CtkWidget    *widget,
                            CdkRectangle *allocation)
{
  TrashApplet *applet = TRASH_APPLET (widget);

  CTK_WIDGET_CLASS (trash_applet_parent_class)
    ->size_allocate (widget, allocation);

  switch (cafe_panel_applet_get_orient (CAFE_PANEL_APPLET (applet)))
  {
    case CAFE_PANEL_APPLET_ORIENT_LEFT:
    case CAFE_PANEL_APPLET_ORIENT_RIGHT:
      trash_applet_set_icon_size (applet, allocation->width);
      break;

    case CAFE_PANEL_APPLET_ORIENT_UP:
    case CAFE_PANEL_APPLET_ORIENT_DOWN:
      trash_applet_set_icon_size (applet, allocation->height);
      break;
  }

}

static void
trash_applet_dispose (GObject *object)
{
  TrashApplet *applet = TRASH_APPLET (object);

  if (applet->trash_monitor)
    g_object_unref (applet->trash_monitor);
  applet->trash_monitor = NULL;

  if (applet->trash)
    g_object_unref (applet->trash);
  applet->trash = NULL;

  if (applet->image)
    g_object_unref (applet->image);
  applet->image = NULL;

  if (applet->icon)
    g_object_unref (applet->icon);
  applet->icon = NULL;

  G_OBJECT_CLASS (trash_applet_parent_class)->dispose (object);
}

static void
trash_applet_init (TrashApplet *applet)
{
  const CtkTargetEntry drop_types[] = { { "text/uri-list" } };

  /* needed to clamp ourselves to the panel size */
  cafe_panel_applet_set_flags (CAFE_PANEL_APPLET (applet), CAFE_PANEL_APPLET_EXPAND_MINOR);

  /* enable transparency hack */
  cafe_panel_applet_set_background_widget (CAFE_PANEL_APPLET (applet),
                                      CTK_WIDGET (applet));

  /* setup the image */
  applet->image = g_object_ref_sink (CTK_IMAGE (ctk_image_new ()));
  ctk_container_add (CTK_CONTAINER (applet),
                     CTK_WIDGET (applet->image));
  ctk_widget_show (CTK_WIDGET (applet->image));

  /* setup the trash backend */
  applet->trash = g_file_new_for_uri ("trash:/");
  applet->trash_monitor = g_file_monitor_file (applet->trash, 0, NULL, NULL);
  g_signal_connect_swapped (applet->trash_monitor, "changed",
                            G_CALLBACK (trash_applet_monitor_changed),
                            applet);

  /* setup drag and drop */
  ctk_drag_dest_set (CTK_WIDGET (applet), CTK_DEST_DEFAULT_ALL,
                     drop_types, G_N_ELEMENTS (drop_types),
                     CDK_ACTION_MOVE);

  /* synthesise the first update */
  applet->items = -1;
  trash_applet_monitor_changed (applet);
}

#define PANEL_SCHEMA "org.cafe.panel"
#define PANEL_ENABLE_ANIMATIONS "enable-animations"
static gboolean
trash_applet_button_release (CtkWidget      *widget,
                             CdkEventButton *event)
{
  TrashApplet *applet = TRASH_APPLET (widget);
  static GSettings *settings;

  if (settings == NULL)
    settings = g_settings_new (PANEL_SCHEMA);

  if (event->button == 1)
    {
      if (g_settings_get_boolean (settings, PANEL_ENABLE_ANIMATIONS))
        xstuff_zoom_anicafe (widget, NULL);

      trash_applet_open_folder (NULL, applet);

      return TRUE;
    }

  if (CTK_WIDGET_CLASS (trash_applet_parent_class)->button_release_event)
    return CTK_WIDGET_CLASS (trash_applet_parent_class)
        ->button_release_event (widget, event);
  else
    return FALSE;
}
static gboolean
trash_applet_key_press (CtkWidget   *widget,
                        CdkEventKey *event)
{
  TrashApplet *applet = TRASH_APPLET (widget);

  switch (event->keyval)
    {
     case CDK_KEY_KP_Enter:
     case CDK_KEY_ISO_Enter:
     case CDK_KEY_3270_Enter:
     case CDK_KEY_Return:
     case CDK_KEY_space:
     case CDK_KEY_KP_Space:
      trash_applet_open_folder (NULL, applet);
      return TRUE;

     default:
      break;
    }

  if (CTK_WIDGET_CLASS (trash_applet_parent_class)->key_press_event)
    return CTK_WIDGET_CLASS (trash_applet_parent_class)
      ->key_press_event (widget, event);
  else
    return FALSE;
}

static gboolean
trash_applet_drag_motion (CtkWidget      *widget,
                          CdkDragContext *context,
                          gint            x,
                          gint            y,
                          guint           time)
{
  GList *target;

  /* refuse drops of panel applets */
  for (target = cdk_drag_context_list_targets (context); target; target = target->next)
    {
      const char *name = cdk_atom_name (target->data);

      if (!strcmp (name, "application/x-panel-icon-internal"))
        break;
    }

  if (target)
    cdk_drag_status (context, 0, time);
  else
    cdk_drag_status (context, CDK_ACTION_MOVE, time);

  return TRUE;
}

/* TODO - Must HIGgify this dialog */
static void
error_dialog (TrashApplet *applet, const gchar *error, ...)
{
  va_list args;
  gchar *error_string;
  CtkWidget *dialog;

  g_return_if_fail (error != NULL);

  va_start (args, error);
  error_string = g_strdup_vprintf (error, args);
  va_end (args);

  dialog = ctk_message_dialog_new (NULL, CTK_DIALOG_MODAL,
                                   CTK_MESSAGE_ERROR, CTK_BUTTONS_OK,
                                   "%s", error_string);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (ctk_widget_destroy),
                    NULL);

  ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);
  ctk_window_set_screen (CTK_WINDOW(dialog),
                         ctk_widget_get_screen (CTK_WIDGET (applet)));
  ctk_widget_show (dialog);

  g_free (error_string);
}

static void
trash_applet_do_empty (CtkAction   *action,
                       TrashApplet *applet)
{
  trash_empty (CTK_WIDGET (applet));
}

static void
trash_applet_open_folder (CtkAction   *action,
                          TrashApplet *applet)
{
  GError *err = NULL;

  ctk_show_uri_on_window (NULL,
                          "trash:",
                          ctk_get_current_event_time (),
                          &err);

  if (err)
    {
      error_dialog (applet, _("Error while spawning baul:\n%s"),
      err->message);
      g_error_free (err);
    }
}

static void
trash_applet_show_help (CtkAction   *action,
                        TrashApplet *applet)
{
  GError *err = NULL;

  /* FIXME - Actually, we need a user guide */
  ctk_show_uri_on_window (NULL,
                          "help:cafe-trashapplet",
                          ctk_get_current_event_time (),
                          &err);

  if (err)
    {
      error_dialog (applet,
                    _("There was an error displaying help: %s"),
                    err->message);
      g_error_free (err);
    }
}


static void
trash_applet_show_about (CtkAction   *action,
                         TrashApplet *applet)
{
  static const char *authors[] = {
    "Michiel Sikkes <michiel@eyesopened.nl>",
    "Emmanuele Bassi <ebassi@gmail.com>",
    "Sebastian Bacher <seb128@canonical.com>",
    "James Henstridge <james.henstridge@canonical.com>",
    "Ryan Lortie <desrt@desrt.ca>",
    NULL
  };

  static const char *documenters[] = {
    "Michiel Sikkes <michiel@eyesopened.nl>",
    N_("CAFE Documentation Team"),
    NULL
  };

#ifdef ENABLE_NLS
  const char **p;
  for (p = documenters; *p; ++p)
    *p = _(*p);
#endif

  ctk_show_about_dialog (NULL,
                         "title", _("About Trash Applet"),
                         "version", VERSION,
                         "copyright", _("Copyright \xc2\xa9 2004 Michiel Sikkes\n"
                                        "Copyright \xc2\xa9 2008 Ryan Lortie\n"
                                        "Copyright \xc2\xa9 2012-2020 CAFE developers"),
                         "comments", _("A CAFE trash bin that lives in your panel. "
                                       "You can use it to view the trash or drag "
                                       "and drop items into the trash."),
                         "authors", authors,
                         "documenters", documenters,
                         "translator-credits", _("translator-credits"),
                         "logo_icon_name", "user-trash-full",
                         NULL);
}

static gboolean
confirm_delete_immediately (CtkWidget *parent_view,
                            gint num_files,
                            gboolean all)
{
  CdkScreen *screen;
  CtkWidget *dialog, *hbox, *vbox, *image, *label;
  gchar *str, *prompt, *detail;
  int response;

  screen = ctk_widget_get_screen (parent_view);

  dialog = ctk_dialog_new ();
  ctk_window_set_screen (CTK_WINDOW (dialog), screen);
  atk_object_set_role (ctk_widget_get_accessible (dialog), ATK_ROLE_ALERT);
  ctk_window_set_title (CTK_WINDOW (dialog), _("Delete Immediately?"));
  ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
  ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);

  ctk_widget_realize (dialog);
  cdk_window_set_transient_for (ctk_widget_get_window (CTK_WIDGET (dialog)),
                                cdk_screen_get_root_window (screen));
  ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);

  ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), 14);

  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
  ctk_widget_show (hbox);
  ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (dialog))), hbox,
                      FALSE, FALSE, 0);

  image = ctk_image_new_from_icon_name ("dialog-question",
                                        CTK_ICON_SIZE_DIALOG);
  ctk_widget_set_halign (image, CTK_ALIGN_CENTER);
  ctk_widget_set_valign (image, CTK_ALIGN_START);
  ctk_widget_show (image);
  ctk_box_pack_start (CTK_BOX (hbox), image, FALSE, FALSE, 0);

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
  ctk_box_pack_start (CTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  ctk_widget_show (vbox);

  if (all)
    {
      prompt = _("Cannot move items to trash, do you want to delete them immediately?");
      detail = g_strdup_printf ("None of the %d selected items can be moved to the Trash", num_files);
    }
  else
    {
      prompt = _("Cannot move some items to trash, do you want to delete these immediately?");
      detail = g_strdup_printf ("%d of the selected items cannot be moved to the Trash", num_files);
    }

  str = g_strconcat ("<span weight=\"bold\" size=\"larger\">",
                     prompt, "</span>", NULL);
  label = ctk_label_new (str);
  ctk_label_set_use_markup (CTK_LABEL (label), TRUE);
  ctk_label_set_justify (CTK_LABEL (label), CTK_JUSTIFY_LEFT);
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);
  ctk_widget_show (label);
  g_free (str);

  label = ctk_label_new (detail);
  ctk_label_set_justify (CTK_LABEL (label), CTK_JUSTIFY_LEFT);
  ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);
  ctk_widget_show (label);
  g_free (detail);

  ctk_dialog_add_button (CTK_DIALOG (dialog), "ctk-cancel",
                         CTK_RESPONSE_CANCEL);
  ctk_dialog_add_button (CTK_DIALOG (dialog), "ctk-delete",
                         CTK_RESPONSE_YES);
  ctk_dialog_set_default_response (CTK_DIALOG (dialog),
                                   CTK_RESPONSE_YES);

  response = ctk_dialog_run (CTK_DIALOG (dialog));

  ctk_widget_destroy (CTK_WIDGET (dialog));

  return response == CTK_RESPONSE_YES;
}

static void
trash_applet_drag_data_received (CtkWidget        *widget,
                                 CdkDragContext   *context,
                                 gint              x,
                                 gint              y,
                                 CtkSelectionData *selectiondata,
                                 guint             info,
                                 guint             time_)
{
  gchar **list;
  gint i;
  GList *trashed = NULL;
  GList *untrashable = NULL;
  GList *l;
  GError *error = NULL;

  list = g_uri_list_extract_uris ((gchar *)ctk_selection_data_get_data (selectiondata));

  for (i = 0; list[i]; i++)
    {
      GFile *file;

      file = g_file_new_for_uri (list[i]);

      if (!g_file_trash (file, NULL, NULL))
        {
          untrashable = g_list_prepend (untrashable, file);
        }
      else
        {
          trashed = g_list_prepend (trashed, file);
        }
    }

  if (untrashable)
    {
      if (confirm_delete_immediately (widget,
                                      g_list_length (untrashable),
                                      trashed == NULL))
        {
          for (l = untrashable; l; l = l->next)
            {
              if (!g_file_delete (l->data, NULL, &error))
                {
/*
* FIXME: uncomment me after branched (we're string frozen)
                  error_dialog (applet,
                                _("Unable to delete '%s': %s"),
                                g_file_get_uri (l->data),
                                error->message);
*/
                                g_clear_error (&error);
                }
            }
        }
    }

  g_list_free_full (untrashable, g_object_unref);
  g_list_free_full (trashed, g_object_unref);

  g_strfreev (list);

  ctk_drag_finish (context, TRUE, FALSE, time_);
}

static void
trash_applet_class_init (TrashAppletClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (class);

  gobject_class->dispose = trash_applet_dispose;
  widget_class->size_allocate = trash_applet_size_allocate;
  widget_class->button_release_event = trash_applet_button_release;
  widget_class->key_press_event = trash_applet_key_press;
  widget_class->drag_motion = trash_applet_drag_motion;
  widget_class->drag_data_received = trash_applet_drag_data_received;
}

static gboolean
trash_applet_factory (CafePanelApplet *applet,
                      const gchar *iid,
                      gpointer     data)
{
  gboolean retval = FALSE;

  if (!strcmp (iid, "TrashApplet"))
    {
      CtkActionGroup *action_group;
      gchar          *ui_path;

      g_set_application_name (_("Trash Applet"));

      ctk_window_set_default_icon_name ("user-trash");

      g_object_set (ctk_settings_get_default (), "ctk-menu-images", TRUE, NULL);

      /* Set up the menu */
      action_group = ctk_action_group_new ("Trash Applet Actions");
      ctk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
      ctk_action_group_add_actions (action_group,
				    trash_applet_menu_actions,
				    G_N_ELEMENTS (trash_applet_menu_actions),
				    applet);
      ui_path = g_build_filename (TRASH_MENU_UI_DIR, "trashapplet-menu.xml", NULL);
      cafe_panel_applet_setup_menu_from_file (applet, ui_path, action_group);
      g_free (ui_path);
      g_object_unref (action_group);

      ctk_widget_show (CTK_WIDGET (applet));

      retval = TRUE;
  }

  return retval;
}

CAFE_PANEL_APPLET_OUT_PROCESS_FACTORY ("TrashAppletFactory",
				  TRASH_TYPE_APPLET,
				  "TrashApplet",
				  trash_applet_factory,
				  NULL)
