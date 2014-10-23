#!/bin/sh

make ultraclean
make -j _SO=".dll" CC=x86_64-w64-mingw32-gcc G_PLUS_PLUS=x86_64-w64-mingw32-g++ EXTRA_FLAGS="-I/usr/include/event2 -I/usr/include/tbb" LDFLAGS="dll/x64/a/libevent.a dll/x64/a/libevent_core.a dll/x64/a/libevent_extra.a -Ldll/x64/ -lwsock32 -lws2_32 -ltbb64 -llua51" LUA_VER=5.1
mkdir -p dll/x64/lua51/
cp clp.dll dll/x64/lua51/clp.dll

make ultraclean
make -j _SO=".dll" CC=x86_64-w64-mingw32-gcc G_PLUS_PLUS=x86_64-w64-mingw32-g++ EXTRA_FLAGS="-I/usr/include/event2 -I/usr/include/tbb" LDFLAGS="dll/x64/a/libevent.a dll/x64/a/libevent_core.a dll/x64/a/libevent_extra.a -Ldll/x64/ -lwsock32 -lws2_32 -ltbb64 -llua52" LUA_VER=5.2
mkdir -p dll/x64/lua52/
cp clp.dll dll/x64/lua52/clp.dll 
make ultraclean

make -j _SO=".dll" CC=i686-w64-mingw32-gcc G_PLUS_PLUS=i686-w64-mingw32-g++ EXTRA_FLAGS="-I/usr/include/event2 -I/usr/include/tbb" LDFLAGS="dll/x32/a/libevent.a dll/x32/a/libevent_core.a dll/x32/a/libevent_extra.a -Ldll/x32/ -lwsock32 -lws2_32 -ltbb32 -lliblua" LUA_VER=5.1
mkdir -p dll/x32/lua51/
cp clp.dll dll/x32/lua51/clp.dll
make ultraclean

make -j _SO=".dll" CC=i686-w64-mingw32-gcc G_PLUS_PLUS=i686-w64-mingw32-g++ EXTRA_FLAGS="-I/usr/include/event2 -I/usr/include/tbb" LDFLAGS="dll/x32/a/libevent.a dll/x32/a/libevent_core.a dll/x32/a/libevent_extra.a -Ldll/x32/ -lwsock32 -lws2_32 -ltbb32 -llua51" LUA_VER=5.1
cp clp.dll dll/x32/lua51/clp-lua51.dll
make ultraclean

make -j _SO=".dll" CC=i686-w64-mingw32-gcc G_PLUS_PLUS=i686-w64-mingw32-g++ EXTRA_FLAGS="-I/usr/include/event2 -I/usr/include/tbb" LDFLAGS="dll/x32/a/libevent.a dll/x32/a/libevent_core.a dll/x32/a/libevent_extra.a -Ldll/x32/ -lwsock32 -lws2_32 -ltbb32 -llua52" LUA_VER=5.2
mkdir -p dll/x32/lua52/
cp clp.dll dll/x32/lua52/clp.dll
chmod a-x `find -iname *.dll`
