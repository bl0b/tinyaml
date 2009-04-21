SUBDIRS=metasm symasm procasm script extensions
layerdir=$(pkgdatadir)

install: install-recursive .post-inst

.post-inst:
	cd $(pkgdatadir); for i in `echo *.script|sed 's/\.script//g'`; do tinyaml -c $$i.script -s $$i.wc; done

clean:
	rm */*.wc

