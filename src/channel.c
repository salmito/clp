#include "channel.h"
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



static const struct luaL_Reg ChannelMetaFunctions[] = {
		{"__eq",channel_eq},
		{"__tostring",channel_tostring},
		{"ptr",channel_ptr},
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
		luaL_loadstring(L,"local ptr=(...):ptr() return function() return require'lstage.channel'.get(ptr) end");
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
	channel_build(L,t);
   return 1;
}

static int channel_get(lua_State * L) {
	channel_t s=lua_touserdata(L,1);
	if(s) {
		channel_build(L,s);
		return 1;
	}
	lua_pushnil(L);
	lua_pushliteral(L,"Channel is null");
	return 2;
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


