#ifndef POOL_H
#define POOL_H

#include <lua.h>

#include "lstage.h"
#include "lf_queue.h"

typedef struct pool_s {
	volatile size_t size;
	LFqueue_t ready;
} * pool_t;

pool_t lstage_topool(lua_State *L, int i);
void lstage_buildpool(lua_State * L,pool_t t);

#endif
