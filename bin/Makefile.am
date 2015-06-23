bin_PROGRAMS = engrave

AM_CFLAGS = -DFILTERS=\"$(pkglibdir)\" -DPSLIB=\"$(pkgdatadir)\" -I ../share -I ../filters

engrave_SOURCES = engrave.c aclocal.m4
engrave_LDADD = -ltiff ../share/libmisc.a

man1dir = $(mandir)/ru/man1
man_MANS = engrave.1
EXTRA_DIST = $(man_MANS)
