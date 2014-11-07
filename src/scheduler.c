#include "clp.h"
#include "scheduler.h"
#include "threading.h"
#include "event.h"
#include "channel.h"
#include "marshal.h"

#include <time.h>

static int thread_tostring (lua_State *L) {
	thread_t * t = luaL_checkudata (L, 1, CLP_THREAD_METATABLE);
	lua_pushfstring (L, "Thread (%p)", *t);
	return 1;
}

thread_t clp_tothread(lua_State *L, int i) {
	thread_t * t = luaL_checkudata (L, i, CLP_THREAD_METATABLE);
	luaL_argcheck (L, *t != NULL, i, "not a Thread");
	return *t;
}

static int thread_rawkill (lua_State *L) {
	thread_t t=clp_tothread(L,1);
	THREAD_KILL(t->th);
	return 0;
}

static int thread_state (lua_State *L) {
	thread_t t=clp_tothread(L,1);
	lua_pushnumber(L,t->state);
	return 1;
}

static int thread_ptr (lua_State *L) {
	thread_t t=clp_tothread(L,1);
	lua_pushlightuserdata(L,t);
	return 1;
}

static int thread_eq(lua_State * L) {
	thread_t t1=clp_tothread(L,1);
	thread_t t2=clp_tothread(L,2);
	lua_pushboolean(L,t1==t2);
	return 1;
}

static const struct luaL_Reg StageMetaFunctions[] = {
	{"__tostring",thread_tostring},
	{"__eq",thread_eq},
	{"__id",thread_ptr},
	{"state",thread_state},
	{"rawkill",thread_rawkill},
	{NULL,NULL}
};

static void get_metatable(lua_State * L) {
	luaL_getmetatable(L,CLP_THREAD_METATABLE);
	if(lua_isnil(L,-1)) {
		lua_pop(L,1);
		luaL_newmetatable(L,CLP_THREAD_METATABLE);
		lua_pushvalue(L,-1);
		lua_setfield(L,-2,"__index");
		LUA_REGISTER(L,StageMetaFunctions);
		luaL_loadstring(L,"local th=(...):__id() return function() return require'clp.scheduler'.build(th) end");
		lua_setfield (L, -2,"__wrap");
	}
}

static void thread_resume_instance(instance_t i) {
	_DEBUG("Resuming instance: %p %d lua_State (%p)\n",i,i->state,i->L);

	lua_State * L=i->L;
	if(i->state==I_CREATED) {
		clp_initinstance(i);
	}
	i->args=0;
	lua_getfield(L,LUA_REGISTRYINDEX,CLP_ERRORFUNCTION_KEY);
	lua_getfield(L,LUA_REGISTRYINDEX,TASK_HANDLER_KEY);

	switch(i->state) {
		case I_CLOSED:
			lua_pop(L,1);
			lua_getglobal(L,"error");
			lua_pushliteral(L,"closed");
			i->args=1;
			break;
		case I_READY:
			if(i->ev) {
				lua_pushcfunction(L,mar_decode);
				lua_pushlstring(L,i->ev->data,i->ev->len);
				clp_destroyevent(i->ev);
				i->ev=NULL;
				if(lua_pcall(L,1,1,0)) {
					lua_getfield(L,LUA_REGISTRYINDEX,CLP_ERRORFUNCTION_KEY);
					lua_insert(L,1);
					if(lua_pcall(i->L,1,0,0)) {
						const char * err=lua_tostring(L,-1);
						fprintf(stderr,"Error unpacking event: %s\n",err);
					}
					clp_destroyinstance(i);
					return;
				}
				int n=
#if LUA_VERSION_NUM < 502
					luaL_getn(L,3);
#else
				luaL_len(L,3);
#endif
				int j;
				for(j=1;j<=n;j++) lua_rawgeti(L,3,j);
				lua_remove(L,3);
				i->args=n;
			}
			break;
		case I_RESUME_SUCCESS:
			lua_pushboolean(L,1);
			i->args=1;
			break;
		case I_RESUME_FAIL:
			lua_pushboolean(L,0);
			i->args=1;
			break;
		default:
			lua_pop(L,2);
			return;
	}
	if(lua_pcall(L,i->args,LUA_MULTRET, -(i->args+2))) {
		const char * err=lua_tostring(L,-1);
		if(err!=NULL)  {
			fprintf(stderr,"Process error: %s\n",err);
		}
		clp_destroyinstance(i);
		return;
	}
	//  	printf("instance %s\n",instance_state[i->state]);
	lua_remove(L,1);
	switch(i->state) {
		case I_READY:
			_DEBUG("Thread Yielded\n");
			break;
		case I_CHANNEL_READ:
			clp_lfqueue_try_push(i->chan->read_queue,&i);
			_DEBUG("get: unlock %p\n",i->chan);
			CHANNEL_UNLOCK(i->chan);
			i->chan=NULL;
			break;
		case I_CHANNEL_WRITE:
			clp_lfqueue_try_push(i->chan->write_queue,&i);
			_DEBUG("push: unlock %p\n",i->chan);
			CHANNEL_UNLOCK(i->chan);
			i->chan=NULL;
			break;
		case I_SLEEP:
			clp_sleepevent(i);
			break;
		default:
			lua_pop(L,1);
			break;
	}
}

/*thread main loop*/
static THREAD_RETURN_T THREAD_CALLCONV thread_mainloop(void *t_val) {
	instance_t i=NULL;
	thread_t self=(thread_t)t_val;
	while(1) {
		_DEBUG("Thread %p wating for ready instaces in %p (%p)\n",self,self->pool,self->pool->ready);
		self->state=THREAD_IDLE;

		clp_lfqueue_pop(self->pool->ready,(void**)&i);
		if(i==NULL) break;
		_DEBUG("Thread %p got a ready instace %p\n",self,i);
		self->state=THREAD_RUNNING;
		thread_resume_instance(i);
	}
	self->state=THREAD_DESTROYED;
	_DEBUG("Thread %p quitting\n",self);
	CHANNEL_LOCK(self->pool);
	self->pool->size--;
	CHANNEL_UNLOCK(self->pool);
	return t_val;
}

int clp_newthread(lua_State *L,pool_t pool) {
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
	return 1;
}

void clp_pushinstance(instance_t i) {
	return clp_lfqueue_push(i->task->pool->ready, &i);
}

static const struct luaL_Reg LuaExportFunctions[] = {
	//	{"new_thread",thread_new},
	//	{"kill_thread",thread_kill},
	{"build",thread_from_ptr},
	{NULL,NULL}
};

CLP_EXPORTAPI	int luaopen_clp_scheduler(lua_State *L) {

	//	if(!ready_queue) ready_queue=clp_lfqueue_new();
	get_metatable(L);
	lua_pop(L,1);
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'clp.scheduler' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};
