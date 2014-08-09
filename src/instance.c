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
	_DEBUG("Initiating instance1 %p\n",i);
	lua_State *L=i->L;
		lua_pushliteral(L,LSTAGE_INSTANCE_KEY);
	lua_pushlightuserdata(L,i);
		_DEBUG("Initiating instance2 %p\n",i);
	lua_settable(L, LUA_REGISTRYINDEX);	
	lua_pushcfunction(L,luaopen_base);
		_DEBUG("Initiating instance3 %p\n",i);
   lua_pcall(L,0,0,0);
   lua_pushcfunction(L,luaopen_package);
   	_DEBUG("Initiating instance4 %p\n",i);
   lua_pcall(L,0,1,0);
     	_DEBUG("Initiating instance4.5 %p\n",i);
   lua_getfield(L,-1,"preload");
   	_DEBUG("Initiating instance5 %p\n",i);
	LUA_REGISTER(L,InstanceLibs);
   	_DEBUG("Initiating instance6 %p\n",i);
   lua_pop(L,2);
//	luaL_openlibs(L);
	lua_pushliteral(L,STAGE_HANDLER_KEY);
	_DEBUG("Initiating instance2 %p\n",i);
	luaL_loadstring(L,"local a={...} "
							"local h=a[1].f "
	                  "local i=a[2] "
	                  "return require'coroutine'.wrap(function() while true do h(i:get()) end end)"
	                  );
	
/*	luaL_loadstring(L,"local a={...} "
							"local h=a[1] "
	                  "local c=a[2] "
	                  "h.e=h.e or function(...) print(...) end "
                     "local d=require'coroutine' "
                  //   "print('instance',h,c) "
	                  "local f=function() while true do "
	                  "xpcall(h.f,h.e,c:get()) "
	                  "end end "
	                  "local r=d.wrap(f) return r");*/
	/*luaL_loadstring(L,//"local a={...} "
							"local h={...}[1] "
	                  "local i={...}[2] "
	                  "return function() while true do xpcall(h.f,h.e,i:get()) end end"
	                  "");*/
	lua_pushcfunction(L,mar_decode);
	lua_pushlstring(L,i->stage->env,i->stage->env_len);
	//stackDump(L,"init1");
	lua_call(L,1,1);
	lua_pushliteral(L,LSTAGE_ERRORFUNCTION_KEY);
	lua_getfield(L,-2,"e");
	lua_settable(L, LUA_REGISTRYINDEX);
	lstage_pushchannel(L,i->stage->input);
	//stackDump(L,"init2");
	lua_call(L,2,1);
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
	_DEBUG("Putting instance %p\n",i);
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
