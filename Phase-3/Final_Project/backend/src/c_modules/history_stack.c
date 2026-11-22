#include "history_stack.h"
#include <stdlib.h>


// Initialise an empty stack
void hs_init(HistoryStack *s)
{
    s->top = NULL;
    s->size = 0;
}


// Delete all elements from the stack
void hs_free(HistoryStack *s)
{
    // Keep popping until stack becomes empty
    // (hs_pop frees each node)
    while (hs_pop(s, NULL))
    {
    }

    hs_init(s);
}


// Insert a new entry at the top of the stack
int hs_push(HistoryStack *s, const HistoryEntry *entry)
{
    // Create a new node
    HistoryNode *newNode = (HistoryNode *)malloc(sizeof(HistoryNode));

    // Check memory allocation
    if (newNode == NULL)
        return 0;

    // Copy entry data into the new node
    newNode->entry = *entry;

    // Connect new node to the previous top node
    newNode->below = s->top;

    // Update top to the new node
    s->top = newNode;

    s->size++;

    return 1;
}


// Remove the top element from the stack
int hs_pop(HistoryStack *s, HistoryEntry *out)
{
    // Store the top node
    HistoryNode *temp = s->top;

    // Check if stack is empty
    if (temp == NULL)
        return 0;

    // Copy data before deleting the node
    if (out != NULL)
    {
        *out = temp->entry;
    }

    // Move top to the next node
    s->top = temp->below;

    // Free the removed node
    free(temp);

    s->size--;

    return 1;
}


// Read the top element without removing it
int hs_peek(const HistoryStack *s, HistoryEntry *out)
{
    // Check if stack is empty
    if (s->top == NULL)
        return 0;

    // Copy top element
    if (out != NULL)
    {
        *out = s->top->entry;
    }

    return 1;
}


// Check whether stack is empty
int hs_is_empty(const HistoryStack *s)
{
    if (s->top == NULL)
        return 1;

    return 0;
}


// Return total number of elements
size_t hs_size(const HistoryStack *s)
{
    return s->size;
}
