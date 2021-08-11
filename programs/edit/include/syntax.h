#pragma once

#include <stdint.h>

#include "editor.h"

enum vga_color {
	VGA_BLACK = 30,
	VGA_RED,
	VGA_GREEN,
	VGA_YELLOW,
	VGA_BLUE,
	VGA_MAGENTA,
	VGA_CYAN,
	VGA_WHITE,
	VGA_BRIGHT_BLACK = 90,
	VGA_BRIGHT_RED,
	VGA_BRIGHT_GREEN,
	VGA_BRIGHT_YELLOW,
	VGA_BRIGHT_BLUE,
	VGA_BRIGHT_MAGENTA,
	VGA_BRIGHT_CYAN,
	VGA_BRIGHT_WHITE,
};

typedef bool (*syntax_f)(row_t*);
syntax_f syntax_function(const char*);
