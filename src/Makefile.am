
bin_PROGRAMS = tinyaml
lib_LTLIBRARIES = libtinyaml.la

tinyaml_SOURCES = tinyaml.c
tinyaml_LDADD = -ldl -ltinyap libtinyaml.la
tinyaml_LDFLAGS = -Wl,--export-dynamic

tinyaml_CFLAGS = -I ../include

libtinyaml_la_CFLAGS = -I ../include

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
			ops/bml_vm.c


ml_core_gram.c: ../ml/ml_core.gram
	tinyap -i $< -p -o .$@.tmp
	sed -i -e 's/\\/\\\\/g' -e 's/"/\\"/g' .$@.tmp
	head -1 .$@.tmp|sed -e 's/^/const char const* ml_core_grammar = "/' -e 's/$$/";\r/' > $@


