cpufreqselectordir = $(bindir)

AM_CPPFLAGS = $(CPUFREQ_SELECTOR_CFLAGS)

if HAVE_POLKIT
AM_CPPFLAGS += \
	$(DBUS_CFLAGS)		\
	$(POLKIT_CFLAGS)
endif

bin_PROGRAMS = cafe-cpufreq-selector

install-exec-hook:
if CPUFREQ_SELECTOR_SUID
	chmod 4755 $(DESTDIR)$(bindir)/cafe-cpufreq-selector
endif


if HAVE_LIBCPUFREQ
cpufreq_files=cpufreq-selector-libcpufreq.c        cpufreq-selector-libcpufreq.h
endif

if HAVE_POLKIT
service_files=cpufreq-selector-service.c       cpufreq-selector-service.h
endif

cafe_cpufreq_selector_SOURCES = \
	cpufreq-selector.c               cpufreq-selector.h        	\
	cpufreq-selector-sysfs.c         cpufreq-selector-sysfs.h  	\
	cpufreq-selector-procfs.c        cpufreq-selector-procfs.h 	\
	$(cpufreq_files)					   	\
	$(service_files)						\
	cpufreq-selector-factory.c	 cpufreq-selector-factory.h	\
	$(BUILT_SOURCES)						\
	main.c

cafe_cpufreq_selector_LDADD = \
	$(CPUFREQ_SELECTOR_LIBS) 	\
	$(LIBCPUFREQ_LIBS)

if HAVE_POLKIT
cafe_cpufreq_selector_LDADD += \
	$(DBUS_LIBS)		\
	$(POLKIT_LIBS)
endif

cafe_cpufreq_selector_LDFLAGS = $(LDFLAGS) -Wl,-z,now

dbus_services_in_files = org.cafe.CPUFreqSelector.service.in
polkit_in_files = org.cafe.cpufreqselector.policy.in

dbus_servicesdir = $(datadir)/dbus-1/system-services
dbus_confdir = $(DBUS_SYS_DIR)
polkitdir = $(datadir)/polkit-1/actions

if HAVE_POLKIT
BUILT_SOURCES = cpufreq-selector-service-glue.h

cpufreq-selector-service-glue.h: $(srcdir)/cpufreq-selector-service.xml
	dbus-binding-tool --prefix=cpufreq_selector_service --mode=glib-server	\
	                  --output=cpufreq-selector-service-glue.h		\
	                  $(srcdir)/cpufreq-selector-service.xml

dbus_services_DATA = $(dbus_services_in_files:.service.in=.service)
$(dbus_services_DATA): $(dbus_services_in_files)
	sed -e "s|\@BINDIR\@|$(DESTDIR)$(bindir)|" $< > $@
dbus_conf_DATA = org.cafe.CPUFreqSelector.conf

polkit_DATA = $(polkit_in_files:.policy.in=.policy)
$(polkit_DATA): $(polkit_in_files)
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@
else
dbus_services_DATA=
dbus_conf_DATA=
polkit_DATA=
endif

EXTRA_DIST = \
	cpufreq-selector-service.xml	\
	$(dbus_services_in_files)	\
	org.cafe.CPUFreqSelector.conf	\
	$(polkit_in_files)

CLEANFILES = \
	$(BUILT_SOURCES)	\
	$(dbus_services_DATA)	\
	$(polkit_DATA)

-include $(top_srcdir)/git.mk
