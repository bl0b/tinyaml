#ifndef __QUICK_AND_DIRTY_MUTEXES__
#define __QUICK_AND_DIRTY_MUTEXES__

#include <pthread.h>

typedef pthread_mutex_t Mutex;

#define mutexInit(__m) do { pthread_mutex_init(&__m,NULL); pthread_mutex_trylock(&__m); pthread_mutex_unlock(&__m); } while(0)
#define mutexLock(__m) pthread_mutex_lock(&__m)
#define mutexUnlock(__m) pthread_mutex_unlock(&__m)

#endif

