include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=script.wc script_compiler.wc

script_compiler.wc: main.asm analyzeFuncDecl.walker functions.asm main.asm compiler.asm globals exprListType.walker grammar structs ../symasm/symasm.wc $(TINYAML)
	rm -f metasm.wc symasm.wc
	ln -s ../metasm/metasm.wc ../symasm/symasm.wc .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc symasm.wc

script.wc: script.stdlib
	rm -f metasm.wc symasm.wc
	ln -s ../metasm/metasm.wc ../symasm/symasm.wc .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc symasm.wc

