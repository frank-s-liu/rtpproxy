#ifndef C_QUEUE_H_
#define C_QUEUE_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct memqueue_t memqueue_s;


__attribute__((visibility("hidden"))) memqueue_s* memQinit(uint32_t cap);// (capacity = 1<< cap)
__attribute__((visibility("hidden"))) int mempush(memqueue_s *q, void *m);
__attribute__((visibility("hidden"))) void* mempop(memqueue_s *q);
__attribute__((visibility("hidden"))) uint32_t memqueue_size(memqueue_s* queue);




#ifdef __cplusplus
}
#endif

#endif
