#include <unistd.h> 
#include <errno.h>
#include <string.h>  

#include "clp.h"
#include "marshal.h"
#include "task.h"
#include "pool.h"
#include "threading.h"


//can be found here  http://www.lua.org/pil/24.2.3.html
void stackDump (lua_State *L, const char *text) {
      int i;
      int top = lua_gettop(L);
	  if (text == NULL)
		printf("--------Start Dump------------\n");
	  else
	    printf("--------Start %s------------\n", text);
      for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {
    
          case LUA_TSTRING:  /* strings */
            printf("`%s'", lua_tostring(L, i));
            break;
    
          case LUA_TBOOLEAN:  /* booleans */
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;
    
          case LUA_TNUMBER:  /* numbers */
            printf("%g", lua_tonumber(L, i));
            break;
    
          default:  /* other values */
            printf("%s", lua_typename(L, t));
            break;
    
        }
        printf("  ");  /* put a separator */
      }
      printf("\n");  /* end the listing */
	  printf("--------End Dump------------\n");
    }
#ifdef DEBUG
void tableDump(lua_State *L, int idx, const char* text)
{
	lua_pushvalue(L, idx);		// copy target table
	lua_pushnil(L);
	  if (text == NULL)
		printf("--------Table Dump------------\n");
	  else
	    printf("--------Table dump: %s------------\n", text);
	while (lua_next(L,-2) != 0) {
		printf("%s - %s\n",
			lua_typename(L, lua_type(L, -2)),
			lua_typename(L, lua_type(L, -1)));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);	// remove table copy
    printf("--------End Table dump------------\n");
}
#endif

static int clp_version(lua_State * L) {
	lua_pushliteral(L,CLP_VERSION);
	return 1;
}

static int clp_gettime(lua_State * L) {
   lua_pushnumber(L,now_secs());
   return 1;
}


static int clp_getself(lua_State *L) {
	lua_pushliteral(L,CLP_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);	
	if(!lua_isnil(L,-1)) {
		instance_t i=lua_touserdata(L,-1);
		lua_pop(L,1);
		clp_buildtask(L,i->stage);
	}
	return 1;
}

static int get_cpus() {
   #ifdef _WIN32
      #ifndef _SC_NPROCESSORS_ONLN
         SYSTEM_INFO info;
         GetSystemInfo(&info);
         #define sysconf(a) info.dwNumberOfProcessors
         #define _SC_NPROCESSORS_ONLN
      #endif
   #endif
   #ifdef _SC_NPROCESSORS_ONLN
	   long nprocs = -1;
      nprocs = sysconf(_SC_NPROCESSORS_ONLN);
      if (nprocs >= 1) 
      	return nprocs;
   #endif
   return 0;
}

static int clp_setmetatable(lua_State *L) {
   lua_pushvalue(L,2);
   lua_setmetatable(L,1);
   lua_pushvalue(L,1);
   return 1;
}

static int clp_getmetatable(lua_State *L) {
   if(lua_type(L,1)==LUA_TSTRING) {
      const char *tname=lua_tostring(L,1);
      luaL_getmetatable(L,tname);
   } else {
      lua_getmetatable (L,1);
   }
   return 1;
}


static int clp_cpus(lua_State *L) {
   lua_pushnumber(L,get_cpus());
   return 1;
}

static void clp_require(lua_State *L, const char *lib, lua_CFunction func) {
#if LUA_VERSION_NUM < 502 
	lua_getglobal(L,"require");
	lua_pushstring(L, lib);
	lua_call(L,1,1);
#else
 	luaL_requiref(L, lib, func, 0); 
#endif
}

CLP_EXPORTAPI int luaopen_clp_event(lua_State *L);
CLP_EXPORTAPI int luaopen_clp_scheduler(lua_State *L);
CLP_EXPORTAPI int luaopen_clp_task(lua_State *L);
CLP_EXPORTAPI int luaopen_clp_pool(lua_State *L);
CLP_EXPORTAPI int luaopen_clp_channel(lua_State *L);

static const struct luaL_Reg LuaExportFunctions[] = {
	{"_VERSION",clp_version},
	{"now",clp_gettime},
	{"cpus",clp_cpus},
	{"getmetatable",clp_getmetatable},
	{"setmetatable",clp_setmetatable},
	{"self",clp_getself},
	{NULL,NULL}
	};

pool_t clp_defaultpool=NULL;

CLP_EXPORTAPI int luaopen_clp(lua_State *L) {
	lua_newtable(L);
	clp_require(L,"clp.pool",luaopen_clp_pool);	
	lua_getfield(L,-1,"new");
	if(!clp_defaultpool) {
		lua_pushvalue(L,-1);
		lua_call(L,0,1);
		clp_defaultpool=clp_topool(L,-1);
		lua_getfield(L,-1,"add");
		lua_pushvalue(L,-2);
		lua_call(L,1,0);
	} else {
		clp_buildpool(L,clp_defaultpool);
	}
	lua_setfield(L,-4,"pool");
	//lua_setfield(L,-3,"pool");
	lua_pop(L,2);
	clp_require(L,"clp.event",luaopen_clp_event);
	lua_getfield(L,-1,"encode");
	lua_setfield(L,-3,"encode");
	lua_getfield(L,-1,"decode");
	lua_setfield(L,-3,"decode");
	lua_setfield(L,-2,"event");
	clp_require(L,"clp.scheduler",luaopen_clp_scheduler);
	lua_setfield(L,-2,"scheduler");
	clp_require(L,"clp.channel",luaopen_clp_channel);
	lua_getfield(L,-1,"new");
	lua_setfield(L,-3,"channel");
	lua_pop(L,1);
	clp_require(L,"clp.task",luaopen_clp_task);
	lua_getfield(L,-1,"new");
	lua_setfield(L,-3,"task");
	lua_pop(L,1);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'clp' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif
	return 1;
};

