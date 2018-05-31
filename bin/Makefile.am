bin_PROGRAMS = engrave

if WINDOWS
  AM_CFLAGS = -DFILTERS=\"filters\" -DPSLIB=\"filters\" -I ../share -I ../filters
else
  AM_CFLAGS = -DFILTERS=\"$(pkglibexecdir)\" -DPSLIB=\"$(pkgdatadir)\" -I ../share -I ../filters
endif

engrave_SOURCES = engrave.c
EXTRA_engrave_SOURCES = pdfwriter.cc
engrave_LDADD = -ltiff ../share/libmisc.a
engrave_DEPENDENCIES = ../share/libmisc.a

if WITH_PDFWRITER
engrave_LDADD += pdfwriter.$(OBJEXT) -lPDFWriter
engrave_DEPENDENCIES += pdfwriter.$(OBJEXT)
endif

man1dir = $(mandir)/ru/man1
man_MANS = engrave.1
EXTRA_DIST = $(man_MANS)
