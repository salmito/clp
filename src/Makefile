LUA_VER=5.1

LUA_INCDIR=/usr/include/lua$(LUA_VER)

#EXTRA_FLAGS=-DDEBUG -ggdb
INCDIRFLAGS=-I$(LUA_INCDIR)
CFLAGS=-fPIC -Wall -O2
MODCFLAGS=$(CFLAGS) $(INCDIRFLAGS) $(EXTRA_FLAGS)
LIBDIRFLAGS=
LIBEV_FLAGS=-levent -levent_pthreads
LIBTBB_FLAGS=-ltbb
PTHREAD_FLAGS=-lpthread -lrt
LDFLAGS=$(EXTRA_FLAGS) $(LIBDIRFLAGS) $(PTHREAD_FLAGS) $(LIBTBB_FLAGS) $(LIBEV_FLAGS)
LIBFLAG=-shared

LUA=lua

CC=gcc
G_PLUS_PLUS=g++

OBJ=  clp.o \
		process.o \
		lf_queue.o \
		marshal.o \
		event.o \
		threading.o \
		scheduler.o \
		pool.o \
		channel.o
      
HEADERS=*.h

_SO=.so

MODULE=clp$(_SO)

all: $(MODULE)

%.o: %.c $(HEADERS) Makefile
	$(CC) -c -o $@ $< $(MODCFLAGS)

p_queue.o: p_queue.cpp
	$(G_PLUS_PLUS) -c -o $@ $< $(MODCFLAGS)

lf_queue.o: lf_queue.cpp
	$(G_PLUS_PLUS) -c -o $@ $< $(MODCFLAGS)
	
$(MODULE): $(OBJ)
	$(G_PLUS_PLUS) $(LIBFLAG) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o

ultraclean: clean
	rm -f $(MODULE) rm -f *.dll *.so `find -iname "*~"`
