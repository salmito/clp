local lstage=require'lstage'


local function add_timer(time,f)
	return lstage.stage(function()
	 	while true do
			lstage.event.sleep(time)
			f()		
		end
	end):push()
end

function handler(id) return function()
		thn=thn or lstage.now()
		print(require'string'.format("id %d slept for %.6f",id,lstage.now()-thn))
		thn=lstage.now()
	end
end

add_timer(0.1,handler(1))
add_timer(0.4,handler(2))
add_timer(0.8,handler(3))

lstage.channel():get() --dorme para sempre
