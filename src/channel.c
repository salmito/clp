#include "channel.h"
#include "task.h"
#include "event.h"
#include "scheduler.h"
#include "marshal.h"
#include <stdlib.h>

static event_t waiting_event;

channel_t clp_tochannel(lua_State *L, int i) {
	channel_t * t = luaL_checkudata (L, i, CLP_CHANNEL_METATABLE);
	luaL_argcheck (L, *t != NULL, i, "not a Channel");
	return *t;
}

static void
channel_putcache(lua_State * L, channel_t t)
{
	lua_pushliteral(L,CLP_CHANNEL_CACHE);
	lua_gettable(L,LUA_REGISTRYINDEX);
	if(!lua_isuserdata(L,-1)) {
		lua_pop(L,1);
		lua_newtable(L);
		lua_newtable(L);
		lua_pushliteral(L,"v");
		lua_setfield(L,-2,"__mode");
		lua_setmetatable(L,-2);
		lua_pushliteral(L,CLP_CHANNEL_CACHE);
		lua_pushvalue(L,-2);
		lua_settable(L,LUA_REGISTRYINDEX);
	}
	lua_pushlightuserdata(L,t);
	lua_pushvalue(L,-3);
	lua_settable(L,-3);
	lua_pop(L,1);
}

static void
channel_getcached(lua_State * L, channel_t t)
{
	lua_pushliteral(L,CLP_CHANNEL_CACHE);
	lua_gettable(L,LUA_REGISTRYINDEX);
	if(!lua_istable(L,-1)) {
		lua_pop(L,1);
		lua_newtable(L);
		lua_newtable(L);
		lua_pushliteral(L,"v");
		lua_setfield(L,-2,"__mode");
		lua_setmetatable(L,-2);
		lua_pushliteral(L,CLP_CHANNEL_CACHE);
		lua_pushvalue(L,-2);
		lua_settable(L,LUA_REGISTRYINDEX);
	}
	lua_pushlightuserdata(L,t);
	lua_gettable(L,-2);
	lua_remove(L,-2);
}


static int channel_eq(lua_State * L) {
	channel_t s1=clp_tochannel(L,1);
	channel_t s2=clp_tochannel(L,2);
	lua_pushboolean(L,s1==s2);
	return 1;
}

static int channel_getsize(lua_State * L) {
	channel_t s=clp_tochannel(L,1);
	lua_pushnumber(L,clp_lfqueue_size(s->event_queue));
	lua_pushnumber(L,clp_lfqueue_getcapacity(s->event_queue));
	lua_pushnumber(L,clp_lfqueue_size(s->read_queue));
	lua_pushnumber(L,clp_lfqueue_size(s->write_queue));
	return 4;
}

static int channel_setsize(lua_State * L) {
	channel_t s=clp_tochannel(L,1);
	luaL_checktype (L, 2, LUA_TNUMBER);
	int capacity=lua_tointeger(L,2);
	int waitsize=luaL_optint(L,3,-1);
	clp_lfqueue_setcapacity(s->event_queue,capacity);
	clp_lfqueue_setcapacity(s->read_queue,waitsize);
	lua_pushvalue(L,1);
	return 1;
}

static int channel_close(lua_State * L) {
	channel_t c=clp_tochannel(L,1);
   instance_t ins=NULL;
   CHANNEL_LOCK(c);
   while(clp_lfqueue_try_pop(c->read_queue,&ins)) {
  		ins->state=I_CLOSED;
 		ins->args=0;
		clp_pushinstance(ins);
   }
   while(clp_lfqueue_try_pop(c->write_queue,&ins)) {
  		ins->state=I_CLOSED;
 		ins->args=0;
		clp_pushinstance(ins);
   }
   c->closed=1;
  	CHANNEL_UNLOCK(c);
	lua_pushvalue(L,1);
	return 1;
}

static int channel_tostring (lua_State *L) {
  channel_t * s = luaL_checkudata (L, 1, CLP_CHANNEL_METATABLE);
  lua_pushfstring (L, "Channel (%p)", *s);
  return 1;
}

static int channel_ptr(lua_State * L) {
	channel_t * s = luaL_checkudata (L, 1, CLP_CHANNEL_METATABLE);
	lua_pushlightuserdata(L,*s);
	return 1;
}

int clp_pushevent(lua_State *L) {
	channel_t c = clp_tochannel(L,1);
   int top=lua_gettop(L);
  	_DEBUG("CHANNEL PUSH EVENT: %p %d\n",c,top);
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
   instance_t ins=NULL;
   event_t ev=clp_newevent(str,len);
  	lua_pushliteral(L,CLP_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);
   CHANNEL_LOCK(c);
   if(c->closed) {
  	   CHANNEL_UNLOCK(c);
  	   _DEBUG("channel closed\n");
  	   lua_pop(L,1);
   	lua_pushnil(L);
   	lua_pushliteral(L,"closed");
   	return 2;
   } if(c->waiting) {
  	   _DEBUG("Push: main process waiting\n");
	   c->waiting--;
   	c->event=ev;
	   SIGNAL_ONE(&c->cond);
  	   CHANNEL_UNLOCK(c);
  	   lua_pop(L,1);
   	lua_pushboolean(L,1);
      return 1;
   }
   if(clp_lfqueue_try_pop(c->read_queue,&ins)) {
  		CHANNEL_UNLOCK(c);
  	   _DEBUG("Push: got waiter\n");
   	lua_settop(ins->L,0);
   	ins->ev=ev;
		ins->state=I_READY;
		clp_pushinstance(ins);
  	   lua_pop(L,1);
		lua_pushboolean(L,1);
		return 1;
   } else if(clp_lfqueue_try_push(c->event_queue,&ev)) {
	   CHANNEL_UNLOCK(c);
  	   _DEBUG("Push: used event queue\n");
  	   lua_pop(L,1);
      lua_pushboolean(L,1);
      return 1;
   } else if(c->sync) {
	   ins=lua_touserdata(L,-1);
  	   _DEBUG("Push: channel is sync %p\n",ins);
		lua_pop(L,1);
  	   if(ins==NULL) {
	  	   MUTEX_LOCK(&c->mutex);
			waiting_event=ev;
			c->waiting++;
	  		CHANNEL_UNLOCK(c);
			SIGNAL_WAIT(&c->cond,&c->mutex,-1.0);
			MUTEX_UNLOCK(&c->mutex);
	   	lua_pushboolean(L,1);
			return 1;
  	   }
  	   _DEBUG("Push: waiting %p\n",ins);
		ins->ev=ev;
		ins->state=I_CHANNEL_WRITE;
		ins->chan=c;
		return lua_yield(L,0);
	}
	CHANNEL_UNLOCK(c);
   _DEBUG("async queue full\n");
   clp_destroyevent(ev);
   lua_pushnil(L);
   lua_pushliteral(L,"Event queue is full");
   return 2;
}

static int channel_getevent(lua_State *L) {
	channel_t c = clp_tochannel(L,1);
	event_t ev=NULL;
	_DEBUG("CHANNEL GET EVENT: %p\n",c);
	lua_pushliteral(L,CLP_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);
	instance_t i=NULL;
   CHANNEL_LOCK(c);
   if(c->waiting) {
	   _DEBUG("get: main process is waiting \n");
		c->waiting--;
		int n=clp_restoreevent(L,waiting_event);
		clp_destroyevent(waiting_event);
	   waiting_event=NULL;
	   SIGNAL_ONE(&c->cond);
  	   CHANNEL_UNLOCK(c);
  	   return n;
   }
	if(clp_lfqueue_try_pop(c->event_queue,&ev)) {
  	   _DEBUG("get: got from event queue \n");
		if(clp_lfqueue_try_pop(c->write_queue,&i)) {
	  	   _DEBUG("get: has writers, escalate its event\n");
			clp_lfqueue_try_push(c->event_queue,&i->ev);
	  		CHANNEL_UNLOCK(c);
			i->ev=NULL;
			i->state=I_RESUME_SUCCESS;
			clp_pushinstance(i);
			int n=clp_restoreevent(L,ev);
			clp_destroyevent(ev);
			return n;
		}
  		CHANNEL_UNLOCK(c);
  	   _DEBUG("get: event restored %p\n",ev);
		int n=clp_restoreevent(L,ev);
		clp_destroyevent(ev);
		return n;
	}
	if(c->closed) {
	   CHANNEL_UNLOCK(c);
	   luaL_error(L,"Channel was closed");
	   return 0;
	}
	if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA) {
  	   _DEBUG("get: main process, gotta wait :( %p\n",ev);
		MUTEX_LOCK(&c->mutex);
		c->waiting++;
  		CHANNEL_UNLOCK(c);
		SIGNAL_WAIT(&c->cond,&c->mutex,-1.0);
		int n=0;
		if(c->event) {
			n=clp_restoreevent(L,c->event);
			clp_destroyevent(c->event);
			c->event=NULL;
		}
		MUTEX_UNLOCK(&c->mutex);
		return n;
	}
	i=lua_touserdata(L,-1);
	lua_pop(L,1);
	i->state=I_CHANNEL_READ;
	i->chan=c;
	return lua_yield(L,0);
}

static const struct luaL_Reg ChannelMetaFunctions[] = {
		{"__eq",channel_eq},
		{"__tostring",channel_tostring},
		{"__id",channel_ptr},
		{"size",channel_getsize},
		{"setsize",channel_setsize},
		{"get",channel_getevent},
		{"close",channel_close},
		{"put",clp_pushevent},
		{NULL,NULL}
};

static void get_metatable(lua_State * L) {
	luaL_getmetatable(L,CLP_CHANNEL_METATABLE);
   if(lua_isnil(L,-1)) {
   	lua_pop(L,1);
  		luaL_newmetatable(L,CLP_CHANNEL_METATABLE);
  		#if LUA_VERSION_NUM < 502
			luaL_register(L, NULL, ChannelMetaFunctions);
		#else
			luaL_setfuncs(L, ChannelMetaFunctions, 0);
		#endif 
  		lua_pushvalue(L,-1);
  		lua_setfield(L,-2,"__index");
		luaL_loadstring(L,"local id=(...):__id() return function() return require'clp.channel'.get(id) end");
		lua_setfield (L, -2,"__wrap");
  	}
}

void clp_pushchannel(lua_State * L,channel_t t) {
	channel_getcached(L,t);
	if(lua_type(L,-1)==LUA_TUSERDATA) 
		return;
	lua_pop(L,1);
	channel_t *s=lua_newuserdata(L,sizeof(channel_t *));
	*s=t;
	get_metatable(L);
   lua_setmetatable(L,-2);
   channel_putcache(L,t);
}

int clp_channelnew(lua_State *L) {
	channel_t t=malloc(sizeof(struct channel_s));
	int size=luaL_optint(L, 1, -1);
	int sync=1;
	if(lua_type(L,2)==LUA_TBOOLEAN) {
		sync=!lua_toboolean(L, 2);
	}
	SIGNAL_INIT(&t->cond);
	MUTEX_INIT(&t->mutex);
	t->waiting=0;
	t->event=NULL;
	t->lock=0;
	t->closed=0;
	t->event_queue=clp_lfqueue_new();
	t->read_queue=clp_lfqueue_new();
	t->write_queue=clp_lfqueue_new();
	t->sync=sync;
	clp_lfqueue_setcapacity(t->event_queue,size);
	clp_lfqueue_setcapacity(t->read_queue,-1);
	clp_pushchannel(L,t);
   return 1;
}

static int channel_get(lua_State * L) {
	channel_t s=lua_touserdata(L,1);
	clp_pushchannel(L,s);
	return 1;
}

static const struct luaL_Reg LuaExportFunctions[] = {
		{"new",clp_channelnew},
		{"get",channel_get},
		{NULL,NULL}
};

CLP_EXPORTAPI	int luaopen_clp_channel(lua_State *L) {
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'clp.channel' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};


