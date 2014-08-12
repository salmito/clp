print('package',package.cpath)
local lstage=require'lstage'
local p=print
--local d=debug.traceback
local s1=lstage.stage(function(str) 
print(str)
    error('some error '..str)
end,
function(...) print('error',...) return require'debug'.traceback() end
):push("par2",'test'):push('par3')

local function f()
	error('error here')
end

local function g()
	f()
end

lstage.stage(function() g() end):push()
	

lstage.event.sleep(1)
