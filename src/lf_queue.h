#ifndef lf_queue_H_
#define lf_queue_H_

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct LFqueue * LFqueue_t;

#define clp_lfqueue_try_push(q,p) clp_lfqueue_try_push_((q),(void **)(p))
#define clp_lfqueue_try_pop(q,p) clp_lfqueue_try_pop_((q),(void **)(p))
#define clp_lfqueue_push(q,p) clp_lfqueue_push_((q),(void **)(p))
#define clp_lfqueue_pop(q,p) clp_lfqueue_pop_((q),(void **)(p))


	LFqueue_t clp_lfqueue_new();
	int clp_lfqueue_try_push_(LFqueue_t q,void ** source);
	void clp_lfqueue_push_(LFqueue_t q,void ** source);
	int clp_lfqueue_try_pop_(LFqueue_t q, void ** destination);
	void clp_lfqueue_pop_(LFqueue_t q, void ** destination);
	void clp_lfqueue_setcapacity(LFqueue_t q, int capacity);
	int clp_lfqueue_getcapacity(LFqueue_t q);
	int clp_lfqueue_isempty(LFqueue_t q);
	int clp_lfqueue_size(LFqueue_t q);

	//possibly thread unsafe
	void clp_lfqueue_free(LFqueue_t q);

#ifdef __cplusplus
}
#endif


#endif //lf_queue_H_
