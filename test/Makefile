
OPTS = 
TILEOPTS =
LPI = 50
DPI = 600
GSOPTS =
DEVICE = tiffsep1
SUF = tif

IMAGES = smp smp_usm Gates gravure test Gates_cmyk

all: epsfiles

epsfiles: $(addsuffix .eps,$(IMAGES))
bitmaps: $(addsuffix .$(LPI)lpi.$(DPI)dpi.(Black).$(SUF),$(IMAGES))

%.eps: %.tif
	../bin/engrave $(OPTS) -F ../filters -P ../filters -f tile32 $(TILEOPTS) -f ct $<

get_width_height = $(shell set -x; sed -n -e '/^%%BoundingBox:/ { s/^%%BoundingBox:[[:space:]]\+0[[:space:]]\+0[[:space:]]\+//; s/[[:space:]]\+/x/; s/[[:space:]]\+$$//; $(2); p; q; }' $(1))
get_width = $(shell echo $$((($(call get_width_height,$(1),s/x.*$$//) * $(DPI)) / 72)))
get_height = $(shell echo $$((($(call get_width_height,$(1),s/^[^x]\+x//) * $(DPI)) / 72)))

%.$(LPI)lpi.$(DPI)dpi.(Black).$(SUF): %.eps %.bbox
	gs -sDEVICE=$(DEVICE) -sOutputFile=$*.$(LPI)lpi.$(DPI)dpi. -dBATCH -dNOPAUSE -r$(DPI) -g$(call get_width,$*.bbox)x$(call get_height,$*.bbox) -dCOLORSCREEN -dDITHERPPI=$(LPI) $(GSOPTS) $<

%.bbox: %.eps
	gs -q -sDEVICE=bbox -dNOPAUSE -dBATCH $< 2>$@

clean:
	rm -fv *.eps *.bbox
	rm -fv *.*lpi.*.$(SUF)

.SECONDARY:
