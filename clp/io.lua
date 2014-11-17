local clp=require'clp'

local t={}

function t.line(file,c)
	local c=c or clp.channel()
	clp.process(function()
		require'io'
		f=assert(io.open(file,"r"))
		local f=f
		local count = 1
		while true do
			local line = f:read()
			if line == nil then break end
			assert(c:put(line,count))
			count = count + 1
		end
		error()
	end,function(e) c:close() if f then io.close(f) end return e end)()
	return c
end



return t
