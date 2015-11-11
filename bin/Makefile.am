bin_PROGRAMS = engrave

if WINDOWS
  AM_CFLAGS = -DFILTERS=\"filters\" -DPSLIB=\"filters\" -I ../share -I ../filters
else
  AM_CFLAGS = -DFILTERS=\"$(pkglibdir)\" -DPSLIB=\"$(pkgdatadir)\" -I ../share -I ../filters
endif

engrave_SOURCES = engrave.c aclocal.m4
engrave_LDADD = -ltiff ../share/libmisc.a

man1dir = $(mandir)/ru/man1
man_MANS = engrave.1
EXTRA_DIST = $(man_MANS)
