#include "channel.h"
#include "instance.h"
#include "event.h"
#include "scheduler.h"
#include "marshal.h"
#include <stdlib.h>

#define LOCK(q) while (__sync_lock_test_and_set(&(q)->lock,1)) {}
#define UNLOCK(q) __sync_lock_release(&(q)->lock);

channel_t lstage_tochannel(lua_State *L, int i) {
	channel_t * t = luaL_checkudata (L, i, LSTAGE_CHANNEL_METATABLE);
	luaL_argcheck (L, *t != NULL, i, "not a Channel");
	return *t;
}

static int channel_eq(lua_State * L) {
	channel_t s1=lstage_tochannel(L,1);
	channel_t s2=lstage_tochannel(L,2);
	lua_pushboolean(L,s1==s2);
	return 1;
}

static int channel_getsize(lua_State * L) {
	channel_t s=lstage_tochannel(L,1);
	lua_pushnumber(L,lstage_lfqueue_getcapacity(s->event_queue));
	return 1;
}

static int channel_setsize(lua_State * L) {
	channel_t s=lstage_tochannel(L,1);
	luaL_checktype (L, 2, LUA_TNUMBER);
	int capacity=lua_tointeger(L,2);
	lstage_lfqueue_setcapacity(s->event_queue,capacity);
	lua_pushvalue(L,1);
	return 1;
}

static int channel_tostring (lua_State *L) {
  channel_t * s = luaL_checkudata (L, 1, LSTAGE_CHANNEL_METATABLE);
  lua_pushfstring (L, "Channel (%p)", *s);
  return 1;
}

static int channel_ptr(lua_State * L) {
	channel_t * s = luaL_checkudata (L, 1, LSTAGE_CHANNEL_METATABLE);
	lua_pushlightuserdata(L,*s);
	return 1;
}

static int channel_pushevent(lua_State *L) {
	channel_t c = lstage_tochannel(L,1);
   int top=lua_gettop(L);
   lua_pushcfunction(L,mar_encode);
   lua_newtable(L);
   int i;
   for(i=2;i<=top;i++) {
      lua_pushvalue(L,i);
      lua_rawseti(L,-2,i-1);
   }
   lua_call(L,1,1);
   size_t len;
   const char * str=lua_tolstring(L,-1,&len);
   lua_pop(L,1);
   event_t ev=lstage_newevent(str,len);
   LOCK(c);
   if(c->waiting>0) {
  	   UNLOCK(c);
	   MUTEX_LOCK(&c->mutex);
	   c->waiting--;
   	c->event=ev;
	   SIGNAL_ONE(&c->cond);
	   MUTEX_UNLOCK(&c->mutex);
   	lua_pushboolean(L,1);
      return 1;
   }
   instance_t ins=NULL;
   if(lstage_lfqueue_try_pop(c->wait_queue,&ins)) {
  		UNLOCK(c);
   	lua_settop(ins->L,0);
   	ins->ev=ev;
   	ins->channel=NULL;
		ins->flags=I_READY;
		lstage_pushinstance(ins);
		lua_pushboolean(L,1);
		return 1;
   } else if(lstage_lfqueue_try_push(c->event_queue,&ev)) {
	   UNLOCK(c);
      lua_pushboolean(L,1);
      return 1;
   } 
   UNLOCK(c);
   lstage_destroyevent(ev);
   lua_pushnil(L);
   lua_pushliteral(L,"Event queue is full");
   return 2;
}

static int channel_tryget(lua_State *L) {
   channel_t c = lstage_tochannel(L,1);
	event_t ev=NULL;
   if(lstage_lfqueue_try_pop(c->event_queue,(void **)&ev)) {
      lua_pushboolean(L,1);
		int n=lstage_restoreevent(L,ev);
		lstage_destroyevent(ev);
		return n+1;
	}
	return 0;
}

static int channel_getevent(lua_State *L) {
	channel_t c = lstage_tochannel(L,1);
	event_t ev=NULL;
	lua_pushliteral(L,LSTAGE_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if(lstage_lfqueue_try_pop(c->event_queue,&ev)) {
		int n=lstage_restoreevent(L,ev);
		lstage_destroyevent(ev);
		return n;
	}
	if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA) {
		MUTEX_LOCK(&c->mutex);
		c->waiting++;
		SIGNAL_WAIT(&c->cond,&c->mutex,-1.0);
		int n=0;
		if(c->event) {
			n=lstage_restoreevent(L,c->event);
			lstage_destroyevent(c->event);
			c->event=NULL;
		}
		MUTEX_UNLOCK(&c->mutex);
		return n;
	}
	instance_t i=lua_touserdata(L,-1);
	lua_pop(L,1);
	i->flags=I_WAITING_CHANNEL;
	i->channel=c;
	return lua_yield(L,0);
}


static const struct luaL_Reg ChannelMetaFunctions[] = {
		{"__eq",channel_eq},
		{"__tostring",channel_tostring},
		{"id",channel_ptr},
		{"size",channel_getsize},
		{"setsize",channel_setsize},
		{"get",channel_getevent},
		{"push",channel_pushevent},
		{"tryget",channel_tryget},
		{NULL,NULL}
};

static void get_metatable(lua_State * L) {
	luaL_getmetatable(L,LSTAGE_CHANNEL_METATABLE);
   if(lua_isnil(L,-1)) {
   	lua_pop(L,1);
  		luaL_newmetatable(L,LSTAGE_CHANNEL_METATABLE);
  		#if LUA_VERSION_NUM < 502
			luaL_register(L, NULL, ChannelMetaFunctions);
		#else
			luaL_setfuncs(L, ChannelMetaFunctions, 0);
		#endif 
  		lua_pushvalue(L,-1);
  		lua_setfield(L,-2,"__index");
		luaL_loadstring(L,"local id=(...):id() return function() return require'lstage.channel'.get(id) end");
		lua_setfield (L, -2,"__wrap");
  	}
}

static void channel_build(lua_State * L,channel_t t) {
	channel_t *s=lua_newuserdata(L,sizeof(channel_t *));
	*s=t;
	get_metatable(L);
   lua_setmetatable(L,-2);
}

static int channel_new(lua_State *L) {
	channel_t t=malloc(sizeof(struct channel_s));
	int size=luaL_optint(L, 1, -1);
	SIGNAL_INIT(&t->cond);
	MUTEX_INIT(&t->mutex);
	t->waiting=0;
	t->event=NULL;
	t->lock=0;
	t->event_queue=lstage_lfqueue_new();
	t->wait_queue=lstage_lfqueue_new();
	lstage_lfqueue_setcapacity(t->event_queue,size);
	lstage_lfqueue_setcapacity(t->wait_queue,-1);
	channel_build(L,t);
   return 1;
}

static int channel_get(lua_State * L) {
	channel_t s=lua_touserdata(L,1);
	channel_build(L,s);
	return 1;
}

static const struct luaL_Reg LuaExportFunctions[] = {
		{"new",channel_new},
		{"get",channel_get},
		{NULL,NULL}
};

LSTAGE_EXPORTAPI	int luaopen_lstage_channel(lua_State *L) {
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'lstage.channel' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};


