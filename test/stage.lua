local lstage=require'lstage'
local s1=lstage.stage()
s1:setenv(function() print('run',lstage.default:size(),s1==self,self:getpool()==lstage.default) end)
s1:instantiate(1)
s1:push()
lstage.event.sleep(1)