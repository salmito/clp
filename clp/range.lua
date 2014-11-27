local channel=require'clp.channel'
local t=getmetatable(channel.new())

local function geniter(c)
  local select=select
  local unpack=unpack or require'table'.unpack
  local pcall=pcall
  return function()
    local r={pcall(c.get,c)}
    if r[1]==true then
      return select(2,unpack(r))
    end
    return nil
   end
end

t.range=function(self,b,step)
  local is=channel.ischannel
  assert(is(self) or type(self)=='number', "Channel or number expected")
  if channel.ischannel(self) then
    return geniter(self), self
  else
    local a=self
    if (step == nil) then
      step = 1
    end
    assert(type(step) == 'number', 'Step parameter must be a number.')
    assert(step ~= 0, 'Step parameter cannot be zero.')
    
    if step > 0 then
      if a > b then 
        error("First parameter must be less than second parameter.")
      end
    else 
      if a < b then 
        error("First parameter must be greater than second parameter.") 
      end 
    end
    
    local i = a
  
    return function()
      local ret = i
      i = i + step
      if step > 0 then
        if ret <= b then
          return ret
        end 
      else 
        if ret >= b then
          return ret
        end         
      end
    end
  end
end

t.__wrap=function(c)
  local id=c:__id()
  return function() 
    require('clp.range')
    return require'clp.channel'.get(id)
  end
end

--test
if not (...) then
  local clp=require'clp'
  local c=clp.channel()
  local range=t.range
  print(c)
  clp.process(function()
      for i=1,10 do for j in range(1,2) do c:put(i,j) end end
      c:close()
  end)()
  
  for i in c:range() do
    print(i)
  end
  
 
end

return t.range