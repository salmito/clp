local lstage=require'lstage'
local pool=require'lstage.pool'


local sLoop=lstage.stage(function()
	print('loop1',lstage.self():pool():size())
	while true do end
end):setpool(pool.new(1)):push()

local sLoop2=lstage.stage(function()
	print('loop2',lstage.self():pool():size())
	local t=lstage.now()
	while lstage.now()-t<2 do end
end):setpool(pool.new(1)):push()

local sLoop3=lstage.stage(function()
	print('Happened after 2 seconds',lstage.self():pool():size())
	while true do end
end):setpool(sLoop2:pool()):push()

lstage.event.sleep(4)
