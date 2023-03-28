SUBDIRS = help

AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	-DCHARPICK_MENU_UI_DIR=\""$(uidir)"\"	\
	$(CAFE_APPLETS4_CFLAGS)		\
	$(CAFEDESKTOP_CFLAGS)		\
	$(GUCHARMAP_CFLAGS)

libexec_PROGRAMS = cafe-charpick-applet

cafe_charpick_applet_SOURCES = charpick.c \
                          charpick.h \
                          properties.c

cafe_charpick_applet_LDADD = \
		       $(CAFE_APPLETS4_LIBS)	\
		       $(CAFEDESKTOP_LIBS)	\
		       $(GUCHARMAP_LIBS)

appletdir       = $(datadir)/cafe-panel/applets
applet_in_files = org.cafe.applets.CharpickerApplet.cafe-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.cafe-panel-applet.desktop.in=.cafe-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.cafe.panel.applet.CharpickerAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.cafe.panel.applet.CharpickerAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

charpick_gschema_in_files = org.cafe.panel.applet.charpick.gschema.xml.in
gsettings_SCHEMAS = $(charpick_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

uidir = $(datadir)/cafe/ui
ui_DATA = charpick-applet-menu.xml

CLEANFILES = $(applet_DATA)  $(applet_in_files) $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

EXTRA_DIST =							\
	$(applet_in_files).in	\
	$(service_in_files)					\
	$(ui_DATA)						\
	$(charpick_gschema_in_files)

-include $(top_srcdir)/git.mk
