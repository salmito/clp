local lstage=require'lstage'
local time=lstage.now()

local th=lstage.scheduler.new_thread()

for i=1,50000 do
	lstage.stage(function() end)
end

lstage.scheduler.kill_thread()
th:join()
print("ended",lstage.now()-time)
