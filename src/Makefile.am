
bin_PROGRAMS = tinyaml tinyaml_dbg
lib_LTLIBRARIES = libtinyaml.la

libtinyaml_la_CFLAGS = -I ../include -DTINYAML_EXT_DIR="\"$(pkglibdir)\""
libtinyaml_la_LIBADD = -ldl -ltinyap -lm -lpthread
#libtinyaml_la_LIBADD = -ldl /usr/local/lib/libtinyap.a -lm -lpthread

libtinyaml_la_SOURCES =	containers/list.c \
			containers/dynarray.c \
			containers/generic_stack.c \
			 \
			compiler/opcode_dict.c \
			compiler/opcode_chain.c \
			compiler/abstract_io.c \
			compiler/ape_compiler.c \
			compiler/ape_virtual.c \
			 \
			ml_core_gram.c \
			 \
			vm/vm.c \
			vm/thread.c \
			vm/objects.c \
			vm/program.c \
			vm/text_seg.c \
			vm/vm_engine.c \
			 \
			ops/data.c \
			ops/bml_vm.c \
			ops/arithbin.c \
			ops/containers.c \
			ops/compiler.c

tinyaml_dbg_SOURCES = tinyaml_dbg.c
tinyaml_dbg_LDADD = libtinyaml.la -lcurses
tinyaml_dbg_LDFLAGS = -Wl,--export-dynamic
tinyaml_dbg_CFLAGS = -I ../include


tinyaml_SOURCES = tinyaml.c
tinyaml_LDADD = libtinyaml.la
tinyaml_LDFLAGS = -Wl,--export-dynamic
tinyaml_CFLAGS = -I ../include



ml_core_gram.c: ../ml/ml_core.gram ../ml/ml_core.lib
	rm -f .$@.tmp
	tinyap -g short -i $< -p -o .$@.tmp
	sed -i -e 's/\\/\\\\/g' -e 's/"/\\"/g' .$@.tmp
	head -1 .$@.tmp|sed -e 's/^/const char* ml_core_grammar = "/' -e 's/$$/";\r/' > $@
	echo >> $@
	echo "const char* ml_core_lib =" >> $@
	sed -e 's/^/"/' -e 's/$$/\\n"/' ../ml/ml_core.lib >> $@
	echo ";" >> $@
	rm -f .$@.tmp

# The following is for profiling purpose only

tinyaml_static:
	gcc -ldl -lm -pthread -DTINYAML_EXT_DIR="\"/usr/local/lib/tinyaml/\"" -I ../include/ -Wl,--export-dynamic -pg tinyaml.c ./*/*.c ml_core_gram.c /usr/local/lib/libtinyap.a -o tinyaml_static

