local lstage=require'lstage'
local p=print
--local d=debug.traceback
local s1=lstage.stage(function(str) 
	print(str)
    error('some error '..str)
end,
function(...) print('error',...) print(require'debug'.traceback()) end
):push("par2",'test'):push('par3')
lstage.event.sleep(1)
