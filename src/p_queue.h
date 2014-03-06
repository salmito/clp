#ifndef p_queue_H_
#define p_queue_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Pqueue_s * Pqueue_t;

#include "instance.h"

Pqueue_t lstage_pqueue_new();
void lstage_pqueue_push(Pqueue_t q,void ** source);
void lstage_pqueue_pop(Pqueue_t q, instance_t* destination);
void lstage_pqueue_setcapacity(Pqueue_t q, int capacity);
int lstage_pqueue_getcapacity(Pqueue_t q);
int lstage_pqueue_isempty(Pqueue_t q);
int lstage_pqueue_size(Pqueue_t q);

//thread unsafe
void lstage_pqueue_free(Pqueue_t q);

#ifdef __cplusplus
}
#endif


#endif //p_queue_H_
