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

#include <curses.h>
#include <string.h>
#include <pthread.h>

#include "vm.h"
#include "fastmath.h"
#include "_impl.h"
#include "vm_engine.h"
#include "object.h"
#include "program.h"

/*! \addtogroup tinyaml_dbg Debugger
 * @{
 */

/*! \addtogroup dbg_internals
 * @{
 */

struct _std_io_to_buf {
	FILE*f;
	pthread_t filler;
	long ateol;
	struct _dynarray_t lines;
};

struct _std_io_to_buf StdOut, StdErr;


void dbg_out(const char*str) {
	char*tmp = strdup(str);
	char*tok = strtok(tmp, "\r\n");
	fputs(str,StdOut.f);
	while(tok) {
		if(StdOut.ateol) {
			dynarray_set(&StdOut.lines,StdOut.lines.size,(word_t)strdup(tok));
		} else {
			char*old = (char*)dynarray_get(&StdOut.lines,StdOut.lines.size-1);
			char*new = (char*)malloc(strlen(old)+strlen(str)+1);
			strcpy(new,old);
			strcat(new,str);
			dynarray_set(&StdOut.lines,StdOut.lines.size-1,new);
			free(old);
		}
		StdOut.ateol=1;
		tok=strtok(NULL,"\r\n");
	}
	if(*str) {
		tok=str;
		while(*(tok+1)) { tok+=1; }
	} else {
		tok="";
	}
	StdOut.ateol=(*tok=='\r'||*tok=='\n');
	free(tmp);
}

void dbg_err(const char*str) {
	char*tmp = strdup(str);
	char*tok = strtok(tmp, "\r\n");
	fputs(str,StdErr.f);
	while(tok) {
		dynarray_set(&StdErr.lines,StdErr.lines.size,(word_t)strdup(tok));
		tok=strtok(NULL,"\r\n");
	}
	free(tmp);
}


void init_outerr() {
	dynarray_init(&StdOut.lines);
	dynarray_init(&StdErr.lines);
	StdOut.ateol=1;
	StdErr.ateol=1;
	StdOut.f = fopen("tinyaml_dbg.stdout","w");
	StdErr.f = fopen("tinyaml_dbg.stderr","w");
}

void term_outerr() {
	long i;

	for(i=0;i<StdOut.lines.size;i+=1) {
		free((char*)StdOut.lines.data[i]);
	}
	dynarray_deinit(&StdOut.lines,NULL);
	fclose(StdOut.f);

	for(i=0;i<StdErr.lines.size;i+=1) {
		free((char*)StdErr.lines.data[i]);
	}
	dynarray_deinit(&StdErr.lines,NULL);
	fclose(StdErr.f);
}



void lookup_label_and_ofs(program_t cs, word_t ip, const char** label, word_t* ofs);

static void dbg_data_stack_renderer(WINDOW* w,vm_data_t d) {
	_IFC conv;
	/*long i;*/
	/*const unsigned char*s;*/
	switch(d->type) {
	case DataInt:
		wprintw(w,"Int     %li",d->data);
		break;
	case DataFloat:
		conv.i = d->data;
		wprintw(w,"Float   %f",conv.f);
		break;
	case DataString:
		wprintw(w,"String  \"%s\"",(const char*)d->data);
		break;
	case DataObjStr:
		wprintw(w,"ObjStr  \"%s\"[%lu]",(const char*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	case DataObjSymTab:
		wprintw(w,"SymTab  %p[%lu]",(void*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	case DataObjMutex:
		wprintw(w,"Mutex  %p[%lu]",(void*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	case DataObjThread:
		wprintw(w,"Thread  %p[%lu]",(void*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	case DataObjArray:
		wprintw(w,"Array  %p[%lu]",(void*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	case DataObjEnv:
		wprintw(w,"Map  %p[%lu]",(void*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	case DataObjStack:
		wprintw(w,"Stack  %p[%lu]",(void*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	case DataObjFun:
		wprintw(w,"Function  %p[%lu]",(void*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	case DataObjVObj:
		wprintw(w,"V-Obj  %p[%lu]",(void*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	case DataManagedObjectFlag:
		wprintw(w,"Undefined Object ! %p[%lu]",(void*)d->data,vm_obj_refcount_ptr((void*)d->data));
		break;
	default:
		wprintw(w, "Unknown (%u, 0x%lX)",d->type,d->data);
	};
}

static void dbg_closure_stack_renderer(WINDOW* w,dynarray_t* da) {
	vm_data_t tab = (vm_data_t) (*da)->data;
	word_t i;
	wprintw(w,"[%lu]",(*da)->size);
	for(i=0;i<(*da)->size;i+=1) {
		wprintw(w,"\n    %li : ",i);
		dbg_data_stack_renderer(w,&(tab[i]));
	}
}

static void dbg_call_stack_renderer(WINDOW* w,struct _call_stack_entry_t* cse) {
	word_t ofs;
	const char*label;
	lookup_label_and_ofs(cse->cs,cse->ip,&label,&ofs);
	if(label) {
		wprintw(w,"%p:%lx\n     (%s+%lu)",cse->ip, cse->cs, label, ofs);
	} else {
		wprintw(w,"%p:%lu",cse->cs,cse->ip);
	}
}


static void dbg_catch_stack_renderer(WINDOW* w,struct _call_stack_entry_t* cse) {
	word_t ofs;
	const char*label;
	lookup_label_and_ofs(cse->cs,cse->ip,&label,&ofs);
	if(label) {
		wprintw(w,"%p:%lx [%li]\n     (%s+%lu)",cse->ip, cse->cs, (long)cse->has_closure, label, ofs);
	} else {
		wprintw(w,"%p:%lu [%li]",cse->cs, cse->ip, (long)cse->has_closure);
	}
}


static void dbg_render_stack(WINDOW* w, generic_stack_t s, word_t skip, word_t count, const char*prefix, void(*renderer)(WINDOW*,void*), long disp_start, long disp_incr) {
	long counter = -skip;
	long stop = -(skip+count);
	while(counter>stop) {
		wprintw(w,"%s%li : ",prefix,disp_start);
		disp_start+=disp_incr;
		renderer(w,_gpeek(s,counter));
		wprintw(w,"\n");
		counter-=1;
	}
}

/*@}*/


/**************************************************************************
 *	    _   _  ____ _   _ ____  ____  _____ ____  
 *	   | \ | |/ ___| | | |  _ \/ ___|| ____/ ___| 
 *	   |  \| | |   | | | | |_) \___ \|  _| \___ \ 
 *	   | |\  | |___| |_| |  _ < ___) | |___ ___) |
 *	   |_| \_|\____|\___/|_| \_\____/|_____|____/ 
 *
 *************************************************************************/

/*! \addtogroup dbg_ncurses NCurses
 * @{
 */

typedef struct _window_t* window_t;


struct _window_t {
	WINDOW* w;
	WINDOW* cliw;
	void (*renderer)(WINDOW*,long,vm_t,thread_t);
	long vofs;
};


enum {
	CodeWin=0,
	DataSeg,
	DataStack,
	LocalsStack,
	ClosureStack,
	CallStack,
	CatchStack,
	StdOutWin,
	StdErrWin,

	N_WINDOWS,

	Popup
};

struct _window_t windows[N_WINDOWS];

long cur_win=0;


void create_window(window_t ww, void (*r)(WINDOW*,long,vm_t,thread_t), long x, long y, long W, long H) {
	ww->w = subwin(stdscr, H, W, y, x);
	ww->cliw = subwin(ww->w, H-2, W-2, y+1, x+1);
	ww->renderer = r;
}

void destroy_window(window_t ww) {
	delwin(ww->cliw);
	delwin(ww->w);
}


void print_centered(WINDOW* w, long is_cur, long y, const char* str) {
	if(is_cur) {
		wattron(w,A_STANDOUT|A_UNDERLINE|A_BOLD);
	}
	mvwprintw(w,y,(getmaxx(w)-strlen(str))>>1, str);
	if(is_cur) {
		wattroff(w,A_STANDOUT|A_UNDERLINE|A_BOLD);
	}
}


void render_win(window_t w, const char* title, vm_t vm, thread_t t) {
	wclear(w->cliw);
	w->renderer(w->cliw,w->vofs,vm,t);
	box(w->w,0,0);
	print_centered(w->w,w==&windows[cur_win],0,title);
}

/*long window_vofs[N_WINDOWS] = { 0,0,0,0,0,0,0, };*/

/*WINDOW* call_stack;*/
/*WINDOW* data_stack;*/
/*WINDOW* data_seg;*/
/*WINDOW* locals_stack;*/
/*WINDOW* closure_stack;*/
/*WINDOW* catch_stack;*/

/*WINDOW* code;*/

#define ST_W ((getmaxx(stdscr)-1)/5)
#define ST_H ((getmaxy(stdscr)-1)/3)

/*WINDOW** windows[N_WINDOWS] = {*/
	/*&code,*/
	/*&call_stack,*/
	/*&data_stack,*/
	/*&locals_stack,*/
	/*&closure_stack,*/
	/*&data_seg,*/
	/*&catch_stack,*/
/*};*/

#define STATE_IDLE 0
#define STATE_RUNNING 1
#define STATE_STEPPING 2
#define STATE_DEAD 3

long state=STATE_IDLE;

void do_scroll_down() {
	windows[cur_win].vofs+=1+(!cur_win);
	state=STATE_IDLE;
}

void do_scroll_up() {
	if(windows[cur_win].vofs==0) {
		return;
	}
	windows[cur_win].vofs-=1+(!cur_win);
	state=STATE_IDLE;
}

void do_step() {
	if(state==STATE_IDLE) {
		state=STATE_STEPPING;
	}
}

void do_run() {
	if(state==STATE_IDLE) {
		state=STATE_RUNNING;
	}
}

void do_cycle_win() {
	cur_win=(cur_win+1)%N_WINDOWS;
	/*vm_printerrf("cur_win is now %i\n",cur_win);*/
}

void term();

void do_quit() {
	term();
	exit(0);
}

void do_help();
void do_go();
void do_breakpoints();


typedef struct _command_t {
	long ch;
	const char* descr;
	void (*command)();
} command_t;

command_t commands_all[] = {
	{ 'h', "display this popup", do_help },
	{ KEY_DOWN, "scroll down by one line", do_scroll_down },
	{ KEY_UP, "scroll up by one line", do_scroll_up },
	{ 'm', "scroll down by one line", do_scroll_down },
	{ 'p', "scroll up by one line", do_scroll_up },
	{ '\t', "cycle through windows", do_cycle_win },
	{ 'q', "quit", do_quit },
};

command_t commands[N_WINDOWS][32] = {
/* code window */ {
	{ 's', "execute next opcode", do_step },
	{ 'r', "run this thread", do_run },
	{ 'g', "go to offset in code", do_go },
	{ 'b', "breakpoints", do_breakpoints },
{ 0, NULL, NULL}}, /* data_seg */ {
{ 0, NULL, NULL}}, /* call_stack */ {
{ 0, NULL, NULL}}, /* data_stack */ {
{ 0, NULL, NULL}}, /* locals_stack */ {
{ 0, NULL, NULL}}, /* closure_stack */ {
{ 0, NULL, NULL}}, /* catch_stack */ {
{ 0, NULL, NULL}}, /* stdout */ {
{ 0, NULL, NULL}}, /* stderr */ {
{ 0, NULL, NULL},
}};


void read_command(vm_t vm, thread_t t);

WINDOW* open_popup(long h, long w) {
	return subwin(stdscr, h, w, (getmaxy(stdscr)-h)>>1, (getmaxx(stdscr)-w)>>1);
}


/* FIXME : unchecked dangerous input. */
void input_value(const char* prompt, long len, const char* fmt, void* dest) {
	long h=3, w=5+strlen(prompt)+len;
	WINDOW* popup = open_popup(h, w);
	mvwprintw(popup,1,1,"%s : ",prompt);
	box(popup,0,0);
	touchwin(stdscr);
	wrefresh(popup);
	refresh();
	echo();
	/*noraw();*/
	/*timeout(-1);*/
	nodelay(stdscr,0);
	curs_set(1);
	/*scanw(fmt,dest);*/
	/*mvwscanw(stdscr, (getmaxy(stdscr)-h)>>1, ((getmaxx(stdscr)-w)>>1)+4+strlen(prompt),fmt,dest);*/
	mvwscanw(popup, 1, 4+strlen(prompt),fmt,dest);
	curs_set(0);
	nodelay(stdscr,1);
	/*timeout(0);*/
	noecho();
	delwin(popup);
}

void do_go() {
	word_t ip;
	input_value("Go to offset",6,"%X",&ip);
	windows[CodeWin].vofs = ip&(~1);
}

void do_breakpoints() {
	WINDOW* popup = open_popup(3, 21);
	wprintw(popup,"\n  Not implemented ! ");
	box(popup,0,0);
	touchwin(stdscr);
	wrefresh(popup);
	refresh();
	/*wrefresh(popup);*/
	getch();
	delwin(popup);
}

void do_help() {
	long sz=0,tmp=0;
	WINDOW* popup;
	while(commands_all[tmp].ch) { tmp+=1; }
	while(commands[cur_win][sz].ch) { sz+=1; }
	sz+=tmp+1+(commands[cur_win][0].ch!=0);
	popup = open_popup(sz+2,52);
	wprintw(popup,"\n");
	if(commands[cur_win][0].ch) {
		wprintw(popup,"\n");
		sz=0;
		while(commands[cur_win][sz].ch) {
			switch(commands[cur_win][sz].ch) {
			case KEY_UP:
				wprintw(popup," \t<UP>\t%s\n",commands[cur_win][sz].descr);
				break;
			case KEY_DOWN:
				wprintw(popup," \t<DOWN>\t%s\n",commands[cur_win][sz].descr);
				break;
			case '\t':
				wprintw(popup," \t<TAB>\t%s\n",commands[cur_win][sz].descr);
				break;
			default:;
				wprintw(popup," \t'%c'\t%s\n",commands[cur_win][sz].ch,commands[cur_win][sz].descr);
			};
			sz+=1;
		}
		tmp=sz+2;
	} else {
		tmp=1;
	}
	
	wprintw(popup,"\n");
	sz=0;
	while(commands_all[sz].ch) {
		switch(commands_all[sz].ch) {
		case KEY_UP:
			wprintw(popup," \t<UP>\t%s\n",commands_all[sz].descr);
			break;
		case KEY_DOWN:
			wprintw(popup," \t<DOWN>\t%s\n",commands_all[sz].descr);
			break;
		case '\t':
			wprintw(popup," \t<TAB>\t%s\n",commands_all[sz].descr);
			break;
		default:;
			wprintw(popup," \t'%c'\t%s\n",commands_all[sz].ch,commands_all[sz].descr);
		};
		sz+=1;
	}
	if(tmp!=1) {
		print_centered(popup,1,1,"-- Specific commands --");
	}
	print_centered(popup,1,tmp,"-- General commands --");
	box(popup,0,0);
	print_centered(popup,1,0,"[ Help ]");
	touchwin(stdscr);
	wnoutrefresh(stdscr);
	doupdate();
	getch();
	delwin(popup);
}


word_t calc_count(long n, word_t sz) {
	word_t skip = windows[n].vofs;
	word_t count = sz-skip;
	if(count>getmaxy(windows[n].cliw)) {
		count = getmaxy(windows[n].cliw);
	}
	return count;
}

static void repaint(vm_t vm, thread_t t, long do_command) {
	/*word_t ofs;*/
	const char*label;
	const char*disasm;
	word_t code_start, code_end,i,j;
	/*word_t skip,count;*/
	char title[50];

	clear();

	if(!(vm&&t)) {
		long i;
		for(i=0;i<N_WINDOWS;i+=1) {
			box(windows[i].w,0,0);
		}
		print_centered(windows->w,!cur_win,0," No running thread ");

		wnoutrefresh(stdscr);
		doupdate();

		read_command(vm,t);
		
		return;
	}

	/* fill code window */
	sprintf(title,"[ %s Thread %p CS:%p IP:%lx ]",
		state==STATE_DEAD?"Dying":"Running",
		t,t->program,t->IP);
	render_win(&windows[CodeWin], title, vm, t);


	/* render call stack */

	sprintf(title," Call stack [%lu] ", gstack_size(&t->call_stack));
	render_win(&windows[CallStack], title, vm, t);

	sprintf(title," Catch stack [%lu] ", gstack_size(&t->catch_stack));
	render_win(&windows[CatchStack], title, vm, t);

	sprintf(title," Data stack [%lu] ", gstack_size(&t->data_stack));
	render_win(&windows[DataStack], title, vm, t);

	sprintf(title," Closures stack [%lu] ", gstack_size(&t->closures_stack));
	render_win(&windows[ClosureStack], title, vm, t);

	sprintf(title," Locals stack [%lu] ", gstack_size(&t->closures_stack));
	render_win(&windows[LocalsStack], title, vm, t);

	sprintf(title," Data segment [%lu] ", t->program->data.size>>1);
	render_win(&windows[DataSeg], title, vm, t);

	sprintf(title," Standard Output [%lu] ", StdOut.lines.size);
	render_win(&windows[StdOutWin], title, vm, t);

	sprintf(title," Error Output [%lu] ", StdErr.lines.size);
	render_win(&windows[StdErrWin], title, vm, t);


	wnoutrefresh(stdscr);
	doupdate();

	if(do_command) {
		read_command(vm,t);
	}
}




void read_command(vm_t vm, thread_t t) {
	long c;
	long i;
	if(state==STATE_RUNNING) {
		return;
	}
	if(state==STATE_STEPPING) {
		state=STATE_IDLE;
	}
	while(state==STATE_IDLE||state==STATE_DEAD) {
		c = getch();
		i=0;
		while(commands[cur_win][i].ch && commands[cur_win][i].ch!=c) { i+=1; }
		if(commands[cur_win][i].ch==c) {
			commands[cur_win][i].command();
			repaint(vm,t,0);
		} else {
			i=0;
			while(commands_all[i].ch && commands_all[i].ch!=c) { i+=1; }
			if(commands_all[i].ch==c) {
				commands_all[i].command();
				repaint(vm,t,0);
			}
		}
	}
}


vm_t vm;

extern const vm_engine_t debug_engine;


void term() {
	long i;
	for(i=0;i<N_WINDOWS;i++) {
		destroy_window(&windows[i]);
	}
	delwin(stdscr);
	endwin();
	term_outerr();
	/*fputs("Thank you for using tinyaml.\n",stdout);*/
}



void render_code(WINDOW*w, long vofs, vm_t vm, thread_t t) {
	long i,j,code_start,code_end;
	char* disasm;
	const char*label;
	char title[1024];

	if(state!=STATE_IDLE) {
		windows[CodeWin].vofs=t->IP;
	}

	if(windows[CodeWin].vofs>getmaxy(w)) {
		code_start = (windows[CodeWin].vofs-getmaxy(w))&(~1);
	} else {
		code_start = 0;
	}

	if((long)code_start>(long)(t->program->code.size-2*getmaxy(w))) {
		code_end=t->program->code.size;
	} else {
		code_end=(code_start+2*getmaxy(w))&(~1);
	}

	wattroff(w,A_STANDOUT);

	j = getmaxx(w)-17;

	for(i=code_start;i<code_end;i+=2) {
		disasm = program_disassemble(vm,t->program,i);
		label = program_lookup_label(t->program,i);
		if(i==windows[CodeWin].vofs) {
			wattron(windows[CodeWin].w,A_STANDOUT);
		}
		if(label&&*label) {
			sprintf(title,"%s :",label);
			wprintw(windows[CodeWin].w,"\t%-*.*s\n%8.8lx  %s\t%-*.*s\n",j+8,j+8,title,i,i==t->IP?"=>":"",j,j,disasm);
		} else {
			wprintw(windows[CodeWin].w," %8.8lx  %s\t%-*.*s\n",i,i==t->IP?"=>":"",j,j,disasm);
		}
		if(i==windows[CodeWin].vofs) {
			wattroff(windows[CodeWin].w,A_STANDOUT);
		}
		free((char*)disasm);
	}
}


void render_callstack(WINDOW*w, long vofs, vm_t vm, thread_t t) {
	dbg_render_stack(w,&t->call_stack, vofs, calc_count(CallStack,gstack_size(&t->call_stack)), "  ", (void(*)(WINDOW*,void*)) dbg_call_stack_renderer, 0, 1);
}


void render_catchstack(WINDOW*w, long vofs, vm_t vm, thread_t t) {
	dbg_render_stack(w,&t->catch_stack, vofs, calc_count(CatchStack,gstack_size(&t->catch_stack)), "  ", (void(*)(WINDOW*,void*)) dbg_catch_stack_renderer, 0, 1);
}


void render_datastack(WINDOW*w, long vofs, vm_t vm, thread_t t) {
	dbg_render_stack(w,&t->data_stack, vofs, calc_count(DataStack,gstack_size(&t->data_stack)), "  ", (void(*)(WINDOW*,void*)) dbg_data_stack_renderer, 0, 1);
}


void render_localsstack(WINDOW*w, long vofs, vm_t vm, thread_t t) {
	dbg_render_stack(w,&t->locals_stack, vofs, calc_count(LocalsStack,gstack_size(&t->locals_stack)), "  ", (void(*)(WINDOW*,void*)) dbg_data_stack_renderer, -1, -1);
}


void render_closurestack(WINDOW*w, long vofs, vm_t vm, thread_t t) {
	dbg_render_stack(w,&t->closures_stack, vofs, calc_count(ClosureStack,gstack_size(&t->closures_stack)), "  ", (void(*)(WINDOW*,void*)) dbg_closure_stack_renderer, 0, 1);
}


void render_dataseg(WINDOW*w, long vofs, vm_t vm, thread_t t) {
	vm_data_t tab;
	long i, code_end =t->program->data.size>>1;
	if(code_end>getmaxy(w)+windows[DataSeg].vofs) {
		code_end=getmaxy(w)+windows[DataSeg].vofs;
	}
	tab = (vm_data_t) t->program->data.data;
	for(i=windows[DataSeg].vofs;i<code_end;i+=1) {
		wprintw(w,"\n  %li : ",i);
		dbg_data_stack_renderer(w,&(tab[i]));
	}
}


void render_stdout(WINDOW*w, long vofs, vm_t vm, thread_t t) {
	long i,max;
	wclear(w);
	max=vofs+getmaxy(w);
	if(max>StdOut.lines.size) {
		vofs=StdOut.lines.size-getmaxy(w);
		max = StdOut.lines.size;
	}
	if(vofs<0) {
		vofs=0;
	}
	windows[StdOutWin].vofs=vofs;
	for(i=0;i<max;i+=1) {
		wprintw(w,"%s\n",(const char*)StdOut.lines.data[vofs]);
		vofs+=1;
	}
}

void render_stderr(WINDOW*w, long vofs, vm_t vm, thread_t t) {
	long i,max;
	wclear(w);
	max=vofs+getmaxy(w);
	if(max>StdErr.lines.size) {
		vofs=StdErr.lines.size-getmaxy(w);
		max = StdErr.lines.size;
	}
	if(vofs<0) {
		vofs=0;
	}
	windows[StdErrWin].vofs=vofs;
	for(i=0;i<max;i+=1) {
		wprintw(w,"%s\n",(const char*)StdErr.lines.data[vofs]);
		vofs+=1;
	}
}



void init() {
	long x_sz,y_sz,row2,row3,col,mid;

	initscr(); noecho(); cbreak(); curs_set(0);
	flushinp();

	init_outerr();

	x_sz = getmaxx(stdscr);
	y_sz = getmaxy(stdscr);
	mid = x_sz>>1;
	col = x_sz/5;
	row2 = y_sz/3;
	row3 = (2*y_sz)/3;

	create_window(&windows[CodeWin], render_code,
		0, 0,
		4*col, row2);
	create_window(&windows[DataSeg], render_dataseg,
		4*col, 0,
		col, row2);
	create_window(&windows[DataStack], render_datastack,
		0, row2,
		col, row3-row2);
	create_window(&windows[LocalsStack], render_localsstack,
		col, row2,
		col, row3-row2);
	create_window(&windows[ClosureStack], render_closurestack,
		2*col, row2,
		col, row3-row2);
	create_window(&windows[CallStack], render_callstack,
		3*col, row2,
		col, row3-row2);

	create_window(&windows[CatchStack], render_catchstack,
		4*col, row2,
		col, row3-row2);

	create_window(&windows[StdOutWin], render_stdout,
		0, row3,
		mid, y_sz-row3);

	create_window(&windows[StdErrWin], render_stderr,
		mid, row3,
		mid, y_sz-row3);

	/*code = subwin(stdscr, ST_H, x_sz-ST_W, 0, 0);*/
	/*data_seg = subwin(stdscr, ST_H, ST_W, x_sz-ST_W, 0);*/
	/*data_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 0);*/
	/*locals_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, ST_W);*/
	/*closure_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 2*ST_W);*/
	/*call_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 3*ST_W);*/
	/*catch_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 4*ST_W);*/
	/*data_seg = subwin(stdscr, ST_H, ST_W, 0, x_sz-ST_W);*/
}

/*@}*/


/**************************************************************************
 *			 __  __    _    ___ _   _ 
 *			|  \/  |  / \  |_ _| \ | |
 *			| |\/| | / _ \  | ||  \| |
 *			| |  | |/ ___ \ | || |\  |
 *			|_|  |_/_/   \_\___|_| \_|
 *                                        
 *************************************************************************/

#define TINYAML_ABOUT	"This is not yet another meta-language -- Interactive Debugger.\n" \
			"(c) 2007-2008 Damien 'bl0b' Leroux\n\n"

#define cmp_param(_n,_arg_str_long,_arg_str_short) (i<(argc-_n) && (	\
		(_arg_str_long && !strcmp(_arg_str_long,argv[i]))	\
					||				\
		(_arg_str_short && !strcmp(_arg_str_short,argv[i]))	\
	))

long do_args(vm_t vm, long argc,const char*argv[]) {
	long i,k;
	program_t p=NULL;
	writer_t w=NULL;
	reader_t r=NULL;

	for(i=1;i<argc;i+=1) {
		if(cmp_param(1,"--compile","-c")) {
			i+=1;
			p = vm_compile_file(vm,argv[i]);
		} else if(cmp_param(1,"--load","-l")) {
			i+=1;
			r = file_reader_new(argv[i]);
			p=vm_unserialize_program(vm,r);
			reader_close(r);
		} else if(cmp_param(0,"--run","-f")) {
			state=STATE_RUNNING;
			vm_run_program_bg(vm,p,0,50);
			state=STATE_STEPPING;
		} else if(cmp_param(0,"--debug","-d")) {
			state=STATE_STEPPING;
			vm_run_program_fg(vm,p,0,50);
		} else if(cmp_param(0,"--version","-v")) {
			printf(TINYAML_ABOUT);
			printf("version " TINYAML_VERSION "\n" );
			exit(0);
		} else if(cmp_param(0,"--help","-h")) {
			printf(TINYAML_ABOUT);
			printf("Usage : %s [--compile, -c [filename]] [--save, -s [filename]] [--load, -l [filename]] [--run-foreground, -f] [--run-background, -b] [--version, -v] [--help,-h]\n",argv[0]);
			printf(	"Commands are executed on the fly.\n"
				"\n\t--compile,-c [filename]\tcompile this file\n"
				  "\t--load,-l [filename]\tload a serialized program from this file\n"
				  "\t--run,-f \trun the newest program\n"
				  "\t--debug,-d \trun the newest program in the debugger\n"
				  "\t--version,-v \t\tdisplay program version\n"
				"\n\t--help,-h\t\tdisplay this text\n\n");
			exit(0);
		}
	}

	return 0;
}


long main(long argc, const char**argv) {
	init();
	vm = vm_new();
	vm_set_engine(vm,debug_engine);
	do_args(vm,argc,argv);
	vm_del(vm);
	term();
	return 0;
}



/**************************************************************************
 *	   __     ____  __     _____ _   _  ____ ___ _   _ _____ 
 *	   \ \   / /  \/  |   | ____| \ | |/ ___|_ _| \ | | ____|
 *	    \ \ / /| |\/| |   |  _| |  \| | |  _ | ||  \| |  _|  
 *	     \ V / | |  | |   | |___| |\  | |_| || || |\  | |___ 
 *	      \_/  |_|  |_|   |_____|_| \_|\____|___|_| \_|_____|
 * 
 *************************************************************************/

/*! \weakgroup debug_engine
 * @{
 */

void _VM_CALL dbg_run(vm_engine_t e, program_t p, word_t ip, word_t prio) {
	if(e->vm->current_thread) {
		thread_t t = e->vm->current_thread;
		/* save some thread state */
		program_t jmp_seg = t->jmp_seg;
		word_t jmp_ofs = t->jmp_ofs;
		word_t IP = t->IP;
		program_t program = t->program;
		/* hack new thread state */
		word_t s_sz;
		t->jmp_ofs=0;
		t->jmp_seg=p;
		vm_push_caller(e->vm, t->program, t->IP, 0, NULL);
		s_sz = t->call_stack.sp-1;
		t->program=p;
		t->IP=ip;
		while(e->vm->threads_count&&t->call_stack.sp!=s_sz) {
			vm_schedule_cycle(e->vm);
		}
		if(e->vm->threads_count) {
			/*vm_printf("done with sub thread\n");*/
			/* restore old state */
			t->program=program;
			t->IP=IP;
			t->jmp_seg=jmp_seg;
			t->jmp_ofs=jmp_ofs;
		}
	} else {
		vm_add_thread(e->vm,p,ip,prio,0);
		while(e->vm->threads_count) {
			vm_schedule_cycle(e->vm);
		}
	}
}


void _VM_CALL thread_failed(vm_t vm, thread_t t);

void _VM_CALL dbg_thread_failed(vm_t vm,thread_t t) {
	t->data_stack.sp = t->data_sp_backup;
	state=STATE_DEAD;
	thread_failed(vm,t);
	repaint(vm,t,1);
	getch();
}

void _VM_CALL e_stub(vm_engine_t);

void _VM_CALL dbg_debug(vm_engine_t e) {
	if(state==STATE_RUNNING) {
		return;
	}
	repaint(e->vm,e->vm->current_thread,1);
}




struct _thread_engine_t {
	struct _vm_engine_t _;
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_mutex_t fg_mutex;
	volatile long is_running;
};

void _VM_CALL dbg_cli_lock(struct _thread_engine_t* e) {
	if(pthread_self()==e->thread) {
		return;
	}
	pthread_mutex_lock(&e->mutex);
}

void _VM_CALL dbg_cli_unlock(struct _thread_engine_t* e) {
	if(pthread_self()==e->thread) {
		return;
	}
	pthread_mutex_unlock(&e->mutex);
}

void _VM_CALL dbg_vm_lock(struct _thread_engine_t* e) {
	/*if(pthread_self()!=e->thread) {*/
		/*return;*/
	/*}*/
	pthread_mutex_lock(&e->mutex);
}

void _VM_CALL dbg_vm_unlock(struct _thread_engine_t* e) {
	/*if(pthread_self()!=e->thread) {*/
		/*return;*/
	/*}*/
	pthread_mutex_unlock(&e->mutex);
}


void _VM_CALL dbg_init(struct _thread_engine_t* e) {
	e->thread=pthread_self();
}

/* synchronous engine. init lock unlock and deinit are pointless, and kill can't happen. */
const vm_engine_t debug_engine = (vm_engine_t) (struct _thread_engine_t[])
{{{
	dbg_init,
	e_stub,
	dbg_run,
	e_stub,
	e_stub,
	dbg_run,
	e_stub,
	(vm_engine_func_t)dbg_cli_lock,
	(vm_engine_func_t)dbg_cli_unlock,
	(vm_engine_func_t)dbg_vm_lock,
	(vm_engine_func_t)dbg_vm_unlock,
	dbg_thread_failed,
	dbg_debug,
	dbg_out,
	dbg_err,
	NULL
},
	(pthread_t)0,
	PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,
	0
}};

/*@}*/
/*@}*/

