#include "case_validation.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// Copy an error message safely into the err buffer
static void set_err(char *err, size_t errLen, const char *message)
{
    if (err != NULL && errLen > 0)
    {
        strncpy(err, message, errLen - 1);
        err[errLen - 1] = '\0';
    }
}


// Validate case ID
// Examples: CC-3001 or CC-2026-001
// Format  : 2-4 capital letters, then one or two "-digits" parts
int cc_validate_case_id(const char *id, char *err, size_t errLen)
{
    size_t i = 0;
    int letters = 0;
    int digits;
    int number_parts = 0;

    // Check if case ID is empty
    if (id == NULL || id[0] == '\0')
    {
        set_err(err, errLen, "Case number is required");
        return 0;
    }

    // Maximum length is 20
    if (strlen(id) > 20)
    {
        set_err(err, errLen, "Case number must be at most 20 characters");
        return 0;
    }

    // Count capital letters at the beginning
    while (id[i] != '\0' && isupper((unsigned char)id[i]))
    {
        letters++;
        i++;
    }

    // Prefix must contain 2 to 4 capital letters
    if (letters < 2 || letters > 4)
    {
        set_err(err, errLen, "Use 2-4 capital letters at the beginning");
        return 0;
    }

    // Check number parts, for example "-2026" and "-001"
    while (id[i] != '\0')
    {
        // Every number part must start with -
        if (id[i] != '-')
        {
            set_err(err, errLen, "Invalid case number format");
            return 0;
        }

        i++;
        digits = 0;

        // Count digits after -
        while (isdigit((unsigned char)id[i]))
        {
            digits++;
            i++;
        }

        // Each number part must have 1 to 6 digits
        if (digits < 1 || digits > 6)
        {
            set_err(err, errLen, "Each number part must have 1-6 digits");
            return 0;
        }

        number_parts++;

        // Maximum two number parts are allowed
        if (number_parts > 2)
        {
            set_err(err, errLen, "Maximum two number parts allowed");
            return 0;
        }
    }

    // At least one number part is required
    if (number_parts == 0)
    {
        set_err(err, errLen, "Case number must contain a number");
        return 0;
    }

    return 1;
}


// Check whether a year is a leap year
static int is_leap(int year)
{
    if (year % 400 == 0)
        return 1;

    if (year % 100 == 0)
        return 0;

    if (year % 4 == 0)
        return 1;

    return 0;
}


// Validate date in YYYY-MM-DD format
int cc_validate_date(const char *date)
{
    int year, month, day;
    int max_days;

    // Check date format length and hyphens
    if (date == NULL || strlen(date) != 10)
        return 0;

    if (date[4] != '-' || date[7] != '-')
        return 0;

    // Check that all other characters are digits
    for (int i = 0; i < 10; i++)
    {
        if (i == 4 || i == 7)
            continue;

        if (!isdigit((unsigned char)date[i]))
            return 0;
    }

    // Convert date string into numbers
    if (sscanf(date, "%4d-%2d-%2d", &year, &month, &day) != 3)
        return 0;

    // Check valid year, month and day
    if (year < 1900 || year > 2100)
        return 0;

    if (month < 1 || month > 12 || day < 1)
        return 0;

    // Set maximum days according to month
    if (month == 2)
    {
        if (is_leap(year))
            max_days = 29;
        else
            max_days = 28;
    }
    else if (month == 4 || month == 6 ||
             month == 9 || month == 11)
    {
        max_days = 30;
    }
    else
    {
        max_days = 31;
    }

    // Check whether day is valid
    if (day > max_days)
        return 0;

    return 1;
}


// Validate case type
// Allows letters, digits, spaces and hyphens (length 1 to 50)
int cc_validate_case_type(const char *type)
{
    size_t length;

    if (type == NULL)
        return 0;

    length = strlen(type);

    // Length must be between 1 and 50
    if (length < 1 || length > 50)
        return 0;

    // Check every character
    for (size_t i = 0; i < length; i++)
    {
        if (!isalnum((unsigned char)type[i]) &&
            type[i] != ' ' && type[i] != '-')
        {
            return 0;
        }
    }

    return 1;
}


// Check whether a word exists inside a string
// Uppercase and lowercase are treated equally
static int ci_contains(const char *text, const char *word)
{
    int i, j;

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

        // Complete word found
        if (word[j] == '\0')
            return 1;
    }

    return 0;
}


// Give priority according to case type
// Smaller number means higher priority
int cc_case_type_priority(const char *type)
{
    if (type == NULL || type[0] == '\0')
        return 5;

    if (ci_contains(type, "criminal"))
        return 1;

    if (ci_contains(type, "family"))
        return 2;

    if (ci_contains(type, "property") ||
        ci_contains(type, "civil"))
        return 3;

    if (ci_contains(type, "consumer") ||
        ci_contains(type, "labour") ||
        ci_contains(type, "labor"))
        return 4;

    return 5;
}
