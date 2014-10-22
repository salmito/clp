#include "channel.h"
#include "task.h"
#include "event.h"
#include "scheduler.h"
#include "marshal.h"
#include <stdlib.h>

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
   _DEBUG("Close: lock %p\n",c);
   CHANNEL_LOCK(c);
  
   if(c->read_wait)
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
	if(c->read_wait) {
	  	c->read_event=NULL;

	   SIGNAL_ONE(&c->read_cond);
	}
	if(c->read_wait) {
	  	c->read_event=NULL;
	   SIGNAL_ONE(&c->read_cond);
	}
   _DEBUG("Close: unlock %p\n",c);
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
	_DEBUG("Push: called %p top=%d\n",c,top)
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

  	_DEBUG("push: lock %p\n",c);
	CHANNEL_LOCK(c);
	
	if(c->closed) { //Channel is closed
	_DEBUG("push: unlock %p\n",c);
  	   CHANNEL_UNLOCK(c);
  	   clp_destroyevent(ev);
  	   _DEBUG("Push: Channel is closed %p\n",c);
  	   lua_pop(L,1);
   	lua_pushnil(L);
   	lua_pushliteral(L,"Channel is closed");
   	return 2;
   }
   
   if(c->read_wait) {
  	   _DEBUG("Push: main process is getter %p\n",c);
	   c->read_wait=0;
   	c->read_event=ev;
  	   CHANNEL_UNLOCK(c);
   	MUTEX_LOCK(&c->mutex);
	   SIGNAL_ONE(&c->read_cond);
	   MUTEX_UNLOCK(&c->mutex);
   	_DEBUG("push: unlock %p\n",c);
  	   lua_pop(L,1);
   	lua_pushboolean(L,1);
      return 1;
   }
   
   if(clp_lfqueue_try_pop(c->read_queue,&ins)) {
   	_DEBUG("push: unlock %p\n",c);  		
   	CHANNEL_UNLOCK(c);
 	   _DEBUG("Push: got waiting reader for channel %p\n",c);
   	lua_settop(ins->L,0);
   	ins->ev=ev;
		ins->state=I_READY;
		clp_pushinstance(ins);
  	   lua_pop(L,1);
		lua_pushboolean(L,1);
		return 1;
   }
   
   //no one is waiting, push to event queue
   if(clp_lfqueue_try_push(c->event_queue,&ev)) {
   	_DEBUG("push: unlock %p\n",c);
	   CHANNEL_UNLOCK(c);
  	   _DEBUG("Push: Nobody is waiting, used event queue %p\n",c);
  	   lua_pop(L,1);
      lua_pushboolean(L,1);
      return 1;
   }
   //Cannot push on event queue, wait for a get
   
   if(c->sync) { //channel is sync
	   ins=lua_touserdata(L,-1);
  	   _DEBUG("Push: channel is sync %p\n",c);
		lua_pop(L,1);
  	   if(ins==NULL) { //i am the main process, wait for get
			c->write_event=ev;
			c->write_wait=1;
			_DEBUG("push: unlock %p\n",c);  		
	  		CHANNEL_UNLOCK(c);
	  	   MUTEX_LOCK(&c->mutex);
			SIGNAL_WAIT(&c->write_cond,&c->mutex,-1.0);
			MUTEX_UNLOCK(&c->mutex);  		
	   	lua_pushboolean(L,1);
			return 1;
  	   } //i am a regular task, yield to wait for a read
  	   _DEBUG("Push: waiting %p\n",c);
		ins->ev=ev;
		ins->state=I_CHANNEL_WRITE;
		ins->chan=c;
		return lua_yield(L,0);
	}

	//channel is asynchronous and the event queue is full
	CHANNEL_UNLOCK(c);
   _DEBUG("async queue full %p\n",c);
   clp_destroyevent(ev);
   lua_pushnil(L);
   lua_pushliteral(L,"Channel is full");
   return 2;
}

static int channel_getevent(lua_State *L) {
	channel_t c = clp_tochannel(L,1);
	event_t ev=NULL;
	_DEBUG("Get: called: %p\n",c);
	
	lua_pushliteral(L,CLP_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);
	instance_t i=NULL;
	
	_DEBUG("get: lock %p\n",c);
   CHANNEL_LOCK(c);
	
   if(c->write_wait) { //get the event from the main process
	   _DEBUG("get: Main process is waiting for write %p\n",c);
		lua_pop(L,1);
		c->write_wait=0;
		int n=clp_restoreevent(L,c->write_event);
		clp_destroyevent(c->write_event);
	   c->write_event=NULL;
 	   CHANNEL_UNLOCK(c);
     	MUTEX_LOCK(&c->mutex);
	   SIGNAL_ONE(&c->write_cond);
     	MUTEX_UNLOCK(&c->mutex);
   	_DEBUG("get: unlock %p\n",c);
  	   return n;
   }
   
   //Check if has any event
	if(clp_lfqueue_try_pop(c->event_queue,&ev)) {
  	   _DEBUG("get: got from event queue %p\n",c);
		lua_pop(L,1);
  	   //We just popped an event, push a waiting write for that slot  	   
		if(clp_lfqueue_try_pop(c->write_queue,&i)) {
	  	   _DEBUG("get: has writers, escalate its event %p\n",c);
			if(clp_lfqueue_try_push(c->event_queue,&i->ev)) {
		   	_DEBUG("get: unlock %p\n",c);
		  		CHANNEL_UNLOCK(c);
				i->ev=NULL;				
				i->state=I_RESUME_SUCCESS;
				clp_pushinstance(i);
			} else {
				(void)clp_lfqueue_try_push(c->write_queue,&i);
		   	_DEBUG("get: unlock %p\n",c);
		  		CHANNEL_UNLOCK(c);
			}
			int n=clp_restoreevent(L,ev);
			clp_destroyevent(ev);
			return n;
		}
   	_DEBUG("get: unlock %p\n",c);
  		CHANNEL_UNLOCK(c);
		int n=clp_restoreevent(L,ev);
		clp_destroyevent(ev);
		return n;
	}
	
	
	//Check if there are still any task waiting for writes (in case the channel is unbuffered)
	if(clp_lfqueue_try_pop(c->write_queue,&i)) {
	  	_DEBUG("get: still has writers, get its event %p\n",c);
   	_DEBUG("get: unlock %p\n",c);
		CHANNEL_UNLOCK(c);
		lua_pop(L,1);
		ev=i->ev;
		i->ev=NULL;
		i->state=I_RESUME_SUCCESS;
		clp_pushinstance(i);
		int n=clp_restoreevent(L,ev);
		clp_destroyevent(ev);
		return n;
	}
	
	if(c->closed) {
		_DEBUG("get: Channel is closed %p\n",c);
   	_DEBUG("get: unlock %p\n",c);
	   CHANNEL_UNLOCK(c);
  		lua_pop(L,1);
	   luaL_error(L,"Channel is closed");
	   return 0;
	}
	
	if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA) {
	  _DEBUG("get: i am the main process, gotta wait :( %p\n",c);
	  	lua_pop(L,1);
		c->read_wait=1;
  		CHANNEL_UNLOCK(c);
		MUTEX_LOCK(&c->mutex);
		SIGNAL_WAIT(&c->read_cond,&c->mutex,-1.0);
		int n=0;
		if(c->read_event) {
		  _DEBUG("get: main process got event :) %p\n",c);
			n=clp_restoreevent(L,c->read_event);
			clp_destroyevent(c->read_event);
			c->read_event=NULL;
		} else {
			MUTEX_UNLOCK(&c->mutex);
	   	_DEBUG("get: unlock %p\n",c);
			luaL_error(L,"Channel is closed");
			return 0;
		}
   	_DEBUG("get: unlock %p\n",c);
		MUTEX_UNLOCK(&c->mutex);
		return n;
	}
	_DEBUG("get: Yield to wait for write %p\n",c);
	//Yield to wait for write
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
	SIGNAL_INIT(&t->read_cond);
	SIGNAL_INIT(&t->write_cond);
	MUTEX_INIT(&t->mutex);
	t->read_wait=0;
	t->write_wait=0;
	t->read_event=NULL;
	t->write_event=NULL;
	t->lock=0;
	t->closed=0;
	t->event_queue=clp_lfqueue_new();
	t->read_queue=clp_lfqueue_new();
	t->write_queue=clp_lfqueue_new();
	t->sync=sync;
	clp_lfqueue_setcapacity(t->event_queue,size);
	clp_lfqueue_setcapacity(t->read_queue,-1);
	clp_lfqueue_setcapacity(t->write_queue,-1);
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


