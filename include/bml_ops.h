/* TinyaML
 * Copyright (C) 2007 Damien Leroux
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _BML_OPS_H_
#define _BML_OPS_H_

/*! \addtogroup vm_core_ops
 * @{
 * \todo FINISH THIS DOC.
 */

/*! \brief do NOTHING. */
void _VM_CALL vm_op_nop(vm_t vm, word_t data);
/*@}*/


/*! \addtogroup vcop_comp
 * @{
 */
/*! \brief define a new AST compiler vector (\c data is the offset in current program's code and the vector name is popped from the stack). */
/* FIXME Why is it in .h ? */
void _VM_CALL vm_op___addCompileMethod_Label(vm_t vm, word_t data);

/*@}*/

#endif

