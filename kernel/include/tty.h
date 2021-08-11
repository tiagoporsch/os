#pragma once

#include <stdint.h>

void tty_init();
void tty_putchar(char);
void tty_puts(const char*);
void tty_printf(const char*, ...);
u64 tty_flush();
