
gtop_applets = multiload netspeed

if BUILD_GTOP_APPLETS
gtop_SUBDIRS = $(gtop_applets)
endif

libcafeweather_applets = \
	cafeweather

if BUILD_LIBCAFEWEATHER_APPLETS
libcafeweather_SUBDIRS = $(libcafeweather_applets)
endif

if BUILD_BATTSTAT_APPLET
battstat_SUBDIR = battstat
endif

if APPLET_ACCESSX
accessx_status_SUBDIR = accessx-status
endif

if BUILD_CPUFREQ_APPLET
cpufreq_SUBDIR = cpufreq
endif

if BUILD_TIMERAPPLET
timerapplet_SUBDIR = timerapplet
endif

always_built_SUBDIRS = \
	charpick \
	drivemount \
	geyes \
	stickynotes \
	trashapplet \
	command

SUBDIRS = \
	po \
	$(always_built_SUBDIRS) \
	$(gtop_SUBDIRS) \
	$(libcafeweather_SUBDIRS) \
	man	\
	$(battstat_SUBDIR) \
	$(accessx_status_SUBDIR) \
	$(cpufreq_SUBDIR) \
	$(timerapplet_SUBDIR)

DIST_SUBDIRS = \
	po \
	drivemount \
	charpick \
	geyes \
	battstat \
	multiload \
	cafeweather \
	man \
	accessx-status \
	stickynotes \
	timerapplet \
	trashapplet \
	cpufreq \
	command \
	netspeed

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-compile-warnings=no \
	CFLAGS='-Wno-deprecated-declarations'

EXTRA_DIST = \
	m4/ac-define-dir.m4 \
	COPYING-DOCS \
	xmldocs.make \
	omf.make \
	autogen.sh

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCLEANFILES =

-include $(top_srcdir)/git.mk

