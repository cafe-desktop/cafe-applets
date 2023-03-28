SUBDIRS = help pixmaps src

cpufreq_gschema_in_files = org.cafe.panel.applet.cpufreq.gschema.xml.in
gsettings_SCHEMAS = $(cpufreq_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

appletdir       = $(datadir)/cafe-panel/applets
applet_in_files = org.cafe.applets.CPUFreqApplet.cafe-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.cafe-panel-applet.desktop.in=.cafe-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.cafe.panel.applet.CPUFreqAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.cafe.panel.applet.CPUFreqAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

builder_DATA = cpufreq-preferences.ui

uidir = $(datadir)/cafe/ui
ui_DATA = cpufreq-applet-menu.xml

CLEANFILES = \
             $(applet_DATA)			\
               $(applet_in_files)			\
             $(service_DATA)			\
	     $(gsettings_SCHEMAS)               \
	     *.gschema.valid                    \
	     *.bak                              \
	     *.gladep

EXTRA_DIST = \
		$(applet_in_files).in	\
		$(service_in_files)				\
		$(cpufreq_gschema_in_files)			\
		$(builder_DATA)                 			\
		$(ui_DATA)

-include $(top_srcdir)/git.mk
