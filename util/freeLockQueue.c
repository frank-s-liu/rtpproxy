#include "flqueue.h"
#include "spinlock.h"
#include "commonError.h"


#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

/*****************************************
 *
 *  using array 
 *
 * **************************************/

struct free_lock_queue_t
{
    void **              data;
    volatile int32_t     size;
    volatile int32_t     in_p;            // push position
    volatile int32_t     out_p;           // pop position
    int32_t              max;             // max size of queue
    spinLock             lock;
};


static inline int queue_full(free_lock_queue_s* q)
{
    if(q->size >= q->max)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static inline int queue_empty(free_lock_queue_s* q)
{
    if(q->size <= 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int32_t fl_create_queue(free_lock_queue_s** q, int32_t q_capacity)
{
    int32_t status = SUCCESS;
    free_lock_queue_s* queue = NULL;
    queue = (free_lock_queue_s*)malloc(sizeof(free_lock_queue_s));
    assert(queue); // if queue is NULL, don't need to continue
    *q = queue;
    queue->max = q_capacity;
    queue->size = 0;
    queue->in_p = 0;
    queue->out_p = 0;

    queue->data = (void**)malloc(q_capacity*sizeof(void*));
    assert(queue->data);
    memset(queue->data, 0, q_capacity*sizeof(void*));
    spinlock_init(&queue->lock);
    return status;
}

int32_t fl_queue_push(free_lock_queue_s* queue, void *data)
{
    int32_t ret = SUCCESS;
    int32_t size = 0;
    if(NULL == queue)
    {
        return NULL_POINT;
    }
    size = __sync_fetch_and_add(&queue->size, 0);
    if(size >= queue->max-5)
    {
        return QUEUE_FULL;
    }
    spinlock_lock(&queue->lock);
    if(queue_full(queue))
    {
        spinlock_unlock(&queue->lock);
        return QUEUE_FULL;
    }
    queue->data[queue->in_p] = data;
    queue->in_p = (queue->in_p+1) % queue->max;
    queue->size++;
    spinlock_unlock(&queue->lock);
    return ret;
}


int32_t fl_queue_pop(free_lock_queue_s* queue, void** data)
{
    int32_t ret = SUCCESS;
    int32_t size = 0;
    if(NULL == queue)
    {
        return NULL_POINT;
    }
    size = __sync_fetch_and_add(&queue->size, 0);
    if(size <= 0)
    {
        return QUEUE_EMPTY;
    }
    spinlock_lock(&queue->lock); 
    if(queue_empty(queue))
    {
        spinlock_unlock(&queue->lock);
        return QUEUE_EMPTY;
    }

    *data = queue->data[queue->out_p];
    queue->out_p = (queue->out_p+1) % queue->max;
    queue->size--; 
    spinlock_unlock(&queue->lock);
    return ret;
}

int32_t fl_queue_size(free_lock_queue_s* queue)
{
    return queue->size;
}

