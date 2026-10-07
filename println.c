#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "println.h"

#define ERROR_MEMORY 1
#define ERROR_INVALID_ARGUMENT 2
#define ERROR_STRUCTURE_NULL 3

int resize(void **list, size_t tamaño) {
    void *temp = realloc(*list, tamaño);

    if (temp == NULL) {
        return ERROR_MEMORY;
    }

    *list = temp;
    return 0;
}

int println(const char *structure, ...) {
    if (structure == NULL) {
        return ERROR_STRUCTURE_NULL;
    }
    va_list args;

    size_t capacity = 6;
    size_t n = 0;

    unsigned char *list = malloc(sizeof(*list) * capacity);
    if (list == NULL) {
        return ERROR_MEMORY;
    }
    size_t length = strlen(structure);

    int search_arg = 0;

    for (size_t i=0; i <= length; i++) {
        if (search_arg) {
            n++;
            if (n == capacity) {
                size_t new_size = capacity + 4;
                if (resize((void **)&list, new_size*sizeof(*list))) {
                    free(list);
                    return ERROR_MEMORY; 
                }
                capacity = new_size;
            }
            switch (structure[i])
            {
            case 'd':
                list[n-1] = 1;
                break;
            case 'c':
                list[n-1] = 2;
                break;
            case 's':
                list[n-1] = 3;
                break;
            case 'f':
                list[n-1] = 4;
                break;
            default:
                free(list);
                return ERROR_INVALID_ARGUMENT;
                break;
            }
            search_arg = 0;
            continue;
        }
        if (i>0) {
            if(structure[i] == '%' && structure[i-1] != '\\') {
                search_arg = 1;
            }
        } else {
            if (structure[i] == '%') {
                search_arg = 1;
            }
        }
    }

    if (search_arg) {
        free(list);
        return ERROR_INVALID_ARGUMENT;
    }
    
    va_start(args, structure);

    va_list copy;

    va_copy(copy, args);

    int size = vsnprintf(NULL, 0, structure, copy) + 1;
    va_end(copy);
    if (size < 0) {
        free(list);
        va_end(args);
        return ERROR_INVALID_ARGUMENT;
    }

    char *final = malloc(sizeof(char)*size);
    if (final == NULL) {
        free(list);
        va_end(args);
        return ERROR_MEMORY;
    }
    
    final[0] = '\0';

    size_t x = 0, y = 0;

    for (size_t i = 0; i <= length; i++) {
        if (structure[i] == '\0') {
            final[y] = '\0';
            break;
        } else if (structure[i] == '%' && x < n) {
            final[y] = '\0';
            switch (list[x]) {
                case 1:
                    int a = va_arg(args, int);
                    int written = snprintf(final + y, size - y, "%d", a);
                    y += written;
                    x++;
                    break;
                case 2:
                    char c = va_arg(args, int);
                    int written2 = snprintf(final + y, size - y, "%c", c);
                    y += written2;
                    x++;
                    break;
                case 3:
                    char *s = va_arg(args, char*);
                    int written3 = snprintf(final + y, size - y, "%s", s);
                    y += written3;
                    x++;
                    break;
                case 4:
                    double d = va_arg(args, double);
                    int written4 = snprintf(final + y, size - y, "%f", d);
                    y += written4;
                    x++;
                    break;
            }
            i++;
            continue;
        } else {
            final[y] = structure[i];
            y++;
            final[y] = '\0';
        }
        
    }
    printf("%s\n", final);
    free(final);
    free(list);
    va_end(args);
    return 0;
}