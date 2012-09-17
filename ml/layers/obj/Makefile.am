include ../all.makefile
layerdir=$(pkgdatadir)
layer_DATA=obj.wc
EXTRA_DIST= obj.melang

layer_prereq=../metasm/metasm.wc ../symasm/symasm.wc ../procasm/procasm.wc ../script/script.wc

obj.wc: obj.melang $(layer_prereq) $(TINYAML)
	rm -f procasm.wc metasm.wc symasm.wc script_compiler.wc script.wc obj.wc
	ln -s $(layer_prereq) .
	$(TINYAML_CMD) -q -c $< -s $@
	rm -f metasm.wc symasm.wc procasm.wc script_compiler.wc script.wc

