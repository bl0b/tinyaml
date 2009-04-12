include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=procasm.wc

procasm.wc: procasm.melang $(TINYAML)
	rm -f metasm.wc symasm.wc script_compiler.wc
	ln -s ../metasm/metasm.wc ../symasm/symasm.wc .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc symasm.wc

