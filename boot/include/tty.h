#pragma once

#include <stdarg.h>

void putchar(char);
void puts(const char*);
void printf(const char*, ...);
void vprintf(const char*, va_list);
_Noreturn void panic(const char*, ...);
