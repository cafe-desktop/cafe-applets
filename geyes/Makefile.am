SUBDIRS = themes docs

AM_CPPFLAGS =						\
	-I.						\
	-I$(srcdir) 					\
	$(CAFE_APPLETS4_CFLAGS)			\
	$(CAFEDESKTOP_CFLAGS)			\
	-I$(includedir) 				\
	-DGEYES_THEMES_DIR=\""$(pkgdatadir)/geyes/"\"	\
	-DGEYES_MENU_UI_DIR=\""$(uidir)"\"

libexec_PROGRAMS = cafe-geyes-applet

cafe_geyes_applet_SOURCES =	\
        geyes.c \
	themes.c \
	geyes.h

cafe_geyes_applet_LDADD = 		\
	$(CAFE_APPLETS4_LIBS) \
	$(CAFEDESKTOP_LIBS) \
	-lm

appletdir       = $(datadir)/cafe-panel/applets
applet_in_files = org.cafe.applets.GeyesApplet.cafe-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.cafe-panel-applet.desktop.in=.cafe-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.cafe.panel.applet.GeyesAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.cafe.panel.applet.GeyesAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

uidir = $(datadir)/cafe/ui
ui_DATA = geyes-applet-menu.xml

geyes_gschema_in_files = org.cafe.panel.applet.geyes.gschema.xml.in
gsettings_SCHEMAS = $(geyes_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

themesdir = $(pkgdatadir)/geyes

icons =	\
	cafe-eyes-applet.svg		\
	cafe-eyes-applet.256.png	\
	cafe-eyes-applet.64.png		\
	cafe-eyes-applet.32.png		\
	cafe-eyes-applet.24.png		\
	cafe-eyes-applet.22.png		\
	cafe-eyes-applet.16.png

CLEANFILES = $(applet_DATA) $(applet_in_files) $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

EXTRA_DIST = \
	     README.themes					\
		 $(applet_in_files).in	\
		 $(service_in_files)				\
	     $(geyes_gschema_in_files)				\
	     $(ui_DATA)						\
	     $(icons)

ctk_update_icon_cache = ctk-update-icon-cache -f -t $(datadir)/icons/hicolor

uninstall-local:
	for icon in $(icons); do \
		SIZE=`echo $$icon | cut -d. -f2`; \
		FILE=`echo $$icon | cut -d. -f1,3`; \
		if [ "$$SIZE" = "svg" ]; then \
			SIZE="scalable"; \
			FILE="$$FILE.svg"; \
		else \
			SIZE="$${SIZE}x$${SIZE}"; \
		fi; \
		rm -f $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/$$FILE; \
	done
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Ctk icon cache."; \
		$(ctk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(ctk_update_icon_cache)"; \
	fi

install-data-local:
	for icon in $(icons); do \
		SIZE=`echo $$icon | cut -d. -f2`; \
		FILE=`echo $$icon | cut -d. -f1,3`; \
		if [ "$$SIZE" = "svg" ]; then \
			SIZE="scalable"; \
			FILE="$$FILE.svg"; \
		else \
			SIZE="$${SIZE}x$${SIZE}"; \
		fi; \
		mkdir -p $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/$$FILE; \
	done
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Ctk icon cache."; \
		$(ctk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(ctk_update_icon_cache)"; \
	fi

-include $(top_srcdir)/git.mk
