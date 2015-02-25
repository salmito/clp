local clp=require'clp'

local v=tonumber(({...})[1]) or 36
local n=tonumber(({...})[2]) or 36

local bench={
	start=function(self)
		self.t=clp.now()
		self.c=os.clock()
	end,
	stop=function(self)
		return clp.now()-self.t,os.clock()-self.c
	end,
	clock=function(self,str)
		local t,c=self:stop()
		print(string.format('%s %.3fs real time %.3fs cpu time',str,t,c))
	end
}

bench:start()

--IMPLEMENTATION START

local cutoff=35

local function fib(n)
	if n==0 or n==1 then return n end
	return fib(n-1)+fib(n-2)
end

local fib_t=clp.spawn()

local i=fib_t:input()

fib_t:wrap(function(res,n)
	if n < cutoff then res:put(fib(n)) return end

	local c1,c2=clp.channel(),clp.channel()

	local _,_,r=i:size()
	if r==0 then fib_t:spawn(1) end
	
	fib_t(c1,n-1)
	fib_t(c2,n-2)

	res:put(c1:get()+c2:get())
end)

fib_t:spawn(n)

fib_t:pool():add(clp.cpus()-1)

out=clp.channel()
fib_t(out,v)
local v1=out:get()
print(v1)
bench:clock('par')
bench:start()
local v2=fib(v)
bench:clock('seq')
print(v2)
print('v1==v2',assert(v1==v2,"must be true"))
