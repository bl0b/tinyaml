
compile Script asm
	#pp_curNode
	compileStateDown
end

compile instruction_bloc asm compileStateDown end
compile instruction_expr_list asm compileStateDown end


compile script_throw asm
	<< push s(astGetChildString 0) throw >>
	compileStateNext
end


compile script_try asm
	local try_lbl, catch_lbl, end_lbl {
		%gen_label(push "try") -$try_lbl
		%gen_label(push "catch") -$catch_lbl
		%gen_label(push "end_trycatch") -$end_lbl

		# try: instCatcher @catch
		<< (+$try_lbl): instCatcher l(+$catch_lbl) >>
		# ...
		astCompileChild 0
		# jmp @end_trycatch catch:
		<< uninstCatcher l(+$end_lbl) (+$catch_lbl): >>
		# ...
		+$end_lbl -$current_end_tc
		astCompileChild 1
		# end_trycatch:
		<< (+$end_lbl): >>
	}
	compileStateNext
end


compile catch_statement asm
	local end_lbl {
		%gen_label(push "endcatch_" astGetChildString 0 strcat) -$end_lbl
		<< getException strcmp s(astGetChildString 0) SZ jmp l(+$end_lbl) >>
		astCompileChild 1
		<< jmp l(+$current_end_tc) (+$end_lbl): >>
	}
	compileStateNext
end

compile catch_bloc asm
	local counter {
		push 0 -$counter
	catch_bloc_loop:
		+$counter astGetChildrenCount inf [
			+$counter astCompileChild
			+$counter inc -$counter
			jmp @catch_bloc_loop
		]

		# default: propagate exception
		<< getException throw >>
	}
	compileStateNext
end

#FIXME TODO have onInit and onTerm opcodes for handling user compiler init/term routines to handle things like reset_tables
#FIXED ?

compile script_glob
asm
	#pp_curNode
	# size,counter
	local size, counter {
		# if(!node_opd_count) return
		astGetChildrenCount
		SNZ jmp @done_glob
		# size = node_opd_count
		astGetChildrenCount -$size
		# write("data 0 rep $size end")
		+$size push 0 write_data
		# counter=0
		push 0 -$counter
		# do {
	fill_glob_dict:
		# addsym(node_childString(counter,dic)
#		push "fill glob #" +$counter push "\n" print 3
		+$counter astGetChildString envGet &_GSTAdd call
		# counter += 1
		+$counter inc -$counter
		# } while(counter<size)
		+$counter +$size inf SZ jmp @fill_glob_dict
	done_glob:
	}
	compileStateNext
end


compile script_glob_fun
asm
	local sz {
		#pp_curNode
		astGetChildString 0 -$cur_fname

		+$cur_fname envGet &_GSTGet call push -1 nEq [[
			#push "Error : Symbol " astGetChildString 0 push "\ already defined !\\n" print 3
			push "Warning : shadowing previous definition of function " +$cur_fname push ".\\n" print 3
			#compileStateError
			#ret 0
		][
			push 1 push 0 write_data
			+$cur_fname envGet &_GSTAdd call
		]]

		#+$glob_dic symTabSz -$sz
		+$cur_fname envGet &_GSTGet -$sz

		# do "cur_fname = ...fun..."
		astCompileChild 1

	#	push "Symbol '" +$cur_fname push "' has index " +$sz push "\n" print 5

		<< setmem i(+$cur_fname envGet &_GSTGet call) >>

		push "" -$cur_fname
	}
	compileStateNext
end

compile script_anon_fun
asm
	local clo_backup, fnambak {
		+$call_local_ofs -$clo_backup
		+$cur_fname -$fnambak
		push 0 -$call_local_ofs
		%gen_label(push "anon") -$cur_fname
		astCompileChild 0
		+$fnambak -$cur_fname
		+$clo_backup -$call_local_ofs
	}
	compileStateNext
end

compile script_anon_array
asm
	<< arrayNew >>
	compileStateDown
end

compile script_anon_array_item
asm
	astCompileChild 0	# push data
	<<
		dup -1		# dup array
		arraySize	# get new index
		arraySet
	>>
	compileStateNext
end


compile script_anon_map
asm
	<< mapNew >>
	compileStateDown
end

compile script_anon_map_item
asm
	astCompileChild 1	# push value
	<< dup -1 >>		# dup map
	<< mapSet s(astGetChildString 0) >>
	compileStateNext
end

compile script_anon_array_end
asm
	compileStateNext
end

compile script_anon_map_end
asm
	compileStateNext
end

compile script_fun_decl
asm
	#pp_curNode
	#push "@@@   at start, $cur_fname = " +$cur_fname push "   @@@\n" print 3
	local locsz, fun_lbl, endfun_lbl, fname_backup, supargc, argc, top, count {
		%gen_label(+$cur_fname) -$fun_lbl
		%gen_label(+$cur_fname push "_end" strcat) -$endfun_lbl

		#push "start label \"" +$fun_lbl push "\"\n" print 3
		#push "end label \"" +$endfun_lbl push "\"\n" print 3

		+$cur_fname
		%newFuncDecl()

		<< jmp l(+$endfun_lbl) (+$fun_lbl): >>

		%funcDeclGet(+$cur_fname) +$fun_lbl -(FuncDecl.label)
		%funcDeclGet(+$cur_fname) +$endfun_lbl -(FuncDecl.endlabel)

		+$cur_fname -$fname_backup

		doWalk "analyzeFuncDecl"

		+$fname_backup -$cur_fname

		%funcDeclEnter(+$cur_fname)

		# insert function header
		#	- if no vararg : check against dynamic argc, throw badarg if nEq
		#	- if vararg : compute supplementary argc, copy sup' args into an array, install the array as last arg
		# first, code to fetch dynamic argc
		%funcDeclGet(+$cur_fname) +(FuncDecl.has_vararg) [[
			%funcDeclGet(+$cur_fname) +(FuncDecl.parameters) symTabSz sub 2 -$argc
			# check that supargc>=0
	
			+$cur_fname push "_vastart_loop" strcat -$fun_lbl
			+$cur_fname push "_vastart_done" strcat -$endfun_lbl

			# throw if failed
			%gen_label(+$cur_fname push "_chk_argc_pass" strcat) -$supargc
			<<	dup 0 push i(+$argc) supEq SZ jmp l(+$supargc)
				push "WrongParameterCount"
				throw
			(+$supargc):
				enter 1
				push i(+$argc) sub
				setmem -1
				arrayNew
			(+$fun_lbl):
				getmem -1 push 0 infEq SZ jmp l(+$endfun_lbl)
				# array is now on top of stack
				# swap with data
				swap 1
				# push index
				getmem -1 dec setmem -1 getmem -1
				# set value
				arraySet
				jmp l(+$fun_lbl)
			(+$endfun_lbl):
				leave 1
				#setmem i(push -1 +$argc sub)			# set array
			>>
	
	#	# start count
	#<<
	#	enter 2
	#	arrayNew
	#	arrayResv (+$argc)
	#	-$fun_lbl
	#	push 0 -$locsz
	#	# get -1-count -th arg
	#(:	+$fun_lbl 
	#	push -1 +$locsz sub getmem
	#	+$locsz
	#	arraySet
	#	+$locsz inc -$locsz
	#	+$locsz inf (+$argc) SZ jmp @_loop
	#>>	
	#
	#
	#
	#		# store array counter max
	#		-$supargc
	#		# now copy
	#		push 1 write_ocInt "enter"
	#		push 0 write_ocInt "push"
	#		push -1 write_
	#		push 1 write_ocInt "leave"
			

		][
			%gen_label(push "check_argc_pass") -$supargc
			# throw if failed
			<< push i(%funcDeclGet(+$cur_fname) +(FuncDecl.parameters) symTabSz dec)
			   eq
			   SZ
			   jmp l(+$supargc)
			   push "WrongParameterCount (expected "
			   push i(%funcDeclGet(+$cur_fname) +(FuncDecl.parameters) symTabSz dec) toS strcat
			   push ")" strcat
			   throw
			   (+$supargc):
			>>
		]]
		%funcDeclGet(+$cur_fname) +(FuncDecl.parameters) symTabSz dec
		%funcDeclGet(+$cur_fname) +(FuncDecl.locals) symTabSz dec
		add
		-$locsz
		+$locsz [ << enter i(+$locsz) >> ]
		%funcDeclGet(+$cur_fname) +(FuncDecl.parameters) symTabSz dec
		dup 0 [[
			push 0 %funcDeclGet(+$cur_fname) +(FuncDecl.locals) symTabSz dec sub -$top
			+$top dup -1 sub -$count
		_prep_parm_loop:
			#push "sur le param " +$count push " sur " +$top push "\n" print 5
			<< setmem i(+$count) >>
			+$count inc -$count
			#push "suivant " +$count push " < 0 ?\\n" print 3
			+$count +$top inf SZ jmp @_prep_parm_loop
		_prep_parm_done:
		][
			pop
		]]


		#+$cur_fname
		#call @funcDeclGet
		#+(FuncDecl.locals)
		#symTabSz dec -$locsz
		#+$locsz [ << enter i(+$locsz) >> ]

		# skip parameters list, directly compile body
		astCompileChild 1

		+$fname_backup -$cur_fname

		+$locsz [ << leave i(+$locsz) >> ]

		# if no return statement has been issued, default to void return (size 0)
		<<	push 0
			ret 0
		(%funcDeclGet(+$cur_fname) +(FuncDecl.endlabel)):
			dynFunNew l(%funcDeclGet(+$cur_fname) +(FuncDecl.label))
		>>

		push 0 -$locsz
		#push "\   @@@   NOW $cur_fname = " +$cur_fname push "   @@@\\n" print 3
		%funcDeclGet(+$cur_fname) +(FuncDecl.closure_ofs) -$fun_lbl
	_addcls_loop:
		+$fun_lbl arraySize +$locsz sup [
			<<	getmem i(+$fun_lbl +$locsz arrayGet)
				dynFunAddClosure
			>>
			+$locsz inc -$locsz
			jmp @_addcls_loop
		]

		%funcDeclLeave()
	}
	compileStateNext
end

compile script_fun_body asm compileStateDown end
compile script_local asm compileStateNext end



compile script_expr_list
asm
	# different cases for handling expr size :
	# count number of litteral expressions Nle and count calls Nc
	# one single function call => no supplementary code at all 							Nc==1&&Nle==0
	# at least one call and any other element => use running size code and add number of exprs at end if non-zero
	# only exprs : push number of exprs at end
	doWalk "exprListType"

	enter 3			# leave in script_expr_list_end
	+$expr_size setmem -1
	+$list_algo setmem -2
	+$call_local_ofs setmem -3

	push 0 -$expr_size

	+$expr_list_exprs [[
		+$expr_list_calls push 0 sup [[
			# si Nc > 0, running size
			$exprListRunningSize -$list_algo
			#push "selecting list algo : Running size for expr at " astGetRow push "," astGetCol push "\n" print 5
			#pp_curNode
			+$call_local_ofs inc -$call_local_ofs
			<< enter 1 push i(+$expr_list_exprs) setmem -1 >>
		][
			# sinon, push Nle
			$exprListOnlyExprs -$list_algo
			#push "selecting list algo : Only exprs\n" print 1
			astGetChildrenCount dec -$expr_size
		]]
	][
		+$expr_list_calls push 1 eq [[
			# si Nc == 1, do nothing
			$exprListOnlyCall -$list_algo
			#push "selecting list algo : Only one call\n" print 1
		][
			# sinon, running size
			$exprListRunningSize -$list_algo
			#push "selecting list algo : Running size for expr at " astGetRow push "," astGetCol push "\n" print 5
			#push "BEFORE expr_list : call_local_ofs=" +$call_local_ofs push "\n" print 3
			#pp_curNode
			+$call_local_ofs inc -$call_local_ofs
			<< enter 1 push i(+$expr_list_exprs) setmem -1 >>
		]]
	]]
	compileStateDown
end

compile script_expand
asm
	astCompileChild 0
	local lbl, elbl {
		# assume array on top of stack
		%gen_label(push "expand_loop") -$lbl
		%gen_label(push "expand_done") -$elbl
		<<	enter 2
			setmem -1
			getmem -1 arraySize setmem -2
		(+$lbl):
			getmem -2 SNZ jmp l(+$elbl)
			# push arr[sz-index]
			getmem -1
				dup 0
				arraySize
				getmem -2
				sub
			arrayGet
			getmem -2 dec setmem -2
			jmp l(+$lbl)
		(+$elbl):
			# push number of elements in array
			getmem -1 arraySize
			leave 2
		>>
	}
end

compile script_expr_atom
asm
	astCompileChild 0
#	+$expr_size inc -$expr_size
	compileStateNext
end

compile script_expr_tuple
asm
	# it is a call inside
	astCompileChild 0
	+$list_algo $exprListRunningSize [
		<< getmem -1 add setmem -1 >>
	]
	compileStateNext
end

compile script_expr_list_end
asm
	$exprListRunningSize +$list_algo eq [[
		#push "LIST_ALGO @END Running size !\n" print 1
		<< getmem -1 leave 1 >>		# push elements count
		+$call_local_ofs dec -$call_local_ofs
		#push "AFTER expr_list : call_local_ofs=" +$call_local_ofs push "\n" print 3
	][ $exprListOnlyExprs +$list_algo eq [[
		#push "LIST_ALGO @END Only exprs ! " +$expr_size push "\n" print 3
		<< push i(+$expr_size) >>
	][ $exprListOnlyCall +$list_algo eq [[
		#push "LIST_ALGO @END Only one call !\n" print 1
		nop
	][
		#push "LIST_ALGO @END !!??!!??\n" print 1
		nop
	]]
	]]
	]]
	getmem -1 -$expr_size
	getmem -2 -$list_algo
	getmem -3 -$call_local_ofs
	leave 3
	compileStateNext
end

compile p_ScriptExpr1
asm
	#+$expr_size inc -$expr_size
	compileStateDown
end

compile p_ScriptExpr
asm
	#+$expr_size inc -$expr_size
	compileStateDown
end

compile script_var_list
asm
	local i, lbl_ok, lbl_too_much {
		%gen_label(push "too_much_data") -$lbl_too_much
		%gen_label(push "ok_data") -$lbl_ok
		<<	#regGet 0
			sub i(astGetChildrenCount dec)
			regSet 0
			regGet 0
			push 0 inf SNZ jmp l(+$lbl_ok)
			push "Not enough data in RHS (expected "
			push 0 regGet 0 sub toS strcat push " more)" strcat
			throw
			(+$lbl_ok):
		>>
		astGetChildrenCount dec dec -$i
		push 1 -$is_lvalue
	_vl_R2L:
		+$i push 0 supEq [
			#push "\tscript_var #" +$i push "\n" print 3
			+$i astCompileChild
			+$i dec -$i jmp@_vl_R2L
		]
		<<	regGet 0
			#push "Unwinding stack by " dup -1 push "\n" print 3
			dup 0 SZ popN pop
		>>
		push 0 -$is_lvalue
	}
	#<< push i(astGetChildrenCount) >>	# push elements count
	compileStateNext
end


compile script_return
asm
	#pp_curNode
	# process righthand side
	+$cur_fname push "" eq [ push "EmptyFuncName" throw ]
	astCompileChild 0
	local locsz {
		#push "compiling RETURN in " +$cur_fname push "\n" print 3
		#pp_curNode
		%funcDeclGet(+$cur_fname) +(FuncDecl.parameters) symTabSz dec
		%funcDeclGet(+$cur_fname) +(FuncDecl.locals) symTabSz dec
		add
		-$locsz
		+$locsz [
			<< leave i(+$locsz) >>
		]
	}
	<< ret 0 >>
	compileStateNext
end


compile script_call_ret_any
asm
	#push "@@ script_call_ret_any " astGetRow push "," astGetCol push "\n" print 5
	#enter 1	+$legal_return_size setmem -1
	#push 0 -$legal_return_size
	astCompileChild 0
	$exprListRunningSize +$list_algo eq [
		<< getmem -1 add setmem -1 >>
	]
	#getmem -1 -$legal_return_size
	#leave 1
	compileStateNext
end


compile script_call_ret_0
asm
	#push "@@ script_call_ret_0 " astGetRow push "," astGetCol push "\n" print 5
	#enter 1	+$legal_return_size setmem -1
	#push 0 -$legal_return_size
	astCompileChild 0
	<< 
		#push "Discarding returned values.\n" print 1
		popN
	>>
	#getmem -1 -$legal_return_size
	#leave 1
	compileStateNext
end


compile script_call_ret_1
asm
	#push "@@ script_call_ret_1 " astGetRow push "," astGetCol push "\n" print 5
	#enter 1	+$legal_return_size setmem -1
	#push 1
	#+$legal_return_size
	astCompileChild 0
	local _lbl {
		%gen_label(push "_check_ret_1") -$_lbl
		<<	push 1 eq SZ jmp l(+$_lbl)
			push "Function call was expected to return exactly ONE value." throw
			(+$_lbl):
		>>
	}
	#getmem -1 -$legal_return_size
	#leave 1
	compileStateNext
end








compile script_call
asm
	local counter, max, clo_backup, ret_fail, ret_good {

		push 1
		-$counter
		astGetChildrenCount dec -$max
#		push "before loop : max = " +$max push "\\n" print 3
#		push "before loop : call_local_ofs = " +$call_local_ofs push "\\n" print 3

		astGetChildrenCount dec [[
		_call_param_loop:
			+$counter astGetChildrenCount inf [
				+$counter astCompileChild
				#<< setmem i(push 0 +$counter sub) >>
				+$counter inc -$counter jmp @_call_param_loop
			]
		][
			<< push 0 >>	# manual insertion of argc when no arg is present
		]]
		# (push function object)
		astCompileChild 0
		# call
		<< call >>
	}
	compileStateNext
end



compile script_loc asm compileStateNext end


compile script_id
asm
	local symctxt {
	#	+$cur_fname
		%getSymContext(astGetChildString 0) -$symctxt
				+$symctxt $symIsLocal eq
				+$symctxt $symIsGlobal eq
			or
			+$symctxt $symIsParam eq
		or [[
			+$is_lvalue [[
				<< setmem i(+$_sym_ofs) >>
				#push 0 -$is_lvalue
			][
				<< getmem i(+$_sym_ofs) >>
			]]
			compileStateNext
		][
			+$symctxt $symIsClosure eq [[
				+$is_lvalue [[
					<< setClosure i(+$_sym_ofs) >>
					#push 0 -$is_lvalue
				][
					<< getClosure i(+$_sym_ofs) >>
				]]
				compileStateNext
			][
				+$symctxt $symIsEnv eq [[
					+$is_lvalue [[
						<< envSet e(astGetChildString 0) >>
					][
						<< envGet e(astGetChildString 0) >>
					]]
					compileStateNext
				][
					push "At " astGetRow push ":" astGetCol push "\t: " print 5
					push "Error with symbol context for sym '" astGetChildString 0 push "' : " print 3
					+$symctxt $symMustEnclose eq [ push "can't handle recursive closures yet\n" print 1]
					+$symctxt $symUnknown eq [ push "symbol is unknown.\n" print 1]
					compileStateError
				]]
			]]
		]]
	}
end


compile script_print
asm
	astCompileChild 0
	<< print >>
	compileStateNext
end


### DEBUG ONLY
compile m_expr asm pp_curNode compileStateDown end
compile b_expr asm pp_curNode compileStateDown end
###



compile script_assign
asm
	#push "########################################################\n" print 1
	#pp_curNode
	#push "========================================================\n" print 1
	local sym {
		#getmem 1 astGetChildString 0 getSym -$sym
		# process righthand side
		astCompileChild 1
		#<< regSet 0 >>			# save number of exprs to be popped
		# process lefthand side
		#push 1 -$is_lvalue
		astCompileChild 0		
	}
	compileStateNext
end


compile script_string
asm
	<< push s(astGetChildString 0) >>
	compileStateNext
end



compile script_param_list
asm
	push "script:"
	astGetOp
	push ": Not implemented.\n"
	print 3
	compileStateNext
end


compile script_int
asm
	<< push i(astGetChildString 0 toI) >>
	compileStateNext
end


compile script_float
asm
	<< push f(astGetChildString 0 toF) >>
	compileStateNext
end


#compile m_expr
#asm
#	#pp_curNode
#	astCompileChild 0
#	compileStateNext
#end


#compile b_expr
#asm
#	#pp_curNode
#	astCompileChild 0
#	compileStateNext
#end


compile m_minus
asm
	<< push 0 >>
	astCompileChild 0
	<< sub >>
	compileStateNext
end


compile m_add
asm
	astCompileChild 0
	astCompileChild 1
	<< add >>
	compileStateNext
end


compile m_sub
asm
	astCompileChild 0
	astCompileChild 1
	<< sub >>
	compileStateNext
end


compile m_mul
asm
	#pp_curNode
	astCompileChild 0
	#pp_curNode
	astCompileChild 1
	<< mul >>
	compileStateNext
end


compile m_div
asm
	astCompileChild 0
	astCompileChild 1
	<< div >>
	compileStateNext
end



compile m_pow
asm
	astCompileChild 0
	astCompileChild 1
	<< pow >>
	compileStateNext
end



compile m_sqrt
asm
	astCompileChild 1
	<< sqrt >>
	compileStateNext
end



compile m_exp
asm
	astCompileChild 1
	<< exp >>
	compileStateNext
end



compile m_log
asm
	astCompileChild 1
	<< log >>
	compileStateNext
end



compile m_pi
asm
	<< push 3.14159265358979323846 >>
	compileStateNext
end


compile m_e
asm
	<< push 2.71828182845904523536 >>
	compileStateNext
end

compile b_and
asm
	astCompileChild 0
	astCompileChild 2
	<< and >>
	compileStateNext
end


compile b_or
asm
	astCompileChild 0
	astCompileChild 2
	<< or >>
	compileStateNext
end


compile b_not
asm
	astCompileChild 1
	<< not >>
	compileStateNext
end



compile comp
asm
	astCompileChild 0
	astCompileChild 2
	astGetChildString 1
	dup 0 push ">" strcmp [[
	dup 0 push "<" strcmp [[
	dup 0 push ">=" strcmp [[
	dup 0 push "<=" strcmp [[
	dup 0 push "=" strcmp [[
	dup 0 push "!=" strcmp [[
		push "Hey, unknown comp op '"
		astGetChildString 1
		push "' !\n"
		print 3
	][
		<< nEq >>
	]]
	][
		<< eq >>
	]]
	][
		<< infEq >>
	]]
	][
		<< supEq >>
	]]
	][
		<< inf >>
	]]
	][
		<< sup >>
	]]
	pop
	compileStateNext
end



compile script_if
asm
	astGetChildrenCount push 3 eq [[
		# if then else
		local if_lbl, then_lbl, else_lbl, endif_lbl {
			%gen_label(+$cur_fname strcat "_if") -$if_lbl
			%gen_label(+$cur_fname strcat "_then") -$then_lbl
			%gen_label(+$cur_fname strcat "_else") -$else_lbl
			%gen_label(+$cur_fname strcat "_endif") -$endif_lbl

			<< (+$if_lbl): >>
			astCompileChild 0
			<< SNZ jmp l(+$else_lbl) (+$then_lbl): >>
			astCompileChild 1
			<< jmp l(+$endif_lbl) (+$else_lbl): >>
			astCompileChild 2
			<< (+$endif_lbl): >>
		}
	][
		# if then
		local if_lbl, then_lbl, endif_lbl {
			%gen_label(+$cur_fname toS strcat "_if") -$if_lbl
			%gen_label(+$cur_fname toS strcat "_then") -$then_lbl
			%gen_label(+$cur_fname toS strcat "_endif") -$endif_lbl

			<< (+$if_lbl): >>
			astCompileChild 0
			<< SNZ jmp l(+$endif_lbl) (+$then_lbl): >>
			astCompileChild 1
			<< (+$endif_lbl): >>
		}
	]]
	compileStateNext
end


compile script_while
asm
	#pp_curNode

	local while_lbl, endwhile_lbl {
		%gen_label(+$cur_fname toS strcat "_while") -$while_lbl
		%gen_label(+$cur_fname toS strcat "_endwhile") -$endwhile_lbl

		<< (+$while_lbl): >>
		astCompileChild 0
		<< SNZ jmp l(+$endwhile_lbl) >>
		astCompileChild 1
		<< jmp l(+$while_lbl) (+$endwhile_lbl): >>
	}
	compileStateNext
end


compile sb_end asm compileStateDown end
compile sb_else asm compileStateDown end

compile script_var asm
	+$is_lvalue astGetChildrenCount push 2 eq and [[
		push 0 -$is_lvalue
		astCompileChild 0
		push 1 -$is_lvalue
		astCompileChild 1
		compileStateNext
	][
		compileStateDown
	]]
end


compile script_env asm
	+$is_lvalue [[
		<< envSet e(astGetChildString 0) >>
		#push 0 -$is_lvalue
	][
		<< envGet e(astGetChildString 0) >>
	]]
	compileStateNext
end


compile script_array_access asm
	+$is_lvalue [[
		push 0 -$is_lvalue
		<< dup -1 >>
		astCompileChild 0
		<< arraySet >>
		<< pop 2 >>
		push 1 -$is_lvalue
	][
		astCompileChild 0
		<< arrayGet >>
	]]
	compileStateNext
end

compile script_struc_access asm
	+$is_lvalue [[
		<< mapSet s(astGetChildString 0) >>
		#push 0 -$is_lvalue
	][
		<< mapGet s(astGetChildString 0) >>
	]]
	compileStateNext
end

