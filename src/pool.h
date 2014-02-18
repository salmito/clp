#ifndef POOL_H
#define POOL_H

#include <lua.h>

#include "lstage.h"
#include "p_queue.h"

typedef struct pool_s {
	volatile size_t size;
	Pqueue_t ready;
} * pool_t;

pool_t lstage_topool(lua_State *L, int i);
void lstage_buildpool(lua_State * L,pool_t t);

#endif
