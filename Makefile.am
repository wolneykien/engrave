SUBDIRS = share filters bin

default: all


if WINDOWS

DESTDIR = $(shell pwd)/release/win32

release:
	$(MAKE) install DESTDIR=$(DESTDIR) \
				 bindir=/ libdir=/ pkglibexecdir=/filters \
				 pkgdatadir=/filters man1dir=/
	$(INSTALL) -m0644 -t $(DESTDIR) \
			$(wildcard $(if $(tiffdlldir),$(tiffdlldir)/libtiff*.dll)) \
			$(wildcard /usr/local/bin/libtiff*.dll) \
			$(wildcard /usr/bin/libtiff*.dll)
	$(INSTALL) -m0644 -t $(DESTDIR) $(wildcard /usr/lib/gcc/i686-w64-mingw32/*-win32/libgcc*.dll)

else

release:

endif
