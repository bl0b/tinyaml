include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=symasm.wc
EXTRA_DIST=symasm.melang

layer_prereq = ../metasm/metasm.wc

symasm.wc: symasm.melang $(layer_prereq) $(TINYAML)
	rm -f metasm.wc $@
	ln -s $(layer_prereq) .
	$(TINYAML) -q -c $< -s $@
	rm -f metasm.wc

