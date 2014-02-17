local lstage=require'lstage'
local s=require'lstage.stage'
local e=require'lstage.event'
local sched=require'lstage.scheduler'

local pool=require'lstage.pool'.new()
print("Created a thread_pool",pool)
print("Pool size",pool:size())
local th={}

for i=1,100 do
	th[#th+1]=pool:add() 
	print("Pool size",pool:size())
end

local stage=s.new()

print("Created stage",stage,stage:getpool(),stage:getenv())
--stage:setpool(pool)
print("Set pool of stage",stage,stage:getpool())

stage:setenv(function(str) 
	e.sleep(0.5)
	print'yeah'
	
end)
print("Env stage",stage,stage:getenv())
stage:instantiate(100)
print("Instantiated",stage,stage:instances())
for i=1,100 do
	stage:push('world')
end

for i,t in ipairs(th) do
	print(i,t)
--	pool:kill()
end

for i,t in ipairs(th) do
	t:join()
	print("thread ended")
end

e.sleep(10)

--[[
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
]]
