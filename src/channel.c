/// 
// Channel submodule.
// 
// This module adds the Channel type to the current Lua state.
// Channels connect concurrent process. 
// 
// You can send values into channels from one @{process} and receive 
// those values into another @{process}.
//  
// Send values to a channel using the @{channel:put} method.
//
// Receive values from a channel invoking the @{channel:get} method. 
// 
// Channels can be buffered or unbuffered.
// By default channels are buffered and unbounded, meaning that @{channel:put}
// returns immediatelly and never become full (be carefull with memory
// exaustion), even when there is no corresponding  @{channel:get} call
// waiting to receive the sent values.
//
// Buffered channels may accept a limited number of values without a
// corresponding receiver for those values. This number may be specified 
// during the channel creation: for example, `channel.new(10)` will create a
// channel that blocks only after the 10th put call without matching receivers.
//
// Unbuffered channels are created by the @{channel.new} call.
// Unbuffered channels will block the process in  @{channel:put} until
// there is a corresponding @{channel:get} call ready to receive the sent 
// values and vice versa. Unbuffered channels act as a synchronization
// barrier between two processes.

//
// @module channel
// @author Tiago Salmito
// @license MIT
// @copyright Tiago Salmito - 2014

#include "channel.h"
#include "task.h"
#include "event.h"
#include "scheduler.h"
#include "marshal.h"
#include <stdlib.h>

///
// Creates a new channel
//
// @int[opt=-1] size the maximum size of the buffer. 
// If the value is lower than  `0` then the channel will be unbounded.
// If the value is `0` then the channel will be unbuffered. 
// If the value is greater than  `0` then the channel buffer will increase
// up to the maximum `size`.
//
// @bool[opt=false] async if set to `true` then the new channel will be
// asynchronous (i.e. fails when full/empty). if `false` then it will be 
// synchronous (i.e. blocks when full/empty).
// @treturn channel the new channel
// @function new

///
// Channel type.
//
// Any function in this section belongs to `channel` type methods.
//
// @type channel


channel_t clp_tochannel(lua_State *L, int i) {
	channel_t * t = luaL_checkudata (L, i, CLP_CHANNEL_METATABLE);
	luaL_argcheck (L, *t != NULL, i, "not a Channel");
	return *t;
}

static void channel_putcache(lua_State * L, channel_t t)
{
	lua_pushliteral(L,CLP_CHANNEL_CACHE);
	lua_gettable(L,LUA_REGISTRYINDEX);
	if(!lua_isuserdata(L,-1)) {
		lua_pop(L,1);
		lua_newtable(L);
		lua_newtable(L);
		lua_pushliteral(L,"v");
		lua_setfield(L,-2,"__mode");
		lua_setmetatable(L,-2);
		lua_pushliteral(L,CLP_CHANNEL_CACHE);
		lua_pushvalue(L,-2);
		lua_settable(L,LUA_REGISTRYINDEX);
	}
	lua_pushlightuserdata(L,t);
	lua_pushvalue(L,-3);
	lua_settable(L,-3);
	lua_pop(L,1);
}

static void channel_getcached(lua_State * L, channel_t t)
{
	lua_pushliteral(L,CLP_CHANNEL_CACHE);
	lua_gettable(L,LUA_REGISTRYINDEX);
	if(!lua_istable(L,-1)) {
		lua_pop(L,1);
		lua_newtable(L);
		lua_newtable(L);
		lua_pushliteral(L,"v");
		lua_setfield(L,-2,"__mode");
		lua_setmetatable(L,-2);
		lua_pushliteral(L,CLP_CHANNEL_CACHE);
		lua_pushvalue(L,-2);
		lua_settable(L,LUA_REGISTRYINDEX);
	}
	lua_pushlightuserdata(L,t);
	lua_gettable(L,-2);
	lua_remove(L,-2);
}


static int channel_eq(lua_State * L) {
	channel_t s1=clp_tochannel(L,1);
	channel_t s2=clp_tochannel(L,2);
	lua_pushboolean(L,s1==s2);
	return 1;
}

///
// Get the current and maximum size of the buffer.
// It also returns the current number of pending readers 
// and writers wating on the channel.
//
// @treturn int the current buffer size
// @treturn int the maximum buffer size
// @treturn int the current number of pending readers
// @treturn int the current number of pending writers
//
// @function size
static int channel_getsize(lua_State * L) {
	channel_t s=clp_tochannel(L,1);
	lua_pushnumber(L,clp_lfqueue_size(s->event_queue));
	lua_pushnumber(L,clp_lfqueue_getcapacity(s->event_queue));
	lua_pushnumber(L,clp_lfqueue_size(s->read_queue));
	lua_pushnumber(L,clp_lfqueue_size(s->write_queue));
	return 4;
}

///
// Set the maximum size of the buffer.
//
// @int size the new maximum size
//
// @function setsize
static int channel_setsize(lua_State * L) {
	channel_t s=clp_tochannel(L,1);
	luaL_checktype (L, 2, LUA_TNUMBER);
	int capacity=lua_tointeger(L,2);
	clp_lfqueue_setcapacity(s->event_queue,capacity);
	return 0;
}

///
// Close a channel. 
//
// When closed, any pending readers and writers are awaken and an error
// is thrown in their pendig operation.
// 
// Current stored values remain on the buffer until the channel is
// collected or are read by other processes.
// 
// Any subsequent @{channel:get} operation with an empty buffer fails
// with an error.
//
// Any subsequent @{channel:put} operation fails with `nil, 'closed'`.
//
// @function close
static int channel_close(lua_State * L) {
	channel_t c=clp_tochannel(L,1);
	instance_t ins=NULL;
	_DEBUG("Close: lock %p\n",c);
	CHANNEL_LOCK(c);

	if(c->read_wait)
		while(clp_lfqueue_try_pop(c->read_queue,&ins)) {
			ins->state=I_CLOSED;
			ins->args=0;
			clp_pushinstance(ins);
		}
	while(clp_lfqueue_try_pop(c->write_queue,&ins)) {
		ins->state=I_CLOSED;
		ins->args=0;
		clp_pushinstance(ins);
	}
	c->closed=1;
	if(c->read_wait) {
		c->read_event=NULL;
		SIGNAL_ONE(&c->read_cond);
	}
	if(c->read_wait) {
		c->read_event=NULL;
		SIGNAL_ONE(&c->read_cond);
	}
	_DEBUG("Close: unlock %p\n",c);
	CHANNEL_UNLOCK(c);
	lua_pushvalue(L,1);
	return 1;
}

static int channel_tostring (lua_State *L) {
	channel_t * s = luaL_checkudata (L, 1, CLP_CHANNEL_METATABLE);
	lua_pushfstring (L, "Channel (%p)", *s);
	return 1;
}

static int channel_ptr(lua_State * L) {
	channel_t * s = luaL_checkudata (L, 1, CLP_CHANNEL_METATABLE);
	lua_pushlightuserdata(L,*s);
	return 1;
}

///
// Put values into the channel.
//
// This operation will block the current process if the channel is
// synchronous and it is full until there is a corresponding get.
//
// @param ... The values to send
// @treturn[1] bool true if the value was sent
// @treturn[2] nil
// @treturn[2] string `"Channel is closed"` if the channel is closed
// @treturn[3] nil
// @treturn[3] string `"Channel is full"` if the channel is asynchronous and
// it's full
//
// @raise `"Channel is closed"` if the channel was closed while
// waiting for a get.
//
// @function put
int clp_pushevent(lua_State *L) {
	channel_t c = clp_tochannel(L,1);
	int top=lua_gettop(L);
	_DEBUG("Push: called %p top=%d\n",c,top);
	lua_pushcfunction(L,mar_encode);
	lua_newtable(L);
	int i;
	for(i=2;i<=top;i++) {
		lua_pushvalue(L,i);
		lua_rawseti(L,-2,i-1);
	}
	lua_call(L,1,1);
	size_t len;
	const char * str=lua_tolstring(L,-1,&len);
	lua_pop(L,1);
	instance_t ins=NULL;
	event_t ev=clp_newevent(str,len);
	lua_pushliteral(L,CLP_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);

	_DEBUG("push: lock %p\n",c);
	CHANNEL_LOCK(c);

	if(c->closed) { //Channel is closed
		_DEBUG("push: unlock %p\n",c);
		CHANNEL_UNLOCK(c);
		clp_destroyevent(ev);
		_DEBUG("Push: Channel is closed %p\n",c);
		lua_pop(L,1);
		lua_pushnil(L);
		lua_pushliteral(L,"Channel is closed");
		return 2;
	}

	if(c->read_wait) {
		_DEBUG("Push: main process is getter %p\n",c);
		c->read_wait=0;
		c->read_event=ev;
		MUTEX_LOCK(&c->mutex);
		CHANNEL_UNLOCK(c);
		SIGNAL_ONE(&c->read_cond);
		MUTEX_UNLOCK(&c->mutex);
		_DEBUG("push: unlock %p\n",c);
		lua_pop(L,1);
		lua_pushboolean(L,1);
		return 1;
	}

	if(clp_lfqueue_try_pop(c->read_queue,&ins)) {
		_DEBUG("push: unlock %p\n",c);  		
		CHANNEL_UNLOCK(c);
		_DEBUG("Push: got waiting reader for channel %p\n",c);
		lua_settop(ins->L,0);
		ins->ev=ev;
		ins->state=I_READY;
		clp_pushinstance(ins);
		lua_pop(L,1);
		lua_pushboolean(L,1);
		return 1;
	}

	//no one is waiting, push to event queue
	if(clp_lfqueue_try_push(c->event_queue,&ev)) {
		_DEBUG("push: unlock %p\n",c);
		CHANNEL_UNLOCK(c);
		_DEBUG("Push: Nobody is waiting, used event queue %p\n",c);
		lua_pop(L,1);
		lua_pushboolean(L,1);
		return 1;
	}
	//Cannot push on event queue, wait for a get

	if(c->sync) { //channel is sync
		ins=lua_touserdata(L,-1);
		_DEBUG("Push: channel is sync %p\n",c);
		lua_pop(L,1);
		if(ins==NULL) { //i am the main process, wait for get
			c->write_event=ev;
			c->write_wait=1;
			_DEBUG("push: unlock %p\n",c);  		
			MUTEX_LOCK(&c->mutex);
			CHANNEL_UNLOCK(c);
			SIGNAL_WAIT(&c->write_cond,&c->mutex,-1.0);
			MUTEX_UNLOCK(&c->mutex);  		
			lua_pushboolean(L,1);
			return 1;
		} //i am a regular task, yield to wait for a read
		_DEBUG("Push: waiting %p\n",c);
		ins->ev=ev;
		ins->state=I_CHANNEL_WRITE;
		ins->chan=c;
		return lua_yield(L,0);
	}

	//channel is asynchronous and the event queue is full
	CHANNEL_UNLOCK(c);
	_DEBUG("async queue full %p\n",c);
	clp_destroyevent(ev);
	lua_pushnil(L);
	lua_pushliteral(L,"Channel is full");
	return 2;
}

///
// Get values from the channel.
//
// This operation will block the current process if the channel is
// synchronous and it is empty until there is a corresponding put.
//
// @return[1] `...` The next values in the buffer
// @treturn[2] nil
// @treturn[2] string `"Channel is empty"` if the channel is asynchronous and
// it's empty
//
// @raise `"Channel is closed"` if the channel is closed or was closed while
// waiting for a put.
//
// @function get
static int channel_getevent(lua_State *L) {
	channel_t c = clp_tochannel(L,1);
	event_t ev=NULL;
	_DEBUG("Get: called: %p\n",c);

	lua_pushliteral(L,CLP_INSTANCE_KEY);
	lua_gettable(L, LUA_REGISTRYINDEX);
	instance_t i=NULL;

	_DEBUG("get: lock %p\n",c);
	CHANNEL_LOCK(c);

	if(c->write_wait) { //get the event from the main process
		_DEBUG("get: Main process is waiting for write %p\n",c);
		lua_pop(L,1);
		c->write_wait=0;
		int n=clp_restoreevent(L,c->write_event);
		clp_destroyevent(c->write_event);
		c->write_event=NULL;
		MUTEX_LOCK(&c->mutex);
		CHANNEL_UNLOCK(c);
		SIGNAL_ONE(&c->write_cond);
		MUTEX_UNLOCK(&c->mutex);
		_DEBUG("get: unlock %p\n",c);
		return n;
	}

	//Check if has any event
	if(clp_lfqueue_try_pop(c->event_queue,&ev)) {
		_DEBUG("get: got from event queue %p\n",c);
		lua_pop(L,1);
		//We just popped an event, push a waiting write for that slot  	   
		if(clp_lfqueue_try_pop(c->write_queue,&i)) {
			_DEBUG("get: has writers, escalate its event %p\n",c);
			if(clp_lfqueue_try_push(c->event_queue,&i->ev)) {
				_DEBUG("get: unlock %p\n",c);
				CHANNEL_UNLOCK(c);
				i->ev=NULL;				
				i->state=I_RESUME_SUCCESS;
				clp_pushinstance(i);
			} else {
				(void)clp_lfqueue_try_push(c->write_queue,&i);
				_DEBUG("get: unlock %p\n",c);
				CHANNEL_UNLOCK(c);
			}
			int n=clp_restoreevent(L,ev);
			clp_destroyevent(ev);
			return n;
		}
		_DEBUG("get: unlock %p\n",c);
		CHANNEL_UNLOCK(c);
		int n=clp_restoreevent(L,ev);
		clp_destroyevent(ev);
		return n;
	}

	//Check if there are still any task waiting for writes (in case the channel is unbuffered)
	if(clp_lfqueue_try_pop(c->write_queue,&i)) {
		_DEBUG("get: still has writers, get its event %p\n",c);
		_DEBUG("get: unlock %p\n",c);
		CHANNEL_UNLOCK(c);
		lua_pop(L,1);
		ev=i->ev;
		i->ev=NULL;
		i->state=I_RESUME_SUCCESS;
		clp_pushinstance(i);
		int n=clp_restoreevent(L,ev);
		clp_destroyevent(ev);
		return n;
	}

	if(c->closed) {
		_DEBUG("get: Channel is closed %p\n",c);
		_DEBUG("get: unlock %p\n",c);
		CHANNEL_UNLOCK(c);
		lua_pop(L,1);
		luaL_error(L,"Channel is closed");
		return 0;
	}

	if(!c->sync) { 
		lua_pushnil(L);
		lua_pushliteral(L,"Channel is empty");
		return 2;
	}

	if(lua_type(L,-1)!=LUA_TLIGHTUSERDATA) {
		_DEBUG("get: i am the main process, gotta wait :( %p\n",c);
		lua_pop(L,1);
		c->read_wait=1;
		MUTEX_LOCK(&c->mutex);
		CHANNEL_UNLOCK(c);
		SIGNAL_WAIT(&c->read_cond,&c->mutex,-1.0);
		int n=0;
		if(c->read_event) {
			_DEBUG("get: main process got event :) %p\n",c);
			n=clp_restoreevent(L,c->read_event);
			clp_destroyevent(c->read_event);
			c->read_event=NULL;
		} else {
			MUTEX_UNLOCK(&c->mutex);
			_DEBUG("get: unlock %p\n",c);
			luaL_error(L,"Channel is closed");
			return 0;
		}
		_DEBUG("get: unlock %p\n",c);
		MUTEX_UNLOCK(&c->mutex);
		return n;
	}
	_DEBUG("get: Yield to wait for write %p\n",c);
	//Yield to wait for write
	i=lua_touserdata(L,-1);
	lua_pop(L,1);

	i->state=I_CHANNEL_READ;
	i->chan=c;
	return lua_yield(L,0);
}

static const struct luaL_Reg ChannelMetaFunctions[] = {
	{"__eq",channel_eq},
	{"__tostring",channel_tostring},
	{"__id",channel_ptr},
	{"size",channel_getsize},
	{"setsize",channel_setsize},
	{"get",channel_getevent},
	{"close",channel_close},
	{"put",clp_pushevent},
	{NULL,NULL}
};

static void get_metatable(lua_State * L) {
	luaL_getmetatable(L,CLP_CHANNEL_METATABLE);
	if(lua_isnil(L,-1)) {
		lua_pop(L,1);
		luaL_newmetatable(L,CLP_CHANNEL_METATABLE);
#if LUA_VERSION_NUM < 502
		luaL_register(L, NULL, ChannelMetaFunctions);
#else
		luaL_setfuncs(L, ChannelMetaFunctions, 0);
#endif 
		lua_pushvalue(L,-1);
		lua_setfield(L,-2,"__index");
		luaL_loadstring(L,"local id=(...):__id() return function() return require'clp.channel'.get(id) end");
		lua_setfield (L, -2,"__wrap");
	}
}

void clp_pushchannel(lua_State * L,channel_t t) {
	channel_getcached(L,t);
	if(lua_type(L,-1)==LUA_TUSERDATA) 
		return;
	lua_pop(L,1);
	channel_t *s=lua_newuserdata(L,sizeof(channel_t *));
	*s=t;
	get_metatable(L);
	lua_setmetatable(L,-2);
	channel_putcache(L,t);
}

int clp_channelnew(lua_State *L) {
	channel_t t=malloc(sizeof(struct channel_s));
	int size=luaL_optint(L, 1, -1);
	int sync=1;
	if(lua_type(L,2)==LUA_TBOOLEAN) {
		sync=!lua_toboolean(L, 2);
	}
	SIGNAL_INIT(&t->read_cond);
	SIGNAL_INIT(&t->write_cond);
	MUTEX_INIT(&t->mutex);
	t->read_wait=0;
	t->write_wait=0;
	t->read_event=NULL;
	t->write_event=NULL;
	t->lock=0;
	t->closed=0;
	t->event_queue=clp_lfqueue_new();
	t->read_queue=clp_lfqueue_new();
	t->write_queue=clp_lfqueue_new();
	t->sync=sync;
	clp_lfqueue_setcapacity(t->event_queue,size);
	clp_lfqueue_setcapacity(t->read_queue,-1);
	clp_lfqueue_setcapacity(t->write_queue,-1);
	clp_pushchannel(L,t);
	return 1;
}

static int channel_get(lua_State * L) {
	channel_t s=lua_touserdata(L,1);
	clp_pushchannel(L,s);
	return 1;
}

static const struct luaL_Reg LuaExportFunctions[] = {
	{"new",clp_channelnew},
	{"get",channel_get},
	{NULL,NULL}
};

CLP_EXPORTAPI	int luaopen_clp_channel(lua_State *L) {
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'clp.channel' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};

