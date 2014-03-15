#include "p_queue.h"

#include <tbb/concurrent_priority_queue.h>
#include <stdlib.h>
#include <pthread.h>

struct compare_f {
bool operator()(const instance_t& u, const instance_t& v) const {
	return u->stage->priority < v->stage->priority;
}
};

/* Queue internal structure (Lock Free Queue) */
struct Pqueue_s {
   tbb::concurrent_priority_queue<instance_t, compare_f> * queue;
   pthread_cond_t cond;
   pthread_mutex_t mutex;
};

extern "C" {

/* Create a new queue */
Pqueue_t lstage_pqueue_new() {
   Pqueue_t q;
   q=new Pqueue_s();
   q->queue=new tbb::concurrent_priority_queue<instance_t, compare_f>();
   pthread_cond_init(&q->cond, NULL);
   pthread_mutex_init(&q->mutex, NULL);
   return q;
}

void lstage_pqueue_push(Pqueue_t q,void ** source) {
	instance_t p;
	instance_t * arg=(instance_t *)(source);
	if(arg==NULL){
		p=NULL;
	} else {
   	p=*arg;
   }
//	pthread_mutex_lock(&q->mutex);
	int size=lstage_pqueue_size(q);
	q->queue->push(p);
	if(size<=0)
		pthread_cond_broadcast(&q->cond);
//	pthread_mutex_unlock(&q->mutex);
}

void lstage_pqueue_pop(Pqueue_t q, instance_t* destination) {
	while(!q->queue->try_pop(*destination)) {
		pthread_mutex_lock(&q->mutex);
		pthread_cond_wait(&q->cond, &q->mutex);
		pthread_mutex_unlock(&q->mutex);
	}
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
