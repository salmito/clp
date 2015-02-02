///
// Auxiliary event subsystem
//
// Runs an event loop for dealing with asynchronous IO calls.
//
// @module event
// @author Tiago Salmito
// @license MIT
// @copyright Tiago Salmito - 201
//
#include "clp.h"
#include "event.h"
#include "process.h"
#include "threading.h"
#include "scheduler.h"
#include "marshal.h"

#include <string.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <unistd.h>


#ifdef _WIN32
#define usleep(a) Sleep(a / 1000)
#endif

static THREAD_T * event_thread;
static struct event_base *loop;

event_t clp_newevent(const char * ev, size_t len) {
	event_t e=malloc(sizeof(struct event_s));
	e->data=malloc(len);
	memcpy(e->data,ev,len);
	e->len=len;
	return e;
}

void clp_destroyevent(event_t e) {
	free(e->data);
	free(e);
}

static void dummy_event(evutil_socket_t fd, short events, void *arg) {}

static void io_ready(evutil_socket_t fd, short event, void *arg) {
	instance_t i=(instance_t)arg;
	if(i->state==I_SLEEP)
		i->state=I_RESUME_SUCCESS;
	else if(event&EV_TIMEOUT) 
		i->state=I_RESUME_FAIL;
	clp_pushinstance(i);
}

static int event_wait_io(lua_State * L) {
	int fd=-1;
	fd=lua_tointeger(L,1);
	int mode=-1;
	mode=lua_tointeger(L,2);
	int m=0;
	if(mode==0) 
		m = EV_READ; // read
	else if(mode==1)
		m = EV_WRITE; //write
	else luaL_error(L,"Invalid io operation type (0=read and 1=write)");
	double time=0.0l;  
	if(lua_type(L,3)==LUA_TNUMBER) {
		time=lua_tonumber(L,3);
	}
	lua_pushliteral(L,CLP_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA) luaL_error(L,"Cannot wait outside of an instance");
	instance_t i=lua_touserdata(L,-1);
	i->state=I_RESUME_SUCCESS;
	if(time>=0.0) {
		struct timeval to={time,(((double)time-((int)time))*1000000.0L)};
		event_base_once(loop, fd, m, io_ready, i, &to);
	} else {
		event_base_once(loop, fd, m, io_ready, i, NULL);
	}
	return lua_yield(L,0);
}

///
// Put current process to sleep for a specified time.
//
// If the current process is the main process, it will be
// blocked while sleeping.
//
// Else, the process yields their current thread during the 
// specified time.
// 
// @number t The amount of time to sleep, in seconds
// @raise Error if `t` is negative
// @function sleep
static int event_sleep(lua_State *L) {
	if(lua_gettop(L)>1) luaL_error(L,"Sleep must have at most one parameter");
	if(lua_type(L,1)!=LUA_TNUMBER) luaL_error(L,"Sleep parameter must be a number");
	lua_Number time=lua_tonumber(L,1);
	if(time<0) luaL_error(L,"Sleep time must be zero or positive");
	lua_pushliteral(L,CLP_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA) {
		usleep(time*1000000.0);
		lua_pop(L,1);
		return 0;
	}
	instance_t i=lua_touserdata(L,-1);
	lua_pop(L,1);
	i->state=I_SLEEP;
	return lua_yield(L,1);
}

void clp_sleepevent(instance_t i) {
	double time=lua_tonumber(i->L,1);
	lua_pop(i->L,1);
	struct timeval to={time,(((double)time-((int)time))*1000000.0L)};
	event_base_once(loop,-1,EV_TIMEOUT,io_ready,i,&to);
}


static THREAD_RETURN_T THREAD_CALLCONV event_main(void *t_val) {
	if(!loop) return NULL;
	struct event *listener_event = event_new(loop, -1, EV_READ|EV_PERSIST, dummy_event, NULL);
	event_add(listener_event, NULL);
	event_base_dispatch(loop);
	return NULL;
}

int clp_restoreevent(lua_State *L,event_t ev) {
	lua_pushcfunction(L,mar_decode);
	lua_pushlstring(L,ev->data,ev->len);
	lua_call(L,1,1);
	int top=lua_gettop(L);
	int n=
#if LUA_VERSION_NUM < 502
		luaL_getn(L,-1);
#else
		luaL_len(L,-1);
#endif
	int j;
	for(j=1;j<=n;j++) lua_rawgeti(L,top,j);
	lua_remove(L,top);
	return n;
}

///
// Serializes lua values to a buffer.
//
// Serializes tables, which may contain cycles, Lua functions
// with upvalues and basic data types.
//
// Note: Current implementation uses lua-marshal 
// (https://github.com/richardhundt/lua-marshal)
//
// @param v value to serialize
// @treturn string A buffer with serialized values
// @function encode

///
// Deserialize buffers to their correspondent lua value
//
// Note: Current implementation uses lua-marshal 
// (https://github.com/richardhundt/lua-marshal)
//
// @string buffer the buffer
//
// @return the value
// @function decode

CLP_EXPORTAPI	int luaopen_clp_event(lua_State *L) {
	const struct luaL_Reg LuaExportFunctions[] = {
		{"encode",mar_encode},
		{"decode",mar_decode},
		{"waitfd",event_wait_io},
		{"sleep",event_sleep},
		{NULL,NULL}
	};
	if(!event_thread) {
		event_thread=malloc(sizeof(THREAD_T));
#ifndef _WIN32
		evthread_use_pthreads();
#endif
		loop = event_base_new();
		THREAD_CREATE(event_thread, &event_main, NULL, 0);
	}
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'clp.event' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
	lua_pushliteral(L,"READ");
	lua_pushnumber(L,0);
	lua_settable(L,-3);
	lua_pushliteral(L,"WRITE");
	lua_pushnumber(L,1);
	lua_settable(L,-3);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};
