
asm
	jmp @_skip_0

reset_tables:
	envGet &__init_symasm call
	newSymTab -$func_dic
	arrayNew  -$func_tab
	+$func_tab push 0 push 0 arraySet
	#stackNew -$local_dic_stack
	#stackNew -$lst_backup
	ret 0

inc_exprs:
	+$expr_list_exprs inc -$expr_list_exprs
	ret 0

inc_calls:
	+$expr_list_calls inc -$expr_list_calls
	ret 0

# string:something -> string:label
gen_label:
	+$lbl_counter inc -$lbl_counter
	push "_" +$lbl_counter toS strcat strcat
	ret 0

newFuncDecl:
	local fname {
		# backup func name
		-$fname
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
	}
	ret 0

funcDeclGet:
	local fname { -$fname
		+$func_tab
		+$func_dic +$fname getSym
		arrayGet
		#push "funcDeclGet '" +$fname push "' => " dup -3 push "\n" print 5
	}
	ret 0

# string:func_name X string:param_name -> nil
funcDeclAddParam:
	local pname { -$pname	# pop param name
		call @funcDeclGet +(FuncDecl.parameters)
		+$pname
		addSym
	}
	ret 0

# string:func_name X string:param_name -> nil
funcDeclAddVararg:
	local fname {
		dup -1 -$fname
		call @funcDeclAddParam
		+$fname call @funcDeclGet push 1 -(FuncDecl.has_vararg)
	}
	ret 0

# string:func_name X string:param_name -> nil
funcDeclAddLocal:
	local pname {
		# pop param name
		-$pname
		call @funcDeclGet +(FuncDecl.locals)
		+$pname
		addSym
	}
	ret 0

# string:func_name X string:param_name -> nil
funcDeclAddClosure:
	local pname {
		# pop param name
		-$pname
		call @funcDeclGet +(FuncDecl.closure)
		+$pname
		addSym
	}
	ret 0

# string:func_name -> nil
funcDeclAddReturn:
	call @funcDeclGet dup 0 +(FuncDecl.returns) inc -(FuncDecl.returns)
	ret 0

# string:func_name -> nil
funcDeclEnter:
	local fdecl, i, st {
		#push "## FUNCDECL ENTER " dup -1 push "\n" print 3
		call @funcDeclGet -$fdecl
		+$local_dic_stack +$fdecl stackPush
		#push "lst_backup " +$lst_backup push " <= " envGet &_locSymTab push "\n" print 5
		#+$lst_backup envGet &_locSymTab stackPush
		#newSymTab envSet &_locSymTab

		#envGet &_locSymTab -$st
		#envGet &_locSymTabNew call

		envGet &_LSTNew call

		push 1 -$i
		+$fdecl+(FuncDecl.locals) -$st
	_fde_foreach_local:
		+$i +$st symTabSz inf [
			+$st +$i getSymName
			envGet &_LSTAdd call
			+$i inc -$i
			jmp@_fde_foreach_param
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
	ret 0

funcDeclLeave:
	#push "## FUNCDECL LEAVE\n" print 1
	#+$local_dic_stack stackPop
	#push "POU " +$lst_backup push " " +$lst_backup stackSize push " ET\n" print 5
	#+$lst_backup stackPeek 0
	#push "POUET " dup -1 envGet &_locSymTab push "\n" print 4
	#envSet &_locSymTab
	#push "POUETPOUET\n" print 1
	#+$lst_backup stackPop
	envGet &_LSTPop call
	ret 0

# string:symbol -> int:context (cf. symIs...)
getSymContext:
	local symbol, locdic, symofs, counter, backup {
	-$symbol

	push 0 -$_sym_ofs

	push "entering getSymContext... " +$local_dic_stack stackSize push "\n" print 3

	+$local_dic_stack stackSize [

		push "entering local context...\n" print 1

		+$local_dic_stack stackPeek 0 -$locdic
		+$locdic -$backup

		+$locdic +(FuncDecl.locals) +$symbol getSym -$symofs
		+$symofs push -1 nEq [
			push 0 +$symofs sub +$call_local_ofs sub -$_sym_ofs
			push "Symbol '" +$symbol push "' is local at ofs " +$_sym_ofs push "\n" print 5
			$symIsLocal jmp @_sc_ret ]

		+$locdic +(FuncDecl.parameters) +$symbol getSym -$symofs
		+$symofs push -1 nEq [
			push 0 +$locdic +(FuncDecl.locals) symTabSz dec sub +$symofs sub +$call_local_ofs sub -$_sym_ofs
			push "Symbol '" +$symbol push "' is a parameter at ofs " +$_sym_ofs push "\n" print 5
			$symIsParam jmp @_sc_ret ]

		+$locdic +(FuncDecl.closure) +$symbol getSym -$symofs
		+$symofs push -1 nEq [
			+$symofs dec -$_sym_ofs
			push "Symbol '" +$symbol push "' is in closure at ofs " +$_sym_ofs push "\n" print 5
			$symIsClosure jmp @_sc_ret ]

		push 0 -$counter

		push 0 +$call_local_ofs sub
#		+$locdic +(FuncDecl.locals) symTabSz dec
#		+$locdic +(FuncDecl.parameters) symTabSz dec
#		add sub
		-$_sym_ofs

		+$counter inc -$counter

	_context_loop:
		+$local_dic_stack stackSize +$counter sup [

			push "entering local sub-context #" +$counter push "...\n" print 3
			push "base _sym_ofs is " +$_sym_ofs push "\n" print 3

			+$local_dic_stack +$counter stackPeek -$locdic

			+$locdic +(FuncDecl.locals) +$symbol getSym -$symofs
			+$symofs push -1 nEq [
				+$backup +(FuncDecl.closure) +$symbol addSym
				+$backup +(FuncDecl.closure_ofs)
					+$_sym_ofs
					+$symofs
					sub -$_sym_ofs
					+$_sym_ofs
					+$backup +(FuncDecl.closure_ofs) arraySize
				arraySet
				push "Symbol local '" +$symbol push "' outbound ofs is " +$_sym_ofs push "\n" print 5
				+$backup +(FuncDecl.closure) +$symbol getSym dec -$_sym_ofs
				#push "Symbol '" +$symbol push "' is in closure at ofs " +$_sym_ofs push "\n" print 5
				$symIsClosure jmp @_sc_ret ]
	
			+$locdic +(FuncDecl.parameters) +$symbol getSym -$symofs
			+$symofs push -1 nEq [
				+$backup +(FuncDecl.closure) +$symbol addSym
				+$backup +(FuncDecl.closure_ofs)
					+$_sym_ofs
					+$locdic +(FuncDecl.locals) symTabSz dec
					sub
					+$symofs dec
					sub -$_sym_ofs
					+$_sym_ofs
					+$backup +(FuncDecl.closure_ofs) arraySize
				arraySet
				push "Symbol param '" +$symbol push "' outbound ofs is " +$_sym_ofs push "\n" print 5
				+$backup +(FuncDecl.closure) +$symbol getSym dec -$_sym_ofs
				push "Symbol '" +$symbol push "' is in closure at ofs " +$_sym_ofs push "\n" print 5
				$symIsClosure jmp @_sc_ret ]
	
			+$locdic +(FuncDecl.closure) +$symbol getSym dec -$symofs
			+$symofs push -1 sup [
				push "Error : can't access symbol '" +$symbol push "' in outer closure, offset " +$symofs push ".\n" print 5
				$symMustEnclose jmp @_sc_ret ]

			+$_sym_ofs
			+$locdic +(FuncDecl.locals) symTabSz dec
			+$locdic +(FuncDecl.parameters) symTabSz dec
			add sub
			-$_sym_ofs

			+$counter inc -$counter
			jmp @_context_loop
		]
	]

	#envGet &_globSymTab +$symbol getSym -$symofs
	+$symbol envGet &_GSTFindSym call -$symofs
	+$symofs push -1 nEq [
		+$symofs -$_sym_ofs
		push "Symbol '" +$symbol push "' is global at ofs " +$_sym_ofs push "\n" print 5
		$symIsGlobal jmp @_sc_ret ]

	$symUnknown
_sc_ret:}
	ret 0

# string:symbol -> int:context (cf. symIs...)
getSymContext__old__:
	local symbol, locdic, symofs, counter, backup {
	-$symbol

	push 0 -$_sym_ofs

	push "entering getSymContext... " +$local_dic_stack stackSize push "\n" print 3

	+$local_dic_stack stackSize [

		push "entering local context...\n" print 1

		+$local_dic_stack stackPeek 0 -$locdic
		+$locdic -$backup

		+$locdic +(FuncDecl.locals) +$symbol getSym -$symofs
		+$symofs push -1 nEq [
			push 0 +$symofs sub +$call_local_ofs sub -$_sym_ofs
			push "Symbol '" +$symbol push "' is local at ofs " +$_sym_ofs push "\n" print 5
			$symIsLocal jmp @_sc_ret ]

		+$locdic +(FuncDecl.parameters) +$symbol getSym -$symofs
		+$symofs push -1 nEq [
			push 0 +$locdic +(FuncDecl.locals) symTabSz dec sub +$symofs sub +$call_local_ofs sub -$_sym_ofs
			push "Symbol '" +$symbol push "' is a parameter at ofs " +$_sym_ofs push "\n" print 5
			$symIsParam jmp @_sc_ret ]

		+$locdic +(FuncDecl.closure) +$symbol getSym -$symofs
		+$symofs push -1 nEq [
			+$symofs dec -$_sym_ofs
			push "Symbol '" +$symbol push "' is in closure at ofs " +$_sym_ofs push "\n" print 5
			$symIsClosure jmp @_sc_ret ]

		push 0 -$counter

		push 0 +$call_local_ofs sub
#		+$locdic +(FuncDecl.locals) symTabSz dec
#		+$locdic +(FuncDecl.parameters) symTabSz dec
#		add sub
		-$_sym_ofs

		+$counter inc -$counter

	_context_loop:
		+$local_dic_stack stackSize +$counter sup [

			push "entering local sub-context #" +$counter push "...\n" print 3
			push "base _sym_ofs is " +$_sym_ofs push "\n" print 3

			+$local_dic_stack +$counter stackPeek -$locdic

			+$locdic +(FuncDecl.locals) +$symbol getSym -$symofs
			+$symofs push -1 nEq [
				+$backup +(FuncDecl.closure) +$symbol addSym
				+$backup +(FuncDecl.closure_ofs)
					+$_sym_ofs
					+$symofs
					sub -$_sym_ofs
					+$_sym_ofs
					+$backup +(FuncDecl.closure_ofs) arraySize
				arraySet
				push "Symbol local '" +$symbol push "' outbound ofs is " +$_sym_ofs push "\n" print 5
				+$backup +(FuncDecl.closure) +$symbol getSym dec -$_sym_ofs
				#push "Symbol '" +$symbol push "' is in closure at ofs " +$_sym_ofs push "\n" print 5
				$symIsClosure jmp @_sc_ret ]
	
			+$locdic +(FuncDecl.parameters) +$symbol getSym -$symofs
			+$symofs push -1 nEq [
				+$backup +(FuncDecl.closure) +$symbol addSym
				+$backup +(FuncDecl.closure_ofs)
					+$_sym_ofs
					+$locdic +(FuncDecl.locals) symTabSz dec
					sub
					+$symofs dec
					sub -$_sym_ofs
					+$_sym_ofs
					+$backup +(FuncDecl.closure_ofs) arraySize
				arraySet
				push "Symbol param '" +$symbol push "' outbound ofs is " +$_sym_ofs push "\n" print 5
				+$backup +(FuncDecl.closure) +$symbol getSym dec -$_sym_ofs
				push "Symbol '" +$symbol push "' is in closure at ofs " +$_sym_ofs push "\n" print 5
				$symIsClosure jmp @_sc_ret ]
	
			+$locdic +(FuncDecl.closure) +$symbol getSym dec -$symofs
			+$symofs push -1 sup [
				push "Error : can't access symbol '" +$symbol push "' in outer closure, offset " +$symofs push ".\n" print 5
				$symMustEnclose jmp @_sc_ret ]

			+$_sym_ofs
			+$locdic +(FuncDecl.locals) symTabSz dec
			+$locdic +(FuncDecl.parameters) symTabSz dec
			add sub
			-$_sym_ofs

			+$counter inc -$counter
			jmp @_context_loop
		]
	]

	envGet &_globSymTab +$symbol getSym -$symofs
	+$symofs push -1 nEq [
		+$symofs -$_sym_ofs
		push "Symbol '" +$symbol push "' is global at ofs " +$_sym_ofs push "\n" print 5
		$symIsGlobal jmp @_sc_ret ]

	$symUnknown
_sc_ret:}
	ret 0

_skip_0:
end

