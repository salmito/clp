local clp=require'clp'

local function line(file,c)
  local c=c or clp.channel()
	clp.process(function()
		local io=require'io'
		f=assert(io.open(file,"r"))
		local f=f
		local count = 1
		while true do
			local line = f:read()
			if line == nil then break end
			assert(c:put(line,count))
			count = count + 1
      clp.event.sleep(0)
		end
		error()
	end,function(e) local io=require'io' c:close() if f then io.close(f) end return e end)()
	return c
end

local i=clp.channel()

line('/etc/passwd',i)
line('/proc/cpuinfo',i)

for l in require'clp.range'(i) do print(l) end

return line
