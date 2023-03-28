AM_CPPFLAGS = -I$(top_srcdir)		\
	   $(CAFE_APPLETS4_CFLAGS)	\
	   $(CAFEDESKTOP_CFLAGS)	\
	   $(GIO_CFLAGS)		\
           -DTRASH_MENU_UI_DIR=\""$(datadir)/cafe/ui"\"

libexec_PROGRAMS = trashapplet

trashapplet_SOURCES = \
	trashapplet.c \
	trash-empty.h \
	trash-empty.c \
	xstuff.c \
	xstuff.h

trashapplet_LDADD = 		\
	$(CAFE_APPLETS4_LIBS) 	\
	$(CAFEDESKTOP_LIBS) 	\
	$(GIO_LIBS) \
	-lX11

-include $(top_srcdir)/git.mk
