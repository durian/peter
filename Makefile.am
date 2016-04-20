#SUBDIRS = m4

AUTOMAKE_OPTIONS = subdir-objects

#ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = peter pcmd otpcmd

peter_SOURCES = \
	src/main.cc \
	src/XMLNode.cc \
	src/XMLNode.h \
	src/expat_parser.cc \
	src/expat_parser.h \
	src/debug.h \
	src/debug.cc \
	src/util.h \
	src/util.cc \
	src/Config.h \
	src/Config.cc \
	src/qlog.h \
	src/qlog.cc \
	src/Scheduler.h \
	src/Scheduler.cc \
	src/Program.h \
	src/Program.cc \
	src/WorkUnit.h \
	src/WorkUnit.cc \
	src/Actions.h \
	src/Actions.cc \
	src/Socket.h \
	src/Socket.cc \
	src/Events.h \
	src/Events.cc \
	src/otp.h \
	src/otp.cc

pcmd_SOURCES = \
	src/cmd.cc

otpcmd_SOURCES = \
	src/otp.h \
	src/otp.cc \
	src/util.h \
	src/util.cc \
	src/XMLNode.cc \
	src/XMLNode.h \
	src/expat_parser.cc \
	src/expat_parser.h \
	src/otpcmd.cc

man_MANS = man/peter.1 man/pcmd.1 man/otpcmd.1

EXTRA_DIST = \
	$(man_MANS) \
	etc/config.xml \
	scripts/gen_otp.pl \
	scripts/info.sh \
	scripts/log.sh \
	scripts/quit.sh \
	scripts/remove.pl \
	scripts/start.sh \
	scripts/status.sh \
	scripts/stop.sh

