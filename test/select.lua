local clp=require'clp'

clp.pool:add(clp.cpus()-1)

local select=clp.spawn(function(out,...)
	local arg={...}
  local len=#arg
  local gc=clp.channel()
	for i=1,len do
		local c=arg[i]
		clp.spawn(function() while true do out:put(c:get()) end end,function (e) gc:put() end)()
	end
  clp.spawn(function() 
      for i=len,1,-1 do 
          print('finished',i)
        gc:get() 
      end
      print('closing output')
      out:close()
      clp.self():input():close()
    end,
    function() end)()
	return out
end)

local c1,c2,c3=clp.channel(),clp.channel(),clp.channel()

local done=clp.channel()

local function f(name,c)
	print('sending to',c)
	for i=1,10 do
		c:put(name)
		--print('put',name)
	end
	c:close()
end

local p1,p2,p3=clp.spawn(f),clp.spawn(f),clp.spawn(f)

p1('task1',c1)
p2('task2',c2)
p3('task3',c3)

local out=clp.channel()
select(out,c1,c2,c3)

print('select',out)

local function f() return out:get() end

while true do
  local res,msg=pcall(f)
  if not res then break end
	print('got event',msg)
end
