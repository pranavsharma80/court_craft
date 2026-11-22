#include "case_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------- small helper functions (static = private to this file) ---------- */

// Copy string safely into another string (never overflows)
static void copy_str(char *destination, size_t size, const char *source)
{
    if (source == NULL)
    {
        source = "";
    }

    strncpy(destination, source, size - 1);
    destination[size - 1] = '\0';
}

// Compare two strings without checking uppercase/lowercase
static int ci_equal(const char *str1, const char *str2)
{
    while (*str1 != '\0' && *str2 != '\0')
    {
        if (tolower((unsigned char)*str1) !=
            tolower((unsigned char)*str2))
        {
            return 0;
        }

        str1++;
        str2++;
    }

    // Equal only if both strings ended together
    return (*str1 == '\0' && *str2 == '\0');
}

// Check whether a word is present inside a string (ignores case)
static int ci_contains(const char *text, const char *word)
{
    int i, j;

    // An empty word matches everything
    if (strlen(word) == 0)
        return 1;

    for (i = 0; text[i] != '\0'; i++)
    {
        for (j = 0; word[j] != '\0'; j++)
        {
            if (text[i + j] == '\0')
                break;

            if (tolower((unsigned char)text[i + j]) !=
                tolower((unsigned char)word[j]))
            {
                break;
            }
        }

        // If complete word matches, return 1
        if (word[j] == '\0')
            return 1;
    }

    return 0;
}

// Create a new node and store case details in it
static CaseNode *make_node(int id, const char *number, const char *title,
                           const char *description, const char *type,
                           const char *status, const char *date)
{
    CaseNode *newNode = (CaseNode *)malloc(sizeof(CaseNode));

    if (newNode == NULL)
        return NULL;

    newNode->id = id;

    copy_str(newNode->case_number, sizeof(newNode->case_number), number);
    copy_str(newNode->title, sizeof(newNode->title), title);
    copy_str(newNode->description, sizeof(newNode->description), description);
    copy_str(newNode->case_type, sizeof(newNode->case_type), type);
    copy_str(newNode->status, sizeof(newNode->status), status);
    copy_str(newNode->filing_date, sizeof(newNode->filing_date), date);

    newNode->next = NULL;

    return newNode;
}

/* ---------------- public linked list functions ---------------- */

// Initialise an empty linked list
void case_list_init(CaseList *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

// Delete all nodes from the linked list
void case_list_free(CaseList *list)
{
    CaseNode *temp = list->head;

    while (temp != NULL)
    {
        // Save next address BEFORE freeing the current node
        CaseNode *nextNode = temp->next;

        free(temp);

        temp = nextNode;
    }

    case_list_init(list);
}

// Insert a new case at the end of the list
CaseNode *case_list_append(CaseList *list, int id, const char *number,
                           const char *title, const char *description,
                           const char *type, const char *status,
                           const char *date)
{
    CaseNode *newNode = make_node(id, number, title, description,
                                  type, status, date);

    if (newNode == NULL)
        return NULL;

    // If list is empty, new node becomes first and last
    if (list->head == NULL)
    {
        list->head = newNode;
        list->tail = newNode;
    }
    else
    {
        list->tail->next = newNode;
        list->tail = newNode;
    }

    list->size++;

    return newNode;
}

// Insert a new case at the beginning of the list
CaseNode *case_list_prepend(CaseList *list, int id, const char *number,
                            const char *title, const char *description,
                            const char *type, const char *status,
                            const char *date)
{
    CaseNode *newNode = make_node(id, number, title, description,
                                  type, status, date);

    if (newNode == NULL)
        return NULL;

    // New node points to the current first node
    newNode->next = list->head;

    // Update head
    list->head = newNode;

    // If list was empty, update tail also
    if (list->tail == NULL)
    {
        list->tail = newNode;
    }

    list->size++;

    return newNode;
}

// Search a case using its complete case number
CaseNode *case_list_find_by_number(const CaseList *list, const char *number)
{
    CaseNode *temp = list->head;

    while (temp != NULL)
    {
        if (ci_equal(temp->case_number, number))
            return temp;

        temp = temp->next;
    }

    return NULL;
}

// Search cases using case number, title, type or description
// Matching cases are copied into the output list
size_t case_list_search(const CaseList *src, const char *query, CaseList *out)
{
    CaseNode *temp = src->head;
    size_t found = 0;

    if (query == NULL)
        query = "";

    while (temp != NULL)
    {
        if (ci_contains(temp->case_number, query) ||
            ci_contains(temp->title, query) ||
            ci_contains(temp->case_type, query) ||
            ci_contains(temp->description, query))
        {
            // Count the case only if it was really copied
            if (case_list_append(out, temp->id, temp->case_number,
                                 temp->title, temp->description,
                                 temp->case_type, temp->status,
                                 temp->filing_date) != NULL)
            {
                found++;
            }
        }

        temp = temp->next;
    }

    return found;
}

// Update the status of a case using its ID
int case_list_update_status(CaseList *list, int id, const char *status)
{
    CaseNode *temp = list->head;

    while (temp != NULL)
    {
        if (temp->id == id)
        {
            copy_str(temp->status, sizeof(temp->status), status);
            return 1;
        }

        temp = temp->next;
    }

    return 0;
}

// Delete a case using its ID
int case_list_remove(CaseList *list, int id)
{
    CaseNode *temp = list->head;
    CaseNode *previous = NULL;

    while (temp != NULL)
    {
        if (temp->id == id)
        {
            // If first node is to be deleted
            if (previous == NULL)
            {
                list->head = temp->next;
            }
            else
            {
                previous->next = temp->next;
            }

            // If last node is deleted, update tail
            if (temp == list->tail)
            {
                list->tail = previous;
            }

            free(temp);
            list->size--;

            return 1;
        }

        previous = temp;
        temp = temp->next;
    }

    return 0;
}

// Return total number of nodes
size_t case_list_size(const CaseList *list)
{
    return list->size;
}
