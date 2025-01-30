#pragma once

#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include "queue.h"

typedef struct {
    int connfd;
    pthread_mutex_t *mutex;
} Thread_Task_Payload;

typedef struct {
    pthread_mutex_t mutex;
    pthread_mutex_t func_mutex;
    pthread_cond_t cond_pending_tasks;
    pthread_t *threads;
    size_t max_threads;
    queue_t *task_queue;
    size_t enqueued_tasks;
    int running;
} Thread_Pool;

Thread_Pool *threadpool_init(size_t max_threads);
void threadpool_destroy(Thread_Pool *pool);
void threadpool_dispatch(Thread_Pool *pool, void (*func)(Thread_Task_Payload *), int arg);
