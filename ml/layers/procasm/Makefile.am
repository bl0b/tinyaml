include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=procasm.wc
EXTRA_DIST=procasm.melang

layer_prereq=../metasm/metasm.wc ../symasm/symasm.wc

procasm.wc: procasm.melang $(layer_prereq) $(TINYAML)
	rm -f metasm.wc symasm.wc
	ln -s $(layer_prereq) .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc symasm.wc

