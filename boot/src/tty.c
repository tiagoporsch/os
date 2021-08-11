#include "tty.h"

#include <stdarg.h>
#include <stdint.h>

void putchar(char c) {
	static u16* buffer = (u16*) 0xB8000;
	static i64 cursor_x = 0, cursor_y = 0;

	if (c == '\n') {
		cursor_x = 0;
		if (++cursor_y == 24)
			cursor_y = 0;
	} else if (c == '\b') {
		if (cursor_x-- == 0) {
			if (cursor_y > 0) {
				cursor_x = 79;
				cursor_y--;
			} else {
				cursor_x = 0;
			}
		}
		buffer[cursor_x + 80 * cursor_y] = 0x0F00 | ' ';
	} else if (c == '\t') {
		cursor_x += 4 - (cursor_x % 4);
		if (cursor_x >= 80) {
			cursor_x = 0;
			if (++cursor_y == 24)
				cursor_y = 0;
		}
	} else if (c == '\r') {
		cursor_x = 0;
	} else {
		buffer[cursor_x + 80 * cursor_y] = 0x0F00 | c;
		if (++cursor_x == 80) {
			cursor_x = 0;
			if (++cursor_y == 80)
				cursor_y = 0;
		}
	}
}

void puts(const char* s) {
	while (*s)
		putchar(*s++);
}

char* itoa(u64 i, int base, int pad, char padchar) {
	static const char* rep = "0123456789ABCDEF";
	static char buffer[65] = {0};
	char *ptr = &buffer[64];
	*ptr = '\0';
	do {
		*--ptr = rep[i % base];
		if (pad) pad--;
		i /= base;
	} while (i != 0);
	while (pad--) {
		*--ptr = padchar;
	}
	return ptr;
}

void printf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void vprintf(const char* format, va_list args) {
	u8 pad = 0;
	char padchar = ' ';
	while (*format != '\0') {
		if (*format == '%' || pad != 0) {
			if (*format == '%')
				format++;
			if (*format == '%') {
				putchar('%');
				format++;
			} else if (*format == 'c') {
				putchar((char) va_arg(args, int));
				format++;
			} else if (*format == 's') {
				puts((char*) va_arg(args, char*));
				format++;
			} else if (*format == 'd') {
				i64 d = va_arg(args, i64);
				if (d < 0) {
					d = -d;
					putchar('-');
				}
				puts(itoa(d, 10, pad, padchar));
				pad = 0;
				format++;
			} else if (*format == 'u') {
				puts(itoa(va_arg(args, u64), 10, pad, padchar));
				pad = 0;
				format++;
			} else if (*format == 'x') {
				puts(itoa(va_arg(args, u64), 16, pad, padchar));
				pad = 0;
				format++;
			} else if (*format >= '0' && *format <= '9') {
				if (*format == '0') {
					padchar = '0';
					format++;
				} else {
					padchar = ' ';
				}
				while (*format >= '0' && *format <= '9') {
					pad *= 10;
					pad += (*format++ - '0');
				}
			}
		} else {
			putchar(*format++);
		}
	}
}

_Noreturn void panic(const char* format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	while (1) asm volatile ("cli; hlt");
}
