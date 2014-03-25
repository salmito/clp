#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "instance.h"
#include "threading.h"
#include "pool.h"

enum thread_e {
	THREAD_IDLE,
	THREAD_RUNNING,
	THREAD_DESTROYED,
};

typedef struct thread_s {
	THREAD_T * th;
	pool_t pool;
	volatile enum thread_e state;
} * thread_t;

int lstage_newthread(lua_State *L,pool_t pool);
thread_t lstage_tothread(lua_State *L, int i);
void lstage_pushinstance(instance_t i);
int thread_kill (lua_State *L,pool_t pool);

#endif
