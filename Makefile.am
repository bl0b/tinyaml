SUBDIRS = src include extension ml tests Doc
ACLOCAL_AMFLAGS=-I m4
EXTRA_DIST=tinyaml.pc.in tinyaml.ld.conf gen_pc.pl thread_states.dot

nobase_pkginclude_HEADERS=extension/IO/file.h extension/IO/file_rd.h extension/IO/file_wr.h extension/RTC/list.h extension/RTC/mutex.h extension/RTC/priority_queue.h extension/RTC/rtc_alloc.h extension/RTC/timer.h

thread_states.png: thread_states.dot
	dot -Tpng -LO $< -o $@

tinyaml.pc: tinyaml.pc.in
	./gen_pc.pl tinyaml.pc.in "$(prefix)" "$(VERSION)"

/etc/ld.so.conf.d/tinyaml.conf: tinyaml.ld.conf
	sed -e "s,REPLACEME,$(pkglibdir)," $< > $@

install: install-recursive tinyaml.pc /etc/ld.so.conf.d/tinyaml.conf

