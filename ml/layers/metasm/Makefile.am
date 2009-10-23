include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=metasm.wc
EXTRA_DIST=metasm.melang

metasm.wc: metasm.melang $(TINYAML)
	$(TINYAML) -q -c $< -s $@

