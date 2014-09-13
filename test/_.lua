local lstage=require'lstage'
local _={}
local unpack=unpack or require'table'.unpack

local function assertType(v,t) 
  assert(type(v)==t,"invalid parameter type")
end

function _.__persist() return function() return require'_' end end

function _.par(...)
	local par={...}
	local s={}
	local ret={}
	for i=1,#par do
		if type(par[i])~="function" then error'all arguments must be functions' end
	end
	for i=1,#par do
		s[i]=lstage.stage(par[i])(...)
	end
	for i=1,#par do
		ret[i]=s[i]:output():get()
	end
  	return unpack(ret)
end


local arg={...}
if #arg==0 then
	lstage.pool:add(lstage.cpus())
	print(_.par(
					function() print('hello') lstage.event.sleep(1) return 1 end,
					function() print('world') return 2 end)
					)
	
end


return setmetatable(_,_)
