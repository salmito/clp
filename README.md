# Communicating Lua Processes
CLP is a Lua library for bulding lightweight parallel processes based on the concepts of CSP (Communicating Sequential Processes).

## Compiling and Installing
CLP is compatible with almost every Lua interpreter: it has been tested with PUC-Rio's Lua version 5.1, 5.2 and 5.3 alpha.

It was also tested with other  Lua interpreters such as Luajit, love2d and torch.

Currently, CLP requires Threading Building Blocks (TBB) and Libevent libraries to work properly.

For more information on the TBB library: http://threadingbuildingblocks.org/

For more information on the Libevent library: http://libevent.org/

If you want to try it out on windows, just clone the https://github.com/Salmito/ZeroBraneStudio repository fork with the CLP binary preinstalled.

To install all dependencies for Lua 5.1 on a Debian like linux (Ubuntu, mint, etc) do: 
```
sudo apt-get install libtbb-dev libevent-dev lua5.1-dev lua5.1
```

Equivalently, to install dependencies for Lua 5.2 on a Debian like linux (Ubuntu, mint, etc) do: 
```
sudo apt-get install libtbb-dev libevent-dev lua5.2-dev lua5.2
```

To build and install CLP for Lua 5.1:
```
   $ make
   $ sudo make install
```

To build and install CLP for Lua 5.2:
```
   $ make LUA_VER=5.2
   $ sudo make install LUA_VER=5.2
```

To uninstall CLP on Lua 5.1:
```
   $ sudo make uninstall
```

To uninstall CLP on Lua 5.2:
```
   $ sudo make uninstall LUA_VER=5.2
```

## Testing installation
To test if the installation was successful type this command:

```
$ lua -l clp
```

You should get the lua prompt if clp was installed properly or the error message "module 'clp' not found"  if it cannot be loaded (check if it was installed on a wrong location).

Then, run the scripts in the test directory to validate the installation.

That's it.

## Reporting BUGS
If you find bugs, please report them on GitHub: https://github.com/Salmito/clp/issues

Or e-mail me: Tiago Salmito - tiago _[at]_ salmito _[dot]_ com

## Copyright notice
CLP is published under the same MIT license as Lua 5.1.

Copyright (C) 2014 Tiago Salmito

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
