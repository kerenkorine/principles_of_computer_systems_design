#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>

typedef struct queue {
    void **elements;
    int size;
    int in, out;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    sem_t empty;
    sem_t full;
} queue_t;

queue_t *queue_new(int size) {
    if (size <= 0)
        return NULL;

    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    if (!q)
        return NULL;

    q->elements = (void **) malloc(sizeof(void *) * size);
    if (!q->elements) {
        free(q);
        return NULL;
    }

    q->size = size;
    q->in = 0;
    q->out = 0;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);

    sem_init(&q->full, 0, 0);
    sem_init(&q->empty, 0, size + 1);

    return q;
}

void queue_delete(queue_t **q) {
    if (!q || !*q)
        return;

    pthread_mutex_destroy(&(*q)->mutex);
    pthread_cond_destroy(&(*q)->not_full);
    pthread_cond_destroy(&(*q)->not_empty);
    free((*q)->elements);
    free(*q);

    *q = NULL;
}

bool queue_push(queue_t *q, void *elem) {
    if (!q || !elem)
        return false;

    sem_wait(&q->empty);

    pthread_mutex_lock(&q->mutex);

    // while ((q->in + 1) % q->size == q->out) // queue full
    //     pthread_cond_wait(&q->not_full, &q->mutex);

    q->elements[q->in] = elem;
    q->in = (q->in + 1) % q->size;

    // pthread_cond_signal(&q->not_empty);

    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->full);

    return true;
}

bool queue_pop(queue_t *q, void **elem) {
    if (!q || !elem)
        return false;

    sem_wait(&q->full);

    pthread_mutex_lock(&q->mutex);

    // while (q->in == q->out) // queue empty
    //     pthread_cond_wait(&q->not_empty, &q->mutex);

    *elem = q->elements[q->out];
    q->out = (q->out + 1) % q->size;

    // pthread_cond_signal(&q->not_full);

    pthread_mutex_unlock(&q->mutex);

    sem_post(&q->empty);

    return true;
}
