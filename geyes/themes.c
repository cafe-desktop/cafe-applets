/*
 * Copyright (C) 1999 Dave Camp <dave@davec.dhs.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include <ctk/ctk.h>
#include <gio/gio.h>
#include "geyes.h"

#define NUM_THEME_DIRECTORIES 2
#define HIG_IDENTATION  "    "

static char *theme_directories[NUM_THEME_DIRECTORIES];

enum {
	COL_THEME_DIR = 0,
	COL_THEME_NAME,
	TOTAL_COLS
};

void theme_dirs_create (void)
{
	static gboolean themes_created = FALSE;

	if (themes_created == TRUE)
		return;

	theme_directories[0] = g_build_filename(GEYES_THEMES_DIR, NULL);

		theme_directories[1] = g_build_filename(g_get_user_config_dir(), "cafe", "geyes-themes", NULL);

	themes_created = TRUE;
}

static void
parse_theme_file (EyesApplet *eyes_applet, FILE *theme_file)
{
        gchar line_buf [512]; /* prolly overkill */
        gchar *token;

        if (fgets (line_buf, 512, theme_file) == NULL)
               printf("fgets error\n");

        while (!feof (theme_file)) {
                token = strtok (line_buf, "=");
                if (strncmp (token, "wall-thickness",
                             strlen ("wall-thickness")) == 0) {
                        token += strlen ("wall-thickness");
                        while (!isdigit (*token)) {
                                token++;
                        }
                        sscanf (token, "%d", &eyes_applet->wall_thickness);
                } else if (strncmp (token, "num-eyes", strlen ("num-eyes")) == 0) {
                        token += strlen ("num-eyes");
                        while (!isdigit (*token)) {
                                token++;
                        }
                        sscanf (token, "%d", &eyes_applet->num_eyes);
			if (eyes_applet->num_eyes > MAX_EYES)
				eyes_applet->num_eyes = MAX_EYES;
                } else if (strncmp (token, "eye-pixmap", strlen ("eye-pixmap")) == 0) {
                        token = strtok (NULL, "\"");
                        token = strtok (NULL, "\"");
                        if (eyes_applet->eye_filename != NULL)
                                g_free (eyes_applet->eye_filename);
                        eyes_applet->eye_filename = g_strdup_printf ("%s%s",
                                                                    eyes_applet->theme_dir,
                                                                    token);
                } else if (strncmp (token, "pupil-pixmap", strlen ("pupil-pixmap")) == 0) {
                        token = strtok (NULL, "\"");
                        token = strtok (NULL, "\"");
            if (eyes_applet->pupil_filename != NULL)
                    g_free (eyes_applet->pupil_filename);
            eyes_applet->pupil_filename
                    = g_strdup_printf ("%s%s",
                                       eyes_applet->theme_dir,
                                       token);
                }
                if (fgets (line_buf, 512, theme_file) == NULL)
                        printf("fgets error\n");
        }
}

int
load_theme (EyesApplet *eyes_applet, const gchar *theme_dir)
{
	CtkWidget *dialog;

	FILE* theme_file;
        gchar *file_name;

        eyes_applet->theme_dir = g_strdup_printf ("%s/", theme_dir);

        file_name = g_strdup_printf("%s%s",theme_dir,"/config");
        theme_file = fopen (file_name, "r");
        g_free (file_name);
        if (theme_file == NULL) {
        	g_free (eyes_applet->theme_dir);
        	eyes_applet->theme_dir = g_strdup_printf (GEYES_THEMES_DIR "Default-tiny/");
                theme_file = fopen (GEYES_THEMES_DIR "Default-tiny/config", "r");
        }

	/* if it's still NULL we've got a major problem */
	if (theme_file == NULL) {
		dialog = ctk_message_dialog_new_with_markup (NULL,
				CTK_DIALOG_DESTROY_WITH_PARENT,
				CTK_MESSAGE_ERROR,
				CTK_BUTTONS_OK,
				"<b>%s</b>\n\n%s",
				_("Can not launch the eyes applet."),
				_("There was a fatal error while trying to load the theme."));

		ctk_dialog_run (CTK_DIALOG (dialog));
		ctk_widget_destroy (dialog);

		ctk_widget_destroy (CTK_WIDGET (eyes_applet->applet));

		return FALSE;
	}

        parse_theme_file (eyes_applet, theme_file);
        fclose (theme_file);

        eyes_applet->theme_name = g_strdup (theme_dir);

        if (eyes_applet->eye_image)
        	g_object_unref (eyes_applet->eye_image);
        eyes_applet->eye_image = cdk_pixbuf_new_from_file (eyes_applet->eye_filename, NULL);
        if (eyes_applet->pupil_image)
        	g_object_unref (eyes_applet->pupil_image);
        eyes_applet->pupil_image = cdk_pixbuf_new_from_file (eyes_applet->pupil_filename, NULL);

	eyes_applet->eye_height = cdk_pixbuf_get_height (eyes_applet->eye_image);
        eyes_applet->eye_width = cdk_pixbuf_get_width (eyes_applet->eye_image);
        eyes_applet->pupil_height = cdk_pixbuf_get_height (eyes_applet->pupil_image);
        eyes_applet->pupil_width = cdk_pixbuf_get_width (eyes_applet->pupil_image);

	return TRUE;
}

static void
destroy_theme (EyesApplet *eyes_applet)
{
	/* Dunno about this - to unref or not to unref? */
	if (eyes_applet->eye_image != NULL) {
        	g_object_unref (eyes_applet->eye_image);
        	eyes_applet->eye_image = NULL;
        }
        if (eyes_applet->pupil_image != NULL) {
        	g_object_unref (eyes_applet->pupil_image);
        	eyes_applet->pupil_image = NULL;
	}

        g_free (eyes_applet->theme_dir);
        g_free (eyes_applet->theme_name);
}

static void
theme_selected_cb (CtkTreeSelection *selection, gpointer data)
{
	EyesApplet *eyes_applet = data;
	CtkTreeModel *model;
	CtkTreeIter iter;
	gchar *theme;
	gchar *theme_dir;

	if (!ctk_tree_selection_get_selected (selection, &model, &iter))
		return;

	ctk_tree_model_get (model, &iter, COL_THEME_DIR, &theme, -1);

	g_return_if_fail (theme);

	theme_dir = g_strdup_printf ("%s/", theme);
	if (!g_ascii_strncasecmp (theme_dir, eyes_applet->theme_dir, strlen (theme_dir))) {
		g_free (theme_dir);
		return;
	}
	g_free (theme_dir);

	destroy_eyes (eyes_applet);
        destroy_theme (eyes_applet);
        load_theme (eyes_applet, theme);
        setup_eyes (eyes_applet);

	g_settings_set_string (
		eyes_applet->settings, "theme-path", theme);

	g_free (theme);
}

static void
phelp_cb (CtkDialog *dialog)
{
	GError *error = NULL;

	ctk_show_uri_on_window (CTK_WINDOW (dialog),
	                        "help:cafe-geyes/geyes-settings",
	                        ctk_get_current_event_time (),
	                        &error);

	if (error) {
		CtkWidget *error_dialog = ctk_message_dialog_new (NULL, CTK_DIALOG_MODAL, CTK_MESSAGE_ERROR, CTK_BUTTONS_CLOSE,
								  _("There was an error displaying help: %s"), error->message);
		g_signal_connect (G_OBJECT (error_dialog), "response", G_CALLBACK (ctk_widget_destroy) , NULL);
		ctk_window_set_resizable (CTK_WINDOW (error_dialog), FALSE);
		ctk_window_set_screen (CTK_WINDOW (error_dialog), ctk_widget_get_screen (CTK_WIDGET (dialog)));
		ctk_widget_show (error_dialog);
		g_error_free (error);
		error = NULL;
	}
}

static void
presponse_cb (CtkDialog *dialog, gint id, gpointer data)
{
	EyesApplet *eyes_applet = data;
	if(id == CTK_RESPONSE_HELP){
		phelp_cb (dialog);
		return;
	}


	ctk_widget_destroy (CTK_WIDGET (dialog));

	eyes_applet->prop_box.pbox = NULL;
}

void
properties_cb (CtkAction  *action,
	       EyesApplet *eyes_applet)
{
	CtkWidget *pbox, *hbox;
	CtkWidget *vbox, *indent;
	CtkWidget *categories_vbox;
	CtkWidget *category_vbox, *control_vbox;
        CtkWidget *tree;
	CtkWidget *scrolled;
        CtkWidget *label;
        CtkListStore *model;
        CtkTreeViewColumn *column;
        CtkCellRenderer *cell;
        CtkTreeSelection *selection;
        CtkTreeIter iter;
        DIR *dfd;
        struct dirent *dp;
        int i;
#ifdef PATH_MAX
        gchar filename [PATH_MAX];
#else
	gchar *filename;
#endif
        gchar *title;

	if (eyes_applet->prop_box.pbox) {
		ctk_window_set_screen (
			CTK_WINDOW (eyes_applet->prop_box.pbox),
			ctk_widget_get_screen (CTK_WIDGET (eyes_applet->applet)));
		ctk_window_present (CTK_WINDOW (eyes_applet->prop_box.pbox));
		return;
	}

        pbox = ctk_dialog_new_with_buttons (_("Eyes Preferences"), NULL,
        				     CTK_DIALOG_DESTROY_WITH_PARENT,
					     "ctk-close", CTK_RESPONSE_CLOSE,
					     "ctk-help", CTK_RESPONSE_HELP,
					     NULL);

	ctk_window_set_screen (CTK_WINDOW (pbox),
			       ctk_widget_get_screen (CTK_WIDGET (eyes_applet->applet)));

	ctk_widget_set_size_request (CTK_WIDGET (pbox), 300, 200);
        ctk_dialog_set_default_response(CTK_DIALOG (pbox), CTK_RESPONSE_CLOSE);
        ctk_container_set_border_width (CTK_CONTAINER (pbox), 5);
	ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (pbox))), 2);

        g_signal_connect (pbox, "response",
			  G_CALLBACK (presponse_cb),
			  eyes_applet);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
	ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);
	ctk_widget_show (vbox);

	ctk_box_pack_start (CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (pbox))), vbox,
			    TRUE, TRUE, 0);

	categories_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 18);
	ctk_box_pack_start (CTK_BOX (vbox), categories_vbox, TRUE, TRUE, 0);
	ctk_widget_show (categories_vbox);

	category_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (CTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);
	ctk_widget_show (category_vbox);

	title = g_strconcat ("<span weight=\"bold\">", _("Themes"), "</span>", NULL);
	label = ctk_label_new (_(title));
	ctk_label_set_use_markup (CTK_LABEL (label), TRUE);
	ctk_label_set_justify (CTK_LABEL (label), CTK_JUSTIFY_LEFT);
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);
	ctk_box_pack_start (CTK_BOX (category_vbox), label, FALSE, FALSE, 0);
	g_free (title);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_pack_start (CTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	indent = ctk_label_new (HIG_IDENTATION);
	ctk_label_set_justify (CTK_LABEL (indent), CTK_JUSTIFY_LEFT);
	ctk_box_pack_start (CTK_BOX (hbox), indent, FALSE, FALSE, 0);
	ctk_widget_show (indent);

	control_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (CTK_BOX (hbox), control_vbox, TRUE, TRUE, 0);
	ctk_widget_show (control_vbox);

	label = ctk_label_new_with_mnemonic (_("_Select a theme:"));
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);
	ctk_box_pack_start (CTK_BOX (control_vbox), label, FALSE, FALSE, 0);

	scrolled = ctk_scrolled_window_new (NULL, NULL);
	ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled), CTK_SHADOW_IN);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled),
					CTK_POLICY_AUTOMATIC,
					CTK_POLICY_AUTOMATIC);

	model = ctk_list_store_new (TOTAL_COLS, G_TYPE_STRING, G_TYPE_STRING);
	tree = ctk_tree_view_new_with_model (CTK_TREE_MODEL (model));
	ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (tree), FALSE);
	ctk_label_set_mnemonic_widget (CTK_LABEL (label), tree);
	g_object_unref (model);

	ctk_container_add (CTK_CONTAINER (scrolled), tree);

	cell = ctk_cell_renderer_text_new ();
	column = ctk_tree_view_column_new_with_attributes ("not used", cell,
                                                           "text", COL_THEME_NAME, NULL);
        ctk_tree_view_append_column (CTK_TREE_VIEW (tree), column);

        selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (tree));
	g_signal_connect (selection, "changed",
			  G_CALLBACK (theme_selected_cb),
			  eyes_applet);

	if ( ! g_settings_is_writable (eyes_applet->settings, "theme-path")) {
		ctk_widget_set_sensitive (tree, FALSE);
		ctk_widget_set_sensitive (label, FALSE);
	}

        for (i = 0; i < NUM_THEME_DIRECTORIES; i++) {
                if ((dfd = opendir (theme_directories[i])) != NULL) {
                        while ((dp = readdir (dfd)) != NULL) {
                                if (dp->d_name[0] != '.') {
                                        gchar *theme_dir;
					gchar *theme_name;
#ifdef PATH_MAX
                                        strcpy (filename,
                                                theme_directories[i]);
                                        strcat (filename, dp->d_name);
#else
					asprintf (&filename, theme_directories[i], dp->d_name);
#endif
					theme_dir = g_strdup_printf ("%s/", filename);
					theme_name = g_path_get_basename (filename);

                                        ctk_list_store_append (model, &iter);
                                        ctk_list_store_set (model, &iter,
							    COL_THEME_DIR, &filename,
							    COL_THEME_NAME, theme_name,
							    -1);

					if (!g_ascii_strncasecmp (eyes_applet->theme_dir, theme_dir, strlen (theme_dir))) {
                                        	CtkTreePath *path;
                                        	path = ctk_tree_model_get_path (CTK_TREE_MODEL (model),
                                                        			&iter);
                                                ctk_tree_view_set_cursor (CTK_TREE_VIEW (tree),
                                                			  path,
                                                			  NULL,
                                                			  FALSE);
                                                ctk_tree_path_free (path);
                                        }
					g_free (theme_name);
                                        g_free (theme_dir);
                                }
                        }
                        closedir (dfd);
                }
        }
#ifndef PATH_MAX
	g_free (filename);
#endif

        ctk_box_pack_start (CTK_BOX (control_vbox), scrolled, TRUE, TRUE, 0);

        ctk_widget_show_all (pbox);

        eyes_applet->prop_box.pbox = pbox;

	return;
}


