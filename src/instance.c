#include "instance.h"
#include "marshal.h"
#include "scheduler.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static const struct luaL_Reg InstanceLibs[] = {
		{"io",luaopen_io},
		{"os",luaopen_os},
		{"table",luaopen_table},
		{"string",luaopen_string},
		{"math",luaopen_math},
		{"debug",luaopen_debug},
 	#if LUA_VERSION_NUM > 501
		{"coroutine",luaopen_coroutine},
 	#endif
		{NULL,NULL}
};

void lstage_initinstance(instance_t i) {
	_DEBUG("Initiating instance %p\n",i);
	lua_State *L=i->L;
	lua_pushcfunction(L,luaopen_base);
   lua_pcall(L,0,0,0);
   lua_pushcfunction(L,luaopen_package);
   lua_pcall(L,0,1,0);
   lua_getfield(L,-1,"preload");
	LUA_REGISTER(L,InstanceLibs);
   lua_pop(L,2);
//	luaL_openlibs(L);
	lua_pushliteral(L,STAGE_HANDLER_KEY);
	luaL_loadstring(L,"local h=(...) "
	                  "local c=require'coroutine' "
	                  "local f=function() while true do h(c.yield()) end end "
	                  "local r=c.wrap(f) r() return r");
	lua_pushcfunction(L,mar_decode);
	lua_pushlstring(L,i->stage->env,i->stage->env_len);
	lua_call(L,1,1);
	lua_call(L,1,1);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_pushliteral(L,LSTAGE_INSTANCE_KEY);
	lua_pushlightuserdata(L,i);
	lua_settable(L, LUA_REGISTRYINDEX);	
	i->flags=I_READY;
}

instance_t lstage_newinstance(stage_t s) {
   lua_State * L = luaL_newstate();
	instance_t i=malloc(sizeof(struct instance_s));
	i->L=L;
	i->stage=s;
	i->flags=I_CREATED;
	i->ev=NULL;
	lstage_pushinstance(i);
	return i;
}

void lstage_putinstance(instance_t i) {
	event_t ev=NULL;
	_DEBUG("Putting instance %p\n",i);
		//TODO maybe put a spinlock here
	if(lstage_lfqueue_try_pop(i->stage->event_queue,&ev)) {
		_DEBUG("HAS event %p\n",i);
		i->ev=ev;
		i->flags=I_READY;
		return lstage_pushinstance(i);
	}
	i->flags=I_IDLE;
	_DEBUG("instance is now I_IDLE %p lua_State %p (%u)\n",i,i->L,lstage_lfqueue_size(i->stage->instances));
	if(!lstage_lfqueue_try_push(i->stage->instances,&i)) {
		_DEBUG("Instances FULL, destroying %p\n",i);
		lstage_destroyinstance(i);
	}
}

void lstage_destroyinstance(instance_t i) {
   lua_close(i->L);
   if(i->ev) lstage_destroyevent(i->ev);
   free(i);
}
