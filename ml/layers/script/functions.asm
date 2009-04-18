
func reset_tables()
	#envGet &__init_symasm call
	newSymTab -$func_dic
	arrayNew  -$func_tab
	+$func_tab push 0 push 0 arraySet
	stackNew -$local_dic_stack
	#stackNew -$lst_backup
endfunc

func inc_exprs()
	+$expr_list_exprs inc -$expr_list_exprs
endfunc

func dump_symtab(stname, st)
	+$stname push " :\n" print 2
	local i {
		push 0 -$i
	_dst_lp:
		+$i +$st symTabSz inf [
			+$i push ": " +$st +$i getSymName push "\n" print 4
			+$i inc -$i
			jmp @_dst_lp
		]
	}
endfunc

func inc_calls()
	+$expr_list_calls inc -$expr_list_calls
endfunc

# string:something -> string:label
func gen_label(prefix)
	+$lbl_counter inc -$lbl_counter
	+$prefix push "_" +$lbl_counter toS strcat strcat
endfunc

func newFuncDecl(fname)
	# insert symbol into tab
	+$func_dic
	+$fname
	addSym
	# add new func decl entry into tab
	+$func_tab
	# create structure
	strucNew FuncDecl {
		returns		: push 0
		parameters	: newSymTab
		locals		: newSymTab
		closure		: newSymTab
		closure_ofs	: arrayNew
		label		: push ""
		endlabel	: push ""
		has_vararg	: push 0
	}
	+$func_dic +$fname getSym
	# fetch index of sym
	arraySet
	pop
endfunc

func funcDeclGet(fname)
	+$func_tab
	+$func_dic +$fname getSym
	arrayGet
	#push "funcDeclGet '" +$fname push "' => " dup -3 push "\n" print 5
endfunc

# string:func_name X string:param_name -> nil
func funcDeclAddParam(fname, pname)
	#push "funcDeclAddParam" +$pname push "\n" print 3
	%funcDeclGet(+$fname) +(FuncDecl.parameters)
	+$pname
	addSym
endfunc

# string:func_name X string:param_name -> nil
func funcDeclAddVararg(fname, pname)
	#push "funcDeclAddVararg" +$pname push "\n" print 3
	%funcDeclAddParam(+$fname, +$pname)
	%funcDeclGet(+$fname) push 1 -(FuncDecl.has_vararg)
endfunc

# string:func_name X string:param_name -> nil
func funcDeclAddLocal(fname, pname)
	#push "funcDeclAddLocal" +$pname push "\n" print 3
	%funcDeclGet(+$fname) +(FuncDecl.locals)
	+$pname
	addSym
endfunc

# string:func_name X string:param_name -> nil
func funcDeclAddClosure(fname, pname)
	#push "funcDeclAddClosure" +$pname push "\n" print 3
	%funcDeclGet(+$fname) +(FuncDecl.closure)
	+$pname
	addSym
endfunc

# string:func_name -> nil
func funcDeclAddReturn(fname)
	#push "funcDeclAddReturn\n" print 1
	%funcDeclGet(+$fname) dup 0 +(FuncDecl.returns) inc -(FuncDecl.returns)
endfunc

# string:func_name -> nil
func funcDeclEnter(fname)
	local fdecl, i, st {
		#push "## FUNCDECL ENTER " +$fname push "\n" print 3
		%funcDeclGet(+$fname) -$fdecl
		+$local_dic_stack +$fdecl stackPush

		envGet &_CSTNew call
		envGet &_LSTNew call

		push 1 -$i
		+$fdecl+(FuncDecl.closure) -$st
	_fde_foreach_closure:
		+$i +$st symTabSz inf [
			+$st +$i getSymName
			envGet &_CSTAdd call
			+$i inc -$i
			jmp@_fde_foreach_closure
		]

		push 1 -$i
		+$fdecl+(FuncDecl.locals) -$st
	_fde_foreach_local:
		+$i +$st symTabSz inf [
			+$st +$i getSymName
			envGet &_LSTAdd call
			+$i inc -$i
			jmp@_fde_foreach_local
		]

		push 1 -$i
		+$fdecl+(FuncDecl.parameters) -$st
	_fde_foreach_param:
		+$i +$st symTabSz inf [
			+$st +$i getSymName
			envGet &_LSTAdd call
			+$i inc -$i
			jmp@_fde_foreach_param
		]

	}
endfunc

func funcDeclLeave()
	#push "## FUNCDECL LEAVE => " print 1
	+$local_dic_stack stackPop
	#+$local_dic_stack stackSize push "\n" print 2
	#push "POU " +$lst_backup push " " +$lst_backup stackSize push " ET\n" print 5
	#+$lst_backup stackPeek 0
	#push "POUET " dup -1 envGet &_locSymTab push "\n" print 4
	#envSet &_locSymTab
	#push "POUETPOUET\n" print 1
	#+$lst_backup stackPop
	envGet &_LSTPop call
	envGet &_CSTNew call
endfunc

func testSym(symbol, tab, rettype, typestr)
	local symofs {
		+$tab +$symbol getSym -$symofs
		+$symofs push -1 nEq [[
			+$rettype $symIsClosure eq [[
				+$symofs dec -$_sym_ofs
			][
				+$_sym_ofs +$symofs sub +$call_local_ofs sub -$_sym_ofs
			]]
			#push "Symbol '" +$symbol push "' is " +$typestr push " at ofs " +$_sym_ofs push "\n" print 7
			+$rettype
		][
			+$rettype $symIsClosure nEq [
				+$_sym_ofs +$tab symTabSz dec sub
			]
			$symUnknown
		]]
	}
endfunc

# string:symbol -> int:context (cf. symIs...)
func getSymContext(symbol)
	local locdic, symofs, counter, backup {

	push 0 +$call_local_ofs sub -$_sym_ofs

	#push "entering getSymContext... " +$local_dic_stack stackSize push "\n" print 3

	+$local_dic_stack stackSize [

		#push "entering local context...\n" print 1

		+$local_dic_stack stackPeek 0 -$locdic
		+$locdic -$backup

		%testSym(+$symbol, +$locdic +(FuncDecl.locals), $symIsLocal, push "local")
		dup 0 $symUnknown eq SNZ jmp @_sc_ret
		pop
		nop 0

		%testSym(+$symbol, +$locdic +(FuncDecl.parameters), $symIsParam, push "a parameter")
		dup 0 $symUnknown eq SNZ jmp @_sc_ret
		pop
		nop 0

		%testSym(+$symbol, +$locdic +(FuncDecl.closure), $symIsClosure, push "in closure")
		dup 0 $symUnknown eq SNZ jmp @_sc_ret
		pop

		+$symbol envGet &_LSTFindSym call -$symofs
		+$symofs push 1 nEq [
			+$symofs
				+$locdic +(FuncDecl.locals) symTabSz dec add
				+$locdic +(FuncDecl.parameters) symTabSz dec add
				+$call_local_ofs add
			-$_sym_ofs
			#push "Symbol " +$symbol push " found at offset " +$_sym_ofs push " (now in closure)\n" print 5
			+$backup +(FuncDecl.closure) +$symbol addSym
			+$backup +(FuncDecl.closure_ofs)
				+$_sym_ofs
				+$backup +(FuncDecl.closure_ofs) arraySize
			arraySet
			$symIsClosure
			jmp @_sc_ret
		]
	]

	+$symbol envGet &_GSTGet call -$symofs
	+$symofs push -1 nEq [
		+$symofs -$_sym_ofs
		#push "Symbol '" +$symbol push "' is global at ofs " +$_sym_ofs push "\n" print 5
		$symIsGlobal jmp @_sc_ret ]

	$symUnknown
_sc_ret:}
endfunc

