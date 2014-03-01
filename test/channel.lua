local lstage=require'lstage'
local channel=require'lstage.channel'

local chan=channel.new()

local s1=lstage.stage(function() print('chan',chan:get()) end)
local s2=lstage.stage(function() print('chan',chan:push('test')) end)
s1:push()
s2:push()
lstage.event.sleep(10)
