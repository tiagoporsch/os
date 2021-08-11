#include "string.h"

void* memcpy(void* d, const void* s, u64 n) {
	u8* cd = d;
	const u8* cs = s;
	while (n--) *cd++ = *cs++;
	return d;
}

void* memset(void* d, u8 c, u64 n) {
	u8* cd = d;
	while (n--) *cd++ = c;
	return d;
}

int strcmp(const char* a, const char* b) {
	for (; *a == *b && *a; a++, b++);
	return *a - *b;
}
