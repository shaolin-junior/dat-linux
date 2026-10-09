#include <stdio.h>
#include <stdarg.h>

int println(const char *structure, ...) {
    va_list args;
    va_start(args, structure);

    int result = vprintf(structure, args);

    va_end(args);

    if (result < 0) {
        return result;
    }

    if (putchar('\n') == EOF) {
        return -1;
    }

    return result;
}