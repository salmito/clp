local clp=require'clp'
local pool=require'clp.pool'


local sLoop=clp.task(function()
	print('loop1',clp.self():pool():size())
	while true do end
end)()

local sLoop2=clp.task():setpool(pool.new(1)):wrap(function()
	print('loop2',clp.self():pool():size())
	local t=clp.now()
	while clp.now()-t<2 do end
end)()

local sLoop3=clp.task():setpool(sLoop2:pool()):wrap(function()
	print('Happened after 2 seconds',clp.self():pool():size())
	while true do end
end)()

clp.event.sleep(3)
