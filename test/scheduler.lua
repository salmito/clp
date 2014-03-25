local lstage=require'lstage'
local s=require'lstage.stage'
local e=require'lstage.event'
local sched=require'lstage.scheduler'

local pool=require'lstage.pool'.new()
print("Created a thread_pool",pool)
print("Pool size",pool:size())
local th={}

for i=1,lstage.cpus() do
	th[#th+1]=pool:add() 
	print("Pool size",pool:size())
end

local stage1=s.new()
local stage2=s.new()

print("Created stage",stage1,stage1:pool(),stage1:getenv())
--stage:setpool(pool)
print("Set pool of stage",stage1,stage1:pool())

stage1:wrap(function(str) 
	e.sleep(0.5)
	stage2:push('event')
	print(str)
end)

local i=0
stage2:wrap(function(e) i=i+1 print(e,i) end)


print("Env stage",stage1,stage1:getenv())

print("Instantiated",stage1,stage1:instances())
for i=1,100 do
	stage1:push('world')
end

for i,t in ipairs(th) do
	print(i,t)
--	pool:kill()
end

stage1:setpool(pool)
stage2:setpool(pool)
stage2:instantiate(1)
stage1:instantiate(lstage.cpus())

for i,t in ipairs(th) do
	t:join()
	print("thread ended")
end

e.sleep(10)
