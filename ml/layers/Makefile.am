SUBDIRS=metasm symasm procasm script
layerdir=$(pkgdatadir)

install: install-recursive .post-inst

.post-inst:
	chmod a+x $(pkgdatadir)/*.wc

clean:
	rm */*.wc

