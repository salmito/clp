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
	channel_t output;
	pool_t pool;
	char * env;
	size_t env_len;
   stage_t parent;
};

stage_t lstage_tostage(lua_State *L, int i);
void lstage_buildstage(lua_State * L,stage_t t);

//instance
typedef struct instance_s * instance_t;

#include "lstage.h"
#include "stage.h"
#include "event.h"
#include "channel.h"

#include "lua.h"

#define LSTAGE_INSTANCE_KEY "lstage-instance-key"
#define LSTAGE_HANDLER_KEY "lstage-handler-key"
#define LSTAGE_ERRORFUNCTION_KEY "lstage-error-key"
#define LSTAGE_ENV_KEY "lstage-env-key"

enum instance_flag_t {
	I_CREATED=0x0,
	I_READY,
	I_RUNNING,
	I_WAITING_IO,
	I_TIMEOUT_IO,
	I_WAITING_EVENT,
	I_WAITING_CHANNEL,
	I_IDLE,
};

struct instance_s {
   lua_State * L;
   stage_t stage;
   event_t ev;
   int flags;
   int args;
};

instance_t lstage_newinstance(stage_t s);
void lstage_initinstance(instance_t i);
void lstage_destroyinstance(instance_t i);
void lstage_putinstance(instance_t i);


#endif
