/* charpick.h -- header file for character picker applet */

#include <glib.h>
#include <glib/gi18n.h>
#include <ctk/ctk.h>
#include <gio/gio.h>
#include <cafe-panel-applet.h>
#include <cafe-panel-applet-gsettings.h>

#define NO_LAST_INDEX -1

typedef struct _charpick_data charpick_data;
/* this type has basically all data for this program */
struct _charpick_data {
  GList *chartable;
  gchar * charlist;  
  gunichar selected_unichar;
  gint last_index;
  CtkWidget *box;
  CtkWidget *frame;
  CtkWidget *applet;
  CtkToggleButton *last_toggle_button;
  gint panel_size;
  gboolean panel_vertical;
  CtkWidget *propwindow;
  CtkWidget *about_dialog;
  CtkWidget *pref_tree;
  CtkWidget *menu;
  CtkWidget *add_edit_dialog;
  CtkWidget *add_edit_entry;
  GSettings *settings;
};


typedef struct _charpick_button_cb_data charpick_button_cb_data;
/* This is the data type for the button callback function. */

struct _charpick_button_cb_data {
  gint button_index;
  charpick_data * p_curr_data;
};


void start_callback_update(void);
void register_stock_for_edit (void);

void build_table              (charpick_data     *curr_data);
void add_to_popup_menu (charpick_data *curr_data);
void populate_menu (charpick_data *curr_data);
void save_chartable (charpick_data *curr_data);
void show_preferences_dialog  (CtkAction         *action,
			       charpick_data     *curr_data);

void add_edit_dialog_create (charpick_data	 *curr_data,
			     gchar		 *string,
			     gchar		 *title);
void set_atk_name_description (CtkWidget         *widget,
			       const char        *name,
			       const char        *description);
gboolean key_writable (CafePanelApplet *applet, const char *key);

