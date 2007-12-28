
bin_PROGRAMS = tinyaml
#lib_LTLIBRARIES = libtinyaml.la

libtinyaml_la_CFLAGS = -I ../include
libtinyaml_la_LIBADD = -ldl -ltinyap -lm

libtinyaml_la_SOURCES =	containers/generic_stack.c \
			containers/list.c \
			containers/dynarray.c \
			 \
			compiler/ape_compiler.c \
			compiler/opcode_dict.c \
			compiler/opcode_chain.c \
			compiler/abstract_io.c \
			 \
			ml_core_gram.c \
			vm/text_seg.c \
			vm/vm.c \
			vm/program.c \
			vm/thread.c \
			vm/vm_engine.c \
			 \
			ops/bml_vm.c \
			ops/arithbin.c \
			ops/compiler.c

tinyaml_SOURCES = tinyaml.c $(libtinyaml_la_SOURCES)
#tinyaml_LDADD = libtinyaml.la
tinyaml_LDFLAGS = -Wl,--export-dynamic $(libtinyaml_la_LIBADD)

tinyaml_CFLAGS = -I ../include



ml_core_gram.c: ../ml/ml_core.gram ../ml/ml_core.lib
	rm -f .$@.tmp
	tinyap -i $< -p -o .$@.tmp
	sed -i -e 's/\\/\\\\/g' -e 's/"/\\"/g' .$@.tmp
	head -1 .$@.tmp|sed -e 's/^/const char* ml_core_grammar = "/' -e 's/$$/";\r/' > $@
	echo >> $@
	echo "const char* ml_core_lib =" >> $@
	sed -e 's/^/"/' -e 's/$$/\\n"/' ../ml/ml_core.lib >> $@
	echo ";" >> $@
	rm -f .$@.tmp



