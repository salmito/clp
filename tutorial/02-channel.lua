--[[
  Channels connect concurrent tasks. 
  You can send values into channels from one task and receive those values into another task.
  
  Create a new channel with c=clp.channel(). 
  
  Send a value into a channel using the put method. Here we send "ping" to the messages channel we made above, from a new task.
  
  The get method receives a value from the channel. Here we'll receive the "ping" message we sent above and print it out.
  
  When we run the program the "ping" message is passed from one task to another via our channel.
  
  By default, the get method blocks until messages are available. 
  
  This property allows us to wait at the end of our program for the "ping" message without having to use other synchronization.
--]]


local clp=require'clp'

local messages=clp.channel()

clp.process(function()
    messages:put('ping')
end)()

print(messages:get())

--[[ Output:
ping
--]]