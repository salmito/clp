local lstage=require'lstage'
lstage.defaultpool:add()

local stage1=lstage.stage(function() print('s1') end,10)
local stage2=lstage.stage(function() print('s2') end,10)

stage1:setpriority(1)
stage2:setpriority(1)

for i=1,10 do
	stage1:push()
	stage2:push()
end



lstage.event.sleep(10)
