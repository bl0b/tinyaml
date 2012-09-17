rtcdir=$(pkglibdir)
EXTRA_DIST = list.h  mutex.h  priority_queue.h  rtc_alloc.h  timer.h RTC.tinyalib

rtc_LTLIBRARIES = libtinyaml_RTC.la
rtc_DATA = RTC.tinyalib

libtinyaml_RTC_la_SOURCES = rtc_alloc.c priority_queue.c timer.c libRTC.c
libtinyaml_RTC_la_LDFLAGS = -module -ffast-math -pthread
libtinyaml_RTC_la_CFLAGS = -I ../../include -I . $(tinyap_CFLAGS)



