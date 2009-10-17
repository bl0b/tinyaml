include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=script_compiler.wc script.wc
EXTRA_DIST= analyzeFuncDecl.walker compiler.asm exprListType.walker functions.asm globals grammar main.asm script.stdlib structs

layer_prereq=../metasm/metasm.wc ../symasm/symasm.wc ../procasm/procasm.wc

script_compiler.wc: main.asm analyzeFuncDecl.walker functions.asm main.asm compiler.asm globals exprListType.walker grammar structs script.stdlib $(layer_prereq) $(TINYAML)
	rm -f procasm.wc metasm.wc symasm.wc script_compiler.wc
	ln -s $(layer_prereq) .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc symasm.wc procasm.wc

script.wc: script.stdlib script_compiler.wc
	rm -f procasm.wc metasm.wc symasm.wc
	ln -s $(layer_prereq) .
	$(TINYAML) -c $< -s $@
	rm -f metasm.wc symasm.wc procasm.wc

