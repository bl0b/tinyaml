TINYAML=../../../src/tinyaml
TINYAML_CMD=LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:../../../src/.libs ../../../src/.libs/tinyaml -t
#TINYAML_CMD=LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:../../../src/.libs valgrind ../../../src/.libs/tinyaml
#TINYAML_CMD=LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:../../../src/.libs gdb --args ../../../src/.libs/tinyaml

clean:
	rm *.wc
