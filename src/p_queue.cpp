#include "p_queue.h"

#include <tbb/concurrent_priority_queue.h>
#include <stdlib.h>

struct compare_f {
bool operator()(const instance_t& u, const instance_t& v) const {
	return u->stage->priority < v->stage->priority;
}
};

/* Queue internal structure (Lock Free Queue) */
struct Pqueue_s {
   tbb::concurrent_priority_queue<instance_t, compare_f> * queue; 
};

extern "C" {

/* Create a new queue */
Pqueue_t lstage_pqueue_new() {
   Pqueue_t q;
   q=new Pqueue_s();
   q->queue=new tbb::concurrent_priority_queue<instance_t, compare_f>();
   return q;
}

void lstage_pqueue_push(Pqueue_t q,instance_t source) {
	return q->queue->push(source);
}

int lstage_pqueue_try_pop(Pqueue_t q, instance_t* destination) {
	return q->queue->try_pop(*destination);
}

int lstage_pqueue_isempty(Pqueue_t q) {
   return (int)q->queue->empty();
}

int lstage_pqueue_size(Pqueue_t q) {
	return q->queue->size();
}

//possibly thread unsafe
void lstage_pqueue_free(Pqueue_t q) {
 if(q) {
      q->queue->clear();
      delete (q->queue);
      delete (q);
   }
}

}
