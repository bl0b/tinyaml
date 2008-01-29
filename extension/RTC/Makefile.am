rtcdir=$(pkglibdir)
EXTRA_DIST = list.h  mutex.h  priority_queue.h  rtc_alloc.h  timer.h


rtcldflags=-module -ffast-math -pthread

rtc_LTLIBRARIES = libtinyaml_rtc.la

libtinyaml_rtc_la_SOURCES = rtc_alloc.c priority_queue.c timer.c libRTC.c
libtinyaml_rtc_la_LDFLAGS = -module -ffast-math -pthread
libtinyaml_rtc_la_CFLAGS = -I ../../include -I .



