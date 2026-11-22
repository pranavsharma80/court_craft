#include "priority_queue.h"
#include <stdlib.h>
#include <string.h>


// Compare two items to decide which one has higher priority
// Returns negative if a is more urgent, positive if b is more urgent
static int compare_items(const PriorityItem *a, const PriorityItem *b)
{
    // Rule 1: smaller priority number means more urgent
    if (a->priority != b->priority)
    {
        return a->priority - b->priority;
    }

    // Rule 2: if priority is same, older filing date comes first
    // Empty date is treated as the latest date
    const char *date1 = a->filing_date;
    const char *date2 = b->filing_date;

    if (date1[0] == '\0')
        date1 = "9999-12-31";

    if (date2[0] == '\0')
        date2 = "9999-12-31";

    int result = strcmp(date1, date2);

    if (result != 0)
        return result;

    // Rule 3: if dates are also same, smaller ID comes first
    return a->id - b->id;
}


// Swap two priority queue items
static void swap_items(PriorityItem *a, PriorityItem *b)
{
    PriorityItem temp = *a;

    *a = *b;
    *b = temp;
}


// Initialise an empty priority queue
void pq_init(PriorityQueue *pq)
{
    pq->data = NULL;
    pq->size = 0;
    pq->capacity = 0;
}


// Free the memory used by the priority queue
void pq_free(PriorityQueue *pq)
{
    free(pq->data);

    pq_init(pq);
}


// Insert a new item into the Min Heap
int pq_push(PriorityQueue *pq, const PriorityItem *item)
{
    // Step 1: increase array capacity if it is full
    if (pq->size == pq->capacity)
    {
        size_t new_capacity;

        if (pq->capacity == 0)
            new_capacity = 16;
        else
            new_capacity = pq->capacity * 2;

        PriorityItem *new_data = (PriorityItem *)realloc(
            pq->data, new_capacity * sizeof(PriorityItem));

        if (new_data == NULL)
            return 0;

        pq->data = new_data;
        pq->capacity = new_capacity;
    }

    // Step 2: insert new item at the last position
    size_t index = pq->size;

    pq->data[index] = *item;
    pq->size++;

    // Step 3: move the new item upward while it is more urgent than its parent
    while (index > 0)
    {
        size_t parent = (index - 1) / 2;

        // Stop if parent already has higher or equal priority
        if (compare_items(&pq->data[index],
                          &pq->data[parent]) >= 0)
        {
            break;
        }

        // Swap child and parent
        swap_items(&pq->data[index], &pq->data[parent]);

        index = parent;
    }

    return 1;
}


// Remove the highest-priority item from the Min Heap
int pq_pop(PriorityQueue *pq, PriorityItem *out)
{
    // Check if heap is empty
    if (pq->size == 0)
        return 0;

    // Copy root item before removing it
    if (out != NULL)
    {
        *out = pq->data[0];
    }

    // Reduce heap size
    pq->size--;

    // If heap becomes empty, stop
    if (pq->size == 0)
        return 1;

    // Move last item to the root
    pq->data[0] = pq->data[pq->size];

    size_t index = 0;

    // Move the root downward to restore Min Heap
    while (1)
    {
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;
        size_t smallest = index;

        // Check left child
        if (left < pq->size &&
            compare_items(&pq->data[left],
                          &pq->data[smallest]) < 0)
        {
            smallest = left;
        }

        // Check right child
        if (right < pq->size &&
            compare_items(&pq->data[right],
                          &pq->data[smallest]) < 0)
        {
            smallest = right;
        }

        // Stop if parent is already the smallest
        if (smallest == index)
            break;

        // Swap parent with the smallest child
        swap_items(&pq->data[index],
                   &pq->data[smallest]);

        index = smallest;
    }

    return 1;
}


// Read the highest-priority item without deleting it
int pq_peek(const PriorityQueue *pq, PriorityItem *out)
{
    if (pq->size == 0)
        return 0;

    if (out != NULL)
    {
        *out = pq->data[0];
    }

    return 1;
}


// Check whether priority queue is empty
int pq_is_empty(const PriorityQueue *pq)
{
    return pq->size == 0;
}


// Return total number of elements
size_t pq_size(const PriorityQueue *pq)
{
    return pq->size;
}
