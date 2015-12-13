
OPTS = 
TILEOPTS =
LPI = 50
DPI = 600
GSOPTS =

%.eps: %.tif
	../bin/engrave $(OPTS) -F ../filters -P ../filters -f tile32 $(TILEOPTS) -f ct $<

%.$(LPI)lpi.$(DPI)dpi.(Black).tif: %.eps
	gs -sDEVICE=tiffsep1 -sOutputFile=$*.$(LPI)lpi.$(DPI)dpi. -dBATCH -dNOPAUSE -r$(DPI) -dCOLORSCREEN -dDITHERPPI=$(LPI) $(GSOPTS) $<

clean:
	rm -fv *.eps
	rm -fv *.*lpi.*.tif

.SECONDARY:
