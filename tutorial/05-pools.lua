--[[

Processes share OS threads from a dynamic pool of threads cooperatively.

By default, all created processes share their parent process's pool or the default global pool accesible by the clp.pool field.

The global pool is initialized with a single thread. To add threads to a pool object, call its add(n) method.

The funcion clp.cpus() is a utility function that returns the number os cpu cores, so that the clp.pool:add(clp.cpus()-1)
call will grow the default pool to one thread per CPU on your computer.

--]]

local clp=require'clp'

clp.pool:add(clp.cpus()-1)
