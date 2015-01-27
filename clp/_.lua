local clp=require'clp'
local chan=require'clp.channel'


--============== Useful functions ============--

local id=function(...) return ... end
local printf=function (str) return function(...) print(require'string'.format(str,...)) end end
local inspect=function (prefix) return function(...) print(prefix,...) return ... end end

--============== Useful processes ============--

local map=function(t,f)
  local p=clp.spawn()
  local input=t.input
  p:setinput(input)
  local out=clp.channel()
  p:wrap(function(...) out:put(f(...)) end,function(err) out:close() if require'string'.find(err or "","Channel is closed") then err=nil end return err end)
  t.input=out
  return t
end

local filter=function(t,f)
  local p=clp.spawn()
  local input=t.input
  p:setinput(input)
  local out=clp.channel()
  p:wrap(function(...) if f(...) then out:put(...) end end,function(err) input:close() out:close() if require'string'.find(err or "","Channel is closed") then err=nil end return err end)
  t.input=out
  return t
end

local take=function(t,i)
  local p=clp.spawn()
  local input=t.input
  p:setinput(input)
  local out=clp.channel()
  p:wrap(function(...) out:put(...) i=i-1 if i==0 then error() end end,function(err) input:close() out:close() if require'string'.find(err or "","Channel is closed") then err=nil end return err end)
  t.input=out
  return t
end

local fold=function(t,f,i)
  local chan=clp.channel()
  local temp={i}
  
  local p=clp.spawn()
  local input=t.input
  p:setinput(input)
  local out=clp.channel()
  p:wrap(function(...) temp[1]=f(temp[1],...) end,function(err) out:put(temp[1]) out:close() if require'string'.find(err or "","Channel is closed") then err=nil end return err end)
  t.input=out
  return t
end

function value(t) return t.input:get() end

local mt={
    __index={
        map=map,
        filter=filter,
        take=take,
        fold=fold,
        value=value,
        inspect=inspect
      },
    __newindex=function(...)
      error('')
    end,
    __call=function(t,...) return t.first:put(...) end
}

local _=function(f,chan)
  local p=clp.spawn(f,function(e) chan:close() return e end)
  return setmetatable({input=chan,first=p:input()},mt)
end

local cat=function(arr)
  local out=clp.channel()
  local f=function() 
      for k,v in ipairs(arr) do out:put(k,v) end 
      out:close()
  end
    
  local proc=_(f,out)
  proc()
  return proc
end

if not (...) then 
  print'CLP underscore test procedure' 
 
  local phoneNumbers = {5554445555, 1424445656, 5554443333,31232132,312321213,213321321,431,45,454554,47,7656876,87,867,8,679,78,545555,425,345,54334}
  
  function contains(pat) 
    return function(k,v)
      if require'string'.find(v, pat) then
        return true
      else
        return false
      end 
    end
  end
  
  
  local proc=cat(phoneNumbers)
  :map(inspect'phase 1')
  :filter(contains"555")
  :map(inspect'phase 2')
  :take(10)
  :map(inspect'phase 3')
  :fold(function(a,_,_) return a+1 end,0)
  :map(inspect'phase 4')
  :value()

  print('result',proc)
 
end

return {
  cat=cat,
  lift=_
}