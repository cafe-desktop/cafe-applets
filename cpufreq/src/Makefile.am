if BUILD_CPUFREQ_SELECTOR
selector_SUBDIR = cpufreq-selector
endif

SUBDIRS = $(selector_SUBDIR)

AM_CPPFLAGS = \
	-DCPUFREQ_MENU_UI_DIR=\""$(datadir)/cafe/ui"\" 	\
	$(CAFE_APPLETS4_CFLAGS) 				\
	$(CAFEDESKTOP_CFLAGS) 				\
	$(DBUS_CFLAGS)

libexec_PROGRAMS = cafe-cpufreq-applet

if HAVE_LIBCPUFREQ
cpufreq_files=cpufreq-monitor-libcpufreq.c    cpufreq-monitor-libcpufreq.h
endif

cafe_cpufreq_applet_SOURCES =  \
	cpufreq-applet.c        	cpufreq-applet.h        	\
	cpufreq-utils.c			cpufreq-utils.h			\
	cpufreq-prefs.c         	cpufreq-prefs.h         	\
	cpufreq-selector.c		cpufreq-selector.h		\
	cpufreq-popup.c                 cpufreq-popup.h			\
	cpufreq-monitor.c       	cpufreq-monitor.h	      	\
	cpufreq-monitor-factory.c	cpufreq-monitor-factory.h	\
	cpufreq-monitor-procfs.c	cpufreq-monitor-procfs.h	\
	cpufreq-monitor-sysfs.c	        cpufreq-monitor-sysfs.h	        \
	$(cpufreq_files)						\
	cpufreq-monitor-cpuinfo.c       cpufreq-monitor-cpuinfo.h

cafe_cpufreq_applet_LDADD =  \
	$(CAFE_APPLETS4_LIBS)   	\
	$(CAFEDESKTOP_LIBS)   	\
	$(LIBCPUFREQ_LIBS)		\
	$(DBUS_LIBS)

-include $(top_srcdir)/git.mk
