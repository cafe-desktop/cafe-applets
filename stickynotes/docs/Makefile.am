@YELP_HELP_RULES@

HELP_ID = cafe-stickynotes-applet
HELP_FILES = index.docbook legal.xml

HELP_MEDIA = figures/stickynote-right-menu-lock.png \
		figures/stickynote-right-menu-new.png \
		figures/stickynotes_applet.png \
		figures/stickynotes-note-prefs.png \
		figures/stickynotes-prefs.png

# Add linguas to be ignored, e.g. IGNORE_HELP_LINGUAS = ca de es fr
IGNORE_HELP_LINGUAS =
HELP_LINGUAS = $(if $(IGNORE_HELP_LINGUAS), \
	$(filter-out $(IGNORE_HELP_LINGUAS),$(subst /,,$(dir $(wildcard */*.po)))), \
	$(subst /,,$(dir $(wildcard */*.po))) )

-include $(top_srcdir)/git.mk
