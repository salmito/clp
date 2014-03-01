local lstage=require'lstage'
local channel=require'lstage.channel'

local chan=channel.new()

local s1=lstage.stage(function() print('chan',chan) end)
s1:push()
lstage.event.sleep(1)
