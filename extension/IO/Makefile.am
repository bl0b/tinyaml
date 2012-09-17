iodir=$(pkglibdir)
EXTRA_DIST = file.h file_rd.h file_wr.h IO.tinyalib

io_LTLIBRARIES = libtinyaml_IO.la
io_DATA = IO.tinyalib

libtinyaml_IO_la_SOURCES = IO.c file.c
libtinyaml_IO_la_LDFLAGS = -module -ffast-math -pthread
libtinyaml_IO_la_CFLAGS = -I ../../include -I . $(tinyap_CFLAGS)



