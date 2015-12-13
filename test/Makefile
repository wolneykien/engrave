
%.eps: %.tif
	../bin/engrave -F ../filters -P ../filters -f tile32 -f ct $<

%.50lpi.(Black).tif: %.eps
	gs -sDEVICE=tiffsep1 -sOutputFile=$*.50lpi. -dBATCH -dNOPAUSE -r600 -dCOLORSCREEN -dDITHERPPI=50 $<

clean:
	rm -fv *.eps
	rm -fv *.50lpi.*.tif
