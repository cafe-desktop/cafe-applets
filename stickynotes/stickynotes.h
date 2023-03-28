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

#ifndef __STICKYNOTES_H__
#define __STICKYNOTES_H__

#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>
#include <stickynotes_applet.h>

#include <ctksourceview/ctksource.h>

typedef struct
{
	CtkWidget *w_window;		/* Sticky Note window */
	CtkWidget *w_menu;		/* Sticky Note menu */
	CtkWidget *w_properties;	/* Sticky Note properties dialog */

	CtkWidget *w_entry;		/* Sticky Note title entry */
	CtkWidget *w_color;		/* Sticky Note color picker */
	CtkWidget *w_color_label;	/* Sticky Note color label */
	CtkWidget *w_font_color;	/* Sticky Note font color picker */
	CtkWidget *w_font_color_label;	/* Sticky Note font color label */
	CtkWidget *w_font;		/* Sticky Note font picker */
	CtkWidget *w_font_label;	/* Sticky Note font label */
	CtkWidget *w_def_color;		/* Sticky Note default color setting */
	CtkWidget *w_def_font;		/* Sticky Note default font setting */

	CtkWidget *w_title;		/* Sticky Note title */
	CtkWidget *w_body;		/* Sticky Note text body */
	CtkWidget *w_scroller;          /* Sticky Note scroller */
	CtkWidget *w_lock;		/* Sticky Note lock button */
	CtkWidget *w_close;		/* Sticky Note close button */
	CtkWidget *w_resize_se;		/* Sticky Note resize button (south east) */
	CtkWidget *w_resize_sw;		/* Sticky Note resize button (south west) */

	CtkSourceBuffer *buffer;	/* Sticky Note text buffer for undo/redo */

	CtkToggleAction *ta_lock_toggle_item; /* Lock item in the popup menu */

	CtkImage *img_lock;		/* Lock image */
	CtkImage *img_close;		/* Close image */
	CtkImage *img_resize_se;	/* SE resize image */
	CtkImage *img_resize_sw;	/* SW resize image */

	gchar *color;			/* Note color */
	gchar *font_color;		/* Font color */
	gchar *font;			/* Note font */
	gboolean locked;		/* Note locked state */

	gint x;				/* Note x-coordinate */
	gint y;				/* Note y-coordinate */
	gint w;				/* Note width */
	gint h;				/* Note height */

	int workspace;			/* Workspace the note is on */

} StickyNote;

StickyNote * stickynote_new(GdkScreen *screen);
void stickynote_free(StickyNote *note);

gboolean stickynote_get_empty(const StickyNote *note);

void stickynote_set_title(StickyNote *note, const gchar* title);
void stickynote_set_color (StickyNote  *note,
		           const gchar *color_str,
			   const gchar *font_color_str,
			   gboolean     save);
void stickynote_set_font(StickyNote *note, const gchar* font_str, gboolean save);
void stickynote_set_locked(StickyNote *note, gboolean locked);
void stickynote_set_visible(StickyNote *note, gboolean visible);

void stickynote_change_properties(StickyNote *note);

void stickynotes_add(GdkScreen *screen);
void stickynotes_remove(StickyNote *note);
void stickynotes_save(void);
gboolean stickynotes_save_now (void);
void stickynotes_load(GdkScreen *screen);

#endif /* __STICKYNOTES_H__ */
