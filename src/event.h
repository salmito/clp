#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdlib.h>

typedef struct event_s * event_t;

#include "instance.h"

struct event_s {
   char * data;
   size_t len;
};

event_t lstage_newevent(const char * ev, size_t len);
int lstage_restoreevent(lua_State *L,event_t ev);
void lstage_destroyevent(event_t e);

#endif
