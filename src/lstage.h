/*Adapted from https://github.com/Tieske/Lua_library_template/*/

/*
** ===============================================================
** Leda is a parallel and concurrent framework for Lua.
** Copyright 2014: Tiago Salmito
** License MIT
** ===============================================================
*/

#ifndef _LSTAGE_H
#define _LSTAGE_H

#define LSTAGE_VERSION "1.0.0-beta"

#include <lua.h>
#include <lauxlib.h>

#ifdef DEBUG
void stackDump (lua_State *L, const char *text);
void tableDump(lua_State *L, int idx, const char* text);
#define _DEBUG(...) //fprintf(stderr,"%s:%d (%s):",__FILE__,__LINE__,__func__); fprintf(stderr,__VA_ARGS__); 
#else
#define _DEBUG(...) 
#define stackDump(...) 
#define tableDump(...) 
#endif

#ifndef LSTAGE_EXPORTAPI
        #ifdef _WIN32
                #define LSTAGE_EXPORTAPI __declspec(dllexport)
        #else
                #define LSTAGE_EXPORTAPI extern
        #endif
#endif  

#define LSTAGE_STAGE_METATABLE "lstage-Stage *"
#define LSTAGE_POOL_METATABLE "lstage-Pool *"
#define LSTAGE_THREAD_METATABLE "lstage-Thread *"
#define LSTAGE_CHANNEL_METATABLE "lstage-Channel *"
#if LUA_VERSION_NUM < 502
	#define LUA_REGISTER(L,f) luaL_register(L, NULL, f)
#else
	#define LUA_REGISTER(L,f) luaL_setfuncs(L, f, 0)
#endif 

#endif
