#include "result.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

struct result success(const int x) {
    const struct result o = {
        .value = x,
        .error = NULL,
    };
    return o;
}

struct result errorf(char const* const fmt, ...) {
    va_list args;

    const size_t max_size = 255;
    const struct result o = {
        .value = 0,
        .error = (char*)calloc(max_size, sizeof(o.value)),
    };

    va_start (args, fmt);
    vsnprintf(o.error, max_size, fmt, args);
    va_end (args);

    return o;
}

void free_result(const struct result o) {
    if (o.error != NULL) {
        free(o.error);
    }
}
