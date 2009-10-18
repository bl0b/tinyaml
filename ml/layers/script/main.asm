require "procasm.wc"
#include "header.asm"
include "grammar"
include "globals"
include "structs"
include "functions.asm"
include "analyzeFuncDecl.walker"
include "exprListType.walker"
include "compiler.asm"

asm
	jmp @_skip_init_term

__init:
	#push "ON INIT ! P0UET !\\n" print 1
	%reset_tables()
	ret 0

__term:
	#push "ON TERM ! POUET !\\n" print 1
	ret 0

_skip_init_term:
	dynFunNew @__init
	dup 0 onCompInit
	call
	dynFunNew @__term
	onCompTerm
	ret 0
end

