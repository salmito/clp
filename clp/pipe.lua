local clp=require'clp'

local t={}

function t.stdin(c)
	local chan=c or clp.channel()
	local close=clp.channel(1)
	local p=clp.process(function()
		local io=require'io'
		while true do
			clp.event.waitfd(0,clp.event.READ)
			local t,err=io.stdin:read()
			if not t then error() end
			assert(chan:put(t))
		end
	end,function(err)
		chan:close()
		close:put(err)
		close:close()
	end)()
	return chan,close
end


function t.stdout(c,postfix)
	local chan=c or clp.channel()
	local close=clp.channel(0)
	local p=clp.process(function()
		local io=require'io'
		local i=1
		while true do 
			clp.event.waitfd(1,clp.event.WRITE)
			local t,str=pcall(chan.get,chan)
			if not t then error() end
			io.stdout:write(str)
			io.stdout:write(postfix or '\n')
		end
	end,
	function()
		close:put(err)
		close:close()
	end)()
	return chan,close
end

function t.stderr(c,postfix)
	local chan=c or clp.channel()
	local close=clp.channel(0)
	local p=clp.process(function()
		local io=require'io'
		local i=1
		while true do 
			clp.event.waitfd(1,clp.event.WRITE)
			local t,str=pcall(chan.get,chan)
			if not t then error() end
			io.stderr:write(str)
			io.stderr:write(postfix or '\n')
		end
	end,
	function()
		close:put(err)
		close:close()
	end)()
	return chan,close
end

return t
