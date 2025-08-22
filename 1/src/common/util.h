#pragma once

#include <stdio.h>

#define trace(FMT, ...) \
    printf("[TRACE:%s:%d(%s)] " FMT "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#ifdef DEBUGGER
#define break() \
    __builtin_debugtrap()
#else
#define break()
#endif

#define noop() do {} while (0)