/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* battstat        A CAFE battery meter for laptops. 
 * Copyright (C) 2000 by Jörgen Pehrson <jp@spektr.eu.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_ERR_H
#include <err.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <ctk/ctk.h>

#include <gio/gio.h>

#include "battstat.h"

#ifndef gettext_noop
#define gettext_noop(String) (String)
#endif

#define NEVER_SENSITIVE		"never_sensitive"

/* set sensitive and setup NEVER_SENSITIVE appropriately */
static void
hard_set_sensitive (CtkWidget *w, gboolean sensitivity)
{
	ctk_widget_set_sensitive (w, sensitivity);
	g_object_set_data (G_OBJECT (w), NEVER_SENSITIVE,
			   GINT_TO_POINTER ( ! sensitivity));
}


#if 0
/* set sensitive, but always insensitive if NEVER_SENSITIVE is set */
static void
soft_set_sensitive (CtkWidget *w, gboolean sensitivity)
{
	if (g_object_get_data (G_OBJECT (w), NEVER_SENSITIVE))
		ctk_widget_set_sensitive (w, FALSE);
	else
		ctk_widget_set_sensitive (w, sensitivity);
}
#endif /* 0 */

static void
combo_ptr_cb (CtkWidget *combo_ptr, gpointer data)
{
	ProgressData *battstat = data;
	
	if (ctk_combo_box_get_active (GTK_COMBO_BOX (combo_ptr)))
		battstat->red_value_is_time = TRUE;
	else
		battstat->red_value_is_time = FALSE;
	
	g_settings_set_boolean (battstat->settings,
			"red-value-is-time",
			battstat->red_value_is_time);
}

static void
spin_ptr_cb (CtkWidget *spin_ptr, gpointer data)
{
	ProgressData *battstat = data;

	battstat->red_val = ctk_spin_button_get_value_as_int (
			GTK_SPIN_BUTTON (spin_ptr));
	/* automatically calculate orangle and yellow values from the
	 * red value
	 */
	battstat->orange_val = battstat->red_val * ORANGE_MULTIPLIER;
	battstat->orange_val = CLAMP (battstat->orange_val, 0, 100);

	battstat->yellow_val = battstat->red_val * YELLOW_MULTIPLIER;
	battstat->yellow_val = CLAMP (battstat->yellow_val, 0, 100);
	
	g_settings_set_int (battstat->settings,
			"red-value",
			battstat->red_val);
}

static void
show_text_toggled (CtkToggleButton *button, gpointer data)
{
  ProgressData   *battstat = data;
  
  if (ctk_toggle_button_get_active (GTK_TOGGLE_BUTTON (battstat->radio_text_2))
   && ctk_toggle_button_get_active (GTK_TOGGLE_BUTTON (battstat->check_text)))
	  battstat->showtext = APPLET_SHOW_PERCENT;
  else if (ctk_toggle_button_get_active (
			  GTK_TOGGLE_BUTTON (battstat->radio_text_1)) &&
	   ctk_toggle_button_get_active (
		   	  GTK_TOGGLE_BUTTON (battstat->check_text)))
	  battstat->showtext = APPLET_SHOW_TIME;
  else
	  battstat->showtext = APPLET_SHOW_NONE;

  battstat->refresh_label = TRUE;
 
  reconfigure_layout( battstat ); 

  ctk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_1),
		  battstat->showtext);
  ctk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_2),
		  battstat->showtext);
	
  g_settings_set_int (battstat->settings, "show-text", battstat->showtext);
}

static void
lowbatt_toggled (CtkToggleButton *button, gpointer data)
{
  ProgressData   *battstat = data;
  
  battstat->lowbattnotification = ctk_toggle_button_get_active (button);
  g_settings_set_boolean   (battstat->settings, "low-battery-notification", 
  				 battstat->lowbattnotification);  

  hard_set_sensitive (battstat->hbox_ptr, battstat->lowbattnotification);
}

static void
full_toggled (CtkToggleButton *button, gpointer data)
{
  ProgressData   *battstat = data;
  
  battstat->fullbattnot = ctk_toggle_button_get_active (button);
  g_settings_set_boolean   (battstat->settings, "full-battery-notification", 
  				 battstat->fullbattnot);  
}

static void
response_cb (CtkDialog *dialog, gint id, gpointer data)
{
  ProgressData *battstat = data;
  
  if (id == GTK_RESPONSE_HELP)
    battstat_show_help (battstat, "battstat-appearance");
  else
    ctk_widget_hide (GTK_WIDGET (battstat->prop_win));
}

void
prop_cb (CtkAction    *action,
				 ProgressData *battstat)
{
  CtkBuilder *builder;
  CtkWidget *combo_ptr, *spin_ptr;
  CtkListStore *liststore;
  CtkCellRenderer *renderer;
  CtkTreeIter iter;

  if (DEBUG) g_print("prop_cb()\n");

   if (battstat->prop_win) { 
     ctk_window_set_screen (GTK_WINDOW (battstat->prop_win),
			    ctk_widget_get_screen (battstat->applet));
     ctk_window_present (GTK_WINDOW (battstat->prop_win));
     return;
   } 

  builder = ctk_builder_new ();
  ctk_builder_add_from_file (builder, GTK_BUILDERDIR"/battstat_applet.ui", NULL);

  battstat->prop_win = GTK_DIALOG (ctk_builder_get_object (builder, 
  				   "battstat_properties"));
  ctk_window_set_screen (GTK_WINDOW (battstat->prop_win),
			 ctk_widget_get_screen (battstat->applet));

  g_signal_connect (G_OBJECT (battstat->prop_win), "delete_event",
		  G_CALLBACK (ctk_true), NULL);
  
  battstat->lowbatt_toggle = GTK_WIDGET (ctk_builder_get_object (builder, "lowbatt_toggle"));
  g_signal_connect (G_OBJECT (battstat->lowbatt_toggle), "toggled",
  		    G_CALLBACK (lowbatt_toggled), battstat);

  if (!g_settings_is_writable (battstat->settings,
			  "low-battery-notification"))
  {
	  hard_set_sensitive (battstat->lowbatt_toggle, FALSE);
  }

  battstat->hbox_ptr = GTK_WIDGET (ctk_builder_get_object (builder, "hbox_ptr"));
  hard_set_sensitive (battstat->hbox_ptr, battstat->lowbattnotification);

  combo_ptr = GTK_WIDGET (ctk_builder_get_object (builder, "combo_ptr"));
  g_signal_connect (G_OBJECT (combo_ptr), "changed",
		  G_CALLBACK (combo_ptr_cb), battstat);

  liststore = ctk_list_store_new (1, G_TYPE_STRING);
  ctk_combo_box_set_model (GTK_COMBO_BOX (combo_ptr),
		  GTK_TREE_MODEL (liststore));
  ctk_cell_layout_clear (GTK_CELL_LAYOUT (combo_ptr));
  renderer = ctk_cell_renderer_text_new ();
  ctk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_ptr),
		  renderer, TRUE);
  ctk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_ptr),
		  renderer,
		  "text", 0,
		  NULL);
  ctk_list_store_append (liststore, &iter);
  /* TRANSLATOR: this is a selectable item in a drop-down menu to end
   * this sentence:
   *   "Warn when battery charge drops to: [XX] percent".
   */
  ctk_list_store_set (liststore, &iter, 0, _("Percent"), -1);
  ctk_list_store_append (liststore, &iter);
  /* TRANSLATOR: this is a selectable item in a drop-down menu to end
   * this sentence:
   *   "Warn when battery charge drops to: [XX] minutes remaining"
   */
  ctk_list_store_set (liststore, &iter, 0, _("Minutes Remaining"), -1);

  spin_ptr = GTK_WIDGET (ctk_builder_get_object (builder, "spin_ptr"));
  ctk_spin_button_set_value (GTK_SPIN_BUTTON (spin_ptr),
		  battstat->red_val);
  g_signal_connect (G_OBJECT (spin_ptr), "value-changed",
		  G_CALLBACK (spin_ptr_cb), battstat);

  if (battstat->red_value_is_time)
	  ctk_combo_box_set_active (GTK_COMBO_BOX (combo_ptr), 1);
  else
	  ctk_combo_box_set_active (GTK_COMBO_BOX (combo_ptr), 0);

  battstat->full_toggle = GTK_WIDGET (ctk_builder_get_object (builder, "full_toggle"));
  g_signal_connect (G_OBJECT (battstat->full_toggle), "toggled",
  		    G_CALLBACK (full_toggled), battstat);

  if (!g_settings_is_writable (battstat->settings,
			  "full-battery-notification"))
  {
	  hard_set_sensitive (battstat->full_toggle, FALSE);
  }
  if (battstat->fullbattnot)
  {
     ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (battstat->full_toggle),
		     TRUE);
  }
  if (battstat->lowbattnotification)
  {
    ctk_toggle_button_set_active (GTK_TOGGLE_BUTTON (battstat->lowbatt_toggle),
		    TRUE);
  }

  battstat->radio_text_1 = GTK_WIDGET (ctk_builder_get_object (builder, "show_text_radio"));
  battstat->radio_text_2 = GTK_WIDGET (ctk_builder_get_object (builder,
		  "show_text_radio_2"));
  battstat->check_text = GTK_WIDGET (ctk_builder_get_object (builder,
		  "show_text_remaining"));

  g_object_unref (builder);
  
  g_signal_connect (G_OBJECT (battstat->radio_text_1), "toggled",
		  G_CALLBACK (show_text_toggled), battstat);
  g_signal_connect (G_OBJECT (battstat->radio_text_2), "toggled",
		  G_CALLBACK (show_text_toggled), battstat);
  g_signal_connect (G_OBJECT (battstat->check_text), "toggled",
		  G_CALLBACK (show_text_toggled), battstat);
  
  if (!g_settings_is_writable (battstat->settings, "show-text"))
  {
	  hard_set_sensitive (battstat->check_text, FALSE);
	  hard_set_sensitive (battstat->radio_text_1, FALSE);
	  hard_set_sensitive (battstat->radio_text_2, FALSE);
  }

  if (battstat->showtext == APPLET_SHOW_PERCENT)
  {
	  ctk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->check_text), TRUE);
	  ctk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->radio_text_2), TRUE);
	  ctk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_1),
			  TRUE);
	  ctk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_2),
			  TRUE);
  }
  else if (battstat->showtext == APPLET_SHOW_TIME)
  {
	  ctk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->check_text),
			  TRUE);
	  ctk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->radio_text_1),
			  TRUE);
	  ctk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_1),
			  TRUE);
	  ctk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_2),
			  TRUE);
  }
  else /* APPLET_SHOW_NONE */
  {
	  ctk_toggle_button_set_active (
			  GTK_TOGGLE_BUTTON (battstat->check_text), FALSE);
	  ctk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_1),
			  FALSE);
	  ctk_widget_set_sensitive (GTK_WIDGET (battstat->radio_text_2),
			  FALSE);
  }

   ctk_dialog_set_default_response (GTK_DIALOG (battstat->prop_win),
		   GTK_RESPONSE_CLOSE);
   ctk_window_set_resizable (GTK_WINDOW (battstat->prop_win), FALSE);
   g_signal_connect (G_OBJECT (battstat->prop_win), "response",
   		     G_CALLBACK (response_cb), battstat);
   ctk_widget_show_all (GTK_WIDGET (battstat->prop_win));
}
