--[[

By default channels are buffered and unbounded, meaning that put returns immediatelly and aways succeeds (except when memory is exausted), even 
when there is no corresponding get call waiting to receive the sent value.

Buffered channels may accept a limited number of values without a
corresponding receiver for those values. This number may be specified 
during the channel creation: for example, clp.channel(10) will create a
channel that blocks only after the 10th put call without matching receivers.

Unbuffered channels are created by the c=clp.channel(0) call.
Unbuffered channels will block in put until there is a corresponding
get call ready to receive the sent value, acting as a synchronization
barrier between two tasks.

--]]
local clp=require'clp'

local unbuffered=clp.channel(0)

clp.process(function() 
    print('put', 'event1')
    unbuffered:put('event1') --unlock waiter
    unbuffered:get() --wait
    print('put','event2') 
    unbuffered:put('event2') --unlock waiter
    unbuffered:get() --wait
end)()

local str=unbuffered:get() --wait
print('protected get',str)
unbuffered:put(true) --unlock waiter
str=unbuffered:get() --wait
print('protected get',str)
unbuffered:put(true) --unlock waiter

--[[ Output:
put	event1
got	event1
put	event2
got	event2
--]]