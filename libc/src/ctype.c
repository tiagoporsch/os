#include "ctype.h"

bool isalpha(int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool iscntrl(int c) {
	return c == 0x7F || (c >= 0 && c <= 0x1F);
}

bool isdigit(int c) {
	return c >= '0' && c <= '9';
}

bool isspace(int c) {
	return c == ' ' || (c >= '\t' && c <= '\r');
}

bool isprint(int c) {
	return c > 31 && c < 127;
}

int tolower(int c) {
	if (!isalpha(c)) return c;
	return c | (1 << 5);
}

int toupper(int c) {
	if (!isalpha(c)) return c;
	return c & ~(1 << 5);
}
