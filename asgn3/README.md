Bounded Buffer Implementation in C
This repository contains an implementation of a bounded buffer with synchronization between multiple threads in C. This implementation can be useful in concurrent programming scenarios where multiple threads need to produce or consume data in a synchronized manner.

Design and Implementation
The implementation provides a queue data structure with First-In-First-Out (FIFO) properties, that stores and returns arbitrary pointers to objects. This means that it can store anything in the queue.

The queue is implemented using a circular buffer strategy. The 'in' and 'out' indices in the queue structure are used to keep track of the current position for insertion and removal of elements. To avoid overwriting of elements, the 'in' index is not allowed to catch up with the 'out' index.

A mutex and condition variables are used to ensure thread safety.

Code Structure and Modules
The main module is queue.c which includes the following functions:

queue_t *queue_new(int size): This function initializes a new queue with the given size. It allocates memory for the queue structure and its elements. It also initializes the mutex, condition variables, and semaphores.

void queue_delete(queue_t **q): This function deletes a queue, freeing the memory allocated for the queue structure and its elements. It also destroys the mutex and condition variables.

bool queue_push(queue_t *q, void *elem): This function inserts an element at the end of the queue. It first checks if the queue is not full, then locks the mutex, inserts the element, increments the 'in' index (wrapping around if necessary), and finally unlocks the mutex.

bool queue_pop(queue_t *q, void **elem): This function removes an element from the front of the queue. It first checks if the queue is not empty, then locks the mutex, removes the element, increments the 'out' index (wrapping around if necessary), and finally unlocks the mutex.

