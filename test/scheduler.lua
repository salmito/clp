local s=require'lstage.stage'
local sched=require'lstage.scheduler'
local pool=require'lstage.pool'
local event=require'lstage.event'

local stage3=s.new()
local stage=s.new(function(...) 
	print('first',self,stage3,...)
	local stage2=s.new(function(s1,...) 
		s1:setenv(event.encode(function(...) 
			print('third',...)
		end))
		print('second',s1,...)
		s1:push(...)
	end)
	stage2:setpool(p)
	stage2:push(stage3,...)
end)
local p=pool.new()
print("Pool",p)
stage:setpool(p)
stage3:setpool(p)
local th=p:add()
stage3:instantiate(10)
stage:push(24,"event",math.pi)
print(stage,"instances:",stage:instances())
print(stage3,"instances:",stage3:instances())
local n=stage3:free(10)
print(stage3,"destroyed:",n,stage3:instances())
th:join()

