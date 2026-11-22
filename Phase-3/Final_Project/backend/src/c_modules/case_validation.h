/*
    Case Validation Header File

    This file contains function declarations
    for validating case IDs, dates and case types.
*/

#ifndef CC_CASE_VALIDATION_H
#define CC_CASE_VALIDATION_H

#include <stddef.h>

// Recommended size of the error message buffer
#define CC_ERR_BUF 128

// C++ compatibility
#ifdef __cplusplus
extern "C" {
#endif


// Validate case ID
// Returns 1 if valid, otherwise 0 (and writes a message in err)
int cc_validate_case_id(const char *id,
                        char *err,
                        size_t errLen);


// Validate date in YYYY-MM-DD format
// Returns 1 if valid, otherwise 0
int cc_validate_date(const char *date);


// Validate case type
// Returns 1 if valid, otherwise 0
int cc_validate_case_type(const char *type);


// Return priority according to case type
// Smaller number means higher priority
int cc_case_type_priority(const char *type);


#ifdef __cplusplus
}
#endif

#endif
