#ifndef FLASHUX_TEST_H
#define FLASHUX_TEST_H
#include <stdio.h>
#include <stdlib.h>
#define CHECK(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        abort(); \
    } \
} while (0)
#endif
