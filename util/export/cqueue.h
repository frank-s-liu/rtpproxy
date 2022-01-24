#ifndef C_QUEUE_H_
#define C_QUEUE_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct cqueue_t cqueue_s;


__attribute__((visibility("default"))) cqueue_s* cqinit(uint32_t cap);// (capacity = 1<< cap)
__attribute__((visibility("default"))) int cpush(cqueue_s *q, void *m);
__attribute__((visibility("default"))) void* cpop(cqueue_s *q);
__attribute__((visibility("default"))) uint32_t cqueue_size(cqueue_s* queue);




#ifdef __cplusplus
}
#endif

#endif
