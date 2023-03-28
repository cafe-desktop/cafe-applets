SUBDIRS = docs

AM_CPPFLAGS =			\
	-I$(srcdir)		\
	-DMULTILOAD_MENU_UI_DIR=\""$(uidir)"\" \
	$(MATE_APPLETS4_CFLAGS)	\
	$(GTOP_APPLETS_CFLAGS)	\
	$(MATEDESKTOP_CFLAGS)	\
	$(GIO_CFLAGS)

libexec_PROGRAMS = cafe-multiload-applet

cafe_multiload_applet_SOURCES = \
			global.h	\
			linux-proc.h	\
			load-graph.h	\
			linux-proc.c	\
			load-graph.c	\
			main.c	\
			properties.c	\
			netspeed.c netspeed.h \
			autoscaler.c	\
			autoscaler.h

cafe_multiload_applet_LDADD = \
				$(MATE_APPLETS4_LIBS) 	\
				$(GTOP_APPLETS_LIBS)	\
				$(MATEDESKTOP_LIBS)		\
				$(GIO_LIBS)				\
				-lm

multiload_gschema_in_files = org.cafe.panel.applet.multiload.gschema.xml.in
gsettings_SCHEMAS = $(multiload_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

EXTRA_DIST =							\
	$(applet_in_files).in	\
	$(service_in_files)					\
	$(multiload_gschema_in_files)				\
	$(ui_DATA)

uidir		= $(datadir)/cafe/ui
ui_DATA		= multiload-applet-menu.xml

appletdir       = $(datadir)/cafe-panel/applets
applet_in_files = org.cafe.applets.MultiLoadApplet.cafe-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.cafe-panel-applet.desktop.in=.cafe-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.cafe.panel.applet.MultiLoadAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.cafe.panel.applet.MultiLoadAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_in_files) $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

-include $(top_srcdir)/git.mk
