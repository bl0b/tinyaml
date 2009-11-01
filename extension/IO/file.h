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

#define _GNU_SOURCE
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
/*#include <stdio.h>*/
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include "vm.h"
#include "thread.h"
#include "_impl.h"
#include "vm_types.h"
#include "object.h"

typedef struct _file_t* file_t;
typedef struct _server_t* server_t;

#define FISSYSTEM	(1<<0)
#define FREADABLE	(1<<1)
#define FWRITABLE	(1<<2)
#define FISSEEKABLE	(1<<3)
#define FISOPEN		(1<<4)
#define FISRUNNING	(1<<5)

#define _FTYPE1		(1<<6)
#define _FTYPE2		(1<<7)
#define _FTYPE3		(1<<8)
#define FTYPEMASK	(_FTYPE1|_FTYPE2|_FTYPE3)

#define FISFILE		(0)
#define FISPROCESS	(_FTYPE1)
#define FISBUFFER	(_FTYPE2)
#define FISSOCKET	(_FTYPE1|_FTYPE2)
#define FISTCPSERVER	(_FTYPE3)
#define FISUDPSERVER	(_FTYPE3|_FTYPE1)
#define FISDIR		(_FTYPE3|_FTYPE2)

#define _file_type(_f) ((_f)->flags&FTYPEMASK)

#define file_is_file(_f) (_file_type(_f)==FISFILE)
#define file_is_process(_f) (_file_type(_f)==FISPROCESS)
#define file_is_socket(_f) (_file_type(_f)==FISSOCKET)
#define file_is_buffer(_f) (_file_type(_f)==FISBUFFER)
#define file_is_dir(_f) (_file_type(_f)==FISDIR)
#define file_is_tcpserver(_f) (_file_type(_f)==FISTCPSERVER)
#define file_is_udpserver(_f) (_file_type(_f)==FISUDPSERVER)

#define FSTATEMASK	((1<<10)-1)

#define FCMDREAD	(1<<10)
#define FCMDWRITE	(1<<11)
#define FCMDDONE	(1<<12)
#define FCMDMASK	(FCMDREAD|FCMDWRITE|FCMDDONE)

/*#define _rd(_f, _b, _s) (_f->flags&FISSOCKET ? recv(_f->fd, _b, _s, 0) : read(_f->fd, _b, _s))*/
/*#define _wr(_f, _b, _s) (_f->flags&FISSOCKET ? send(_f->fd, _b, _s, 0) : write(_f->fd, _b, _s))*/



#define FIFOSZ 4096
#define FIFOMASK (FIFOSZ-1)

typedef void (*_pack_handler) (file_t, char, word_t);
typedef word_t (*_unpack_handler) (file_t, char);

struct _format_override {
	_pack_handler packer;
	_unpack_handler unpacker;
	vm_data_type_t data_type;
	word_t magic;	/* for user object types */
};

typedef struct _format_override* format_override_t;

struct _file_t {
	word_t magic;
	union {
		long fd;		/* still used for listening socket */
		FILE* f;
		DIR* d;
	} descr;
	long flags;
	char* source;
	vm_blocker_t blocker;
	thread_t owner;
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	word_t data;
	char data_fmt;
	union {
		struct {
			unsigned long addr;
			unsigned long port;
		} client;
		struct {
			char* buffer_data;
			word_t buffer_size;
		} buffer;
	} extra;
	format_override_t _overrides[127];
};

extern volatile file_t f_out, f_in, f_err;

#define file_enable_thread(_f) mutexUnlock(_f->mutex)
#define file_disable_thread(_f) mutexLock(_f->mutex)

#define file_cmd_reset(_f) do { _f->flags=_f->flags&FSTATEMASK; } while(0)
#define file_cmd_done(_f) do { _f->flags=(_f->flags&FSTATEMASK)|FCMDDONE; } while(0)
#define file_cmd_write(_f) do { _f->flags=(_f->flags&FSTATEMASK)|FCMDWRITE; } while(0)
#define file_cmd_read(_f) do { _f->flags=(_f->flags&FSTATEMASK)|FCMDREAD; } while(0)

#include "file_rd.h"
#include "file_wr.h"


void file_update_state(file_t f, long flags);

void file_deinit(vm_t vm, file_t f);
file_t file_clone(vm_t vm, file_t f);
file_t file_new(vm_t vm, const char* source, FILE*f, long flags);

file_t buffer_new(vm_t vm, const char* source, FILE*f, long flags);
file_t dir_new(vm_t vm, const char* source);

void cmd_pack(vm_t vm, file_t f, char fmt, word_t data);

void cmd_unpack(vm_t vm, file_t f, char fmt);


void file_override_format(file_t, char, _pack_handler, _unpack_handler, vm_data_type_t, word_t);

