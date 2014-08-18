#include "stage.h"
#include "marshal.h"
#include "event.h"
#include "scheduler.h"
#include "instance.h"

#include <stdlib.h>
#include <string.h>

//static qt_hash H;

#define DEFAULT_I_IDLE_CAPACITY 10
#define DEFAULT_QUEUE_CAPACITY -1

static void get_stagemetatale (lua_State * L);
extern pool_t lstage_defaultpool;
stage_t
lstage_tostage (lua_State * L, int i)
{
	stage_t *s = luaL_checkudata (L, i, LSTAGE_STAGE_METATABLE);
	luaL_argcheck (L, s != NULL, i, "Stage expected");
	return *s;
}

static int
stage_input (lua_State * L)
{
	stage_t s = lstage_tostage (L, 1);
	lstage_pushchannel(L,s->input);
//	lua_pushnumber (L, lstage_lfqueue_getcapacity (s->event_queue));
	return 1;
}

static int
get_max_instances (lua_State * L)
{
	stage_t s = lstage_tostage (L, 1);
	lua_pushnumber (L, s->instances);
	return 1;
}

static int
stage_push (lua_State * L)
{
	stage_t s = lstage_tostage (L, 1);
	lua_pushcfunction(L,lstage_pushevent);
	lua_insert(L,2);
	lstage_pushchannel(L,s->input);
	lua_insert(L,3);
	lua_call(L,lua_gettop(L)-2,LUA_MULTRET);
	if(lua_type(L,-1)==LUA_TBOOLEAN) {
		lua_pop(L,1);
	}
	return lua_gettop(L);
}

static int
stage_getenv (lua_State * L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, LSTAGE_HANDLER_KEY);
	if(lua_type(L,-1)!=LUA_TNIL) {
		return 1;
	}
	lua_pop(L,1);
	stage_t s = lstage_tostage (L, 1);
	if (s->env == NULL){
		lua_pushnil (L);
	} else {
		lua_pushcfunction(L,mar_decode);
		lua_pushlstring (L, s->env, s->env_len);
		lua_call(L,1,1);
	}
	return 1;
}

static int
stage_eq (lua_State * L)
{
	stage_t s1 = lstage_tostage (L, 1);
	stage_t s2 = lstage_tostage (L, 2);
	lua_pushboolean (L, s1 == s2);
	return 1;
}

static int stage_instantiate (lua_State * L);

static int
stage_wrap (lua_State * L)
{
	int top=lua_gettop(L);
	stage_t s = lstage_tostage (L, 1);
	if (s->env != NULL)
		luaL_error (L, "Stage handMUTEX_INITler already set");

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

	lua_pushcfunction (L, stage_instantiate);
	lua_pushvalue (L, 1);
	lua_pushnumber (L, 1);
	lua_call (L, 2, 0);

	lua_pushvalue (L, 1);
	return 1;
}

/*tostring method*/
static int
stage_tostring (lua_State * L)
{
	stage_t *s = luaL_checkudata (L, 1, LSTAGE_STAGE_METATABLE);
	lua_pushfstring (L, "Stage (%p)", *s);
	return 1;
}

static int
stage_destroyinstances (lua_State * L)
{
	stage_t s = lstage_tostage (L, 1);
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

static int
stage_instantiate (lua_State * L)
{
	stage_t s = lstage_tostage (L, 1);
	if (s->pool == NULL)
		luaL_error (L, "Stage must be associated to a pool");
	if (s->env == NULL)
		luaL_error (L, "Stage must have an environment");
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
		  (void) lstage_newinstance (s);
	}
   MUTEX_UNLOCK(&s->intances_mutex);

  	
	/*unlock mutex */
	lua_pushvalue (L, 1);
	return 1;
}

static int
stage_ptr (lua_State * L)
{
	stage_t *s = luaL_checkudata (L, 1, LSTAGE_STAGE_METATABLE);
	lua_pushlightuserdata (L, *s);
	return 1;
}

#define LSTAGE_STAGE_CACHE "lstage-stage-cache"

static void
stage_getcached(lua_State * L, stage_t t)
{
	lua_pushliteral(L,LSTAGE_STAGE_CACHE);
	lua_gettable(L,LUA_REGISTRYINDEX);
	if(!lua_istable(L,-1)) {
		lua_pop(L,1);
		lua_newtable(L);
		lua_newtable(L);
		lua_pushliteral(L,"v");
		lua_setfield(L,-2,"__mode");
		lua_setmetatable(L,-2);
		lua_pushliteral(L,LSTAGE_STAGE_CACHE);
		lua_pushvalue(L,-2);
		lua_settable(L,LUA_REGISTRYINDEX);
	}
	lua_pushlightuserdata(L,t);
	lua_gettable(L,-2);
	lua_remove(L,-2);
}

static void
stage_putcache(lua_State * L, stage_t t)
{
	lua_pushliteral(L,LSTAGE_STAGE_CACHE);
	lua_gettable(L,LUA_REGISTRYINDEX);
	if(!lua_isuserdata(L,-1)) {
		lua_pop(L,1);
		lua_newtable(L);
		lua_newtable(L);
		lua_pushliteral(L,"v");
		lua_setfield(L,-2,"__mode");
		lua_setmetatable(L,-2);
		lua_pushliteral(L,LSTAGE_STAGE_CACHE);
		lua_pushvalue(L,-2);
		lua_settable(L,LUA_REGISTRYINDEX);
	}
	lua_pushlightuserdata(L,t);
	lua_pushvalue(L,-3);
	lua_settable(L,-3);
	lua_pop(L,1);
}

void
lstage_buildstage (lua_State * L, stage_t t)
{
	_DEBUG("Building stage %p\n",t);
	stage_getcached(L,t);
	if(lua_type(L,-1)==LUA_TUSERDATA) 
		return;
	lua_pop(L,1);
	stage_t *s = lua_newuserdata (L, sizeof (stage_t *));
	*s = t;
	get_stagemetatale (L);
	lua_setmetatable (L, -2);
	stage_putcache(L,t);
	_DEBUG("Created userdata %p\n",t);

}

static int
stage_getpool (lua_State * L)
{
	if(lua_gettop(L)>1) {
		luaL_error(L,"To many arguments");
	}
	
	stage_t s = lstage_tostage (L, 1);
	if (s->pool)
		lstage_buildpool (L, s->pool);
	else
		lua_pushnil (L);
	return 1;
}

static int
stage_setpool (lua_State * L)
{
	stage_t s = lstage_tostage (L, 1);
	pool_t p = lstage_topool (L, 2);
	s->pool = p;
	lua_pop(L,1);
	return 1;
}

static int
stage_getparent (lua_State * L)
{
	stage_t s = lstage_tostage (L, 1);
	if (s->parent)
		lstage_buildstage (L, s->parent);
	else
		lua_pushnil (L);
	return 1;
}

static int
stage_gc (lua_State * L)
{
//	stage_t s = lstage_tostage (L, 1);
//	_DEBUG("Destroying stage %p\n",s);
	return 0;
}


static const struct luaL_Reg StageMetaFunctions[] = {
	{"__eq", stage_eq},
	{"__gc", stage_gc},
	{"__tostring", stage_tostring},
	{"__call", stage_push},
	{"__id", stage_ptr},
		
	{"size", get_max_instances},
	{"env", stage_getenv},
	{"wrap", stage_wrap},
	{"push", stage_push},
	{"input", stage_input},
	{"add", stage_instantiate},
	{"remove", stage_destroyinstances},
	{"parent", stage_getparent},
	{"pool", stage_getpool},
	{"setpool", stage_setpool},
	
	{NULL, NULL}
};

static void
get_stagemetatale (lua_State * L)
{
	luaL_getmetatable (L, LSTAGE_STAGE_METATABLE);
	if (lua_isnil (L, -1))
	  {
		  lua_pop (L, 1);
		  luaL_newmetatable (L, LSTAGE_STAGE_METATABLE);
		  LUA_REGISTER (L, StageMetaFunctions);
		  lua_pushvalue (L, -1);
		  lua_setfield (L, -2, "__index");
		  luaL_loadstring (L,
				   "local ptr=(...):__id() return function() return require'lstage.stage'.get(ptr) end");
		  lua_setfield (L, -2, "__wrap");
	  }
}


static int
stage_isstage (lua_State * L)
{
	lua_getmetatable (L, 1);
	get_stagemetatale (L);
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

static int
lstage_newstage (lua_State * L)
{
	int idle = 0;
	stage_t *stage_ = NULL;
	int top=lua_gettop(L);
	if (top==0) {
		  stage_ = lua_newuserdata (L, sizeof (stage_t *));
		  (*stage_) = malloc (sizeof (struct lstage_Stage));
		  (*stage_)->env = NULL;
		  (*stage_)->env_len = 0;
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
		  lua_pop (L, 1);
		  stage_ = lua_newuserdata (L, sizeof (stage_t *));
		  (*stage_) = calloc (1, sizeof (struct lstage_Stage));
		  char *envcp = malloc (len + 1);
		  envcp[len] = '\0';
		  memcpy (envcp, env, len + 1);
		  (*stage_)->env = envcp;
		  (*stage_)->env_len = len; 
	  }
	stage_t stage = *stage_;
   //instance queue initialization
   stage->instances = 0;
	MUTEX_INIT(&stage->intances_mutex);
	//create input channel
	lua_pushcfunction(L,lstage_channelnew);
	lua_call(L,0,1);
	lua_getfield(L,-1,"__id");
	lua_insert(L,-2);
	lua_call(L,1,1);
	stage->input=(channel_t)lua_touserdata(L,-1);
	lua_pop(L,1);
	//initialize thread pool
	stage->pool = lstage_defaultpool;
	//assign metatable
	get_stagemetatale (L);
	lua_setmetatable (L, -2);
	//initialize intances
	if (idle > 0) {
		  lua_pushcfunction (L, stage_instantiate);
		  lua_pushvalue (L, -2);
		  lua_pushnumber (L, idle);
		  lua_call (L, 2, 0);
	}
	//initialize parent
	stage->parent = NULL;
	lua_pushliteral (L, LSTAGE_INSTANCE_KEY);
	lua_gettable (L, LUA_REGISTRYINDEX);
	if (lua_type (L, -1) == LUA_TLIGHTUSERDATA)
	  {
		  instance_t i = lua_touserdata (L, -1);
		  stage->parent = i->stage;
	  }
	lua_pop (L, 1);
	
	return 1;
}

static int
lstage_destroystage (lua_State * L)
{
	stage_t *s_ptr = luaL_checkudata (L, 1, LSTAGE_STAGE_METATABLE);
	if (!s_ptr)
		return 0;
	if (!(*s_ptr))
		return 0;
	stage_t s = *s_ptr;
	if (s->env != NULL)
		free (s->env);
/*	while (lstage_lfqueue_try_pop (s->instances, &i))
		lstage_destroyinstance (i);
	lstage_lfqueue_free (s->instances);*/
	s->input=NULL;
	*s_ptr = 0;
	return 0;
}

static int
lstage_getstage (lua_State * L)
{
	stage_t s = lua_touserdata (L, 1);
	if (s)
	  {
		  lstage_buildstage (L, s);
		  return 1;
	  }
	lua_pushnil (L);
	lua_pushliteral (L, "Stage not found");
	return 2;
}

static const struct luaL_Reg LuaExportFunctions[] = {
	{"new", lstage_newstage},
	{"get", lstage_getstage},
	{"destroy", lstage_destroystage},
	{"is_stage", stage_isstage},
	{NULL, NULL}
};

LSTAGE_EXPORTAPI int
luaopen_lstage_stage (lua_State * L)
{
	lua_newtable (L);
	lua_newtable (L);
	luaL_loadstring (L,
			 "return function() return require'lstage.stage' end");
	lua_setfield (L, -2, "__persist");
	lua_setmetatable (L, -2);
#if LUA_VERSION_NUM < 502
	luaL_register (L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs (L, LuaExportFunctions, 0);
#endif
	return 1;
};
