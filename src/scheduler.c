#include "lstage.h"
#include "scheduler.h"
#include "instance.h"
#include "threading.h"
#include "event.h"
#include "marshal.h"
#include "p_queue.h"

#include <time.h>

//static LFqueue_t ready_queue=NULL;

static int thread_tostring (lua_State *L) {
  thread_t * t = luaL_checkudata (L, 1, LSTAGE_THREAD_METATABLE);
  lua_pushfstring (L, "Thread (%p)", *t);
  return 1;
}

thread_t lstage_tothread(lua_State *L, int i) {
	thread_t * t = luaL_checkudata (L, i, LSTAGE_THREAD_METATABLE);
	luaL_argcheck (L, *t != NULL, i, "not a Thread");
	return *t;
}

static int thread_join (lua_State *L) {
	thread_t t=lstage_tothread(L,1);
	int timeout=lua_tointeger(L,2);
	if(timeout>0) {
		struct timespec to;
		clock_gettime(CLOCK_REALTIME, &to);
		to.tv_sec += timeout;
	   pthread_timedjoin_np(*t->th,NULL,&to);
   } else {
	   pthread_join(*t->th,NULL);
   }
   return 0;
}

static int thread_rawkill (lua_State *L) {
   thread_t t=lstage_tothread(L,1);
   THREAD_KILL(t->th);
   return 0;
}

static int thread_state (lua_State *L) {
   thread_t t=lstage_tothread(L,1);
   lua_pushnumber(L,t->state);
   return 1;
}

static int thread_ptr (lua_State *L) {
	thread_t t=lstage_tothread(L,1);
	lua_pushlightuserdata(L,t);
	return 1;
}

static int thread_eq(lua_State * L) {
	thread_t t1=lstage_tothread(L,1);
	thread_t t2=lstage_tothread(L,2);
	lua_pushboolean(L,t1==t2);
	return 1;
}

static const struct luaL_Reg StageMetaFunctions[] = {
		{"__tostring",thread_tostring},
		{"__eq",thread_eq},
		{"join",thread_join},
		{"__ptr",thread_ptr},
		{"state",thread_state},
		{"rawkill",thread_rawkill},
		{NULL,NULL}
};

static void get_metatable(lua_State * L) {
	luaL_getmetatable(L,LSTAGE_THREAD_METATABLE);
   if(lua_isnil(L,-1)) {
   	lua_pop(L,1);
  		luaL_newmetatable(L,LSTAGE_THREAD_METATABLE);
  		lua_pushvalue(L,-1);
  		lua_setfield(L,-2,"__index");
		LUA_REGISTER(L,StageMetaFunctions);
		luaL_loadstring(L,"local th=(...):ptr() return function() return require'lstage.scheduler'.build(th) end");
		lua_setfield (L, -2,"__wrap");
  	}
}

static void thread_resume_instance(instance_t i) {
	_DEBUG("Resuming instance: %p %d lua_State (%p)\n",i,i->flags,i->L);
	lua_State * L=i->L;
	
	switch(i->flags) {
		case I_CREATED:
			lstage_initinstance(i);
			break;
		case I_WAITING_IO:
			i->flags=I_READY;
			lua_pushliteral(L,STAGE_HANDLER_KEY);
			lua_gettable(L,LUA_REGISTRYINDEX);
			lua_pushboolean(L,1);
			if(lua_pcall(i->L,1,0,0)) {
		      const char * err=lua_tostring(L,-1);
		      fprintf(stderr,"Error resuming instance: %s\n",err);
		   }
		   break;
		case I_TIMEOUT_IO:
			i->flags=I_READY;
			lua_pushliteral(L,STAGE_HANDLER_KEY);
			lua_gettable(L,LUA_REGISTRYINDEX);
			lua_pushboolean(L,0);
			if(lua_pcall(i->L,1,0,0)) {
		      const char * err=lua_tostring(L,-1);
		      fprintf(stderr,"Error resuming instance: %s\n",err);
		   }
		   break;
		case I_READY:
			if(i->ev) {
				lua_pushliteral(L,STAGE_HANDLER_KEY);
				lua_gettable(L,LUA_REGISTRYINDEX);
		      lua_pushcfunction(L,mar_decode);
		      lua_pushlstring(L,i->ev->data,i->ev->len);
		      
		      lstage_destroyevent(i->ev);
  		      i->ev=NULL;
				if(lua_pcall(L,1,1,0)) {
					const char * err=lua_tostring(L,-1);
			      fprintf(stderr,"Error decoding event: %s\n",err);
			      break;
				}
				int n=
				#if LUA_VERSION_NUM < 502
					luaL_getn(L,2);
			   #else
					luaL_len(L,2);
				#endif
				int j;
				for(j=1;j<=n;j++) lua_rawgeti(L,2,j);
				lua_remove(L,2);
				i->args=n;
			} else {
				lua_pushliteral(L,STAGE_HANDLER_KEY);
				lua_gettable(L,LUA_REGISTRYINDEX);
				i->args=0;
			}
			if(lua_pcall(L,i->args,0,0)) {
		      const char * err=lua_tostring(L,-1);
		      fprintf(stderr,"Error resuming instance: %s\n",err);
		   } 
			break;
		case I_WAITING_EVENT:
			return;
		case I_WAITING_CHANNEL:
			return;
		case I_IDLE:
			break;
	}
	_DEBUG("Instance Yielded: %p %d lua_State (%p)\n",i,i->flags,i->L);
	if(i->flags==I_READY || i->flags==I_IDLE) {
	   lstage_putinstance(i);
	}
}

/*thread main loop*/
static THREAD_RETURN_T THREAD_CALLCONV thread_mainloop(void *t_val) {
   instance_t i=NULL;
   thread_t self=(thread_t)t_val;
   while(1) {
   	_DEBUG("Thread %p wating for ready instaces\n",self);
   	self->state=THREAD_IDLE;
      lstage_pqueue_pop(self->pool->ready,&i);
      if(i==NULL) break;
     	_DEBUG("Thread %p got a ready instace %p\n",self,i);
     	self->state=THREAD_RUNNING;
      thread_resume_instance(i);
   }
   self->state=THREAD_DESTROYED;
  	_DEBUG("Thread %p quitting\n",self);
  	self->pool->size--; //TODO atomic
   return t_val;
}

int lstage_newthread(lua_State *L,pool_t pool) {
	_DEBUG("Creating new thread for pool %p\n",pool);
	thread_t * thread=lua_newuserdata(L,sizeof(thread_t));
	thread_t t=malloc(sizeof(struct thread_s));
	t->th=calloc(1,sizeof(THREAD_T));
	t->pool=pool;
	t->state=THREAD_IDLE;
	*thread=t;
   get_metatable(L);
   lua_setmetatable(L,-2);
   THREAD_CREATE(t->th, thread_mainloop, t, 0 );
   return 1;
}

static int thread_from_ptr (lua_State *L) {
	thread_t * ptr=lua_touserdata(L,1);
	thread_t ** thread=lua_newuserdata(L,sizeof(thread_t));
	*thread=ptr;
   get_metatable(L);
   lua_setmetatable(L,-2);   
//   THREAD_CREATE(*thread, thread_mainloop, *thread, 0 );
   return 1;
}

void lstage_pushinstance(instance_t i) {
	return lstage_pqueue_push(i->stage->pool->ready,(void **) &(i));
}

static const struct luaL_Reg LuaExportFunctions[] = {
//	{"new_thread",thread_new},
//	{"kill_thread",thread_kill},
	{"build",thread_from_ptr},
	{NULL,NULL}
	};

LSTAGE_EXPORTAPI	int luaopen_lstage_scheduler(lua_State *L) {

//	if(!ready_queue) ready_queue=lstage_lfqueue_new();
	get_metatable(L);
	lua_pop(L,1);
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'lstage.scheduler' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};
