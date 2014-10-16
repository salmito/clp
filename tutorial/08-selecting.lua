local clp=require'clp'

clp.pool:add(clp.cpus()-1)

local function pipe(input,out) 
  while true do 
    out:put(input:get()) 
  end
end

local multiplex=clp.channel()

local select=clp.task(function(...)
	local arg={...}
	for i=1,#arg do
		clp.task(pipe,function(e) print(e) multiplex:put(nil,'channel was closed') end)(arg[i],multiplex)
	end
end)

local c1,c2,c3=clp.channel(),clp.channel(),clp.channel()

local function f(name,c)
	for i=1,10 do
		c:put(name,i)
	end
  c:close()
end

local p1,p2,p3=clp.task(f),clp.task(f),clp.task(f)

p1('task1',c1)
p2('task2',c2)
p3('task3',c3)

select(c1,c2,c3)
print('select',multiplex)

local e=0
while true do
  local i,t=multiplex:get()
  e=not i and e+1 or e
  print('got event',i,t)
  if e==3 then break end
end
