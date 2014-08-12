local lstage=require'lstage'
local pool=require'lstage.pool'


local sLoop=lstage.stage(function()
	print('loop1',lstage.self():pool():size())
	while true do end
end):push()

local sLoop2=lstage.stage():setpool(pool.new(1)):wrap(function()
	print('loop2',lstage.self():pool():size())
	local t=lstage.now()
	while lstage.now()-t<2 do end
end):push()

local sLoop3=lstage.stage():setpool(sLoop2:pool()):wrap(function()
	print('Happened after 2 seconds',lstage.self():pool():size())
	while true do end
end):push()

lstage.event.sleep(3)
