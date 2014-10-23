local clp=require'clp'
local t=getmetatable(clp.channel())
local select=select
local unpack=unpack or table.unpack
 
local function iter (c)
    local r={pcall(c.get,c)}
    if r[1]==true then
      return select(2,unpack(r))
    end
    return nil
end

t.range=function(self)
    return iter, self
end

if not (...) then
  local c=clp.channel()
  clp.process(function()
      for i=1,10 do c:put(i) end
      c:close()
  end)()
  
  for i in c:range() do
    print(i)
  end
  
end