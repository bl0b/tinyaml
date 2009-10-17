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
CFLAGS=$(shell pkg-config tinyaml --variable=extlibcflags)
#LIBS=$(shell pkg-config tinyaml --variable=extliblibs)
LIBS=-L/usr/local/lib -ltinyaml -pthread -shared
INSTALLDIR=$(shell pkg-config tinyaml --variable=extlibdir)


TARGET=libtinyaml_$(EXTENSION).so
OBJECTS=$(SOURCES:.c=.o)

all: .depend $(TARGET)

.depend: $(SOURCES)
	$(CC) --depend $(SOURCES) > .depend

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

