--[[
  A process is a lightweight thread of execution with its own independent state.
  
  To create a process, use t=csp.process(f), where f is the function of the task and t will point to the new process object.
  
  To invoke a process, call the process object. This process will execute concurrently with the calling one.
  
  You can also start a process for an anonymous function.
--]]

local clp=require'clp'

local function f(string) 
    for i=1,4 do        
      print(string, i)
    end
end

f("direct")

local t=clp.spawn(f)
t("task")

clp.spawn(function (...) 
    f(...)
end)("anonymous")

io.stdin:read()

--[[ Output:
direct	1
direct	2
direct	3
direct	4
task	1
task	2
task	3
task	4
anonymous	1
anonymous	2
anonymous	3
anonymous	4
--]]