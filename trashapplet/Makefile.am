SUBDIRS = src docs

appletdir       = $(datadir)/cafe-panel/applets
applet_in_files = org.cafe.applets.TrashApplet.cafe-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.cafe-panel-applet.desktop.in=.cafe-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.cafe.panel.applet.TrashAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.cafe.panel.applet.TrashAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

uidir = $(datadir)/cafe/ui
ui_DATA = trashapplet-menu.xml

builder_DATA = trashapplet-empty-progress.ui

EXTRA_DIST = \
	$(ui_DATA) \
	$(applet_in_files).in	\
	$(service_in_files) \
	$(builder_DATA)

CLEANFILES = $(applet_DATA) $(applet_in_files) $(service_DATA)

-include $(top_srcdir)/git.mk
