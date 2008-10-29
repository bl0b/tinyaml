include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=script_compiler.wc script.wc

script_compiler.wc: main.asm analyzeFuncDecl.walker functions.asm main.asm compiler.asm globals exprListType.walker grammar structs script.stdlib ../symasm/symasm.wc $(TINYAML)
	rm -f metasm.wc symasm.wc script_compiler.wc
	ln -s ../metasm/metasm.wc ../symasm/symasm.wc .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc symasm.wc

script.wc: script.stdlib
	rm -f metasm.wc symasm.wc script.wc
	ln -s ../metasm/metasm.wc ../symasm/symasm.wc .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc symasm.wc

