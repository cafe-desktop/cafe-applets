AM_CPPFLAGS = -I$(top_srcdir) -I$(includedir) \
       $(GIO_CFLAGS) $(GTOP_APPLETS_CFLAGS) \
       $(CAFE_APPLETS4_CFLAGS) $(CAFEDESKTOP_CFLAGS) \
       -DNETSPEED_MENU_UI_DIR=\""$(datadir)/cafe/ui"\"

libexec_PROGRAMS = cafe-netspeed-applet

cafe_netspeed_applet_SOURCES = backend.h backend.c netspeed.c

cafe_netspeed_applet_LDADD = $(GIO_LIBS) $(GTOP_APPLETS_LIBS) \
       $(CAFE_APPLETS4_LIBS) $(CAFEDESKTOP_LIBS) \
       $(IWLIB) $(INTLLIBS) -lm


-include $(top_srcdir)/git.mk
