local clp=require "clp"
local interpreter=jit and jit.version or _VERSION or 'unknown'
clp.pool:add(clp.cpus()-1)
local format=require'string'.format
local pool=clp.pool
local arg={...}
--pool:add(tonumber(arg[3] or clp.cpus()-1))
local np=tonumber(arg[1]) or 10000
local nm=tonumber(arg[2]) or 500000
local it=tonumber(arg[3]) or 4096

local bench=nil
bench=function () return {
	start=function(self)
		self.t=clp.now()
		self.c=os.clock()
	end,
	tick=function(self)
		return clp.now()-self.t,os.clock()-self.c
	end,
	clock=function(self,str)
		local t,c=self:tick()
		print(format('%s\t%.3f\t%.3f',str,t,c))
	end,
  latency=function(self,str,n)
    local t,c=self:tick()
		print(format('%s\t%.3f\t%.3f',str,(t/n)*1000.0,(c/n)*1000.0))
  end,
  troughput=function(self,str,n)
    local t,c=self:tick()
		print(format('%s\t%.3f\t%.3f',str,n/t,n/c))
  end,
  bench=bench
} end

local str='\t'..interpreter..'\t'..np..'\t'..nm..'\t'..it
local startup=bench()
bench=bench()

bench:start()
startup:start()

local finish=clp.channel()

for i=1,np do
  clp.process(function() finish:put() error() end)()
end

print('test','interpreter','n_proc','n_msg','size','real_time','cpu_time')
startup:clock('creation startup (s)'..str)
for i=1,np do
  finish:get()
end
bench:clock('creation time (s)'..str)
bench:latency('creation latency (ms per proc)'..str,np)
bench:troughput('creation troughput (proc/s)'..str,np)

bench:start()

local finish,iteration=clp.channel(),clp.channel()

clp.process(function(str) 
      for i=1,nm do
        iteration:put(str)
      end
      error() 
 end)(string.rep('X',it))
 
 clp.process(function() 
      for i=1,nm do
        assert(#iteration:get()==it)
      end
      finish:put()
      error() 
 end)()
 
finish:get()
 
bench:clock('roundtrip time (s)'..str)
bench:latency('roundtrip latency (ms per msg)'..str,nm)
bench:troughput('roundtrip troughput (msg/s)'..str,nm)