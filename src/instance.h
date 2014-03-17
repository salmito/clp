#ifndef _INSTANCE_H
#define _INSTANCE_H

typedef struct instance_s * instance_t;

#include "lstage.h"
#include "stage.h"
#include "event.h"
#include "channel.h"

#include "lua.h"

#define LSTAGE_INSTANCE_KEY "lstage-instance-key"

enum instance_flag_t {
	I_CREATED=0x0,
	I_IDLE,
	I_READY,
	I_WAITING_IO,
	I_TIMEOUT_IO,
	I_WAITING_EVENT,
	I_WAITING_CHANNEL,
};

struct instance_s {
   lua_State * L;
   stage_t stage;
   event_t ev;
   int flags;
   int args;
   channel_t channel;
};

instance_t lstage_newinstance(stage_t s);
void lstage_initinstance(instance_t i);
void lstage_destroyinstance(instance_t i);
void lstage_putinstance(instance_t i);

#endif
