local lstage=require'lstage'
local _={}

local function assertType(v,t) 
  assert(type(v)==t,"invalid parameter type")
end

function _.__persist() return function() return require'_' end end

function _.future(f,e)
	local mt=nil
	mt={
		__call=function(self,...) 
			local future=lstage.channel(1)
			self.s(future,...)
			return future
		end,
		push=function(self,...)
			return self(...)
		end,
		wrap=function(self,f,e)
			return self.s:wrap(function(c,...) c:push(f(...)) end ,e)
		end,
		__persist=function (self)
			local s=self.s
			return function() 
				return setmetatable({s=s},mt)
			end
		end,
		__tostring=function (self)
			return "future ["..tostring(self.s).."]"
		end
	}
	mt.__index=mt

  	 local ret={
  	  s=f and lstage.stage(function(c,...) c:push(f(...)) end ,e) or lstage.stage()
  	}
  	return setmetatable(ret,mt)
end


local arg={...}
if #arg==0 then
    print('_ test')
    local s=_.future()
    s:wrap(function(...) print('event',s,...) return 'return' end)
    print(s)
    local f=s('test')
    print(f,f:size())
    print(f:get())
    local f2=_.future(function() local f=s('inner') print('inner',f:get()) end)()
    print(f2:get())
end


return setmetatable(_,_)
