#include <string.h>
#include "strdup.h"


char* strdup(const char* str) {
    size_t len = strlen(str) + 1;    // Include space for the null-terminator
    char* copy = malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}


