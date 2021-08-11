#pragma once

#include "stdint.h"

void* malloc(u64);
void* calloc(u64);
void* realloc(void*, u64);
void free(void*);

char* realpath(const char*);

_Noreturn void exit(u64);
void yield();
