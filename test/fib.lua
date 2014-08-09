local lstage=require'lstage'

local v=tonumber(({...})[1]) or 10

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
local function fib(n)
	if n==0 then return 0 end
	if n==1 then return 1 end
	return fib(n-1)+fib(n-2)
end

local cutoff=35

lstage.channel()

local memoize=function(f) return 
	function(v)
		ret=ret or {}
		if not ret[v] then
			ret[v]=f(v)
		end
		return ret[v]
	end
end


local s=lstage.stage(function(v,c)
	i=(i or 0)+1
	local fib_=memoize(fib)
	if v < cutoff then return c:push(fib_(v)) end
	local l=lstage.channel()
	local r=lstage.channel()
	lstage.self():push(v-1,l)
	lstage.self():push(v-2,r)
	c:push(l:get()+r:get())
end):add(cutoff)



local r=lstage.channel()

s:pool():add(4)
s(v,r)
print(r:get())
bench:clock('par')
--bench:start()
--local ret=fib(v)
--bench:clock('seq')
--print(ret)
