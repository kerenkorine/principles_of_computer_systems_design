#include "threadpool.h"
#include <stdlib.h>
#include "bool.h"
#include "debug.h"

typedef struct {
    void (*func)(Thread_Task_Payload *);
    Thread_Task_Payload payload;
} Thread_Task;

static Thread_Task *create_task(
    Thread_Pool *pool, void (*func)(Thread_Task_Payload *), int connfd) {
    Thread_Task *new_task = malloc(sizeof(Thread_Task));
    if (new_task == NULL) {
        return NULL;
    }
    new_task->func = func;
    new_task->payload.connfd = connfd;
    new_task->payload.mutex = &pool->func_mutex;
    return new_task;
}

static void destroy_task(Thread_Task *task) {
    free(task);
}

static Thread_Task *fetch_task(Thread_Pool *pool) {
    pthread_mutex_lock(&pool->mutex);
    // check if there's a task in the queue
    while (pool->running && pool->enqueued_tasks == 0) {
        // sleep if there are no tasks
        pthread_cond_wait(&pool->cond_pending_tasks, &pool->mutex);
    }

    Thread_Task *task = NULL;
    // return the task (or NULL if you've been told to finish execution)
    if (pool->running) {
        queue_pop(pool->task_queue, (void **) &task);
        pool->enqueued_tasks -= 1;
    }

    pthread_mutex_unlock(&pool->mutex);
    return task;
}

static void threadpool_thread_main(void *arg) {
    Thread_Pool *pool = arg;
    while (pool->running) {
        Thread_Task *task = fetch_task(pool);
        if (task != NULL) {
            task->func(&task->payload);
            destroy_task(task);
        }
    }
}

Thread_Pool *threadpool_init(size_t max_threads) {
    Thread_Pool *pool = malloc(sizeof(Thread_Pool));
    if (pool == NULL) {
        return NULL;
    }

    pool->threads = malloc(sizeof(pthread_t) * max_threads);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->cond_pending_tasks, NULL) != EXIT_SUCCESS) {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->mutex, NULL) != EXIT_SUCCESS) {
        pthread_cond_destroy(&pool->cond_pending_tasks);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->mutex, NULL) != EXIT_SUCCESS) {
        pthread_cond_destroy(&pool->cond_pending_tasks);
        pthread_mutex_destroy(&pool->mutex);
        free(pool->threads);
        free(pool);
        return NULL;
    }

    // TODO: queue size?
    pool->task_queue = queue_new(max_threads);
    pool->max_threads = max_threads;
    pool->enqueued_tasks = 0;
    pool->running = TRUE;

    for (size_t i = 0; i < max_threads; i++) {
        pthread_create(&pool->threads[i], NULL, (void *(*) (void *) ) threadpool_thread_main, pool);
    }

    return pool;
}

void threadpool_destroy(Thread_Pool *pool) {
    pool->running = FALSE;
    pthread_cond_destroy(&pool->cond_pending_tasks);
    pthread_mutex_destroy(&pool->mutex);

    queue_delete(&pool->task_queue);

    for (size_t i = 0; i < pool->max_threads; i++) {
        pthread_cancel(pool->threads[i]);
    }

    free(pool->threads);
    free(pool);
}

void threadpool_dispatch(Thread_Pool *pool, void (*func)(Thread_Task_Payload *), int connfd) {
    if (pool->running) {
        Thread_Task *task = create_task(pool, func, connfd);
        pthread_mutex_lock(&pool->mutex);
        pool->enqueued_tasks += 1;
        pthread_mutex_unlock(&pool->mutex);
        queue_push(pool->task_queue, task);
        pthread_cond_signal(&pool->cond_pending_tasks);
    }
}
