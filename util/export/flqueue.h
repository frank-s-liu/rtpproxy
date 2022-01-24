#ifndef COMMON_FREE_LOCK_QUEUE_H_
#define COMMON_FREE_LOCK_QUEUE_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct free_lock_queue_t free_lock_queue_s;

__attribute__((visibility("default"))) int32_t fl_create_queue(free_lock_queue_s** q, int32_t q_capacity);
__attribute__((visibility("default"))) int32_t fl_queue_push(free_lock_queue_s* queue, void *data);
__attribute__((visibility("default"))) int32_t fl_queue_pop(free_lock_queue_s* queue, void** data);
__attribute__((visibility("default"))) int32_t fl_queue_size(free_lock_queue_s* queue);




#ifdef __cplusplus
}
#endif

#endif
