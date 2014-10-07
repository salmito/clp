local clp=require'clp'
local p=print
local i={10}
local s1=clp.task(function(str) 
  i[1]=20
  print(str,i[1])
  
    error('some error '..str)
end,
function(...)  print('error',i[1],...) return require'debug'.traceback() end
):push("par2",'test'):push('par3')

local function f()
	error('error here')
end

local function g()
	f()
end

clp.task(function() g() end):push()
	

clp.event.sleep(1)
