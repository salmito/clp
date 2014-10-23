--[[

Asynchronous channels do not block when there is no space left in their buffer, instead the corresponding put(...) call fails. To create an assynchronous channel, 
pass 'true' as the second parameter of the channel create call.

--]]
local clp=require'clp'

local async=clp.channel(2,true) --purely asynchronous

print(async:put'event1')
print(async:put'event2')
print(async:put'event3')
--[[ Output:
true
true
nil	Event queue is full
--]]