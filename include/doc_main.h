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


/*! \mainpage This is not yet another Meta-Language
 *
 * \section sec_cts Contents
 *
 * <div style="background-color:#E8E8E8; border:solid 1px #808080; width:300px; padding:6px;">
 * \ref sec_i <br>
 * \ref sec_bi <br>
 * \ref sec_u <br>
 * \ref seq_tut <br>
 * \ref sec_vm <br>
 * \ref sec_ml <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\ref ml_asm <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\ref ml_ml <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\ref ml_cpl <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\ref ml_wlk <br>
 * \ref sec_ll <br>
 * \ref sec_op <br>
 * \ref sec_ext <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\ref ext_rtc <br>
 * &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\ref ext_msg <br>
 * </div>
 *
 * \section sec_i Introduction
 *
 * Tinyaml is a Virtual Machine, a Compiler, and a Compiler-Compiler, all in one, powered
 * by the abstract parser <a href="http://code.google.com/p/tinyap/">tinyap</a>.
 *
 * - What is it not ?
 *   - yet another meta-language.
 *   - a SNUSP compiler/interpreter.
 *   - real-time. It is quite responsive, but no latency is guaranteed. It may be nice to make it real-time one day.
 *   - feature-rich. It currently comes with only core opcodes and a few short extensions. This is subject to change with time.
 * - What is it ?
 *   - a hopefully convenient environment for superposing levels of abstraction without wasting too much resources
 *   - a C library to run the VM in your own applications.
 *   - a command-line main binary (tinyaml)
 *   - a debugger running with ncurses (tinyaml_dbg) \note the debugger currently lacks MANY features
 * - What can it do ?
 *   - compile and execute programs from command line,
 *   - dynamically plug new variants (languages) into the parser's grammar,
 *   - compile, and execute as needed, methods to compile resulting AST nodes.
 *
 * Tinyaml defines a \ref sec_ml for tinyap to parse. The ASTs that result
 * from successful parses can be compiled (i.e. code/data is output to a new program) and
 * walked by user programs.
 *
 * One goal with tinyaml is to define some kind of high-level scripting language to
 * define and handle a GUI, and more globally, the whole application behaviour. The two
 * current extensions are a first step to define the message bus. Bindings of 2D/3D and
 * event APIs will come later. The purpose of tinyaml is to remain generic and open to
 * all uses, so such extensions will probably come apart.
 *
 * \section sec_bi Build and Install
 * <p>First, get the source code.</p>
 * \note You need to build and install <a href="http://code.google.com/p/tinyap/">tinyap</a> beforehand.
 *
 * <ul>
 * <li><b>tarball</b> : ( version 0.2 is used in commands below, or get latest tarball at http://code.google.com/p/tinyaml/downloads/list ).
 * \code
 * $ wget http://tinyaml.googlecode.com/files/tinyaml-0.2.tar.gz
 * $ tar xzf tinyaml-0.2.tar.gz
 * \endcode
 * <li><b>using SVN</b> :
 * \code
 * $ svn checkout http://tinyaml.googlecode.com/svn/trunk/ tinyaml-read-only
 * \endcode
 * </ul>
 * <p>Now you have your tinyaml source distribution at hands, build it.</p>
 * \code
 * $ cd tinyaml
 * $ CFLAGS=-O3 ./configure -C --prefix=/my/install/prefix/if/not/slash/usr
 * $ make all
 * $ make install
 * \endcode
 * You might need root privileges to run make install.
 * \li 
 *
 * \section sec_u Usage
 * In both \c tinyaml and \c tinyaml_dbg, commands are executed on the fly, left-to-right.
 *
 * - tinyaml :
 * \code --compile,-c [filename] \endcode compile this file
 * \code --save,-s [filename] \endcode    save the newest program into this file
 * \code --load,-l [filename] \endcode    load a serialized program from this file
 * \code --run-foreground,-f \endcode     run the newest program in foreground
 * \code --run-background,-b \endcode     run the newest program in background
 * \code --trace,-t \endcode              start tracing execution cycles
 * \code --no-trace,-nt \endcode          stop tracing execution cycles
 * \code --version,-v \endcode            display program version
 * \code --help,-h \endcode               display usage
 *
 * - tinyaml_dbg :
 * \code --compile,-c [filename] \endcode compile this file
 * \code --load,-l [filename] \endcode    load a serialized program from this file
 * \code --run,-f \endcode		  run the newest program
 * \code --debug,-d \endcode		  run the newest program in the debugger
 * \code --version,-v \endcode            display program version
 * \code --help,-h \endcode               display usage
 *
 * \section seq_tut Quick tests
 * Jump into the /tests/ directory.
 *
 * Start by compiling the bundled script compiler.
 * \code $ tinyaml -c script_typeless.melang -s script.compiler \endcode
 *
 * You can now compile quickly and execute any of the files in the directory (except Makefile*).
 * \code $ tinyaml -l script.compiler -f -c test.script -f \endcode
 * Oops ! Time to discover the (wannabe) debugger.
 * \code $ tinyaml_dbg -l script.compiler -f -c test.script -d \endcode
 * If you don't like pressing repetedly 's', just press 'r' and watch it run until it fails.
 *
 * Now using an extension :
 * \code $ tinyaml -c ../extension/RTC/RTC.lib -f -c rtc.asm -f \endcode
 *
 * \section sec_vm Virtual Machine internals
 *
 * Processing inside the VM is based on 32bit words.
 * The VM processes exactly one instruction per execution cycle. An instruction is a two-word compound containing an opcode and a 32bit argument. An opcode is defined by its mnemonic, its argument type, and the corresponding C function.
 *
 * Argument types are :
 * - \b Int : 32-bit signed integer value
 * - \b Float : 32-bit floating point value
 * - \b String : a static string (owned by the program that mentions the string)
 * - \b Label : signed integer value representing the relative offset to jump to in the same code segment (inter-segment calls can be achieved by using function objects, not direct jumps).
 * - \b EnvSym : a symbol global to the VM, dynamically resolved on program loading/compilation.
 * - \b none : yes, opcodes can have no argument also.
 *
 * Instructions are assembled in one code segment per \ref vm_prgs "program". At runtime, the code segment contains direct pointers to the C routines associated to the opcodes, so the processing overhead is minimized.
 *
 * Data in the VM is also represented by a two-word compound, containing the \ref vm_data_type_t "data type" and the actual data word.
 *
 * Now, how do programs fit in the VM ?
 *
 * \ref Threads define the execution context for a program.
 * They provide an instruction pointer, a call stack, a data stack, and a little more.
 * Threads are prioritized using an integer value.
 * The default priority value is 50, the lowest is 0, and the highest is is 99.
 * Actually these values are not bounded, so 2^31-1 is the highest, and 2^31 is the lowest.
 * The Virtual Machine schedules threads based on priority and a configurable timeslice that defaults to 100 instructions.
 *
 * \section sec_ml Meta Assembly Language
 * You should be familiar with tinyap's language description grammar and features such as grammar plugins before you read further.
 *
 * The grammar consists mainly of the base assembly language itself,
 * sections to define new grammars, plug them, and compile them.
 *
 * Comments start with a \c # and stop at the next end-of-line character. They are treated like whitespace.
 *
 * The parser accepts whitespace anywhere between tokens.
 *
 * \subsection ml_asm Assembly language
 *
 * An instruction may be preceded by a label declaration. \code my_label : \endcode
 * An instruction is the opcode mnemonic optionally followed by its argument.
 * - \b Int : \code Syntax : decimal integer \endcode
 * - \b Float : \code Syntax : decimal dotted floating point \endcode
 * - \b String : \code Syntax : "standard string" \endcode \note Due to the way strings are implemented in the grammar, whitespace at the beginning of a string must be escaped with a single \ as in \code "\   this strings starts with blanks." \endcode
 * - \b Label : \code
Syntax : @my_label
         +2
         -42 \endcode
 * - \b EnvSym : \code Syntax : &some_symbol \endcode
 * - \b none : yes, opcodes can have no argument also.
 *
 * For instance, this code will output "Hello, World." to screen, and demonstrates the use of Int, String and Label arguments. \code
 * asm
 *     push "Hello, world.\n" print 1	# the parser doesn't care about indentation and instructions per line
 *     jmp @skippy			# we don't want execute the nops..
 *     nop nop nop
 * skippy:				# a ret instruction is always appended to the end of a code segment
 *     ret 0				# after compilation, but the label declaration must be followed by
 * end					# an instruction.                                                        			
 * \endcode
 *
 * Data sections are enclosed between \c data and \c end keywords.
 * A data declaration is an initializer optionally followed by the keyword \c rep and the number of repetitions of this initializer.
 * Data types \c String, \c Int, and \c Float can be used as initializers.
 * For instance, the following declaration :
 * \code data
 *   0
 *   23.42 rep 2
 *   "Wibble" rep 1
 * end \endcode
 * will result in the following data segment :
 * \code
 * Offset |    0    |    1    |    2    |    3     |
 * Data   |    0    |  23.42  |  23.42  | "Wibble" |
 * \endcode
 *
 * \subsection ml_ml Meta-language
 *
 * The grammar definitions to append to the current grammar are defined in sections enclosed by the keywords \c language and \c end.
 * Tinyaml uses tinyap's language description grammar.
 *
 * The following grammar defines a rule \c foobar that recognizes "Hello, world".
 * \code
 * language
 *     Hello ::= "hello".
 *     World = /\<world\>/.			# don't do this at home !
 *     foobar ::= <Hello> "," <World>.
 *  end
 *  \endcode
 *
 *  This rule must be plugged into the grammar to be effective. This is done via a \c plug \c into statement. Several plugs are defined :
 * - p_Opcode : can define new opcode syntax here.
 * - p_Opcode_* : used by compiler when defining new opcodes, please don't use.
 * - p_Code : can define new types of code blocs here.
 * - p_ProgramTopLevel : can define new top-level statements (data, code, walker, language...)
 * - p_Data : can define new data declarations here.
 * - p_libStatement : can define new library statements here (might not be useful).
 * - _start : the entry point can also be used to shunt everything else
 *
 * Since this is a quick and dirty example, we won't bother and plug \c foobar at the top-most level, right into \c _start.
 * \code plug foobar into _start \endcode
 *
 * Once the new rule is plugged, the parser is ready to produce it.
 * \note Since compilation is done <b>after</b> parsing, the rule will be plugged <b>after</b> the whole file is parsed, so you can't use a rule in the same file it is defined in.
 *
 * For details, you can have a look at the whole grammar in file \c ml/ml_core.gram.
 *
 * \subsection ml_cpl Declaring new compile methods
 *
 * \see Compiler
 *
 * Compile methods are defined with \c compile statements. A \c compile statement associates a bloc of code (plug rule \c p_Code) with an AST node label. This code will be executed each time the compiler walks on a node with this label (to know more about AST node labels, refer to tinyap's AST creation). Actually, when it walks on node labeled "foobar", the compiler will look for compile method name &ldquo; <em> _internal_prefix_ </em> foobar &rdquo;.
 * The following code will "pretty-print" the foobar node and write instructions to display "hi there".
 * \code
 * compile foobar asm
 *     pp_curNode		# prettyprint current node
 *     push "hi there."
 *     write_oc_String "push"	# write opcode << push "hi there." >> to output program
 *     compileStateNext		# all done, OK, go on.
 * end
 * \endcode
 *
 * \subsection ml_wlk Defining new AST Walkers
 *
 * Static analysis is not possible with compile methods, or would be seriously awkward, but tinyaml allows the definition of other AST walkers.
 * A walker is invoked to evaluate a given node, and returns the result of its evaluation.
 * It must provide an init method, a termination method, and a default method to visit unknown nodes.
 * Other methods in the walker are declared by the \c on keyword, and will behave like compile methods, i.e. the method \c Twist will be invoked to process the AST node named \c Twist.
 *
 * Short example :
 * \code
 * walker Toto
 *
 *   init  asm   push "Toto::init\n"  print 1   end
 *
 *   term  asm   push "Toto::term\n"  print 1   end
 *
 *   default
 *   asm
 *   	push "Unknown node or generic behaviour : "
 *   	astGetOp
 *   	push "\n"
 *   	print 3
 *   	compileStateNext		# we are not in the compiler, but the walking
 *   end					# mechanism remains the same.
 *
 *   on foobar
 *   asm
 *     pp_curNode
 *     push "We are on node foobar which has "
 *     astGetChildrenCount
 *     push "\ children.\n"
 *     print 3
 *     compileStateDone			# our foobar node is top-level node, so we have finished after processing it.
 *   end
 *
 * end
 * \endcode
 *
 * \section sec_ll Binding new libraries
 *
 * Two files are necessary to define a Tinyaml extension, a binary shared object containing the C opcode routines, and
 * a tinyaml source file starting with a \c lib section.
 *
 * The \c lib section contains \c file statements (generally one) and \c opcode statements.
 * \c file followed by a string opens the corresponding shared object. Currently on *NIX it
 * is $pkglibdir/libtinyaml_ <em>string</em>.so. The \c opcode keyword is followed by the
 * opcode mnemonic without quotes. If the opcode has an argument, the mnemonic is followed
 * by a colon ":" and the argument type \c Int, \c Float, \c String, \c EnvSym, or \c Label.
 *
 * The argument word for \c EnvSym is the index of the symbol in the current environment, computed at compile/unserialize-time.
 *
 * The argument word for \c Label is the relative offset of the label, computed at compile/unserialize-time.
 *
 * When the tinyaml library file is compiled, the opcodes are added to the dictionary and their C functions are resolved. Tinyaml searches for a function named vm_op_MNEMONIC_ARGTYPE or vm_op_MNEMONIC if the opcode has no argument. Such a function must be of type compatible with \ref opcode_stub_t.
 *
 * Here is a quick foobar example :
 * - foobar.lib
 *   \code
 *   lib
 *   	file "foobar"
 *   	opcode foo
 *   	opcode bar:Int
 *   end
 *   \endcode
 * - foobar.c
 *   \code
 * #include <tinyaml/vm.h>
 * #include <stdio.h>
 * void _VM_CALL vm_op_foo(vm_t vm, word_t unused) {
 * 	printf("foo\n");
 * }
 *
 * void _VM_CALL vm_op_bar_Int(vm_t vm, long myArg) {
 * 	printf("pressure : %i mbar\n", myArg);
 * }
 * \endcode
 * - compile and install the extension (quick and dirty way)
 *   \code
 * $ gcc -shared -Wall foobar.c -ltinyaml -o libtinyaml_foobar.so
 * $ sudo cp !$ /usr/local/lib/tinyaml/
 * \endcode
 * - foobar.asm
 *   \code asm foo bar 42 end \endcode
 * - running tinyaml to test it out (provided foobar.c has been compiled and installed)
 *   \code
 * $ tinyaml -q -c foobar.lib -c foobar.asm -f
 * foo
 * pressure : 42 mbar
 * \endcode
 *
 *
 *
 * \section sec_op Core library
 * Tinyaml comes with a small set of opcodes to handle the basic data and containers it defines, and to perform
 * arithmetic boolean and bitwise operations. All the opcodes are listed in the module \ref vm_core_ops.
 *
 * \see The corresponding library file is ml/ml_core.lib
 *
 * \section sec_ext Bundled extensions
 * 	\subsection ext_rtc Real-Time Clock (GNU/Linux)
 * - Purpose : allow real-time sequencing of tasks and blocking waits.
 * - Opcodes :
 *   - TODO
 * 	\subsection ext_msg Message Queues
 * - Purpose : implement message queues as FIFOs with single writer and many readers, with blocking reads.
 * - Opcodes :
 *   - TODO
 *
 *
 */
