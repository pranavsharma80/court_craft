/*
    Case List Header File
    ---------------------
    DATA STRUCTURE : Singly Linked List
    PURPOSE        : Used by the "search cases" feature.

    Each node stores one case and a pointer to the next node:

        head -> [case1|next] -> [case2|next] -> [case3|NULL]
                                                    ^
                                                   tail
*/

#ifndef CC_CASE_LIST_H
#define CC_CASE_LIST_H

#include <stddef.h>

// Allows these C functions to be used from C++ files
#ifdef __cplusplus
extern "C" {
#endif

// Node structure to store one case
typedef struct CaseNode
{
    int id;

    char case_number[51];
    char title[256];
    char description[2048];
    char case_type[101];
    char status[20];
    char filing_date[11];

    struct CaseNode *next;  // Address of next node

} CaseNode;


// Linked list structure
typedef struct CaseList
{
    CaseNode *head;  // First node
    CaseNode *tail;  // Last node (makes append O(1))
    size_t size;     // Number of nodes

} CaseList;


// Initialise the linked list
void case_list_init(CaseList *list);

// Free all nodes
void case_list_free(CaseList *list);

// Insert case at the end
CaseNode *case_list_append(CaseList *list, int id,
                           const char *number,
                           const char *title,
                           const char *description,
                           const char *type,
                           const char *status,
                           const char *date);

// Insert case at the beginning
CaseNode *case_list_prepend(CaseList *list, int id,
                            const char *number,
                            const char *title,
                            const char *description,
                            const char *type,
                            const char *status,
                            const char *date);

// Find case using complete case number
CaseNode *case_list_find_by_number(const CaseList *list,
                                   const char *number);

// Search cases using a keyword
size_t case_list_search(const CaseList *src,
                        const char *query,
                        CaseList *out);

// Update case status using ID
int case_list_update_status(CaseList *list,
                            int id,
                            const char *status);

// Delete case using ID
int case_list_remove(CaseList *list, int id);

// Return total number of nodes
size_t case_list_size(const CaseList *list);


#ifdef __cplusplus
}
#endif

#endif
