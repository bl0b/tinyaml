%prep
%setup -q -n %{name}-%{version}
%patch0 -p1 -b .Fix

sed -i -e 's|\-lreadline|-lreadline -lncurses -lpthread|g' 	\
	   -e 's|\-lcurses|-lcurses -lpthread|g' 				\
	   -e 's|/lib/|/%{_lib}/|g' 							\
	   -e 's|pkg-config\ \+tinyap|pkg-config --libs tinyap|g' src/Makefile.am

sed -i.Orig -e '\|^\.post-inst:|,\|^$|d' \
	  	    -e 's|\.post-inst||g' ml/layers/Makefile.am

sed -i.damien -e '\|^tinyaml.pc:|,\|^$|d' 					 \
			  -e 's|tinyaml.pc||g'		  					 \
			  -e '\|/etc/ld.so.conf.d/tinyaml.conf:|,\|^$|d' \
		      -e 's|/etc/ld.so.conf.d/tinyaml.conf||g' Makefile.am

autoreconf -fi

%build
%configure2_5x
%make

%install
rm -rf %buildroot
# in the buildroot, thus is no ldconfig, se above sed script [.post-inst:]
# It can be genereted int the building tree [%post]
make DESTDIR=%buildroot install

%post 
#
# There is no tinyaml in package building time 
#
pushd %{_datadir}/%{name} &> /dev/null
	for i in `echo *.script|sed 's/\.script//g'`; do 
		tinyaml -c $i.script -s $i.wc; 
	done
popd &> /dev/null

