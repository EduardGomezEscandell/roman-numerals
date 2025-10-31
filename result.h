#pragma once

// Option type that contains a value or an error message
// On success, error will be NULL
// On failure, error will contain a string
//
// free_result must be called when error is not NULL
struct result {
    int value;
    char* error;
};

// convenience function to populate successful results.
struct result success(int x);

// convenience function to populate failures with sprintf-type arguments.
struct result errorf(char const* fmt, ...);

// free the error message. Can be skipped on success.
void free_result(struct result o);
