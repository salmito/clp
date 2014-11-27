local clp=require'clp'
local s1=clp.process()
local function handler(...) 
--print('args',...)

print('s1',s1,clp:self(),s1==clp:self())
assert(s1==clp:self())

print('pool',clp.pool,s1:pool(),clp.pool==s1:pool())
assert(clp.pool==s1:pool())

	
	clp.process(function()
		print(
		'nested task result', 
			assert(clp:self():parent()==s1)
		)
		collectgarbage('collect')
	end)()
	
	collectgarbage('collect')
end

s1:wrap(handler)
s1:spawn(1)
print('put',s1,s1('par1',math.pi))

print(s1,s1:env(),s1:size(),s1:input())


s1=nil
collectgarbage('collect')
clp.event.sleep(1)
