@YELP_HELP_RULES@

HELP_ID = cafeweather
HELP_FILES = index.docbook legal.xml

HELP_MEDIA = figures/cafeweather_applet.png \
		figures/cafeweather-details.png \
		figures/cafeweather-menu-prefs.png \
		figures/cafeweather-prefs-general.png \
		figures/cafeweather-prefs-locations.png \
		figures/stock_weather-cloudy.png \
		figures/stock_weather-few-clouds.png \
		figures/stock_weather-fog.png \
		figures/stock_weather-night-clear.png \
		figures/stock_weather-night-few-clouds.png \
		figures/stock_weather-showers.png \
		figures/stock_weather-snow.png \
		figures/stock_weather-storm.png \
		figures/stock_weather-sunny.png

# Add linguas to be ignored, e.g. IGNORE_HELP_LINGUAS = ca de es fr
IGNORE_HELP_LINGUAS =
HELP_LINGUAS = $(if $(IGNORE_HELP_LINGUAS), \
	$(filter-out $(IGNORE_HELP_LINGUAS),$(subst /,,$(dir $(wildcard */*.po)))), \
	$(subst /,,$(dir $(wildcard */*.po))) )

-include $(top_srcdir)/git.mk
