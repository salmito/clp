local clp=require'clp'
local p=print
local i={10}
local s1=clp.spawn(function(str) 
  i[1]=20
  print(str,i[1])
  
    error('some error '..str)
end,
function(...)  print('error',i[1],...) return require'debug'.traceback() end
)("par2",'test')('par3')

local function f()
	error('error here')
end

local function g()
	f()
end

clp.spawn(function() g() end)()
	

clp.event.sleep(1)
