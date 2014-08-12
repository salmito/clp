local lstage=require "lstage"

--lstage.pool:add(lstage.cpus())

local pool=lstage.pool
pool:add(th or 0)

local n=n or 100
local it=it or 100000
local thn=lstage.now()
local thn_clk=os.clock()
local now=lstage.now

local finish=lstage.channel()

local s2=lstage.stage(function()
	i=(i or 0) +1
	if i==n then
		finish:push('end')
	end 
end)

local function f(data)
  local n=0
  for i=1,it do
    n=(n+i)
  end
  s2=s2 and s2:push(data)
  --print('enddd',i)

end

local s={}

--local out=lstage.channel()
local s1=lstage.stage(f,n)
for i=1,n do
	s1:push(1)
end

print("initiated",now()-thn)
thn=now()
 print("finished",finish:get())

print("end",pool:size(),n,it,now()-thn,os.clock()-thn_clk)
thn=lstage.now()
thn_clk=os.clock()
for i=0,n do
  f(1)
end
print("serial",now()-thn,os.clock()-thn_clk)
