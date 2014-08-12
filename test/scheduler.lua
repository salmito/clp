local lstage=require'lstage'
local s=require'lstage.stage'
local e=require'lstage.event'
local sched=require'lstage.scheduler'

local pool=require'lstage.pool'.new()
print("Created a thread_pool",pool)

local th={}

for i=1,lstage.cpus() do
	th[#th+1]=pool:add() 
end
print("Pool size",pool:size())
print(assert(pool:size()==lstage.cpus()," incorrect pool size") and  "pool size OK")

local stage1=s.new()
local stage2=s.new()



print("Created stage",stage1,stage1:pool(),stage1:env())

print(assert(stage2:pool()==lstage.pool,"incorrect pool") and "stage2 pool OK "..tostring(lstage.pool))
print("stage2 pool size",lstage.pool:size())

local stage1=lstage.stage(function(str) 
	while true do
		print(str)
		e.sleep(0.5)
		stage2:push('event','test2')
	end
end,function() print(require'debug'.traceback()) end)

stage1:setpool(pool)
print(assert(stage1:pool()==pool,"incorrect pool") and "stage1 pool OK "..tostring(pool))
local i=0
stage2:wrap(function(e,f) i=i+1 print(e,f,i) end)

print("stage1",stage1)
print("stage2",stage2)

print("Instantiated",stage1,stage1:size())

stage1:push('test')

--stage2:add(1)

e.sleep(10)
