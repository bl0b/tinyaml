#include <curses.h>
#include <string.h>

#include "vm.h"
#include "fastmath.h"
#include "_impl.h"
#include "vm_engine.h"
#include "object.h"
#include "program.h"

void lookup_label_and_ofs(program_t cs, word_t ip, const char** label, word_t* ofs);

static void data_stack_renderer(WINDOW* w,vm_data_t d) {
	_IFC conv;
	/*int i;*/
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

static void closure_stack_renderer(WINDOW* w,dynarray_t* da) {
	vm_data_t tab = (vm_data_t) (*da)->data;
	word_t i;
	wprintw(w,"[%lu]",(*da)->size);
	for(i=0;i<(*da)->size;i+=1) {
		wprintw(w,"\n    %li : ",i);
		data_stack_renderer(w,&(tab[i]));
	}
}

static void call_stack_renderer(WINDOW* w,struct _call_stack_entry_t* cse) {
	word_t ofs;
	const char*label;
	lookup_label_and_ofs(cse->cs,cse->ip,&label,&ofs);
	if(label) {
		wprintw(w,"%p:%lx\n     (%s+%lu)",cse->ip, cse->cs, label, ofs);
	} else {
		wprintw(w,"%p:%lu",cse->cs,cse->ip);
	}
}


static void render_stack(WINDOW* w, generic_stack_t s, word_t skip, word_t count, const char*prefix, void(*renderer)(WINDOW*,void*), int disp_start, int disp_incr) {
	long counter = -skip;
	long stop = -(skip+count);
	wprintw(w,"\n");
	while(counter>stop) {
		wprintw(w,"%s%li : ",prefix,disp_start);
		disp_start+=disp_incr;
		renderer(w,_gpeek(s,counter));
		wprintw(w,"\n");
		counter-=1;
	}
}




/**************************************************************************
 *	    _   _  ____ _   _ ____  ____  _____ ____  
 *	   | \ | |/ ___| | | |  _ \/ ___|| ____/ ___| 
 *	   |  \| | |   | | | | |_) \___ \|  _| \___ \ 
 *	   | |\  | |___| |_| |  _ < ___) | |___ ___) |
 *	   |_| \_|\____|\___/|_| \_\____/|_____|____/ 
 *
 *************************************************************************/

int x_sz,y_sz;

WINDOW* call_stack;
WINDOW* data_stack;
WINDOW* data_seg;
WINDOW* locals_stack;
WINDOW* closure_stack;

WINDOW* code;

#define N_WINDOWS 6


#define ST_W ((getmaxx(stdscr)>>2))
#define ST_H ((getmaxy(stdscr)>>1))


int cur_win=0;

int window_vofs[N_WINDOWS] = { 0,0,0,0,0,0, };

WINDOW** windows[N_WINDOWS] = {
	&code,
	&call_stack,
	&data_stack,
	&locals_stack,
	&closure_stack,
	&data_seg,
};

#define STATE_IDLE 0
#define STATE_RUNNING 1
#define STATE_STEPPING 2
#define STATE_DEAD 3

int state=STATE_IDLE;

void do_scroll_down() {
	window_vofs[cur_win]+=1+(!cur_win);
	state=STATE_IDLE;
}

void do_scroll_up() {
	if(window_vofs[cur_win]==0) {
		return;
	}
	window_vofs[cur_win]-=1+(!cur_win);
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
	/*fprintf(stderr,"cur_win is now %i\n",cur_win);*/
}

void term();

void do_quit() {
	term();
	exit(0);
}

void do_help();

typedef struct _command_t {
	int ch;
	const char* descr;
	void (*command)();
} command_t;

command_t commands[N_WINDOWS][10] = {
/* code window */ {
	{ 'h', "display this popup", do_help },
	{ 's', "execute next opcode", do_step },
	{ 'r', "run this thread", do_run },
	{ KEY_DOWN, "scroll down by one opcode", do_scroll_down },
	{ KEY_UP, "scroll up by one opcode", do_scroll_up },
	{ 'm', "scroll down by one opcode", do_scroll_down },
	{ 'p', "scroll up by one opcode", do_scroll_up },
	{ '\t', "cycle through windows", do_cycle_win },
	{ 'q', "quit", do_quit },
{ 0, NULL, NULL}}, /* call_stack */ {
	{ 'h', "display this popup", do_help },
	{ KEY_DOWN, "scroll down by one opcode", do_scroll_down },
	{ KEY_UP, "scroll up by one opcode", do_scroll_up },
	{ 'm', "scroll down by one opcode", do_scroll_down },
	{ 'p', "scroll up by one opcode", do_scroll_up },
	{ '\t', "cycle through windows", do_cycle_win },
	{ 'q', "quit", do_quit },
{ 0, NULL, NULL}}, /* data_stack */ {
	{ 'h', "display this popup", do_help },
	{ KEY_DOWN, "scroll down by one opcode", do_scroll_down },
	{ KEY_UP, "scroll up by one opcode", do_scroll_up },
	{ 'm', "scroll down by one opcode", do_scroll_down },
	{ 'p', "scroll up by one opcode", do_scroll_up },
	{ '\t', "cycle through windows", do_cycle_win },
	{ 'q', "quit", do_quit },
{ 0, NULL, NULL}}, /* locals_stack */ {
	{ 'h', "display this popup", do_help },
	{ KEY_DOWN, "scroll down by one opcode", do_scroll_down },
	{ KEY_UP, "scroll up by one opcode", do_scroll_up },
	{ 'm', "scroll down by one opcode", do_scroll_down },
	{ 'p', "scroll up by one opcode", do_scroll_up },
	{ '\t', "cycle through windows", do_cycle_win },
	{ 'q', "quit", do_quit },
{ 0, NULL, NULL}}, /* closure_stack */ {
	{ 'h', "display this popup", do_help },
	{ KEY_DOWN, "scroll down by one opcode", do_scroll_down },
	{ KEY_UP, "scroll up by one opcode", do_scroll_up },
	{ 'm', "scroll down by one opcode", do_scroll_down },
	{ 'p', "scroll up by one opcode", do_scroll_up },
	{ '\t', "cycle through windows", do_cycle_win },
	{ 'q', "quit", do_quit },
{ 0, NULL, NULL}}, /* data_seg */ {
	{ 'h', "display this popup", do_help },
	{ KEY_DOWN, "scroll down by one opcode", do_scroll_down },
	{ KEY_UP, "scroll up by one opcode", do_scroll_up },
	{ 'm', "scroll down by one opcode", do_scroll_down },
	{ 'p', "scroll up by one opcode", do_scroll_up },
	{ '\t', "cycle through windows", do_cycle_win },
	{ 'q', "quit", do_quit },
{ 0, NULL, NULL},
}};


void read_command(vm_t vm, thread_t t);


void print_centered(int n, int y, const char* str) {
	if(n==cur_win) {
		wattron(*windows[n],A_STANDOUT|A_UNDERLINE|A_BOLD);
	}
	mvwprintw(*windows[n],y,(getmaxx(*windows[n])-strlen(str))>>1, str);
	if(n==cur_win) {
		wattroff(*windows[n],A_STANDOUT|A_UNDERLINE|A_BOLD);
	}
}


void do_help() {
	int sz=0;
	WINDOW* popup;
	while(commands[cur_win][sz].ch) { sz+=1; }
	popup = subwin(stdscr, sz+2, 52, (getmaxy(stdscr)-sz-2)>>1, (getmaxx(stdscr)-52)>>1);
	sz=0;
	wprintw(popup,"\n");
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
	box(popup,0,0);
	touchwin(stdscr);
	wnoutrefresh(stdscr);
	doupdate();
	getch();
	delwin(popup);
}


word_t calc_count(int n, word_t sz) {
	word_t skip = window_vofs[n];
	word_t count = sz-skip;
	if(count>getmaxy(*windows[n])-2) {
		count = getmaxy(*windows[n])-2;
	}
	return count;
}

void repaint(vm_t vm, thread_t t, int do_command) {
	word_t ofs;
	vm_data_t tab;
	const char*label;
	const char*disasm;
	word_t code_start, code_end,i,j,skip,count;
	char title[50];

	clear();

	if(!(vm&&t)) {

		box(code,0,0);
		print_centered(0,0," No running thread ");
		box(call_stack,0,0);

		box(data_stack,0,0);
		box(locals_stack,0,0);
		box(closure_stack,0,0);
		box(data_seg,0,0);

		wnoutrefresh(stdscr);
		doupdate();

		read_command(vm,t);
		
		return;
	}

	/* fill code window */
	sprintf(title," %s Thread %p CS:%p IP:%lx ",
		t->state==ThreadRunning?"Running":
			t->state==ThreadZombie?"Zombie":
				t->state==ThreadDying?"Dying":
					"TODO",
		t,t->program,t->IP);

	if(state==STATE_STEPPING||state==STATE_RUNNING) {
		window_vofs[0]=t->IP;
	}

	if(window_vofs[0]>ST_H) {
		code_start = (window_vofs[0]-ST_H)&(~1);
	} else {
		code_start = 0;
	}

	if((long)code_start>(long)(t->program->code.size-2*ST_H)) {
		code_end=t->program->code.size;
	} else {
		code_end=(code_start+2*ST_H)&(~1);
	}

	wattroff(code,A_STANDOUT);

	j = getmaxx(code)-17;

	for(i=code_start;i<code_end;i+=2) {
		disasm = program_disassemble(vm,t->program,i);
		label = program_lookup_label(t->program,i);
		if(i==window_vofs[0]) {
			wattron(code,A_STANDOUT);
		}
		if(label&&*label) {
			sprintf(title,"%s :",label);
			wprintw(code," \t%-*.*s\n %8.8lx  %s\t%-*.*s\n",j+8,j+8,title,i,i==t->IP?"=>":"",j,j,disasm);
		} else {
			wprintw(code," %8.8lx  %s\t%-*.*s\n",i,i==t->IP?"=>":"",j,j,disasm);
		}
		if(i==window_vofs[0]) {
			wattroff(code,A_STANDOUT);
		}
		free((char*)disasm);
	}

	box(code,0,0);
	sprintf(title," %s Thread %p CS:%p IP:%lx \n",
		state==STATE_DEAD?"Dying":"Running",
		t,t->program,t->IP);
	print_centered(0,0,title);

	/* render call stack */

	sprintf(title," Call stack [%lu] ", gstack_size(&t->call_stack));
	render_stack(call_stack,&t->call_stack, window_vofs[1], calc_count(1,gstack_size(&t->call_stack)), "  ", (void(*)(WINDOW*,void*)) call_stack_renderer, 0, 1);
	box(call_stack,0,0);
	print_centered(1,0,title);

	sprintf(title," Data stack [%lu] ", gstack_size(&t->data_stack));
	render_stack(data_stack,&t->data_stack, window_vofs[2], calc_count(2,gstack_size(&t->data_stack)), "  ", (void(*)(WINDOW*,void*)) data_stack_renderer, 0, 1);
	box(data_stack,0,0);
	print_centered(2,0,title);

	sprintf(title," Closures stack [%lu] ", gstack_size(&t->closures_stack));
	render_stack(closure_stack,&t->closures_stack, window_vofs[3], calc_count(3,gstack_size(&t->closures_stack)), "  ", (void(*)(WINDOW*,void*)) closure_stack_renderer, 0, 1);
	box(closure_stack,0,0);
	print_centered(4,0,title);

	sprintf(title," Locals stack [%lu] ", gstack_size(&t->closures_stack));
	render_stack(locals_stack,&t->locals_stack, window_vofs[4], calc_count(4,gstack_size(&t->locals_stack)), "  ", (void(*)(WINDOW*,void*)) data_stack_renderer, -1, -1);
	box(locals_stack,0,0);
	print_centered(3,0,title);

	code_end =t->program->data.size>>1;
	sprintf(title," Data segment [%lu] ", code_end);
	if(code_end>getmaxy(data_seg)+window_vofs[5]-2) {
		code_end=getmaxy(data_seg)+window_vofs[5]-2;
	}
	tab = (vm_data_t) t->program->data.data;
	for(i=window_vofs[5];i<code_end;i+=1) {
		wprintw(data_seg,"\n  %li : ",i);
		data_stack_renderer(data_seg,&(tab[i]));
	}
	box(data_seg,0,0);
	print_centered(5,0,title);


	wnoutrefresh(stdscr);
	doupdate();

	if(do_command) {
		read_command(vm,t);
	}
}




void read_command(vm_t vm, thread_t t) {
	int c;
	int i;
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
		}
	}
}


vm_t vm;

extern const vm_engine_t debug_engine;


void term() {
	vm_del(vm);
	delwin(code);
	delwin(data_stack);
	delwin(locals_stack);
	delwin(closure_stack);
	delwin(data_seg);
	delwin(call_stack);
	delwin(stdscr);
	endwin();
	fputs("Thank you for using tinyaml.\n",stdout);
}



void init() {
	initscr(); noecho(); cbreak(); curs_set(0);
	flushinp();

	x_sz = getmaxx(stdscr);
	y_sz = getmaxy(stdscr);

	code = subwin(stdscr,y_sz-ST_H, x_sz-ST_W, 0, 0);
	call_stack = subwin(stdscr, ST_H, ST_W, 0, x_sz-ST_W);
	data_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 0);
	locals_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, ST_W);
	closure_stack = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 2*ST_W);
	data_seg = subwin(stdscr, ST_H, ST_W, y_sz-ST_H, 3*ST_W);

	vm = vm_new();
	vm_set_engine(vm,debug_engine);
}


/**************************************************************************
 *			 __  __    _    ___ _   _ 
 *			|  \/  |  / \  |_ _| \ | |
 *			| |\/| | / _ \  | ||  \| |
 *			| |  | |/ ___ \ | || |\  |
 *			|_|  |_/_/   \_\___|_| \_|
 *                                        
 *************************************************************************/

int main(int argc, const char**argv) {
	reader_t r;
	program_t p;
	if(argc==1) {
		return 1;
	}
	init();
	r = file_reader_new(argv[1]);
	p = vm_unserialize_program(vm,r);
	reader_close(r);
	vm_run_program_fg(vm,p,0,50);
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
		vm_push_caller(e->vm,t->program,t->IP,0);
		s_sz = t->call_stack.sp-1;
		t->program=p;
		t->IP=ip;
		while(e->vm->threads_count&&t->call_stack.sp!=s_sz) {
			vm_schedule_cycle(e->vm);
		}
		if(e->vm->threads_count) {
			/*printf("done with sub thread\n");*/
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



void _VM_CALL dbg_thread_failed(vm_t vm,thread_t t) {
	t->data_stack.sp = t->data_sp_backup;
	state=STATE_DEAD;
	repaint(vm,t,1);
	getch();
}

void _VM_CALL e_stub(vm_engine_t);

void _VM_CALL dbg_debug(vm_engine_t e) {
	repaint(e->vm,e->vm->current_thread,1);
}

/* synchronous engine. init lock unlock and deinit are pointless, and kill can't happen. */
const vm_engine_t debug_engine = (struct _vm_engine_t[])
{{
	e_stub,
	e_stub,
	dbg_run,
	e_stub,
	e_stub,
	dbg_run,
	e_stub,
	e_stub,
	e_stub,
	e_stub,
	e_stub,
	dbg_thread_failed,
	dbg_debug,
	NULL
}};



