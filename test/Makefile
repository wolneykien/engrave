
OPTS = 
TILEOPTS =
LPI = 50
DPI = 600
GSOPTS =
DEVICE = tiffsep1
FORMAT = eps
SUF = tif

IMAGES = smp smp_usm Gates gravure test Gates_cmyk

all: $(FORMAT)files

$(FORMAT)files: $(addsuffix .$(FORMAT),$(IMAGES))
bitmaps: $(addsuffix .$(LPI)lpi.$(DPI)dpi.$(FORMAT).(Black).$(SUF),$(IMAGES))
display: $(addsuffix .$(LPI)lpi.$(DPI)dpi.$(FORMAT).(Black).$(SUF),$(firstword $(IMAGES)))
	display '$<'

%.$(FORMAT): %.tif
	../bin/engrave $(OPTS) -t$(FORMAT) -F ../filters -P ../filters -f tile32 $(TILEOPTS) -f ct $<

get_width_height = $(shell set -x; sed -n -e '/^%%BoundingBox:/ { s/^%%BoundingBox:[[:space:]]\+0[[:space:]]\+0[[:space:]]\+//; s/[[:space:]]\+/x/; s/[[:space:]]\+$$//; $(2); p; q; }' $(1))
get_width = $(shell echo $$((($(call get_width_height,$(1),s/x.*$$//) * $(DPI)) / 72)))
get_height = $(shell echo $$((($(call get_width_height,$(1),s/^[^x]\+x//) * $(DPI)) / 72)))

%.$(LPI)lpi.$(DPI)dpi.$(FORMAT).(Black).$(SUF): %.$(FORMAT) %.bbox
	gs -sDEVICE=$(DEVICE) -sOutputFile=$*.$(LPI)lpi.$(DPI)dpi.$(FORMAT). -dBATCH -dNOPAUSE -r$(DPI) -g$(call get_width,$*.bbox)x$(call get_height,$*.bbox) -dCOLORSCREEN -dDITHERPPI=$(LPI) $(GSOPTS) $<

%.bbox: %.$(FORMAT)
	gs -q -sDEVICE=bbox -dNOPAUSE -dBATCH $< 2>$@

clean:
	rm -fv *.$(FORMAT) *.bbox
	rm -fv *.*lpi.*dpi.$(FORMAT).*.$(SUF)

.SECONDARY:
