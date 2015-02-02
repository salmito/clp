/// 
// Process submodule.
//
// A process is a lightweight thread of execution with its own
// independent state.
//
// @module process
// @author Tiago Salmito
// @license MIT
// @copyright Tiago Salmito - 2014


#include "process.h"
#include "marshal.h"
#include "scheduler.h"

#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


#define DEFAULT_I_IDLE_CAPACITY 10
#define DEFAULT_QUEUE_CAPACITY -1
#define CLP_PROCESS_CACHE "clp-process-cache"


static void get_processmetatale (lua_State * L);
static int process_instantiate (lua_State * L);
static instance_t clp_newinstance(lua_State * L,process_t s);
extern pool_t clp_defaultpool;

process_t clp_toprocess (lua_State * L, int i) {
	process_t *s = luaL_checkudata (L, i, CLP_PROCESS_METATABLE);
	luaL_argcheck (L, s != NULL, i, "Process expected");
	return *s;
}

///
// Creates a new process.
//
// If called without parameters, creates a new empty process 
// without environment. It can be set later with 
// the @{process:wrap} function.
//
// If `f` is a function then it will be used as the new process environment.
// Any upvalues will be copied to the state of the new process.
//
// If `e` is a function then it will be used as the error 
// function of the new process. It will be called whenever the process 
// raise a error. The process is destroyed after it returns.
//
// All spawned processes share the same input channel.
//
// @tparam[opt] function f environment of the process.
// @tparam[optchain] function e error function of the process.
// @tparam[opt=1] number n number of processes to create.
// @return the new process
// @function new

///
// Tests whether the userdata is a process
// @tparam process p value to be tested
// @return true if it has the process type
//
// @function isprocess


///
// Our class. Any function or table in this section belongs to `process`
// @type process

///
// get the input channel of the process.
//
// @return the input channel
//
// @function input
static int process_input (lua_State * L) {
	process_t s = clp_toprocess (L, 1);
	clp_pushchannel(L,s->input);
	return 1;
}

///
// Changes the input channel of the process.
//
// @tparam channel c the new input channel
//
// @function setinput
static int process_setinput (lua_State * L) {
	process_t s = clp_toprocess (L, 1);
	channel_t c=clp_tochannel(L, 2);
	s->input=c;
	lua_pushvalue(L,1);
	return 1;
}

///
// Get the number of spawned processes.
//
// @return the number of process instances
//
// @function size
static int get_max_instances (lua_State * L) {
	process_t s = clp_toprocess (L, 1);
	lua_pushnumber (L, s->instances);
	return 1;
}

///
// Get the environment function of a process
//
// @treturn function the environment function
//
// @function env
static int process_getenv (lua_State * L) {
	lua_getfield(L, LUA_REGISTRYINDEX, CLP_HANDLER_KEY);
	if(lua_type(L,-1)!=LUA_TNIL) {
		return 1;
	}
	lua_pop(L,1);
	process_t s = clp_toprocess (L, 1);
	if (s->env == NULL){
		lua_pushnil (L);
	} else {
		lua_pushcfunction(L,mar_decode);
		lua_pushlstring (L, s->env, s->env_len);
		lua_call(L,1,1);
	}
	return 1;
}

static int process_eq (lua_State * L) {
	process_t s1 = clp_toprocess (L, 1);
	process_t s2 = clp_toprocess (L, 2);
	lua_pushboolean (L, s1 == s2);
	return 1;
}

///
// Wraps an empty process with an environment and error function.
// 
// A process can be wrapped only once.
//
// Has the same parameters of the @{new} function.
// 
// @tparam[opt] function f environment of the process.
// @tparam[optchain] function e error function of the process.
// @tparam[opt=1] number n number of processes to create.
//
// @function wrap
// @treturn process itself
// @see new
static int process_wrap (lua_State * L) {
	int top=lua_gettop(L);
	process_t s = clp_toprocess (L, 1);
	if (s->env != NULL)
		luaL_error (L, "Process already have a environment");

	luaL_checktype (L, 2, LUA_TFUNCTION);
	lua_pushcfunction (L, mar_encode);
	lua_newtable(L);
	lua_pushvalue (L, 2);
	lua_setfield(L,-2,"f");
	if(lua_type(L,3) == LUA_TFUNCTION && top>=3) {   
		lua_pushvalue (L, 3);
		lua_setfield(L,-2,"e");
	}
	lua_call (L, 1, 1);

	const char *env = NULL;
	size_t len = 0;
	env = lua_tolstring (L, -1, &len);
	char *envcp = malloc (len + 1);
	envcp[len] = '\0';
	memcpy (envcp, env, len + 1);
	s->env = envcp;
	s->env_len = len;
	lua_pop (L, 1);

	lua_pushcfunction (L, process_instantiate);
	lua_pushvalue (L, 1);
	lua_pushnumber (L, 1);
	lua_call (L, 2, 0);

	lua_pushvalue (L, 1);
	return 1;
}

static int process_tostring (lua_State * L) {
	process_t *s = luaL_checkudata (L, 1, CLP_PROCESS_METATABLE);
	lua_pushfstring (L, "Process (%p)", *s);
	return 1;
}


static int process_destroyinstances (lua_State * L) {
	process_t s = clp_toprocess (L, 1);
	int n = luaL_optint(L, 2, 0);
	if (n < 0)
		luaL_error (L, "Argument must be positive");
	if (n == 0) {
		lua_pushvalue(L,1);
		return 1;
	}

	MUTEX_LOCK(&s->intances_mutex);
	s->instances -= n;
	MUTEX_UNLOCK(&s->intances_mutex);

	lua_pushvalue(L,1);
	return 1;
}

///
// Create a new process from an existing one.
//
// All spawned processes share the same input channel.
// @int n number of new processes
// @treturn process itself
// @function spawn
static int process_instantiate (lua_State * L) {
	process_t s = clp_toprocess (L, 1);
	if (s->pool == NULL)
		luaL_error (L, "Process must be associated to a pool");
	if (s->env == NULL)
		luaL_error (L, "Process must have an environment");
	int n = lua_tointeger (L, 2);
	int i;
	if (n < 0)
		luaL_error (L, "Argument must be positive");
	if (n == 0) {
		lua_pushvalue(L,1);
		return 1;
	}

	MUTEX_LOCK(&s->intances_mutex);
	s->instances += n;
	for (i = 0; i < n; i++) {
		(void) clp_newinstance (L,s);
	}
	MUTEX_UNLOCK(&s->intances_mutex);


	/*unlock mutex */
	lua_pushvalue (L, 1);
	return 1;
}

static int process_ptr (lua_State * L) {
	process_t *s = luaL_checkudata (L, 1, CLP_PROCESS_METATABLE);
	lua_pushlightuserdata (L, *s);
	return 1;
}

static void process_getcached(lua_State * L, process_t t) {
	lua_pushliteral(L,CLP_PROCESS_CACHE);
	lua_gettable(L,LUA_REGISTRYINDEX);
	if(!lua_istable(L,-1)) {
		lua_pop(L,1);
		lua_newtable(L);
		lua_newtable(L);
		lua_pushliteral(L,"v");
		lua_setfield(L,-2,"__mode");
		lua_setmetatable(L,-2);
		lua_pushliteral(L,CLP_PROCESS_CACHE);
		lua_pushvalue(L,-2);
		lua_settable(L,LUA_REGISTRYINDEX);
	}
	lua_pushlightuserdata(L,t);
	lua_gettable(L,-2);
	lua_remove(L,-2);
}

static void process_putcache(lua_State * L, process_t t) {
	lua_pushliteral(L,CLP_PROCESS_CACHE);
	lua_gettable(L,LUA_REGISTRYINDEX);
	if(!lua_isuserdata(L,-1)) {
		lua_pop(L,1);
		lua_newtable(L);
		lua_newtable(L);
		lua_pushliteral(L,"v");
		lua_setfield(L,-2,"__mode");
		lua_setmetatable(L,-2);
		lua_pushliteral(L,CLP_PROCESS_CACHE);
		lua_pushvalue(L,-2);
		lua_settable(L,LUA_REGISTRYINDEX);
	}
	lua_pushlightuserdata(L,t);
	lua_pushvalue(L,-3);
	lua_settable(L,-3);
	lua_pop(L,1);
}

void clp_buildprocess (lua_State * L, process_t t) {
	_DEBUG("Building process %p\n",t);
	process_getcached(L,t);
	if(lua_type(L,-1)==LUA_TUSERDATA) 
		return;
	lua_pop(L,1);
	process_t *s = lua_newuserdata (L, sizeof (process_t *));
	*s = t;
	get_processmetatale (L);
	lua_setmetatable (L, -2);
	process_putcache(L,t);
	_DEBUG("Created userdata %p\n",t);

}

///
// Get the current pool associated to the process
//
// @treturn pool the pool of the process
// @function pool
static int process_getpool (lua_State * L) {
	if(lua_gettop(L)>1) {
		luaL_error(L,"To many arguments");
	}

	process_t s = clp_toprocess (L, 1);
	if (s->pool)
		clp_buildpool (L, s->pool);
	else
		lua_pushnil (L);
	return 1;
}
///
// Associate a pool to the process
//
// @tparam pool the pool to be associated
// @treturn process itself
// @function pool
static int process_setpool (lua_State * L) {
	process_t s = clp_toprocess (L, 1);
	if(s->env!=NULL) {
		luaL_error(L,"Cannot set a pool for a process in execution");
	}
	pool_t p = clp_topool (L, 2);
	s->pool = p;
	lua_pop(L,1);
	return 1;
}

///
// Get the parent of the current process
//
// @treturn[1] process the parent process
// @treturn[2] nil if the process has no parent.
// @function parent
static int process_getparent (lua_State * L) {
	process_t s = clp_toprocess (L, 1);
	if (s->parent)
		clp_buildprocess (L, s->parent);
	else
		lua_pushnil (L);
	return 1;
}

static int process_gc (lua_State * L) {
	//	process_t s = clp_toprocess (L, 1);
	//	_DEBUG("Destroying process %p\n",s);
	return 0;
}


static const struct luaL_Reg ProcessMetaFunctions[] = {
	{"__eq", process_eq},
	{"__gc", process_gc},
	{"__tostring", process_tostring},
	//	{"__call", process_push},
	{"__id", process_ptr},
	{"size", get_max_instances},
	{"env", process_getenv},
	{"wrap", process_wrap},
	{"input", process_input},
	{"setinput", process_setinput},
	{"spawn", process_instantiate},
	{"remove", process_destroyinstances},
	{"parent", process_getparent},
	{"pool", process_getpool},
	{"setpool", process_setpool},
	{NULL, NULL}
};

static void get_processmetatale (lua_State * L) {
	luaL_getmetatable (L, CLP_PROCESS_METATABLE);
	if (lua_isnil (L, -1))
	{
		lua_pop (L, 1);
		luaL_newmetatable (L, CLP_PROCESS_METATABLE);
		LUA_REGISTER (L, ProcessMetaFunctions);
		lua_pushvalue (L, -1);
		lua_setfield (L, -2, "__index");
		luaL_loadstring (L,
				"local t=(...) assert(t:input():put(select(2,...))) return t");
		lua_setfield (L, -2, "__call");				   
		luaL_loadstring (L,
				"local ptr=(...):__id() return function() return require'clp.process'.get(ptr) end");
		lua_setfield (L, -2, "__wrap");
	}
}


static int process_isprocess (lua_State * L) {
	lua_getmetatable (L, 1);
	get_processmetatale (L);
	int has = 0;
#if LUA_VERSION_NUM > 501
	if (lua_compare (L, -1, -2, LUA_OPEQ))
		has = 1;
#else
	if (lua_equal (L, -1, -2))
		has = 1;
#endif
	lua_pop (L, 2);
	lua_pushboolean (L, has);
	return 1;
}

static int clp_newprocess (lua_State * L) {
	int idle = 0;
	process_t *process_ = NULL;
	int top=lua_gettop(L);
	if (top==0) {
		process_ = lua_newuserdata (L, sizeof (process_t *));
		(*process_) = malloc (sizeof (struct clp_Task));
		(*process_)->env = NULL;
		(*process_)->env_len = 0;
	}
	else {
		luaL_checktype (L, 1, LUA_TFUNCTION);
		if(lua_type(L,2) == LUA_TNUMBER) { 
			idle = luaL_optint (L, 2, 1);
		} else if(lua_type(L,3) == LUA_TNUMBER) {
			idle = luaL_optint (L, 3, 1);
		} else {
			idle=1;
		}
		lua_pushcfunction (L, mar_encode);
		lua_newtable(L);
		lua_pushvalue (L, 1);
		lua_setfield(L,-2,"f");
		if(lua_type(L,2) == LUA_TFUNCTION && top>=2) {
			lua_pushvalue (L, 2);
			lua_setfield(L,-2,"e");
		}
		lua_call (L, 1, 1);
		const char *env = NULL;
		size_t len = 0;
		env = lua_tolstring (L, -1, &len);
		process_ = lua_newuserdata (L, sizeof (process_t *));
		(*process_) = calloc(1, sizeof (struct clp_Task));
		char *envcp = malloc ((sizeof(char))*len);
		memcpy (envcp, env, len);
		(*process_)->env = envcp;
		(*process_)->env_len = len; 
		lua_remove (L, -2);
	}
	process_t process = *process_;
	//instance queue initialization
	process->instances = 0;
	MUTEX_INIT(&process->intances_mutex);
	//create input channel
	lua_pushcfunction(L,clp_channelnew);
	lua_call(L,0,1);
	lua_getfield(L,-1,"__id");
	lua_insert(L,-2);
	lua_call(L,1,1);
	process->input=(channel_t)lua_touserdata(L,-1);
	lua_pop(L,1);
	
	//initialize parent
	process->parent = NULL;
	lua_pushliteral (L, CLP_INSTANCE_KEY);
	lua_gettable (L, LUA_REGISTRYINDEX);
	if (lua_type (L, -1) == LUA_TLIGHTUSERDATA)
	{
		instance_t i = lua_touserdata (L, -1);
		process->parent = i->process;
	}
	lua_pop (L, 1);
	//initialize pool
	if(process->parent==NULL) {
		process->pool = clp_defaultpool;
	} else {
		/* use parent's pool */
		process->pool = process->parent->pool;
	}

	
	//assign metatable
	get_processmetatale (L);
	lua_setmetatable (L, -2);
	//initialize intances
	if (idle > 0) {
		lua_pushcfunction (L, process_instantiate);
		lua_pushvalue (L, -2);
		lua_pushnumber (L, idle);
		lua_call (L, 2, 0);
	}
	
	return 1;
}

static int clp_destroyprocess (lua_State * L) {
	process_t *s_ptr = luaL_checkudata (L, 1, CLP_PROCESS_METATABLE);
	if (!s_ptr)
		return 0;
	if (!(*s_ptr))
		return 0;
	process_t s = *s_ptr;
	if (s->env != NULL)
		free (s->env);
	/*	while (lprocess_lfqueue_try_pop (s->instances, &i))
		clp_destroyinstance (i);
		lprocess_lfqueue_free (s->instances);*/
	s->input=NULL;
	*s_ptr = 0;
	return 0;
}

static int clp_getprocess (lua_State * L) {
	process_t s = lua_touserdata (L, 1);
	if (s)
	{
		clp_buildprocess (L, s);
		return 1;
	}
	lua_pushnil (L);
	lua_pushliteral (L, "Process not found");
	return 2;
}

static const struct luaL_Reg LuaExportFunctions[] = {
	{"new", clp_newprocess},
	{"get", clp_getprocess},
	{"destroy", clp_destroyprocess},
	{"isprocess", process_isprocess},
	{NULL, NULL}
};

CLP_EXPORTAPI int luaopen_clp_process (lua_State * L) {
	lua_newtable (L);
	lua_newtable (L);
	luaL_loadstring (L,
			"return function() return require'clp.process' end");
	lua_setfield (L, -2, "__persist");
	lua_setmetatable (L, -2);
#if LUA_VERSION_NUM < 502
	luaL_register (L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs (L, LuaExportFunctions, 0);
#endif
	return 1;
};


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

void clp_initinstance(instance_t i) {
	_DEBUG("Initiating instance %p\n",i);
	lua_State *L=i->L;
	lua_pushliteral(L,CLP_INSTANCE_KEY);
	lua_pushlightuserdata(L,i);
	lua_settable(L, LUA_REGISTRYINDEX);	
	lua_pushcfunction(L,luaopen_base);
	lua_pcall(L,0,0,0);
	lua_pushcfunction(L,luaopen_package);
	lua_pcall(L,0,1,0);
	lua_getfield(L,-1,"preload");
	LUA_REGISTER(L,InstanceLibs);
	lua_pop(L,2);
	//	luaL_openlibs(L);
	lua_pushliteral(L,PROCESS_HANDLER_KEY);
	const char buff[]="local a={...} "
		"local h=a[1].f "
		"local s=a[2] "
		"a[1].e = a[1].e or function(e) return e end "
		"return require'coroutine'.wrap(function() while true do h(s:input():get()) end end)\n";
	luaL_loadbuffer (L,buff, strlen(buff), "Process");
	lua_pushcfunction(L,mar_decode);
	lua_pushlstring(L,i->process->env,i->process->env_len);
	lua_call(L,1,1);
	lua_pushvalue(L,-1);
	lua_setfield(L, LUA_REGISTRYINDEX,CLP_ENV_KEY);
	clp_buildprocess(L,i->process);
	lua_call(L,2,1);
	lua_settable(L, LUA_REGISTRYINDEX);
	lua_getfield(L, LUA_REGISTRYINDEX,CLP_ENV_KEY);
	lua_getfield(L,-1,"e");
	lua_setfield(L,LUA_REGISTRYINDEX,CLP_ERRORFUNCTION_KEY);
	lua_pop(L,1);
	i->state=I_READY;
}
//static int i=0;
static instance_t clp_newinstance(lua_State *L, process_t s) {
	lua_State * newstate = luaL_newstate();
	if(L==NULL) {
		luaL_error(L,"Error creating new state");
	} else {
		_DEBUG("Created state %p\n",newstate);
	}
//	printf("created %p %d\n",L,i++);
	instance_t i=malloc(sizeof(struct instance_s));
	i->L=newstate;
	i->process=s;
	i->state=I_CREATED;
	i->chan=NULL;
	i->ev=NULL;
	clp_pushinstance(i);
	return i;
}

void clp_destroyinstance(instance_t i) {
	_DEBUG("Closing %p %p\n",i,i->L);
	lua_close(i->L);
	if(i->ev) clp_destroyevent(i->ev);
	free(i);
}

