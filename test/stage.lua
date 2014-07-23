local lstage=require'lstage'
local s1=lstage.stage()
s1:wrap(function() print(
	'stage results',
	assert(lstage.pool==s1:pool(),"pool error"),
	assert(s1==lstage.self(),"self error"),
	assert(s1:setpriority(10))
	)
	lstage.stage(function()
		print(
		'nested stage result', 
			assert(lstage.self():parent()==s1)
		)
		collectgarbage('collect')
	end):push()
	collectgarbage('collect')
end)
s1:instantiate(1)
s1:push()
s1=nil
collectgarbage('collect')
lstage.event.sleep(1)
