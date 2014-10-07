/*Adapted from https://github.com/Tieske/Lua_library_template/*/

/*
** ===============================================================
** Leda is a parallel and concurrent framework for Lua.
** Copyright 2014: Tiago Salmito
** License MIT
** ===============================================================
*/

#ifndef TASK_H
#define TASK_H

typedef struct clp_Task * task_t;

#define TASK_HANDLER_KEY "task.handler"

#include "clp.h"
#include "lf_queue.h"
#include "pool.h"
#include "channel.h"
#include "threading.h"
#include "event.h"
#include "channel.h"

#include "lua.h"

#define CLP_INSTANCE_KEY "task-instance-key"
#define CLP_HANDLER_KEY "task-handler-key"
#define CLP_ERRORFUNCTION_KEY "task-error-key"
#define CLP_ENV_KEY "task-env-key"

struct clp_Task {
   MUTEX_T intances_mutex;
	volatile int instances;
	channel_t input;
	channel_t output;
	pool_t pool;
	char * env;
	size_t env_len;
   task_t parent;
};

task_t clp_totask(lua_State *L, int i);
void clp_buildtask(lua_State * L,task_t t);

//instance
typedef struct instance_s * instance_t;

enum instance_flag_t {
	I_CREATED=0x0,
	I_READY,
	I_RUNNING,
	I_WAITING_IO,
	I_TIMEOUT_IO,
	I_WAITING_EVENT,
	I_WAITING_CHANNEL,
	I_IDLE,
	I_CLOSED,
	I_WAITING_WRITE,
};

struct instance_s {
   lua_State * L;
   task_t stage;
   event_t ev;
   int flags;
   int args;
};

instance_t clp_newinstance(task_t s);
void clp_initinstance(instance_t i);
void clp_destroyinstance(instance_t i);
void clp_putinstance(instance_t i);


#endif //TASK_H
