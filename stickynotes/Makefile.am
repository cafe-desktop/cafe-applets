## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps docs

ui_files = stickynotes-applet-menu.xml
builder_files = stickynotes.ui
applet_in_files = org.cafe.applets.StickyNotesApplet.cafe-panel-applet.desktop.in
service_in_files = org.cafe.panel.applet.StickyNotesAppletFactory.service.in
schemas_in_files = stickynotes.schemas.in

if BUILD_STICKYNOTES_APPLET
AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	$(STICKYNOTES_CFLAGS)		\
	$(CAFE_APPLETS4_CFLAGS)		\
	$(CAFEDESKTOP_CFLAGS)		\
	$(LIBWNCK_CFLAGS)			\
	$(XML2_CFLAGS)				\
	-DSTICKYNOTES_MENU_UI_DIR=\""$(uidir)"\"

libexec_PROGRAMS = stickynotes-applet

stickynotes_applet_SOURCES = \
	util.h \
	util.c \
	stickynotes.h \
	stickynotes_callbacks.h \
	stickynotes_applet.h \
	stickynotes_applet_callbacks.h \
	stickynotes.c \
	stickynotes_callbacks.c \
	stickynotes_applet.c \
	stickynotes_applet_callbacks.c

stickynotes_applet_LDADD =	\
	$(STICKYNOTES_LIBS)	\
	$(CAFE_APPLETS4_LIBS)	\
	$(CAFEDESKTOP_LIBS)	\
	$(LIBWNCK_LIBS)		\
	$(XML2_LIBS) \
	-lX11

builder_DATA = $(builder_files)

uidir = $(datadir)/cafe/ui
ui_DATA = $(ui_files)

stickynotes_gschema_in_files = org.cafe.stickynotes.gschema.xml.in
gsettings_SCHEMAS = $(stickynotes_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

appletdir   = $(datadir)/cafe-panel/applets
applet_DATA = $(applet_in_files:.cafe-panel-applet.desktop.in=.cafe-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir   = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)

org.cafe.panel.applet.StickyNotesAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_in_files) $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

endif

EXTRA_DIST = \
	     $(builder_files) \
	     $(ui_files) \
	     $(stickynotes_gschema_in_files) \
             $(service_in_files) \
			 $(applet_in_files).in


-include $(top_srcdir)/git.mk
