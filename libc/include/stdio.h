#pragma once

#include "stdarg.h"
#include "stdint.h"
#include "stream.h"

enum std_key {
	KEY_SEQ = 0x1B,

	KEY_ESC = 0x100,

	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
	KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,

	KEY_PRINT, KEY_PAUSE,
	KEY_INSERT, KEY_HOME, KEY_PAGE_UP,
	KEY_DELETE, KEY_END, KEY_PAGE_DOWN,

	KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT,
};

extern struct stream* stdout;
extern struct stream* stdin;

int getkey();
char getchar();
void putchar(char);
u64 puts(const char*);
u64 printf(const char*, ...);
u64 vprintf(const char*, va_list);
u64 vsnprintf(char*, u64, const char*, va_list);
