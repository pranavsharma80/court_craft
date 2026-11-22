/*
    History Stack Header File
    -------------------------
    DATA STRUCTURE : Stack (LIFO - Last In, First Out) using a linked list
    PURPOSE        : Shows case history with the newest event first.

        top -> [newest] -> [older] -> [oldest] -> NULL
*/

#ifndef CC_HISTORY_STACK_H
#define CC_HISTORY_STACK_H

#include <stddef.h>

// C++ compatibility
#ifdef __cplusplus
extern "C" {
#endif


// One history event of a case
typedef struct HistoryEntry
{
    char action[256];
    char description[512];
    char created_at[20];   // YYYY-MM-DD HH:MM:SS
    char actor[101];       // Name of the person (may be empty)

} HistoryEntry;


// Node structure of the stack
typedef struct HistoryNode
{
    HistoryEntry entry;         // Stores history details
    struct HistoryNode *below;  // Points to the previous node

} HistoryNode;


// Stack structure
typedef struct HistoryStack
{
    HistoryNode *top;  // Points to the top node
    size_t size;       // Total number of nodes

} HistoryStack;


// Initialise an empty stack
void hs_init(HistoryStack *s);

// Delete all elements from the stack
void hs_free(HistoryStack *s);

// Insert a new entry at the top (1 = ok, 0 = memory error)
int hs_push(HistoryStack *s, const HistoryEntry *entry);

// Remove the top entry (1 = ok, 0 = stack empty)
int hs_pop(HistoryStack *s, HistoryEntry *out);

// Read the top entry without removing it
int hs_peek(const HistoryStack *s, HistoryEntry *out);

// Check whether stack is empty
int hs_is_empty(const HistoryStack *s);

// Return total number of elements
size_t hs_size(const HistoryStack *s);


#ifdef __cplusplus
}
#endif

#endif
