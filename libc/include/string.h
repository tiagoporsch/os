#pragma once

#include "stdint.h"

void* memcpy(void*, const void*, u64);
void* memmove(void*, const void*, u64);
void* memset(void*, u8, u64);
char* strchr(const char*, char);
int strcmp(const char*, const char*);
char* strcpy(char*, const char*);
char* strdup(const char*);
u64 strlen(const char*);
int strncmp(const char*, const char*, u64);
char* strncpy(char*, const char*, u64);
char* strrchr(const char*, char);
