
OPTS = 
TILEOPTS =
LPI = 50
DPI = 600
GSOPTS =
DEVICE = tiffsep1
SUF = tif

%.eps: %.tif
	../bin/engrave $(OPTS) -F ../filters -P ../filters -f tile32 $(TILEOPTS) -f ct $<

%.$(LPI)lpi.$(DPI)dpi.(Black).$(SUF): %.eps
	gs -sDEVICE=$(DEVICE) -sOutputFile=$*.$(LPI)lpi.$(DPI)dpi. -dBATCH -dNOPAUSE -r$(DPI) -dCOLORSCREEN -dDITHERPPI=$(LPI) $(GSOPTS) $<

clean:
	rm -fv *.eps
	rm -fv *.*lpi.*.$(SUF)

.SECONDARY:
