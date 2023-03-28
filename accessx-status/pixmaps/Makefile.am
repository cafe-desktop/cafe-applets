SUBDIRS = 16x16 24x24 32x32 48x48

ctk_update_icon_cache = ctk-update-icon-cache -f -t $(iconsdir)/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Ctk icon cache."; \
		$(ctk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(ctk_update_icon_cache)"; \
	fi

-include $(top_srcdir)/git.mk
