local lstage=require'lstage'

local _=require'_'

local v=tonumber(({...})[1]) or 36
local n=tonumber(({...})[2]) or 36

local bench={
	start=function(self)
		self.t=lstage.now()
		self.c=os.clock()
	end,
	stop=function(self)
		return lstage.now()-self.t,os.clock()-self.c
	end,
	clock=function(self,str)
		local t,c=self:stop()
		print(string.format('%s %.3fs real time %.3fs cpu time',str,t,c))
	end
}

bench:start()

local cutoff=35

local function fib(n)
	if n==0 or n==1 then return n end
	return fib(n-1)+fib(n-2)
end

local s=_.future()
s:wrap(function(n)
	if n < cutoff then return fib(n) end
	local f1,f2=s(n-1),s(n-2)
	return f1:get()+f2:get()
	--return s(v-1):get()+s(v-2):get()
end)

s.s:add(n)

s.s:pool():add(lstage.cpus()-1)

local f=s(v)
local v1=f:get()
print(v1)
bench:clock('par')
bench:start()
local v2=fib(v)
bench:clock('seq')
print(v2)
print('v1==v2',assert(v1==v2,"must be true"))
