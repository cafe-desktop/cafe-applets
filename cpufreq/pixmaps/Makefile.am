
cpufreq_pixmapdir = $(pixmapsdir)/cafe-cpufreq-applet
cpufreq_pixmap_DATA =		\
	cpufreq-100.png         \
	cpufreq-75.png          \
	cpufreq-50.png          \
	cpufreq-25.png          \
	cpufreq-na.png

icons = \
	cafe-cpu-frequency-applet.16.png \
	cafe-cpu-frequency-applet.22.png \
	cafe-cpu-frequency-applet.24.png \
	cafe-cpu-frequency-applet.32.png \
	cafe-cpu-frequency-applet.64.png \
	cafe-cpu-frequency-applet.256.png \
	cafe-cpu-frequency-applet.svg

EXTRA_DIST = \
	$(cpufreq_pixmap_DATA) \
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
		echo "Updating Gtk icon cache."; \
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
		echo "Updating Gtk icon cache."; \
		$(ctk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(ctk_update_icon_cache)"; \
	fi

-include $(top_srcdir)/git.mk
