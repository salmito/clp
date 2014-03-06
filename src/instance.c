#include "instance.h"
#include "marshal.h"
#include "scheduler.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

void lstage_initinstance(instance_t i) {
	_DEBUG("Initiating instance %p\n",i);
	lua_State *L=i->L;
	lua_pushcfunction(L,luaopen_base);
   lua_pcall(L,0,0,0);
   lua_pushcfunction(L,luaopen_package);
   lua_pcall(L,0,1,0);
   lua_getfield(L,-1,"preload");
   lua_pushcfunction(L,luaopen_io);
   lua_setfield(L,-2,"io");
   lua_pushcfunction(L,luaopen_os);
   lua_setfield(L,-2,"os");
   lua_pushcfunction(L,luaopen_table);
   lua_setfield(L,-2,"table");
   lua_pushcfunction(L,luaopen_string);
   lua_setfield(L,-2,"string");
   lua_pushcfunction(L,luaopen_math);
   lua_setfield(L,-2,"math");
   lua_pushcfunction(L,luaopen_debug);
   lua_setfield(L,-2,"debug");
   #if LUA_VERSION_NUM > 501
   lua_pushcfunction(L,luaopen_coroutine);
   lua_setfield(L,-2,"coroutine");
	#endif
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
	lstage_buildstage(L,i->stage);
	lua_setglobal(L,"self");
	lua_pushliteral(L,LSTAGE_INSTANCE_KEY);
	lua_pushlightuserdata(L,i);
	lua_settable(L, LUA_REGISTRYINDEX);	
	i->flags=READY;
}

instance_t lstage_newinstance(stage_t s) {
   lua_State * L = luaL_newstate();
	instance_t i=malloc(sizeof(struct instance_s));
	i->L=L;
	i->stage=s;
	i->flags=CREATED;
	i->waiting=NULL;
	i->ev=NULL;
	lstage_pushinstance(i);
	return i;
}

void lstage_putinstance(instance_t i) {
	event_t ev=NULL;
	_DEBUG("Putting instance %p\n",i);
		//TODO maybe put a spinlock here
	if(lstage_lfqueue_try_pop(i->stage->event_queue,(void **)&ev)) {
		_DEBUG("HAS event %p\n",i);
		i->ev=ev;
		i->flags=READY;
		return lstage_pushinstance(i);
	}
	i->flags=IDLE;
	_DEBUG("instance is now IDLE %p (%u)\n",i,lstage_lfqueue_size(i->stage->instances));
	if(!lstage_lfqueue_try_push(i->stage->instances,(void **) &i)) {
		_DEBUG("Instances FULL, destroying %p\n",i);
		lstage_destroyinstance(i);
	}
}

void lstage_destroyinstance(instance_t i) {
   lua_close(i->L);
   if(i->ev) lstage_destroyevent(i->ev);
   free(i);
}
