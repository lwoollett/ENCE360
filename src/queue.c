#include "queue.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)


/*
 * Circular Buffer Queue
 */
typedef struct QueueStruct {
    void **base;
    int head;
    int tail;
    int count;
    int size;

    //Good old semaphores
    sem_t sem_enqueue;
    sem_t sem_dequeue;
    pthread_mutex_t mutex_lock;

} Queue;

/*
 * Allocate memory for our queue
 */
Queue *queue_alloc(int size) {

    //Allocate memory for the struct
    Queue *queue = (Queue *) malloc(sizeof(Queue));

    //Allocate memory for last item in queue
    queue->base = malloc(sizeof(void *) * size);

    //Init values of queue
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->size = size;

    //Init sem and mutex
    sem_init(&queue->sem_enqueue, 0, size);
    sem_init(&queue->sem_dequeue, 0, 0);
    pthread_mutex_init(&queue->mutex_lock, NULL);

    return queue;
}

/*
 * Free memory of queue
 */
void queue_free(Queue *queue) {
    free(queue->base);
    free(queue);
}

/*
 * Add an item into the queue
 */
void queue_put(Queue *queue, void *item) {

    //Block and wait
    sem_wait(&queue->sem_enqueue);

    //Put item at the end of queue
    pthread_mutex_lock(&queue->mutex_lock);

    queue->base[queue->tail++] = item;
    queue->tail %= queue->size;
    queue->count += 1;

    pthread_mutex_unlock(&queue->mutex_lock);
    sem_post(&queue->sem_dequeue);
}

/*
 * Get the item at the head of the queue.
 */
void *queue_get(Queue *queue) {

    //Block and wait
    sem_wait(&queue->sem_dequeue);

    //Get the item at head of queue
    pthread_mutex_lock(&queue->mutex_lock);
    void *item = queue->base[queue->head++];
    queue->head %= queue->size;
    queue->count -= 1;

    pthread_mutex_unlock(&queue->mutex_lock);
    sem_post(&queue->sem_enqueue);

    return item;

}