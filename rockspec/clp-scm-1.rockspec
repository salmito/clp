package = "clp"
version = "scm-1"
source = {
   url = "git://github.com/salmito/clp",
}
description = {
   summary = "Communicating Lua Processes",
   detailed = [[
      CLP is a Lua library for bulding lightweight parallel processes based on
      the concepts of CSP (Communicating Sequential Processes).
   ]],
   homepage = "http://github.com/salmito/clp",
   license = "MIT/X11"
}
dependencies = {
   "lua >= 5.1, <= 5.3"
}
external_dependencies = {
   LIBTBB = {
      library = "tbb",
      header = "tbb/concurrent_queue.h",
   },
   LIBEVENT = {
      library = "event",
   },
   platforms = {
      unix = {
         PTHREAD = {
            library = "pthread",
         }
      }
   }
}
build = {
   type = "make",
   variables = {
      LIBFLAG = "$(LIBFLAG)",
      INCDIRFLAGS = "-I$(LIBTBB_INCDIR) -I$(LIBEVENT_INCDIR) -I$(PTHREAD_INCDIR) -I$(LUA_INCDIR)",
      LIBDIRFLAGS = "-L$(LIBTBB_LIBDIR) -L$(LIBEVENT_LIBDIR) -L$(PTHREAD_LIBDIR) -L$(LUA_LIBDIR)",
      CFLAGS = "$(CFLAGS)",
      INSTALL_LIBDIR = "$(LIBDIR)",
   },   
}
