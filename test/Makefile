
OPTS = 
TILEOPTS =
LPI = 50
DPI = 600

%.eps: %.tif
	../bin/engrave $(OPTS) -F ../filters -P ../filters -f tile32 $(TILEOPTS) -f ct $<

%.$(LPI)lpi.(Black).tif: %.eps
	gs -sDEVICE=tiffsep1 -sOutputFile=$*.$(LPI)lpi. -dBATCH -dNOPAUSE -r$(DPI) -dCOLORSCREEN -dDITHERPPI=$(LPI) $<

clean:
	rm -fv *.eps
	rm -fv *.*lpi.*.tif
