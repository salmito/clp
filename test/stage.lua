local lstage=require'lstage'
local s1=lstage.stage()
s1:wrap(function() print('run',lstage.pool:size(),s1==lstage.self(),lstage.self():pool()==lstage.pool) end)
s1:instantiate(1)
s1:push()
lstage.event.sleep(1)
