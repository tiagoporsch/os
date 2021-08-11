#include "string.h"

#include "stddef.h"
#include "stdlib.h"

void* memcpy(void* d, const void* s, u64 n) {
	u8* cd = d;
	const u8* cs = s;
	while (n--) *cd++ = *cs++;
	return d;
}

void* memmove(void* d, const void* s, u64 n) {
	u8* cd = d;
	const u8* cs = s;
	if (s < d) while (n--) cd[n] = cs[n];
	else while (n--) *cd++ = *cs++;
	return d;
}

void* memset(void* d, u8 c, u64 n) {
	u8* cd = d;
	while (n--) *cd++ = c;
	return d;
}

char* strchr(const char* s, char c) {
	while (*s) if (*s++ == c) return (char*) s - 1;
	return NULL;
}

int strcmp(const char* a, const char* b) {
	for (; *a == *b && *a; a++, b++);
	return *a - *b;
}

char* strcpy(char* d, const char* s) {
	for (u64 i = 0; ; i++) {
		d[i] = s[i];
		if (!s[i]) break;
	}
	return d;
}

char* strdup(const char* s) {
	u64 n = strlen(s) + 1;
	char* d = malloc(n);
	if (d == NULL) return NULL;
	strncpy(d, s, n);
	return d;
}

u64 strlen(const char* s) {
	u64 len = 0;
	while (*s++) len++;
	return len;
}

int strncmp(const char* a, const char* b, u64 n) {
	for (; *a == *b && --n && *a; a++, b++);
	return *a - *b;
}

char* strncpy(char* d, const char* s, u64 n) {
	for (u64 i = 0; i < n; i++) {
		d[i] = s[i];
		if (!s[i]) break;
	}
	return d;
}

char* strrchr(const char* s, char c) {
	u64 i = strlen(s) - 1;
	while (1) {
		if (s[i] == c) return (char*) &s[i];
		if (i-- == 0) return NULL;
	}
}
