local lstage=require'lstage'

local init=lstage.now()
lstage.pool:add(lstage:cpus())
print('pool',lstage.pool:size())

local function call(s,...)
  local input,output=lstage.channel(1),lstage.channel(1)
  
  input:push(...)
  s:push(input,output)
  return output:get()
end

local function sync(f,...)
  return lstage.stage(function(i,o)
     o:push(f(i:get()))
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
      return 'success',user.user,lstage.now()-init
    end
    return nil,'password not match',lstage.now()-init
end,lstage:cpus())


print(lstage.now()-init,call(authenticate,{user='Me',hash='pass'},'pass2'))
print(lstage.now()-init,call(authenticate,{user='Me',hash='pass123'},'pass'))
print(lstage.now()-init,call(authenticate,{user='Me',hash='pass123'},'pass123'))
print(lstage.now()-init,call(authenticate,{user='Me',hash='pass123'},'pass321'))

local function chain(s1,s2,...) 
  return lstage.stage(
    function(...)
      s2:push(call(s1,...))
    end,...)
end

local print_s=lstage.stage(function(...)
    print(lstage.now()-init,...)
end)

local a=chain(authenticate,print_s,lstage:cpus())

print(lstage.now()-init,'pushing')
a:push({user='Me',hash='pass'},'pass2')
a:push({user='Me',hash='pass123'},'pass')
a:push({user='Me',hash='pass123'},'pass321')
a:push({user='Me',hash='pass123'},'pass123')
print(lstage.now()-init,'asynchronous call')
while true do lstage.event.sleep(1000) end