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
