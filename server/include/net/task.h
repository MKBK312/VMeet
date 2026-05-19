#pragma once

#include <cstddef>

typedef void (*TaskFunc)(void*);

typedef struct {
    TaskFunc func;
    void* arg;
} Task;
