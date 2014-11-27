local clp=require'clp'

clp.pool:add(2)

local function add_timer(time,f)
	return clp.process(function()
	 	while true do
			clp.event.sleep(time)
			f()		
		end
	end)()
end

function handler(id) return function()
		thn=thn or clp.now()
		print(require'string'.format("id %d slept for %.6f",id,clp.now()-thn))
		thn=clp.now()
	end
end

add_timer(0.1,handler(1))
add_timer(0.4,handler(2))
add_timer(0.8,handler(3))

clp.channel():get() --sleeps forever
