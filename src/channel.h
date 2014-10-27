#ifndef _CHANNEL_H_
#define _CHANNEL_H_

typedef struct channel_s  * channel_t;

#include "clp.h"
#include "lf_queue.h"
#include "threading.h"
#include "event.h"


#define CLP_CHANNEL_CACHE "clp-channel-cache"

struct channel_s {
	LFqueue_t event_queue;
	LFqueue_t read_queue;
	LFqueue_t write_queue;
	MUTEX_T mutex;
	SIGNAL_T read_cond;
	SIGNAL_T write_cond;
	volatile int read_wait;
	volatile int write_wait;
	volatile event_t read_event;
	volatile event_t write_event;
	volatile int lock;
	volatile int closed;
	int sync;
};

#define CHANNEL_LOCK(q) while (__sync_lock_test_and_set(&(q)->lock,1)) {}
#define CHANNEL_UNLOCK(q) __sync_lock_release(&(q)->lock);

channel_t clp_tochannel(lua_State *L, int i);
int clp_channelnew(lua_State *L);
void clp_pushchannel(lua_State * L,channel_t t);
int clp_pushevent(lua_State *L);

#endif
