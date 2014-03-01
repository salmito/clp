#include "channel.h"
#include "instance.h"
#include "event.h"
#include "scheduler.h"
#include "marshal.h"
#include <stdlib.h>

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
   instance_t ins=NULL;
   if(lstage_lfqueue_trypop(c->wait_queue,ins)) {
   	ins->ev=ev;
		ins->flags=READY;
		lstage_pushinstance(ins);
		lua_pushboolean(L,1);
		return 1;
   } else if(lstage_lfqueue_trypush(c->event_queue,ev)) {
      lua_pushboolean(L,1);
      return 1;
   } 
   lstage_destroyevent(ev);
   lua_pushnil(L);
   lua_pushliteral(L,"Event queue is full");
   return 2;

}


static int channel_getevent(lua_State *L) {
	channel_t c = lstage_tochannel(L,1);
	event_t ev=NULL;
	
	lua_pushliteral(L,LSTAGE_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA) luaL_error(L,"Cannot wait outside of a stage (yet)");
	instance_t i=lua_touserdata(L,-1);
	lua_pop(L,1);
	
	if(lstage_lfqueue_try_pop(c->event_queue,(void **)&ev)) {
		i->ev=ev;
		i->flags=READY;
		int y=lua_yield(L,0);
		lstage_pushinstance(i);
		return y; 
	}
	
	i->flags=WAITING_EVENT;
	if(!lstage_lfqueue_trypush(c->wait_queue,i)) {
		return 0;
	}
	return lua_yield(L,0);
}


static const struct luaL_Reg ChannelMetaFunctions[] = {
		{"__eq",channel_eq},
		{"__tostring",channel_tostring},
		{"id",channel_ptr},
		{"getsize",channel_getsize},
		{"setsize",channel_setsize},
		{"get",channel_getevent},
		{"push",channel_pushevent},
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
	SIGNAL_INIT(&t->cond);
	MUTEX_INIT(&t->mutex);
	t->event_queue=lstage_lfqueue_new();
	t->wait_queue=lstage_lfqueue_new();
	lstage_lfqueue_setcapacity(t->event_queue,-1);
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
//	if(!H) H=qt_hash_create();
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


