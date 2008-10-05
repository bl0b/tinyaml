include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=symasm.wc

symasm.wc: symasm.melang ../metasm/metasm.wc $(TINYAML)
	rm -f metasm.wc
	ln -s ../metasm/metasm.wc .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc

