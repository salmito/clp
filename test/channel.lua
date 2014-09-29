local lstage=require'lstage'

local s1,s2,s3=lstage.stage(),lstage.stage(),lstage.stage()
print('init',lstage.pool:size())
lstage.pool:kill()
print('init',lstage.pool:size())
lstage.pool:add()
print('init',lstage.pool:size())

local chan=lstage.channel()
local chan2=lstage.channel()

local function p(name)
  print('entering',name)
  while true do
    local t={chan:get()}
    print(name,'received',(require'table'.unpack or unpack)(t))
    if #t==0 then break end
  end
  chan2:push('finished')
end

s1:wrap(function()
  local i=0
  while i<10 do
    chan:push('event',i)
    i=i+1
  end  
  chan:push()
end)()

s3:wrap(p)('stage3')
s2:wrap(p)('stage2')
print('end',chan2:get())
local chan3=lstage.channel(1)
chan3:push(1)
print('should fail',chan3:push(1))

local chan4=lstage.channel():push(1):push(2):push(3)
chan4:close()

print(chan4:get(),chan4:get(),chan4:get())

print(pcall(chan4.get,chan4)) --fail

local s=lstage.stage(function() print('hello') end)

s:input():close()
lstage.event.sleep(1)
