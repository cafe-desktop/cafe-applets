AM_CPPFLAGS = -I$(top_srcdir) -I$(includedir) \
       $(GIO_CFLAGS) $(GTOP_APPLETS_CFLAGS) \
       $(MATE_APPLETS4_CFLAGS) $(MATEDESKTOP_CFLAGS) \
       -DNETSPEED_MENU_UI_DIR=\""$(datadir)/cafe/ui"\"

libexec_PROGRAMS = cafe-netspeed-applet

cafe_netspeed_applet_SOURCES = backend.h backend.c netspeed.c

cafe_netspeed_applet_LDADD = $(GIO_LIBS) $(GTOP_APPLETS_LIBS) \
       $(MATE_APPLETS4_LIBS) $(MATEDESKTOP_LIBS) \
       $(IWLIB) $(INTLLIBS) -lm


-include $(top_srcdir)/git.mk
