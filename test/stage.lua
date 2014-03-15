local lstage=require'lstage'
local s1=lstage.stage()
s1:wrap(function() print('run',lstage.pool:size(),s1==self,self:getpool()==lstage.pool) end)
s1:instantiate(1)
s1:push()
lstage.event.sleep(1)
