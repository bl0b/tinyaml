include_HEADERS = obj.h core/core.h obj_id_container.h  stack.h

bin_PROGRAMS = tinyaml
lib_LTLIBRARIES = libtinyaml.la

tinyaml_SOURCES = test_obj.c
tinyaml_LDADD = -ldl -ltinyap libtinyaml.la
tinyaml_LDFLAGS = -Wl,--export-dynamic

libtinyaml_la_SOURCES = obj.c stack.c obj_id_container.c ape_compiler.c \
	core/Class.c                core/Env.c    core/Func.c  core/Message.c      core/Object.c  core/Stack.c \
	core/DeferredMessageList.c  core/Float.c  core/Int.c   core/MessageList.c  core/Ref.c     core/String.c \
	core/AST.c core/IO.c core/Compiler.c core/Parser.c


