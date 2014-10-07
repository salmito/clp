local clp=require'clp'

local s1,s2,s3=clp.task(),clp.task(),clp.task()
print('init',clp.pool:size())
clp.pool:kill()
print('init',clp.pool:size())
clp.pool:add(clp.cpus()-1)
print('init',clp.pool:size())

local chan=clp.channel()
local chan2=clp.channel()

local function p(name)
  print('entering',name)
  while true do
    local t={chan:get()}
    print(name,'received',(require'table'.unpack or unpack)(t))
    if #t==0 then break end
  end
  chan2:put('finished')
end

s1:wrap(function()
  local i=0
  while i<10 do
    chan:put('event',i)
    i=i+1
  end  
  chan:put()
end)()

s3:wrap(p)('stage3')
s2:wrap(p)('stage2')
print('end',chan2:get())
local chan3=clp.channel(1,true)
chan3:put(1)
print('should fail',chan3:put(1))

local chan4=clp.channel()
chan4:put(1)
chan4:put(2)
chan4:put(3)
chan4:close()

print(chan4:get(),chan4:get(),chan4:get())

print(pcall(chan4.get,chan4)) --fail

local s=clp.task(function(msg) print('hello',msg) return msg end):add(4)('john')('paul')('george')('ringo')

s:input():close()
while true do print('resp',s:output():get()) end
