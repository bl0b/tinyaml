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
#include "vm.h"
#include "thread.h"
#include "_impl.h"
#include "object.h"

typedef struct _file_t* file_t;
typedef struct _server_t* server_t;

#define FISSYSTEM	(2<<0)
#define FREADABLE	(2<<1)
#define FWRITABLE	(2<<2)
#define FISSEEKABLE	(2<<3)
#define FISOPEN		(2<<4)
#define FISRUNNING	(2<<5)

#define _FTYPE1		(2<<6)
#define _FTYPE2		(2<<7)
#define _FTYPE3		(2<<8)
#define FTYPEMASK	(_FTYPE1|_FTYPE2|_FTYPE3)

#define FISFILE		(0)
#define FISPROCESS	(_FTYPE1)
#define FISBUFFER	(_FTYPE2)
#define FISSOCKET	(_FTYPE1|_FTYPE2)
#define FISTCPSERVER	(_FTYPE3)
#define FISUDPSERVER	(_FTYPE3|_FTYPE1)

#define _file_type(_f) ((_f)->flags&FTYPEMASK)

#define file_is_file(_f) (_file_type(_f)==FISFILE)
#define file_is_process(_f) (_file_type(_f)==FISPROCESS)
#define file_is_socket(_f) (_file_type(_f)==FISSOCKET)
#define file_is_buffer(_f) (_file_type(_f)==FISBUFFER)
#define file_is_tcpserver(_f) (_file_type(_f)==FISTCPSERVER)
#define file_is_udpserver(_f) (_file_type(_f)==FISUDPSERVER)

#define FSTATEMASK	((2<<10)-1)

#define FCMDREAD	(2<<10)
#define FCMDWRITE	(2<<11)
#define FCMDDONE	(2<<12)
#define FCMDMASK	(FCMDREAD|FCMDWRITE|FCMDDONE)

/*#define _rd(_f, _b, _s) (_f->flags&FISSOCKET ? recv(_f->fd, _b, _s, 0) : read(_f->fd, _b, _s))*/
/*#define _wr(_f, _b, _s) (_f->flags&FISSOCKET ? send(_f->fd, _b, _s, 0) : write(_f->fd, _b, _s))*/



#define FIFOSZ 4096
#define FIFOMASK (FIFOSZ-1)

struct _file_t {
	word_t magic;
	union {
		int fd;		/* still used for listening socket */
		FILE* f;
	} descr;
	int flags;
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
			unsigned int addr;
			unsigned int port;
		} client;
		struct {
			char* buffer_data;
			size_t buffer_size;
		} buffer;
	} extra;
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


void file_update_state(file_t f, int flags);

void file_deinit(vm_t vm, file_t f);
file_t file_clone(vm_t vm, file_t f);
file_t file_new(vm_t vm, const char* source, FILE*f, int flags);

void cmd_pack(vm_t vm, file_t f, char fmt, word_t data);

void cmd_unpack(vm_t vm, file_t f, char fmt);


