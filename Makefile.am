SUBDIRS = src include extension ml tests

thread_states.png: thread_states.dot
	dot -Tpng -LO $< -o $@

tinyaml.pc: tinyaml.pc.in
	./gen_pc.pl tinyaml.pc.in "$(prefix)" "$(VERSION)"

/etc/ld.so.conf.d/tinyaml.conf: tinyaml.ld.conf
	sed -e "s,REPLACEME,$(pkglibdir)," $< > $@

install: install-recursive tinyaml.pc /etc/ld.so.conf.d/tinyaml.conf

