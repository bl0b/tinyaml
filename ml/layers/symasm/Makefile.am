include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=symasm.wc

layer_prereq = ../metasm/metasm.wc

symasm.wc: symasm.melang $(layer_prereq) $(TINYAML)
	rm -f metasm.wc $@
	ln -s $(layer_prereq) .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc

