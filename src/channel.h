#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "lstage.h"
#include "lf_queue.h"
#include "threading.h"

typedef struct channel_s {
	LFqueue_t event_queue;
	LFqueue_t wait_queue;
	MUTEX_T mutex;
	SIGNAL_T cond;
} * channel_t;

//channel_t * lstage_newchannel(lua_State *L);
channel_t lstage_tochannel(lua_State *L, int i);

#endif
