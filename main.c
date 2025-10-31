#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rome.h"
#include "result.h"

int main() {
    const int max_size = 255;
    char buff[max_size];

    while (true) {
        printf("Write a roman numeral: ");
        if (fgets(buff, max_size, stdin) == NULL) {
            break;
        }

        const struct result res = parse_roman_number(buff);
        if (res.error != NULL) {
            fprintf(stdout, "Invalid input: %s\n", res.error);
            free_result(res);
            continue;
        }

        printf("Result: %d\n", res.value);
        free_result(res);
    }
}

