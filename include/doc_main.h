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
 * <ul>
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
 * \section sec_ml Meta Assembly Language
 * You should be familiar with tinyap's language description grammar and features such as grammar plugins before you read further.
 *
 * Tinyaml's grammar ml/ml_core.gram
 *
 * Several plugs are defined :
 * - p_Opcode : can define new opcode syntax here.
 * - p_Opcode_* : used by compiler when defining new opcodes, please don't use.
 * - p_Code : can define new types of code blocs here.
 * - p_ProgramTopLevel : can define new top-level statements (data, code, walker, language...)
 * - p_Data : can define new data declarations here.
 * - p_libStatement : can define new library statements here (might not be useful).
 * - _start : the entry point can also be used to shunt everything else
 *
 * \section sec_ll Binding new libraries
 * - writing library file
 * - writing opcodes in C
 * - building the extension
 * - using the extension
 * \section sec_hl Creation of higher-level languages
 * - language
 * - plug
 * - compile
 * - walker
 * \section sec_op Core library
 * \section sec_ext Bundled extensions
 * 	\subsection ext_rtc Real-Time Clock (GNU/Linux)
 * - Purpose : allow real-time sequencing of tasks and blocking i/o.
 * - Opcodes :
 *   - TODO
 * 	\subsection ext_msg Message Queues
 * - Purpose : implement message queues as FIFOs with single writer and many readers, with blocking reads.
 * - Opcodes :
 *   - TODO
 * \endsection
 *
 *
 */
