/* Sticky Notes
 * Copyright (C) 2002-2003 Loban A Rahman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef __STICKYNOTES_APPLET_H__
#define __STICKYNOTES_APPLET_H__

#include <glib/gi18n.h>
#include <cdk/cdk.h>
#include <ctk/ctk.h>
#include <gio/gio.h>
#include <cafe-panel-applet.h>


#define STICKYNOTES_SCHEMA	"org.cafe.stickynotes"
#define BUILDER_PATH	CTK_BUILDERDIR "/stickynotes.ui"

#define STICKYNOTES_STOCK_LOCKED	"stickynotes-stock-locked"
#define STICKYNOTES_STOCK_UNLOCKED	"stickynotes-stock-unlocked"
#define STICKYNOTES_STOCK_CLOSE 	"stickynotes-stock-close"
#define STICKYNOTES_STOCK_RESIZE_SE 	"stickynotes-stock-resize-se"
#define STICKYNOTES_STOCK_RESIZE_SW	"stickynotes-stock-resize-sw"

/* Global Sticky Notes instance */
typedef struct
{
	CtkBuilder *builder;		

	CtkWidget *w_prefs;		/* The prefs dialog */
	CtkAdjustment *w_prefs_width;
	CtkAdjustment *w_prefs_height;
	CtkWidget *w_prefs_color;
	CtkWidget *w_prefs_font_color;
	CtkWidget *w_prefs_sys_color;
	CtkWidget *w_prefs_font;
	CtkWidget *w_prefs_sys_font;
	CtkWidget *w_prefs_sticky;
	CtkWidget *w_prefs_force;
	CtkWidget *w_prefs_desktop;

	GList *notes;			/* Linked-List of all the sticky notes */
	GList *applets;			/* Linked-List of all the applets */
	
	cairo_surface_t *icon_normal;	/* Normal applet icon */
	cairo_surface_t *icon_prelight;	/* Prelighted applet icon */

	GSettings *settings;		/* Shared GSettings */

	gint max_height;
	guint last_timeout_data;

    gboolean visible;       /* Toggle show/hide notes */
} StickyNotes;

/* Sticky Notes Applet */
typedef struct
{
	CtkWidget *w_applet;		/* The applet */
	CtkWidget *w_image;		/* The applet icon */

	CtkWidget *destroy_all_dialog;	/* The applet it's destroy all dialog */
	
	gboolean prelighted;		/* Whether applet is prelighted */

	gint panel_size;
	CafePanelAppletOrient panel_orient;

	CtkActionGroup *action_group;
	CtkWidget *menu_tip;
} StickyNotesApplet;
	
typedef enum
{
	STICKYNOTES_NEW = 0,
	STICKYNOTES_SET_VISIBLE,
	STICKYNOTES_SET_LOCKED

} StickyNotesDefaultAction;

extern StickyNotes *stickynotes;

void stickynotes_applet_init(CafePanelApplet *cafe_panel_applet);
void stickynotes_applet_init_prefs(void);

StickyNotesApplet * stickynotes_applet_new(CafePanelApplet *cafe_panel_applet);

void stickynotes_applet_update_icon(StickyNotesApplet *applet);
void stickynotes_applet_update_prefs(void);
void stickynotes_applet_update_menus(void);
void stickynotes_applet_update_tooltips(void);

void stickynotes_applet_do_default_action(CdkScreen *screen);

void stickynotes_applet_panel_icon_get_geometry (int *x, int *y, int *width, int *height);

#endif /* __STICKYNOTES_APPLET_H__ */
