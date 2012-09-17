require "procasm.wc"

glob
	object = 0
	metaclass = 0
	class = 0
end

asm
	jmp @_skip_0

_cls_sname: pop _vcls_sname push 0 ret 0
_cls_sbase: pop _vcls_sbase push 0 ret 0
_obj_scls: pop _vobj_scls push 0 ret 0
_obj_smbr: pop _vobj_smbr push 0 ret 0
_cls_gname: pop _vcls_gname push 1 ret 0
_cls_gbase: pop _vcls_gbase push 1 ret 0
_obj_gmbr: pop _vobj_gmbr push 1 ret 0
_obj_gcls: pop _vobj_gcls push 1 ret 0

_skip_0:
	dynFunNew @_cls_sname 	push "class_set_name" envAdd
	dynFunNew @_cls_gname 	push "class_get_name" envAdd
	dynFunNew @_obj_scls 	push "object_set_class" envAdd
	dynFunNew @_obj_gcls 	push "object_get_class" envAdd
	dynFunNew @_obj_smbr 	push "object_set_member" envAdd
	dynFunNew @_obj_gmbr 	push "object_get_member" envAdd
	dynFunNew @_cls_sbase 	push "class_set_base" envAdd
	dynFunNew @_cls_gbase 	push "class_get_base" envAdd
end


# Définition de la classe object :
# Il s'agit d'un fac-simile de map, doté d'un membre 'clone'
# (oui, c'est du simili-orienté-prototype)

asm
	jmp @_skip_obj_defs

_obj_mapSet:
	swap 3			# argc <-> obj
	_vobj_gmbr 0		# obj -> map
	swap 3			# map <-> argc
	pop			# discard argc
	mapSet			
	ret 0

_obj_mapGet:
	swap 2			# argc <-> obj
	_vobj_gmbr 0		# obj -> map
	swap 2			# map <-> argc
	pop			# discard argc
	mapGet			
	ret 0

_vobj_new_hook:
	dup -1 mapGet "clone"
	call 	# using call convention with argc, expecting raw return convention
	ret 0


_skip_obj_defs:
	push 0 _vcls_new -$object pop
	+$object push "Object" _vcls_sname
	+$object mapNew _vobj_smbr 0
	+$object push "mapSet" envGet &OpcodeNoArg dynFunNew @_obj_mapSet _vcls_soo
	+$object push "mapSet" envGet &OpcodeArgString dynFunNew @_obj_mapSet _vcls_soo
	+$object push "mapGet" envGet &OpcodeNoArg dynFunNew @_obj_mapGet _vcls_soo
	+$object push "mapGet" envGet &OpcodeArgString dynFunNew @_obj_mapGet _vcls_soo
	+$object push "_vobj_new" envGet &OpcodeNoArg dynFunNew @_vobj_new_hook _vcls_soo
	+$object +$object _vobj_scls
	#+$object push "clone" envGet &OpcodeNoArg dynFunNew @_obj_clone_hook _vcls_soo
	#+$object dynFunNew @_obj_clone mapSet "clone"
	+$object push "TEST\n" push "toto" mapSet
	+$object push "Object" envAdd
end



















# opérations sur une instance : (définies dans sa classe)
# - get_member(obj, name)                mapGet
# - set_member(obj, name, value)         mapSet
# - init(...)

# opérations sur une classe : (définies dans sa métaclasse)
# - define_member(cls, name, getter, setter)   définir un membre des INSTANCES
# OU
# - declare_member(cls, name)                addSym
# - define_member_getter(cls, name, getter)  ?
# - define_member_setter(cls, name, setter)  ?

struc Member      { magic name getter setter }
struc TypedMember { magic name getter setter type }

func new_member(name, getter, setter)
	strucNew Member {
		magic: push 1
		name: +$name
		getter: +$getter
	}
endfunc

func new_typedmember(name, getter, setter, type)
	strucNew TypedMember {
		magic: push 1
		name: +$name
		getter: +$getter
		type: +$type
	}
endfunc
	

func new_raw_class(name)
	local cls {
		push 0 _vcls_new -$cls pop
		+$cls newSymTab _vobj_smbr 0
		+$cls _vobj_gmbr 0 push "members" addSym
		#+$cls push "addSym" envGet &OpcodeNoArg +$oo_class_add_member _vcls_soo
		#+$cls push "findSym" envGet &OpcodeNoArg +$oo_class_find_member _vcls_soo
	}
endfunc

func class_add_member(cls, member)
	local name {
		+$member +(Member.name) -$name
		+$cls +$name addSym
		+$cls +$cls +$name getSym +$member _vobj_smbr
	}
endfunc
