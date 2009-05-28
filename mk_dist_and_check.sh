#!/bin/bash

sudo find /usr/ -iname '*tinyaml*' -exec echo removing '{}' ';' -exec rm -rf '{}' ';' 2>/dev/null

autoreconf &&make dist && mv tinyaml-0.4.tar.gz .. && (cd ..&&tar xvzf tinyaml-0.4.tar.gz && cd tinyaml-0.4 && ./configure &&make clean all&&sudo make install)
