data
 0
 0	# glob dict
 0	# loc dict
end

asm
	newSymTab setmem 1	# glob dict
## tests
#	getmem 1 push "blah" addSym
#	getmem 1 push "coin" addSym
#	getmem 1 push "pouet" addSym
#	getmem 1 push "pan !" addSym
#	push "blah : "
#	getmem 1 push "blah" getSym
#	push "\ncoin : "
#	getmem 1 push "coin" getSym
#	push "\npouet : "
#	getmem 1 push "pouet" getSym
#	push "\npan ! : "
#	getmem 1 push "pan !" getSym
#	push "\n"
#	print 9
#
## reset the table
#	newSymTab setmem 1	# glob dict
end

language
 hello ::= "Hello, world.".

 Script ::= "script" script_loop.
 script_loop = ( script_statement script_loop | "end" ).
 script_statement = ( script_glob | script_fun | script_instruction ).

 script_glob ::= "global" script_sym_loop.
 script_loc ::= "local" script_sym_loop.
 script_sym_loop = ( sym "," script_sym_loop | sym ).
 script_id ::= sym.
 script_string ::= string.

 script_fun ::= "function" sym script_param_list script_fun_loop.
 script_param_list ::= "(" ( script_sym_loop ")" | ")" ).
 script_fun_loop = ( script_local | script_instruction ).

 script_instruction = ( script_assign | AsmBloc | script_print ).

 script_print ::= "print" script_print_loop.
 script_print_loop = script_expr "," script_print_loop | script_expr.
 script_expr = script_string | m_expr.

 script_assign ::= sym "=" script_expr.

 script_int ::= int.
 script_float ::= float.
 number = ( script_float | script_int | script_id ).

m_expr = ( m_add
	 | m_sub
	 | m_mul
	 | m_div
	 | m_minus
	 | number
	 ).

m_minus ::= "-" m_atom.

m_add  ::= m_add_l "+" m_add_r.
m_sub  ::= m_sub_l "-" m_sub_r.

m_mul  ::= m_mul_l "*" m_mul_r.
m_div  ::= m_div_l "/" m_div_r.



m_atom = ( number
	 | m_minus
	 | "(" m_expr ")"
	 ).


m_add_l = ( m_sub
	  | m_mul
	  | m_div
	  | m_atom
	  ).

m_add_r = ( m_add
	  | m_sub
	  | m_mul
	  | m_div
	  | m_atom
	  ).


m_sub_l = ( m_mul
	  | m_div
	  | m_atom
	  ).

m_sub_r = ( m_mul
	  | m_div
          | m_atom
	  ).


m_mul_l = ( m_div
	  | m_atom
	  ).

m_mul_r = ( m_mul
	  | m_div
	  | m_atom
	  ).

m_div_l = m_atom.

m_div_r = m_atom.


p_Code = hello | Script.

end



#plug hello into p_Code
#plug Script into p_Code

compile Script asm
	#pp_curNode
	compileStateDown
end



compile script_glob
asm
	#pp_curNode
	# size,counter
	enter 2
	# if(!node_opd_count) return
	astGetChildrenCount
	SNZ jmp @done_glob
	# size = node_opd_count
	astGetChildrenCount setmem -1
	# write("data 0 rep $size end")
	getmem -1 push 0 write_data
	# counter=0
	push 0 setmem -2
	# do {
fill_glob_dict:
	# addsym(node_childString(counter,dic)
	getmem 1 getmem -2 astGetChildString addSym
	# counter += 1
	getmem -2 inc setmem -2
	# } while(counter!=size)
	getmem -2 getmem -1 sub SZ jmp @fill_glob_dict
done_glob:
	leave 2
	compileStateNext
end


compile script_loc
asm
	push "script:"
	astGetOp
	push ": Not implemented.\n"
	print 3
	compileStateNext
end


compile script_id
asm
	#pp_curNode
	getmem 1 astGetChildString 0 getSym
	dup 0 SNZ jmp @_id_no_def_error
	write_oc_Int "getmem"
done_id:
	compileStateNext
	ret 0
_id_no_def_error:
	compileStateError
	ret 1
end


compile script_print
asm
	# size,counter
	enter 2
	# if(!node_opd_count) return
	astGetChildrenCount
	SNZ jmp @done_print
	# size = node_opd_count
	astGetChildrenCount setmem -1
	# counter=0
	push 0 setmem -2
	# do {
fill_print:
	getmem -2 astCompileChild
	# counter += 1
	getmem -2 inc setmem -2
	# } while(counter!=size)
	getmem -2 getmem -1 sub SZ jmp @fill_print
done_print:
	astGetChildrenCount write_oc_Int "print"
	leave 2
	compileStateNext
end


compile script_assign
asm
	enter 1
	getmem 1 astGetChildString 0 getSym setmem -1
	# process righthand side
	astCompileChild 1
	# fetch memory location
	getmem -1 write_oc_Int "setmem"
	leave 1
	compileStateNext
end


compile script_string
asm
	astGetChildString 0
	write_oc_String "push"
	compileStateNext
end


compile script_fun
asm
	push "script:"
	astGetOp
	push ": Not implemented.\n"
	print 3
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
	astGetChildString 0 toI
	write_oc_Int "push"
	compileStateNext
end


compile script_float
asm
	astGetChildString 0 toF
	write_oc_Float "push"
	compileStateNext
end


compile m_expr
asm
	astCompileChild 0
	compileStateNext
end


compile m_minus
asm
	push 0 write_oc_Int "push"
	astCompileChild 0
	write_oc "sub"
	compileStateNext
end


compile m_add
asm
	astCompileChild 0
	astCompileChild 1
	write_oc "add"
	compileStateNext
end


compile m_sub
asm
	astCompileChild 0
	astCompileChild 1
	write_oc "sub"
	compileStateNext
end


compile m_mul
asm
	#pp_curNode
	astCompileChild 0
	#pp_curNode
	astCompileChild 1
	write_oc "mul"
	compileStateNext
end


compile m_div
asm
	astCompileChild 0
	astCompileChild 1
	write_oc "div"
	compileStateNext
end




compile hello
asm
	write_label "prout"
	push "hello, world."
	write_oc_String "push"
	push "\n"
	write_oc_String "push"
	push 2
	write_oc_Int "print"
	compileStateNext
end

