LUA_VER=5.1

LUA_INCDIR=/usr/include/lua$(LUA_VER)

INCFLAGS=-I$(LUA_INCDIR)
CFLAGS=-fPIC $(INCFLAGS) -Wall $(EXTRA_FLAGS) -O2
LIBEV_FLAGS=-levent -levent_pthreads
EXTRA_FLAGS=-DDEBUG -ggdb
LIBTBB_FLAGS=-ltbb
PTHREAD_FLAGS=-lpthread -lrt
LDFLAGS=$(EXTRA_FLAGS) $(PTHREAD_FLAGS) $(LIBTBB_FLAGS) $(LIBEV_FLAGS)

LUA=lua

CC=gcc
G_PLUS_PLUS=g++

OBJ=  lstage.o \
		stage.o \
		lf_queue.o \
		p_queue.o \
		marshal.o \
		event.o \
		instance.o \
		threading.o \
		scheduler.o \
		pool.o \
		channel.o
      
HEADERS=*.h

_SO=.so

MODULE=lstage$(_SO)

all: $(MODULE)

%.o: %.c $(HEADERS) Makefile
	$(CC) -c -o $@ $< $(CFLAGS)

p_queue.o: p_queue.cpp
	$(G_PLUS_PLUS) -c -o $@ $< $(CFLAGS)

lf_queue.o: lf_queue.cpp
	$(G_PLUS_PLUS) -c -o $@ $< $(CFLAGS)
	
$(MODULE): $(OBJ)
	$(G_PLUS_PLUS) -shared $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o

ultraclean: clean
	rm -f $(MODULE) `find -iname "*~"`
