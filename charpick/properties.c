/* properties.c -- properties dialog box and session management for character
 * picker applet. Much of this is adapted from modemlights/properties.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "charpick.h"

#include <string.h>

#include <atk/atkrelation.h>
#include <ctk/ctk.h>

#define CHARPICK_STOCK_EDIT "charpick-stock-edit"

void
register_stock_for_edit (void)
{
  static gboolean registered = FALSE;
  if (!registered)
  {
    CtkIconFactory *factory;
    CtkIconSet     *icons;

    static const CtkStockItem edit_item [] = {
           { CHARPICK_STOCK_EDIT, N_("_Edit"), 0, 0, GETTEXT_PACKAGE },
    };
    icons = ctk_icon_factory_lookup_default ("ctk-preferences");
    factory = ctk_icon_factory_new ();
    ctk_icon_factory_add (factory, CHARPICK_STOCK_EDIT, icons);
    ctk_icon_factory_add_default (factory);
    ctk_stock_add_static (edit_item, 1);
    registered = TRUE;
  }
}

#if 0
static void
set_atk_relation (CtkWidget *label, CtkWidget *widget)
{
  AtkObject *atk_widget;
  AtkObject *atk_label;
  AtkRelationSet *relation_set;
  AtkRelation *relation;
  AtkObject *targets[1];

  atk_widget = ctk_widget_get_accessible (widget);
  atk_label = ctk_widget_get_accessible (label);
  
  /* set label-for relation */
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), widget);	

  /* return if gail is not loaded */
  if (CTK_IS_ACCESSIBLE (atk_widget) == FALSE)
    return;

  /* set label-by relation */
  relation_set = atk_object_ref_relation_set (atk_widget);
  targets[0] = atk_label;
  relation = atk_relation_new (targets, 1, ATK_RELATION_LABELLED_BY);
  atk_relation_set_add (relation_set, relation);
  g_object_unref (G_OBJECT (relation)); 
}
#endif /* 0 */

/* sets accessible name and description */

static void
set_access_namedesc (CtkWidget *widget, const gchar *name, const gchar *desc)
{
    AtkObject *obj;

    obj = ctk_widget_get_accessible (widget);
    if (! CTK_IS_ACCESSIBLE (obj))
       return;

    if ( desc )
       atk_object_set_description (obj, desc);
    if ( name )
       atk_object_set_name (obj, name);
}

void
add_edit_dialog_create (charpick_data *curr_data, gchar *string, gchar *title)
{
	CtkWidget *dialog;
	CtkWidget *entry;
	CtkWidget *dbox;
	CtkWidget *vbox, *hbox;
	CtkWidget *label;

	dialog = ctk_dialog_new_with_buttons (_(title), CTK_WINDOW (curr_data->propwindow),
							    CTK_DIALOG_DESTROY_WITH_PARENT,
							    "ctk-cancel", CTK_RESPONSE_CANCEL,
							    "ctk-ok", CTK_RESPONSE_OK,
							    NULL);

	ctk_window_set_transient_for (CTK_WINDOW (dialog), CTK_WINDOW (curr_data->propwindow));
	ctk_widget_set_sensitive (curr_data->propwindow, FALSE);

	ctk_dialog_set_default_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);
	ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
	ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area(CTK_DIALOG (dialog))), 2);

	dbox = ctk_dialog_get_content_area(CTK_DIALOG (dialog));
	
	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);
	ctk_box_pack_start (CTK_BOX (dbox), vbox, TRUE, TRUE, 0);
	ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);
	
	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
	ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	label = ctk_label_new_with_mnemonic (_("_Palette:"));
	ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);
		
	entry = ctk_entry_new ();
	ctk_label_set_mnemonic_widget (CTK_LABEL (label), entry);
	 ctk_entry_set_activates_default (CTK_ENTRY (entry), TRUE);
	ctk_box_pack_start (CTK_BOX (hbox), entry, TRUE, TRUE, 0);

	set_access_namedesc (entry, _("Palette entry"),
				         _("Modify a palette by adding or removing characters"));
	if (string)
		ctk_entry_set_text (CTK_ENTRY (entry), string);

	curr_data->add_edit_dialog = dialog;
	curr_data->add_edit_entry = entry;
}

static void
add_palette_cb (CtkDialog *dialog, int response_id, charpick_data *curr_data)
{
	GList *list = curr_data->chartable;
	CtkTreeModel *model;
	CtkTreeIter iter;
	CtkTreeSelection *selection;
	CtkTreePath *path;
	char *new;

	ctk_widget_set_sensitive (curr_data->propwindow, TRUE);

	if (response_id != CTK_RESPONSE_OK) {
		ctk_widget_destroy (curr_data->add_edit_dialog);
		return;
	}
	
	new = ctk_editable_get_chars (CTK_EDITABLE (curr_data->add_edit_entry), 0, -1);

	ctk_widget_destroy (curr_data->add_edit_dialog);

	if (!new || strlen (new) == 0)
		return;
	
	list = g_list_append (list, new);

	if (curr_data->chartable == NULL) {
		curr_data->chartable = list;
		curr_data->charlist = curr_data->chartable->data;
		build_table (curr_data);

		if (g_settings_is_writable (curr_data->settings, "current-list"))
			g_settings_set_string (curr_data->settings,
						      "current-list", 
					  	       curr_data->charlist);
	}

	save_chartable (curr_data);
  	populate_menu (curr_data);
  	
  	model = ctk_tree_view_get_model (CTK_TREE_VIEW (curr_data->pref_tree));

	ctk_list_store_append (CTK_LIST_STORE (model), &iter);
	ctk_list_store_set (CTK_LIST_STORE (model), &iter, 0, new, 1, new, -1);

	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (curr_data->pref_tree));
	ctk_tree_selection_select_iter (selection, &iter);

	path = ctk_tree_model_get_path (model, &iter);
	ctk_tree_view_scroll_to_cell (CTK_TREE_VIEW (curr_data->pref_tree), path,
						  NULL, FALSE, 0.0, 0.0);

	ctk_tree_path_free (path);
}

static void
edit_palette_cb (CtkDialog *dialog, int response_id, charpick_data *curr_data)
{
	CtkTreeSelection *selection;
	CtkTreeIter iter;
	CtkTreeModel *model;
	GList *list;
	char *new, *charlist;

	ctk_widget_set_sensitive (curr_data->propwindow, TRUE);

        if (response_id != CTK_RESPONSE_OK) {
		ctk_widget_destroy (curr_data->add_edit_dialog);
		return;
	}

	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (curr_data->pref_tree));

	if (!ctk_tree_selection_get_selected (selection, &model, &iter))
		return;

	ctk_tree_model_get (model, &iter, 1, &charlist, -1);

	new = ctk_editable_get_chars (CTK_EDITABLE (curr_data->add_edit_entry), 0, -1);

	ctk_widget_destroy (curr_data->add_edit_dialog);
	
	if (!new || (g_ascii_strcasecmp (new, charlist) == 0))
		return;
		
	list = g_list_find (curr_data->chartable, charlist);
	list->data = new;
	save_chartable (curr_data);
  	populate_menu (curr_data);
	ctk_list_store_set (CTK_LIST_STORE (model), &iter, 0, new, 1, new, -1);
	
	if (g_ascii_strcasecmp (curr_data->charlist, charlist) == 0) {
		curr_data->charlist = new;
		build_table (curr_data);

		if (g_settings_is_writable (curr_data->settings, "current-list"))
			g_settings_set_string (curr_data->settings, "current-list", curr_data->charlist);
	}
	
	g_free (charlist);
}

static void
add_palette (CtkButton *buttonk, charpick_data *curr_data)
{
	if (curr_data->add_edit_dialog == NULL) {
		add_edit_dialog_create (curr_data, NULL, _("Add Palette"));

		g_signal_connect (curr_data->add_edit_dialog, 
				  "response", 
				  G_CALLBACK (add_palette_cb),
				  curr_data);

		g_signal_connect (curr_data->add_edit_dialog,
				  "destroy",
				  G_CALLBACK (ctk_widget_destroyed),
				  &curr_data->add_edit_dialog);

		ctk_widget_show_all (curr_data->add_edit_dialog);
	} else {
		ctk_window_set_screen (CTK_WINDOW (curr_data->add_edit_dialog),
				       ctk_widget_get_screen (CTK_WIDGET (curr_data->applet)));

		ctk_window_present (CTK_WINDOW (curr_data->add_edit_dialog));
	}
}

static void
edit_palette (CtkButton *button, charpick_data *curr_data)
{
	CtkTreeSelection *selection;
	CtkTreeIter iter;
	CtkTreeModel *model;
	char *charlist;

	if (curr_data->add_edit_dialog == NULL) {
		selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (curr_data->pref_tree));

		if (!ctk_tree_selection_get_selected (selection, &model, &iter))
			return;

		ctk_tree_model_get (model, &iter, 1, &charlist, -1);
		
		add_edit_dialog_create (curr_data, charlist, _("Edit Palette"));

		g_signal_connect (curr_data->add_edit_dialog, 
				  "response", 
				  G_CALLBACK (edit_palette_cb),
				  curr_data);

		g_signal_connect (curr_data->add_edit_dialog,
				  "destroy",
				  G_CALLBACK (ctk_widget_destroyed),
				  &curr_data->add_edit_dialog);

		ctk_widget_show_all (curr_data->add_edit_dialog);
	} else {
		ctk_window_set_screen (CTK_WINDOW (curr_data->add_edit_dialog),
				       ctk_widget_get_screen (CTK_WIDGET (curr_data->applet)));

		ctk_window_present (CTK_WINDOW (curr_data->add_edit_dialog));
	}
}

static void
delete_palette (CtkButton *button, charpick_data *curr_data)
{
	CtkTreeSelection *selection;
	CtkTreeIter iter, next;
	CtkTreeModel *model;
	gchar *charlist;
	
	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (curr_data->pref_tree));
	
	if (!ctk_tree_selection_get_selected (selection, &model, &iter))
		return;
		
	ctk_tree_model_get (model, &iter, 1, &charlist, -1);
	
	curr_data->chartable = g_list_remove (curr_data->chartable, charlist);
	
	if (g_ascii_strcasecmp (curr_data->charlist, charlist) == 0) {
		curr_data->charlist = curr_data->chartable != NULL ? 
				      curr_data->chartable->data : "";
		if (g_settings_is_writable (curr_data->settings, "current-list"))
			g_settings_set_string (curr_data->settings, "current-list", curr_data->charlist);
	}
	g_free (charlist);
	
	save_chartable (curr_data);
  	populate_menu (curr_data);
  	
  	ctk_widget_grab_focus (curr_data->pref_tree);
  	next = iter;
	if (ctk_tree_model_iter_next (model, &next) )
		ctk_tree_selection_select_iter (selection, &next);
	else {
		CtkTreePath *path;
		path = ctk_tree_model_get_path (model, &iter);
		if (ctk_tree_path_prev (path))
			ctk_tree_selection_select_path (selection, path);
		ctk_tree_path_free (path);
	}
	ctk_list_store_remove (CTK_LIST_STORE (model), &iter);

}

static void
selection_changed (CtkTreeSelection *selection, gpointer data)
{
	CtkWidget *scrolled = data;
	CtkWidget *edit_button, *delete_button;
	gboolean selected;
	
	selected = ctk_tree_selection_get_selected (selection, NULL, NULL);
	edit_button = g_object_get_data (G_OBJECT (scrolled), "edit_button");
	ctk_widget_set_sensitive (edit_button, selected);
	delete_button = g_object_get_data (G_OBJECT (scrolled), "delete_button");
	ctk_widget_set_sensitive (delete_button, selected);
}

static CtkWidget *
create_palettes_tree (charpick_data *curr_data, CtkWidget *label)
{
	CtkWidget *scrolled;
	CtkWidget *tree;
	CtkListStore *model;
	CtkCellRenderer *cell;
	CtkTreeViewColumn *column;
	GList *list = curr_data->chartable;
	CtkTreeSelection *selection;
	
	scrolled = ctk_scrolled_window_new (NULL,NULL);
	ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (scrolled), CTK_SHADOW_IN);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled),
						CTK_POLICY_AUTOMATIC,
						CTK_POLICY_AUTOMATIC);
	
	model = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
	tree = ctk_tree_view_new_with_model (CTK_TREE_MODEL (model));
	curr_data->pref_tree = tree;
	ctk_label_set_mnemonic_widget (CTK_LABEL (label), tree);
	ctk_container_add (CTK_CONTAINER (scrolled), tree);
	set_access_namedesc (tree, 
				         _("Palettes list"),
				         _("List of available palettes"));
	g_object_unref (G_OBJECT (model));
	cell = ctk_cell_renderer_text_new ();
  	column = ctk_tree_view_column_new_with_attributes ("hello",
						    	   cell,
						     	   "text", 0,
						     	   NULL);
	ctk_tree_view_append_column (CTK_TREE_VIEW (tree), column);
        ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (tree), FALSE);
        
        while (list) {
        	CtkTreeIter iter;
        	gchar *charlist = list->data;
        	
        	ctk_list_store_append (CTK_LIST_STORE (model), &iter);
		ctk_list_store_set (CTK_LIST_STORE (model), &iter, 0, charlist, 1, charlist, -1);
        
        	list = g_list_next (list);
        }
        
        selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (tree));
        g_signal_connect (G_OBJECT (selection), "changed",
        			   G_CALLBACK (selection_changed), scrolled);

	return scrolled;

}

static CtkWidget *
create_hig_catagory (CtkWidget *main_box, gchar *title)
{
	CtkWidget *vbox, *vbox2, *hbox;
	CtkWidget *label;
	gchar *tmp;
		
	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (CTK_BOX (main_box), vbox, TRUE, TRUE, 0);

	tmp = g_strdup_printf ("<b>%s</b>", title);
	label = ctk_label_new (NULL);
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);
	ctk_label_set_markup (CTK_LABEL (label), tmp);
	g_free (tmp);
	ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_pack_start (CTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	label = ctk_label_new ("    ");
	ctk_box_pack_start (CTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (CTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

	return vbox2;
		
}

static void default_chars_frame_create(charpick_data *curr_data)
{
  CtkWidget *dialog = curr_data->propwindow;
  CtkWidget *dbox, *vbox, *vbox1, *vbox2, *vbox3;
  CtkWidget *hbox;
  CtkWidget *label;
  CtkWidget *scrolled;
  CtkWidget *button;

  dbox = ctk_dialog_get_content_area(CTK_DIALOG (dialog));

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 18);
  ctk_container_set_border_width (CTK_CONTAINER (vbox), 5);
  ctk_box_pack_start (CTK_BOX (dbox), vbox, TRUE, TRUE, 0);

  vbox1 = create_hig_catagory (vbox, _("Character Palette")); 
  
  vbox3 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_box_pack_start (CTK_BOX (vbox1), vbox3, TRUE, TRUE, 0);
  
  label = ctk_label_new_with_mnemonic(_("_Palettes:"));
  ctk_box_pack_start(CTK_BOX(vbox3), label, FALSE, FALSE, 0);
  ctk_label_set_xalign (CTK_LABEL (label), 0.0);
  ctk_widget_show(label);
	  
  hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
  ctk_box_pack_start (CTK_BOX (vbox3), hbox, TRUE, TRUE, 0); 
  scrolled = create_palettes_tree (curr_data, label);
  ctk_box_pack_start (CTK_BOX (hbox), scrolled, TRUE, TRUE, 0);
  
  vbox2 = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
  ctk_box_pack_start (CTK_BOX (hbox), vbox2, FALSE, FALSE, 0);

  button = CTK_WIDGET (g_object_new (CTK_TYPE_BUTTON,
  				     "label", "ctk-add",
  				     "use-stock", TRUE,
  				     "use-underline", TRUE,
  				     NULL));

  ctk_box_pack_start (CTK_BOX (vbox2), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
  			     G_CALLBACK (add_palette), curr_data);
  set_access_namedesc (button, _("Add button"),
				         _("Click to add a new palette"));
 
  button = CTK_WIDGET (g_object_new (CTK_TYPE_BUTTON,
  				     "label", CHARPICK_STOCK_EDIT,
  				     "use-stock", TRUE,
  				     "use-underline", TRUE,
  				     NULL));

  ctk_box_pack_start (CTK_BOX (vbox2), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
  			     G_CALLBACK (edit_palette), curr_data);
  g_object_set_data (G_OBJECT (scrolled), "edit_button", button);
  set_access_namedesc (button, _("Edit button"),
				         _("Click to edit the selected palette"));
  
  button = CTK_WIDGET (g_object_new (CTK_TYPE_BUTTON,
  				     "label", "ctk-delete",
  				     "use-stock", TRUE,
  				     "use-underline", TRUE,
  				     NULL));

  ctk_box_pack_start (CTK_BOX (vbox2), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
  			     G_CALLBACK (delete_palette), curr_data);
  g_object_set_data (G_OBJECT (scrolled), "delete_button", button);
  set_access_namedesc (button, _("Delete button"),
				         _("Click to delete the selected palette"));

  if ( ! g_settings_is_writable (curr_data->settings, "chartable"))
	  ctk_widget_set_sensitive (vbox3, FALSE);
   
  return;
}

static void
phelp_cb (CtkDialog *dialog, gint tab, gpointer data)
{
  GError *error = NULL;

  ctk_show_uri_on_window (CTK_WINDOW (dialog),
                          "help:cafe-char-palette/charpick-prefs",
                          ctk_get_current_event_time (),
                          &error);

  if (error) { /* FIXME: the user needs to see this */
    g_warning ("help error: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
response_cb (CtkDialog *dialog, gint id, gpointer data)
{
  charpick_data *curr_data = data;

  if(id == CTK_RESPONSE_HELP){
    phelp_cb (dialog,id,data);
    return;
  }
  
  ctk_widget_destroy (curr_data->propwindow);
  curr_data->propwindow = NULL;
  
}

void
show_preferences_dialog (CtkAction     *action,
			 charpick_data *curr_data)
{
  if (curr_data->propwindow) {
    ctk_window_set_screen (CTK_WINDOW (curr_data->propwindow),
			   ctk_widget_get_screen (curr_data->applet));
    ctk_window_present (CTK_WINDOW (curr_data->propwindow));
    return;
  }

  curr_data->propwindow = ctk_dialog_new_with_buttons (_("Character Palette Preferences"), 
  					    NULL,
					    CTK_DIALOG_DESTROY_WITH_PARENT,
					    "ctk-close", CTK_RESPONSE_CLOSE,
					    "ctk-help", CTK_RESPONSE_HELP,
					    NULL);
  ctk_window_set_screen (CTK_WINDOW (curr_data->propwindow),
			 ctk_widget_get_screen (curr_data->applet));
  ctk_window_set_default_size (CTK_WINDOW (curr_data->propwindow), 350, 350);
  ctk_container_set_border_width (CTK_CONTAINER (curr_data->propwindow), 5);
  ctk_box_set_spacing (CTK_BOX (ctk_dialog_get_content_area(CTK_DIALOG (curr_data->propwindow))), 2);
  ctk_dialog_set_default_response (CTK_DIALOG (curr_data->propwindow), CTK_RESPONSE_CLOSE);

  default_chars_frame_create(curr_data);
  g_signal_connect (G_OBJECT (curr_data->propwindow), "response",
  		    G_CALLBACK (response_cb), curr_data);
  		    
  ctk_widget_show_all (curr_data->propwindow);
}
