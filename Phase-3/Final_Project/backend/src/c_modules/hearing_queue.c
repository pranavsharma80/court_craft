#include "hearing_queue.h"
#include <stdlib.h>

// Initialise an empty queue
void hq_init(HearingQueue *q)
{
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
}


// Remove all elements from the queue
void hq_free(HearingQueue *q)
{
    HearingItem item;

    // Keep deleting elements until queue becomes empty
    // (hq_dequeue already frees each node)
    while (hq_dequeue(q, &item))
    {
    }

    hq_init(q);
}


// Insert a new hearing at the rear
int hq_enqueue(HearingQueue *q, const HearingItem *item)
{
    // Create a new node
    HearingNode *newNode = (HearingNode *)malloc(sizeof(HearingNode));

    if (newNode == NULL)
        return 0;

    // Copy hearing details into the new node
    newNode->item = *item;
    newNode->next = NULL;

    // If queue is empty
    if (q->front == NULL)
    {
        q->front = newNode;
        q->rear = newNode;
    }
    else
    {
        // Connect the last node to the new node
        q->rear->next = newNode;

        // Update rear
        q->rear = newNode;
    }

    q->size++;

    return 1;
}


// Delete a hearing from the front
int hq_dequeue(HearingQueue *q, HearingItem *out)
{
    // Store the first node
    HearingNode *temp = q->front;

    // Check if queue is empty
    if (temp == NULL)
        return 0;

    // Copy data before deleting the node
    if (out != NULL)
    {
        *out = temp->item;
    }

    // Move front to the next node
    q->front = temp->next;

    // If queue becomes empty, rear also becomes NULL
    if (q->front == NULL)
    {
        q->rear = NULL;
    }

    // Free the deleted node
    free(temp);

    q->size--;

    return 1;
}


// Read the first element without deleting it
int hq_peek(const HearingQueue *q, HearingItem *out)
{
    // Check if queue is empty
    if (q->front == NULL)
        return 0;

    // Copy front element
    if (out != NULL)
    {
        *out = q->front->item;
    }

    return 1;
}


// Check whether queue is empty
int hq_is_empty(const HearingQueue *q)
{
    if (q->front == NULL)
        return 1;

    return 0;
}


// Return total number of elements
size_t hq_size(const HearingQueue *q)
{
    return q->size;
}
