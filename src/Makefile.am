
#bin_PROGRAMS = tinyaml tinyaml_dbg tinyaml_static
bin_PROGRAMS = tinyaml tinyaml_dbg
lib_LTLIBRARIES = libtinyaml.la

libtinyaml_la_CFLAGS = $(tinyap_CFLAGS) -I ../include -I ../../include -DTINYAML_EXT_DIR="\"$(pkglibdir)\"" -DTINYAML_DATA_DIR="\"$(pkgdatadir)\""
libtinyaml_la_LIBADD = $(tinyap_LIBS) -lm -ltinyap
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
tinyaml_dbg_CFLAGS = $(tinyap_CFLAGS) -I ../include


tinyaml_SOURCES = tinyaml.c
tinyaml_LDADD = libtinyaml.la -lreadline
tinyaml_LDFLAGS = -Wl,--export-dynamic
tinyaml_CFLAGS = $(tinyap_CFLAGS) -I ../include



ml_core_gram.c: ../ml/ml_core.gram ../ml/ml_core.lib
	rm -f .$@.tmp
	tinyap -g short -i $< -p -o .$@.tmp
	sed -i -e 's/\\/\\\\/g' -e 's/"/\\"/g' .$@.tmp
	echo "/* TinyaML" > $@
	echo " * Copyright (C) 2007 Damien Leroux" >> $@
	echo " *" >> $@
	echo " * This program is free software; you can redistribute it and/or" >> $@
	echo " * modify it under the terms of the GNU General Public License" >> $@
	echo " * as published by the Free Software Foundation; either version 2" >> $@
	echo " * of the License, or (at your option) any later version." >> $@
	echo " *" >> $@
	echo " * This program is distributed in the hope that it will be useful," >> $@
	echo " * but WITHOUT ANY WARRANTY; without even the implied warranty of" >> $@
	echo " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the" >> $@
	echo " * GNU General Public License for more details." >> $@
	echo " *" >> $@
	echo " * You should have received a copy of the GNU General Public License" >> $@
	echo " * along with this program; if not, write to the Free Software" >> $@
	echo " * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA." >> $@
	echo " */" >> $@
	echo >> $@
	head -1 .$@.tmp|sed -e 's/^/const char* ml_core_grammar = "/' -e 's/$$/";\r/' >> $@
	echo >> $@
	echo "const char* ml_core_lib =" >> $@
	sed -e 's/^/"/' -e 's/$$/\\n"/' ../ml/ml_core.lib >> $@
	echo ";" >> $@
	rm -f .$@.tmp

# The following is for profiling purpose only (and to perform the pre-install language layers compilations)

tinyaml_static: $(libtinyaml_la_SOURCES) tinyaml.c
	$(CC) -ggdb -ldl -lm -pthread $(tinyap_CFLAGS) -DTINYAML_EXT_DIR="\"$(pkglibdir)\"" -DTINYAML_DATA_DIR="\"$(pkgdatadir)\"" -I ../include/ -Wl,--export-dynamic -pg tinyaml.c $(libtinyaml_la_SOURCES) -ltinyap -lreadline -o tinyaml_static

tinyaml_full_static : $(libtinyaml_la_SOURCES) tinyaml.c
	$(CC) -ggdb -ldl -lm -pthread $(tinyap_CFLAGS) -DTINYAML_EXT_DIR="\"$(pkglibdir)\"" -DTINYAML_DATA_DIR="\"$(pkgdatadir)\"" -I ../include/ -Wl,--export-dynamic -pg tinyaml.c $(libtinyaml_la_SOURCES) `pkg-config tinyap --variable=staticlink` /usr/lib/libreadline.a /usr/lib/libncurses.a -o tinyaml_full_static

#all: tinyaml_static tinyaml_full_static
