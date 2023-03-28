## Process this file with automake to produce Makefile.in

SUBDIRS = docs

AM_CPPFLAGS =					\
	-I$(srcdir)				\
	-I$(top_srcdir)				\
	-DCAFEWEATHER_MENU_UI_DIR=\""$(uidir)"\"	\
	$(CAFE_APPLETS4_CFLAGS)		\
	$(CAFEDESKTOP_CFLAGS)		\
	$(LIBNOTIFY_CFLAGS)			\
	$(LIBCAFEWEATHER_CFLAGS)			\
	$(DBUS_CFLAGS)

libexec_PROGRAMS = cafeweather-applet

cafeweather_applet_SOURCES = \
	cafeweather.h \
	main.c \
	cafeweather-about.c cafeweather-about.h \
	cafeweather-pref.c cafeweather-pref.h \
	cafeweather-dialog.c cafeweather-dialog.h \
	cafeweather-applet.c cafeweather-applet.h

cafeweather_applet_LDADD = 	\
	$(LIBNOTIFY_LIBS) 	\
	$(CAFE_APPLETS4_LIBS)	\
	$(CAFEDESKTOP_LIBS)	\
	$(CAFE_LIBS2_LIBS)	\
	$(LIBCAFEWEATHER_LIBS)	\
	$(DBUS_LIBS)

uidir = $(datadir)/cafe/ui
ui_DATA = cafeweather-applet-menu.xml

appletdir       = $(datadir)/cafe-panel/applets
applet_in_files = org.cafe.applets.CafeWeatherApplet.cafe-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.cafe-panel-applet.desktop.in=.cafe-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.cafe.panel.applet.CafeWeatherAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.cafe.panel.applet.CafeWeatherAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_in_files) $(service_DATA)

EXTRA_DIST = \
	$(applet_in_files).in	\
	$(service_in_files)					\
	$(ui_DATA)

-include $(top_srcdir)/git.mk
