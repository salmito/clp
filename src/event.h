#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdlib.h>

typedef struct event_s * event_t;

struct event_s {
   char * data;
   size_t len;
};

event_t clp_newevent(const char * ev, size_t len);
int clp_restoreevent(lua_State *L,event_t ev);
void clp_destroyevent(event_t e);

#endif
