#ifndef COMMON_QUEUE_H_
#define COMMON_QUEUE_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct queue_t queue_s;

__attribute__((visibility("default"))) int32_t create_queue(queue_s** q, uint32_t q_capacity);
__attribute__((visibility("default"))) int32_t queue_push(queue_s* queue, void *data);
__attribute__((visibility("default"))) int32_t queue_trypush(queue_s* queue, void *data);
__attribute__((visibility("default"))) int32_t queue_pop(queue_s* queue, void** data);
__attribute__((visibility("default"))) int32_t queue_trypop(queue_s* queue, void** data);
__attribute__((visibility("default"))) int32_t queue_size(queue_s* queue);




#ifdef __cplusplus
}
#endif

#endif
