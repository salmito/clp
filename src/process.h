/*Adapted from https://github.com/Tieske/Lua_library_template/*/

/*
 ** ===============================================================
 ** Leda is a parallel and concurrent framework for Lua.
 ** Copyright 2014: Tiago Salmito
 ** License MIT
 ** ===============================================================
 */

#ifndef PROCESS_H
#define PROCESS_H

typedef struct clp_Task * process_t;
typedef struct instance_s * instance_t;

#define PROCESS_HANDLER_KEY "process.handler"

#include "clp.h"
#include "lf_queue.h"
#include "pool.h"
#include "channel.h"
#include "threading.h"
#include "event.h"
#include "channel.h"

#include "lua.h"

#define CLP_INSTANCE_KEY "process-instance-key"
#define CLP_HANDLER_KEY "process-handler-key"
#define CLP_ERRORFUNCTION_KEY "process-error-key"
#define CLP_ENV_KEY "process-env-key"

struct clp_Task {
	MUTEX_T intances_mutex;
	volatile int instances; /* number of instances */
	channel_t input; /* input channel */
	pool_t pool;
	char * env;
	size_t env_len;
	process_t parent; /* parent process */
};

process_t clp_toprocess(lua_State *L, int i);
void clp_buildprocess(lua_State * L,process_t t);

//instance

enum instance_state_t {
	I_CREATED=0x0,
	I_READY,
	I_SLEEP,
	I_RUNNING,
	I_RESUME_SUCCESS,
	I_RESUME_FAIL,
	I_CHANNEL_READ,
	I_CHANNEL_WRITE,
	I_IDLE,
	I_CLOSED
};

struct instance_s {
	lua_State * L;
	process_t process; /* parent process */
	event_t ev;
	enum instance_state_t state;
	int args;
	channel_t chan; /* waiting channel */
};

void clp_initinstance(instance_t i);
void clp_destroyinstance(instance_t i);
void clp_putinstance(instance_t i);


#endif //PROCESS_H
