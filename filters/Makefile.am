pkglibexec_PROGRAMS = ct tile32 bg
ct_SOURCES = ct.c system.h
ct_LDADD = ../share/libmisc.a libfilter.a
bg_LDADD = ../share/libmisc.a libfilter.a

tile32_SOURCES = tile32.c
tile32_LDADD = -lm -ltiff libtile32f.a libfilter.a ../share/libmisc.a

pkgdata_DATA = tile32.ps

noinst_LIBRARIES = libfilter.a libtile32f.a
libfilter_a_SOURCES = filter.c
libtile32f_a_SOURCES = tile32f.c

AM_CFLAGS = -DFILTERS=\"$(pkglibexecdir)\" -DPSLIB=\"$(pkgdatadir)\" -I ../share -I ../filters
