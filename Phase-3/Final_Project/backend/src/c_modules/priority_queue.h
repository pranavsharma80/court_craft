/*
    Priority Queue Header File
    --------------------------
    DATA STRUCTURE : Min Heap stored in a dynamic array
    PURPOSE        : Decides which case should be handled first.
    RULE           : Smaller priority number = higher urgency.

    Array index relations:  parent = (i - 1) / 2
                            left   = 2 * i + 1
                            right  = 2 * i + 2
*/

#ifndef CC_PRIORITY_QUEUE_H
#define CC_PRIORITY_QUEUE_H

#include <stddef.h>

// C++ compatibility
#ifdef __cplusplus
extern "C" {
#endif


// Structure to store one priority item
typedef struct PriorityItem
{
    int id;
    int priority;
    char case_number[51];
    char title[256];
    char case_type[101];
    char status[20];
    char filing_date[11];

} PriorityItem;


// Structure of the Min Heap
typedef struct PriorityQueue
{
    PriorityItem *data;  // Dynamic array
    size_t size;         // Current number of elements
    size_t capacity;     // Allocated capacity

} PriorityQueue;


// Initialise an empty priority queue
void pq_init(PriorityQueue *pq);

// Free allocated memory
void pq_free(PriorityQueue *pq);

// Insert an item into the Min Heap (1 = ok, 0 = memory error)
int pq_push(PriorityQueue *pq, const PriorityItem *item);

// Remove the highest-priority item (1 = ok, 0 = empty)
int pq_pop(PriorityQueue *pq, PriorityItem *out);

// Read the root without removing it
int pq_peek(const PriorityQueue *pq, PriorityItem *out);

// Check whether queue is empty
int pq_is_empty(const PriorityQueue *pq);

// Return total number of elements
size_t pq_size(const PriorityQueue *pq);


#ifdef __cplusplus
}
#endif

#endif
