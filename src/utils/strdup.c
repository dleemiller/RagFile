#include <string.h>
#include <stdlib.h>
#include "strdup.h"


char* strdup(const char* str) {
    if (!str) {
        return NULL; // Return NULL if input string is NULL
    }

    size_t len = strlen(str) + 1;    // Include space for the null-terminator
    char* copy = malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

