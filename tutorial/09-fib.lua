local clp=require'clp'

local n=1000

clp.pool:add(clp.cpus()-1)

local fib=clp.process()

fib:wrap(function (a,b,current,goal,ret)
    if current>=goal then
      ret:put(b)
      return
    end
    fib(b,a+b,current+1,goal,ret)
end)


local final=clp.channel()

fib(1,1,2,n,final)
print(n..'th fib is',final:get())

local function fib_(a,b,current,goal)
    if current>=goal then
      return b
    end
    return fib_(b,a+b,current+1,goal)
end

print(fib_(1,1,2,n))
