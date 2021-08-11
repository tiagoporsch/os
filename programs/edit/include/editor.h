#pragma once

#include <stdbool.h>

typedef struct row {
	int number;
	int size;
	int rsize;
	char* chars;
	char* render;
	char* color;
	bool in_comment;
	bool in_define;
} row_t;
