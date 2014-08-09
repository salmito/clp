function f(...) print(...) error("e1") end

print(pcall(f,"ae"))
print(xpcall(f,debug.traceback,"parameter"))