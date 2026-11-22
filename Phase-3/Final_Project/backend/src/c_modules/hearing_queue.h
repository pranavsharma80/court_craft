/*
    Hearing Queue Header File
    -------------------------
    DATA STRUCTURE : Queue (FIFO - First In, First Out) using a linked list
    PURPOSE        : Builds the "order of the day" for court hearings.

        front -> [h1] -> [h2] -> [h3] <- rear
        (dequeue here)          (enqueue here)
*/

#ifndef CC_HEARING_QUEUE_H
#define CC_HEARING_QUEUE_H

#include <stddef.h>

// C++ compatibility
#ifdef __cplusplus
extern "C" {
#endif


// Details of one hearing stored in the queue
typedef struct HearingItem
{
    int  id;
    int  case_id;
    char case_number[51];
    char title[256];
    char hearing_date[11];
    char hearing_time[9];
    char court_room[101];
    char judge_name[151];
    char purpose[256];
    char status[20];

} HearingItem;


// Node structure of the queue
typedef struct HearingNode
{
    HearingItem item;          // Hearing details
    struct HearingNode *next;  // Address of next node

} HearingNode;


// Queue structure
typedef struct HearingQueue
{
    HearingNode *front;  // First node
    HearingNode *rear;   // Last node
    size_t size;         // Total number of nodes

} HearingQueue;


// Initialise an empty queue
void hq_init(HearingQueue *q);

// Delete all elements from the queue
void hq_free(HearingQueue *q);

// Insert a hearing at the rear (1 = ok, 0 = memory error)
int hq_enqueue(HearingQueue *q, const HearingItem *item);

// Remove a hearing from the front (1 = ok, 0 = queue empty)
int hq_dequeue(HearingQueue *q, HearingItem *out);

// Read the front element without deleting it
int hq_peek(const HearingQueue *q, HearingItem *out);

// Check whether queue is empty
int hq_is_empty(const HearingQueue *q);

// Return total number of elements
size_t hq_size(const HearingQueue *q);


#ifdef __cplusplus
}
#endif

#endif
