# TinyaML
# Copyright (C) 2007 Damien Leroux
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#################################################################################
#   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #
#                                                                               #
#                                TINYAML TUTORIAL                               #
#                     Part IV : Writing extension libraries                     #
#                                4. The Makefile                                #
#                                                                               #
#   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #   #
#################################################################################

# A typical tinyaml extension Makefile.

# This is the configuration part

# EXTENSION is the extension name to be used in the loadlib EXTENSION statement.
EXTENSION = Tutorial
# SOURCES is the list of all the source files to compile
SOURCES = 3.tutorial.c
# TINYALIB is the tinyaml library description and implementation file
TINYALIB = 2.tutorial.tinyalib

######################################################################

CC=gcc
LD=gcc

######################################################################

# Use pkg-config to know how to compile, link, and where to install.
CFLAGS=$(shell pkg-config tinyaml --variable=extlibcflags) $(shell pkg-config tinyap --cflags)
LIBS=$(shell pkg-config tinyaml --variable=extliblibs)
INSTALLDIR=$(shell pkg-config tinyaml --variable=extlibdir)


TARGET=libtinyaml_$(EXTENSION).so
OBJECTS=$(SOURCES:.c=.o)

all: .depend $(TARGET)

.depend: $(SOURCES)
	$(CC) $(CFLAGS) --depend $(SOURCES) > .depend

clean:
	rm -f ${TARGET}

include .depend

$(TARGET): $(OBJECTS)
	$(LD) $(LIBS) $(OBJECTS) -o $@

$(OBJECTS):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

install:
	cp $(TARGET) $(INSTALLDIR)
	cp $(TINYALIB) $(INSTALLDIR)/$(EXTENSION).tinyalib

# Example command and output :
# $ make -f 4.Makefile && sudo make install -f 4.Makefile 
# gcc -L/usr/local/lib -ltinyaml -pthread -shared 3.tutorial.o -o libtinyaml_Tutorial.so
# cp libtinyaml_Tutorial.so /usr/local/lib/tinyaml
# cp 2.tutorial.tinyalib /usr/local/lib/tinyaml/Tutorial.tinyalib

