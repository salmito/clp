--[[
  A task is a lightweight thread of execution with independent state.
  
  To create a task, use t=csp.task(f), where f is the function of the task and t will point to the new task object.
  
  To invoke a task, call the task object. This task will execute concurrently with the calling one.
  
  You can also start a task for an anonymous function.
--]]

local clp=require'clp'

local function f(string) 
    for i=1,4 do        
      print(string, i)
    end
end

f("direct")

local t=clp.process(f)
t("task")

clp.process(function (...) 
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