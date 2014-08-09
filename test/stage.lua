local lstage=require'lstage'
local s1=lstage.stage()
local function handler(...) 
--print('args',...)

print('s1',s1,lstage:self(),s1==lstage:self())
assert(s1==lstage:self())

print('pool',lstage.pool,s1:pool(),lstage.pool==s1:pool())
assert(lstage.pool==s1:pool())

	
	lstage.stage(function()
		print(
		'nested stage result', 
			assert(lstage:self():parent()==s1)
		)
		collectgarbage('collect')
	end):push()
	
	collectgarbage('collect')
end

s1:wrap(handler)
s1:add(1)
print('push',s1,s1:push('par1',math.pi))

print(s1,s1:env(),s1:size(),s1:input())


s1=nil
collectgarbage('collect')
lstage.event.sleep(1)
