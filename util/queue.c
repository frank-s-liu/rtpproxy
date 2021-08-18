#include "queue.h"
#include "commonError.h"


#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


/*****************************************
 *
 *  using array 
 *
 * **************************************/

struct queue_t
{
    void **          data;
    uint32_t         size;
    uint32_t         in_p;            // push position
    uint32_t         out_p;           // pop position
    uint32_t         max;             // max size of queue
    uint32_t         push_waiters;
    uint32_t         pop_waiters;
    pthread_mutex_t* p_mutex;
    pthread_cond_t*  p_cond_not_empty;
    pthread_cond_t*  p_cond_not_full;
};


static inline int queue_full(queue_s* q)
{
    return q->size == q->max;
}

static inline int queue_empty(queue_s* q)
{
    return q->size == 0;
}

int32_t create_queue(queue_s** q, uint32_t q_capacity)
{
    int32_t status = 0;
    queue_s* queue = NULL;
    queue = (queue_s*)malloc(sizeof(queue_s));
    assert(queue); // if queue is NULL, don't need to continue
    *q = queue;
    queue->max = q_capacity;
    queue->size = 0;
    queue->in_p = 0;
    queue->out_p = 0;
    queue->push_waiters = 0;
    queue->pop_waiters = 0;
    queue->p_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    assert(queue->p_mutex);
    status = pthread_mutex_init(queue->p_mutex, NULL);
    if(0 != status)
    {
        free(queue->p_mutex);
        free(queue);
        return FAILED;
    }
    queue->p_cond_not_empty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    assert(queue->p_cond_not_empty);
    status = pthread_cond_init(queue->p_cond_not_empty, NULL);
    if(0 != status)
    {
        free(queue->p_mutex);
        free(queue->p_cond_not_empty);
        free(queue);
        return FAILED;
    }

    queue->p_cond_not_full = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    assert(queue->p_cond_not_full);
    status = pthread_cond_init(queue->p_cond_not_full, NULL);
    if(0 != status)
    {
        free(queue->p_mutex);
        free(queue->p_cond_not_empty);
        free(queue->p_cond_not_full);
        free(queue);
        return FAILED;
    }
    queue->data = (void**)malloc(q_capacity*sizeof(void*));
    assert(queue->data);
    memset(queue->data, 0, q_capacity*sizeof(void*));
    return SUCCESS;
}

int32_t queue_push(queue_s* queue, void *data)
{
    int32_t ret = SUCCESS;
    if(NULL == queue)
    {
        return NULL_POINT;
    }
    pthread_mutex_lock(queue->p_mutex);
    if(queue_full(queue))
    {
        queue->push_waiters++;
        ret = pthread_cond_wait(queue->p_cond_not_full, queue->p_mutex);
        queue->push_waiters--;
        if(ret != 0)
        {
            pthread_mutex_unlock(queue->p_mutex);
            return ret;
        }
        //If we wake up and it's still full, then we give up push
        if(queue_full(queue))
        {
            pthread_mutex_unlock(queue->p_mutex);
            return FAILED;
        }
    }
    
    queue->data[queue->in_p] = data;
    queue->in_p = (queue->in_p+1) % queue->max;
    queue->size++;

    if(queue->pop_waiters)
    {
        pthread_cond_signal(queue->p_cond_not_empty);
    }
    pthread_mutex_unlock(queue->p_mutex);
    return ret;
}

int32_t queue_trypush(queue_s* queue, void *data)
{
    int32_t ret = SUCCESS;
    if(NULL == queue)
    {
        return NULL_POINT;
    }
    pthread_mutex_lock(queue->p_mutex);
    if(queue_full(queue))
    {
        pthread_mutex_unlock(queue->p_mutex);
        return FAILED;
    }
    queue->data[queue->in_p] = data;
    queue->in_p = (queue->in_p+1) % queue->max;
    queue->size++;

    if(queue->pop_waiters)
    {
        pthread_cond_signal(queue->p_cond_not_empty);
    }
    pthread_mutex_unlock(queue->p_mutex);
    return ret;
}

int32_t queue_pop(queue_s* queue, void** data)
{
    int32_t ret = SUCCESS;
    if(NULL == queue)
    {
        return NULL_POINT;
    }
    pthread_mutex_lock(queue->p_mutex);
    if(queue_empty(queue))
    {
        queue->pop_waiters++;
        ret = pthread_cond_wait(queue->p_cond_not_empty, queue->p_mutex);
        queue->pop_waiters--;
        if(ret != 0)
        {
            pthread_mutex_unlock(queue->p_mutex);
            return ret;
        }
        //If we wake up and it's still empty, then we give up pop
        if(queue_empty(queue))
        {
            pthread_mutex_unlock(queue->p_mutex);
            return QUEUE_EMPTY;
        }
    }

    *data = queue->data[queue->out_p];
    queue->size --;
    queue->out_p = (queue->out_p+1) % queue->max;
    if(queue->push_waiters)
    {
        pthread_cond_signal(queue->p_cond_not_full);
    }
    pthread_mutex_unlock(queue->p_mutex);
    return ret;
}

int32_t queue_trypop(queue_s* queue, void** data)
{
    int32_t ret = SUCCESS;
    if(NULL == queue)
    {
        return NULL_POINT;
    }
    pthread_mutex_lock(queue->p_mutex);
    if(queue_empty(queue))
    {
        pthread_mutex_unlock(queue->p_mutex);
        return FAILED;
    }

    *data = queue->data[queue->out_p];
    queue->size --;
    queue->out_p = (queue->out_p+1) % queue->max;
    if(queue->push_waiters)
    {
        pthread_cond_signal(queue->p_cond_not_full);
    }
    pthread_mutex_unlock(queue->p_mutex);
    return ret;
}


int32_t queue_size(queue_s* queue)
{
    if(queue)
    {
        return queue->size;
    }
    else
    {
        return 0;
    }
}
