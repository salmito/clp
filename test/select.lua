local clp=require'clp'

clp.pool:add(clp.cpus()-1)

local select=clp.task(function(...)
	local arg={...}
	local out=clp.channel()
	for i=1,#arg do
		local c=arg[i]
		clp.task(function() while true do out:put(c:get()) end end)()
	end
	return out
end)

local c1,c2,c3=clp.channel(),clp.channel(),clp.channel()

local done=clp.channel()

local function f(name,c)
	print('sending to',c)
	for i=1,10 do
		c:put(name)
		print('put',name)
	end
	c:close()
end

local p1,p2,p3=clp.task(f),clp.task(f),clp.task(f)

p1('task1',c1)
p2('task2',c2)
p3('task3',c3)

local out=select(c1,c2,c3):output():get()
print('select',out)

while true do
	print('got event',out:get())
end
