#ifndef POOL_H
#define POOL_H

#include <lua.h>

#include "clp.h"
#include "lf_queue.h"

typedef struct pool_s {
	volatile size_t size;
	LFqueue_t ready;
	volatile int lock;
} * pool_t;

pool_t clp_topool(lua_State *L, int i);
void clp_buildpool(lua_State * L,pool_t t);

#endif
