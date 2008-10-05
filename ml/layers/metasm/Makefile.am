include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=metasm.wc

metasm.wc: metasm.melang $(TINYAML)
	$(TINYAML) -c $< -s $@

