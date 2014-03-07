local lstage=require'lstage'
local channel=require'lstage.channel'

lstage.pool:add()
lstage.pool:add()
lstage.pool:add()

print(lstage.pool:size())

local n=1000000

local chan=channel.new()

local s2=lstage.stage(function()
	i=(i or 0)+1
	local init=lstage.now()
	while true do
--		print('waiting')
		local c,t=chan:pop()
		--print('chan',i,c,t)
		if t==n then
			print(lstage.now()-init)
		end
		i=i+1
	end
end,4):push():push():push():push()
print("stage2",s2)
--s2:push()
	
local s1=lstage.stage(function() 
	for i=1,n do
		chan:push('test',i,"TTTT")
	end
end)
lstage.event.sleep(1)
s1:push()
lstage.event.sleep(100)
