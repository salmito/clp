#include "lf_queue.h"

#include <tbb/concurrent_queue.h>
#include <stdlib.h>

#define LOCK(q) while (__sync_lock_test_and_set(&(q)->lock,1)) {}
#define UNLOCK(q) __sync_lock_release(&(q)->lock);

typedef void * __ptr;

/* Queue internal structure (Lock Free Queue) */
struct LFqueue {
   tbb::concurrent_bounded_queue<__ptr> * queue;
   int lock;
};

extern "C" {
	

/* Create a new queue */
LFqueue_t clp_lfqueue_new() {
   LFqueue_t q;
   q=new LFqueue();
   q->lock=0;
   LOCK(q);
   q->queue=new tbb::concurrent_bounded_queue<__ptr>();
   return q;
}

void clp_lfqueue_push_(LFqueue_t q,void ** source) {
   __ptr p;
   p=*source;
   q->queue->push(p);
}

int clp_lfqueue_try_push_(LFqueue_t q,void ** source) {
	__ptr p;
   p=*source;
   return q->queue->try_push(p);
}

int clp_lfqueue_try_pop_(LFqueue_t q, void ** destination) {
   __ptr dest;
   if(q->queue->try_pop(dest)) {
      *destination = dest;
      return 1;
   }
   return 0;
}

void clp_lfqueue_pop_(LFqueue_t q, void ** destination) {
   __ptr dest;
   q->queue->pop(dest);
   *destination = dest;
}

void clp_lfqueue_setcapacity(LFqueue_t q,int capacity) {
	return q->queue->set_capacity(capacity);
}

int clp_lfqueue_getcapacity(LFqueue_t q) {
   return q->queue->capacity();
}

int clp_lfqueue_isempty(LFqueue_t q) {
   return (int)q->queue->empty();
}

int clp_lfqueue_size(LFqueue_t q) {
	return q->queue->size();
}

//possibly thread unsafe
void clp_lfqueue_free(LFqueue_t q) {
 if(q) {
      q->queue->clear();
      delete (q->queue);
      delete (q);
   }
}



}
