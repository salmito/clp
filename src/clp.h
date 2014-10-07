/*Adapted from https://github.com/Tieske/Lua_library_template/*/

/*
** ===============================================================
** Leda is a parallel and concurrent framework for Lua.
** Copyright 2014: Tiago Salmito
** License MIT
** ===============================================================
*/

#ifndef _CLP_H
#define _CLP_H

#define CLP_VERSION "1.0.0-beta"

#include <lua.h>
#include <lauxlib.h>

#ifdef DEBUG
void stackDump (lua_State *L, const char *text);
void tableDump(lua_State *L, int idx, const char* text);
#define _DEBUG(...) fprintf(stderr,"%s:%d (%s):",__FILE__,__LINE__,__func__); fprintf(stderr,__VA_ARGS__); 
#else
#define _DEBUG(...)
//#define stackDump(...) 
#define tableDump(...) 
#endif

#ifndef CLP_EXPORTAPI
        #ifdef _WIN32
                #define CLP_EXPORTAPI __declspec(dllexport)
        #else
                #define CLP_EXPORTAPI extern
        #endif
#endif  

#define CLP_TASK_METATABLE "clp-Task *"
#define CLP_POOL_METATABLE "clp-Pool *"
#define CLP_THREAD_METATABLE "clp-Thread *"
#define CLP_CHANNEL_METATABLE "clp-Channel *"
#if LUA_VERSION_NUM < 502
	#define LUA_REGISTER(L,f) luaL_register(L, NULL, f)
#else
	#define LUA_REGISTER(L,f) luaL_setfuncs(L, f, 0)
#endif 

#endif
