#include "stdio.h"

#include "ctype.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stream.h"
#include "system.h"

struct stream* stdin = NULL;
struct stream* stdout = NULL;

int getkey() {
	char c;
	while (stream_read(stdin, &c, 1) == 0)
		yield();
	if (c != KEY_SEQ)
		return (int) c;
	if (stream_read(stdin, &c, 1) == 0)
		return KEY_SEQ;
	return KEY_ESC + c;
}

char getchar() {
	while (1) {
		char c;
		while (stream_read(stdin, &c, 1) == 0)
			yield();
		if (c != KEY_SEQ)
			return c;
		stream_read(stdin, &c, 1);
	}
}

void putchar(char c) {
	stream_write(stdout, &c, 1);
}

u64 puts(const char* s) {
	u64 len = strlen(s);
	stream_write(stdout, s, len);
	return len;
}

static char* itoa(u64 i, int base, int pad, char padchar) {
	static const char* repr = "0123456789ABCDEF";
	static char buf[65] = { 0 };
	char* ptr = &buf[64];
	*ptr = 0;
	do {
		*--ptr = repr[i % base];
		if (pad) pad--;
		i /= base;
	} while (i != 0);
	while (pad--) *--ptr = padchar;
	return ptr;
}

u64 printf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	u64 written = vprintf(format, args);
	va_end(args);
	return written;
}

u64 vprintf(const char* format, va_list args) {
	u64 written = 0;
	int pad = 0;
	char padchar = ' ';
	while (*format != '\0') {
		if (*format == '%' || pad != 0) {
			if (*format == '%')
				format++;
			if (*format == '%') {
				putchar('%');
				written++;
				format++;
			} else if (*format == 'c') {
				putchar((char) va_arg(args, int));
				written++;
				format++;
			} else if (*format == 's') {
				written += puts(va_arg(args, char*));
				format++;
			} else if (*format == 'd') {
				i64 number = va_arg(args, i64);
				if (number < 0) {
					number = -number;
					putchar('-');
				}
				written += puts(itoa(number, 10, pad, padchar));
				pad = 0;
				format++;
			} else if (*format == 'u') {
				written += puts(itoa(va_arg(args, u64), 10, pad, padchar));
				pad = 0;
				format++;
			} else if (*format == 'x') {
				written += puts(itoa(va_arg(args, u64), 16, pad, padchar));
				pad = 0;
				format++;
			} else if (isdigit(*format)) {
				if (*format == '0') {
					padchar = '0';
					format++;
				} else {
					padchar = ' ';
				}
				while (isdigit(*format)) {
					pad *= 10;
					pad += (*format++ - '0');
				}
			}
		} else {
			putchar(*format++);
			written++;
		}
	}
	return written;
}

u64 vsnprintf(char* buffer, u64 size, const char* format, va_list args) {
	if (size == 0)
		return 0;
	if (size == 1) {
		buffer[0] = 0;
		return 1;
	}

	u64 written = 0;
	int pad = 0;
	char padchar = ' ';
	while (*format != '\0') {
		if (*format == '%' || pad != 0) {
			if (*format == '%')
				format++;
			if (*format == '%') {
				buffer[written] = '%';
				if (++written == size - 1) break;
				format++;
			} else if (*format == 'c') {
				buffer[written] = (char) va_arg(args, int);
				if (++written == size - 1) break;
				format++;
			} else if (*format == 's') {
				char* s = va_arg(args, char*);
				while (*s) {
					buffer[written] = *s++;
					if (++written == size - 1) break;
				}
				format++;
			} else if (*format == 'd') {
				i64 number = va_arg(args, i64);
				if (number < 0) {
					number = -number;
					buffer[written] = '-';
					if (++written == size - 1) break;
				}
				char* s = itoa(number, 10, pad, padchar);
				while (*s) {
					buffer[written] = *s++;
					if (++written == size - 1) break;
				}
				pad = 0;
				format++;
			} else if (*format == 'u') {
				char* s = itoa(va_arg(args, u64), 10, pad, padchar);
				while (*s) {
					buffer[written] = *s++;
					if (++written == size - 1) break;
				}
				pad = 0;
				format++;
			} else if (*format == 'x') {
				char* s = itoa(va_arg(args, u64), 16, pad, padchar);
				while (*s) {
					buffer[written] = *s++;
					if (++written == size - 1) break;
				}
				pad = 0;
				format++;
			} else if (isdigit(*format)) {
				if (*format == '0') {
					padchar = '0';
					format++;
				} else {
					padchar = ' ';
				}
				while (isdigit(*format)) {
					pad *= 10;
					pad += (*format++ - '0');
				}
			}
		} else {
			buffer[written] = *format++;
			if (++written == size - 1) break;
		}
	}
	buffer[written] = 0;
	return written;
}
