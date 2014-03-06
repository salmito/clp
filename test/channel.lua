local lstage=require'lstage'
local channel=require'lstage.channel'

local chan=channel.new()

local s2=lstage.stage(function()
	i=(i or 0)+1
	while true do
		print('waiting')
		local c,t=chan:pop()
		print('chan',i,c,t)
		i=i+1
	end
end,2):push():push()
print("stage2",s2)
--s2:push()
	
local s1=lstage.stage(function() 
	for i=1,10 do
		chan:push('test',i)
		--s2:push()
	end
end)
lstage.event.sleep(1)
s1:push()
lstage.event.sleep(10)
