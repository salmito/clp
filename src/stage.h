/*Adapted from https://github.com/Tieske/Lua_library_template/*/

/*
** ===============================================================
** Leda is a parallel and concurrent framework for Lua.
** Copyright 2014: Tiago Salmito
** License MIT
** ===============================================================
*/

#ifndef stage_h
#define stage_h

typedef struct lstage_Stage * stage_t;

#define STAGE_HANDLER_KEY "stage-handler"

#include "lstage.h"
#include "lf_queue.h"
#include "pool.h"
#include "channel.h"
#include "threading.h"

struct lstage_Stage {
   MUTEX_T intances_mutex;
	volatile int instances;
	channel_t input;
	pool_t pool;
	char * env;
	size_t env_len;
   stage_t parent;
};

stage_t lstage_tostage(lua_State *L, int i);
void lstage_buildstage(lua_State * L,stage_t t);

#endif
