#ifndef POOL_H
#define POOL_H

#include <lua.h>

#include "lstage.h"
#include "lf_hash.h"
#include "lf_queue.h"

typedef struct pool_s {
	qt_hash H;
	LFqueue_t ready;
} * pool_t;

pool_t lstage_topool(lua_State *L, int i);

#endif
