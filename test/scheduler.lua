local clp=require'clp'
local s=require'clp.task'
local e=require'clp.event'
local sched=require'clp.scheduler'

local pool=require'clp.pool'.new()
print("Created a thread_pool",pool)

local th={}

for i=1,clp.cpus() do
	th[#th+1]=pool:add() 
end
print("Pool size",pool:size())
print(assert(pool:size()==clp.cpus()," incorrect pool size") and  "pool size OK")

local task1=s.new()
local task2=s.new()



print("Created task",task1,task1:pool(),task1:env())

print(assert(task2:pool()==clp.pool,"incorrect pool") and "task2 pool OK "..tostring(clp.pool))
print("task2 pool size",clp.pool:size())

local task1=clp.task(function(str) 
	while true do
		print(str)
		e.sleep(0.5)
		task2:push('event','test2')
	end
end,function() print(require'debug'.traceback()) end)

task1:setpool(pool)
print(assert(task1:pool()==pool,"incorrect pool") and "task1 pool OK "..tostring(pool))
local i=0
task2:wrap(function(e,f) i=i+1 print(e,f,i) end)

print("task1",task1)
print("task2",task2)

print("Instantiated",task1,task1:size())

task1:push('test')

--task2:add(1)

e.sleep(10)
