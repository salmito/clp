#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "instance.h"
#include "threading.h"
#include "pool.h"

typedef struct thread_s {
	THREAD_T th;
	pool_t pool;
} * thread_t;

thread_t * lstage_newthread(lua_State *L,pool_t pool);
THREAD_T * lstage_tothread(lua_State *L, int i);
#endif
