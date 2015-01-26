local clp=require'clp'

local init=clp.now()
clp.pool:add(clp:cpus())
print('pool',clp.pool:size())

local function call(s,...)
  local input,output=clp.channel(1),clp.channel(1)
  
  input:put(...)
  s(input,output)
  return output:get()
end

local function sync(f,...)
  return clp.spawn(function(i,o)
     o:put(f(i:get()))
  end,...)
end


local function computeHash(str)
  -- Do fancy calculations
  local i=0
  for i=1,1000000000 do
    i=i+1
  end
  return str,i
end

local authenticate=sync(function(user,passwd)
    i=(i or 0)+1
    print('passwd',user,passwd,i)
    if user.hash==computeHash(passwd) then
      return 'success',user.user,clp.now()-init
    end
    return nil,'password not match',clp.now()-init
end,clp:cpus())


print(clp.now()-init,call(authenticate,{user='Me',hash='pass'},'pass2'))
print(clp.now()-init,call(authenticate,{user='Me',hash='pass123'},'pass'))
print(clp.now()-init,call(authenticate,{user='Me',hash='pass123'},'pass123'))
print(clp.now()-init,call(authenticate,{user='Me',hash='pass123'},'pass321'))

local function chain(s1,s2,...) 
  return clp.spawn(
    function(...)
      s2(call(s1,...))
    end,...)
end

local print_s=clp.spawn(function(...)
    print(clp.now()-init,...)
end)

local a=chain(authenticate,print_s,clp:cpus())

print(clp.now()-init,'puting')
a({user='Me',hash='pass'},'pass2')
a({user='Me',hash='pass123'},'pass')
a({user='Me',hash='pass123'},'pass321')
a({user='Me',hash='pass123'},'pass123')
print(clp.now()-init,'asynchronous call')
while true do clp.event.sleep(1000) end
