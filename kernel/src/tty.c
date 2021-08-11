#include "tty.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>

#include "cpu.h"
#include "memory.h"

enum tty_ansi_state {
	ANSI_ESC,
	ANSI_BRACKET,
	ANSI_ATTR,
	ANSI_ENDVAL,
};

struct tty_ansi_arg {
	int value;
	bool empty;
};

struct tty_ansi_parser {
	enum tty_ansi_state state;
	struct tty_ansi_arg stack[8];
	int index;
};

enum vga_color {
	VGA_BLACK,
	VGA_RED,
	VGA_GREEN,
	VGA_YELLOW,
	VGA_BLUE,
	VGA_MAGENTA,
	VGA_CYAN,
	VGA_WHITE,
	VGA_BRIGHT_BLACK,
	VGA_BRIGHT_RED,
	VGA_BRIGHT_GREEN,
	VGA_BRIGHT_YELLOW,
	VGA_BRIGHT_BLUE,
	VGA_BRIGHT_MAGENTA,
	VGA_BRIGHT_CYAN,
	VGA_BRIGHT_WHITE,
};

struct tty_cell {
	char ch;
	enum vga_color fg : 4;
	enum vga_color bg : 4;
} __attribute__ ((packed));

static struct {
	struct tty_cell* buffer;
	u64 width, height;
	u64 cursor_x, cursor_y;
	enum vga_color fg, bg;
	struct tty_ansi_parser ansi_parser;
} tty = {
	.buffer = (struct tty_cell*) MEM_AT_PHYS(0xB8000),
	.width = 80, .height = 25,
	.cursor_x = 0, .cursor_y = 0,
	.fg = VGA_WHITE, .bg = VGA_BLACK,
	.ansi_parser = {
		.state = ANSI_ESC,
	},
};

static struct {
	u8 index;
	u8 red, green, blue;
} palette[16] = {
	{ 0x00,  0,  0,  0 }, // Black
	{ 0x01, 42,  0,  0 }, // Red
	{ 0x02,  0, 42,  0 }, // Green
	{ 0x03, 42, 21,  0 }, // Yellow
	{ 0x04,  0,  0, 42 }, // Blue
	{ 0x05, 42,  0, 42 }, // Magenta
	{ 0x14,  0, 42, 42 }, // Cyan
	{ 0x07, 42, 42, 42 }, // White
	{ 0x38, 21, 21, 21 }, // Bright Black
	{ 0x39, 63, 21, 21 }, // Bright Red
	{ 0x3A, 21, 63, 21 }, // Bright Green
	{ 0x3B, 63, 63, 21 }, // Bright Yellow
	{ 0x3C, 21, 21, 63 }, // Bright Blue
	{ 0x3D, 63, 21, 63 }, // Bright Magenta
	{ 0x3E, 21, 63, 63 }, // Bright Cyan
	{ 0x3F, 63, 63, 63 }, // Bright White
};

void tty_init() {
	while ((in8(0x03DA) & 0x08));
	while (!(in8(0x03DA) & 0x08));
	for (u8 i = 0; i < 16; i++) {
		out8(0x03C8, palette[i].index);
		out8(0x03C9, palette[i].red);
		out8(0x03C9, palette[i].green);
		out8(0x03C9, palette[i].blue);
	}
}

static void update_cursor() {
	u16 pos = tty.cursor_x + tty.cursor_y * tty.width;
	out8(0x3D4, 0x0F);
	out8(0x3D5, (u8) (pos & 0xFF));
	out8(0x3D4, 0x0E);
	out8(0x3D5, (u8) ((pos >> 8) & 0xFF));
}

static void scroll() {
	tty.cursor_y = tty.height - 1;
	memmove(tty.buffer, tty.buffer + tty.width, tty.width * tty.cursor_y * sizeof(struct tty_cell));
	for (u64 i = tty.width * tty.cursor_y; i < tty.width * tty.height; i++) {
		tty.buffer[i].ch = ' ';
		tty.buffer[i].fg = tty.fg;
		tty.buffer[i].bg = tty.bg;
	}
}

static void set(u64 x, u64 y, char c, enum vga_color fg, enum vga_color bg) {
	struct tty_cell* cell = &tty.buffer[y * tty.width + x];
	cell->ch = c;
	cell->fg = fg;
	cell->bg = bg;
}

static void append(char c) {
	if (c == '\n') {
		tty.cursor_x = 0;
		if (++tty.cursor_y >= tty.height)
			scroll();
	} else if (c == '\b') {
		if (tty.cursor_x-- == 0) {
			if (tty.cursor_y > 0) {
				tty.cursor_x = tty.width - 1;
				tty.cursor_y--;
			} else {
				tty.cursor_x = 0;
			}
		}
		set(tty.cursor_x, tty.cursor_y, ' ', tty.fg, tty.bg);
	} else if (c == '\t') {
		tty.cursor_x += 4 - (tty.cursor_x % 4);
		if (tty.cursor_x >= tty.width) {
			tty.cursor_x = 0;
			if (++tty.cursor_y == tty.height)
				scroll();
		}
	} else if (c == '\r') {
		tty.cursor_x = 0;
	} else {
		set(tty.cursor_x, tty.cursor_y, c, tty.fg, tty.bg);
		if (++tty.cursor_x >= tty.width) {
			tty.cursor_x = 0;
			if (++tty.cursor_y >= tty.height)
				scroll();
		}
	}
}

static void tty_write(int c) {
	struct tty_ansi_parser* p = &tty.ansi_parser;
	switch (p->state) {
		case ANSI_ESC:
			if (c == '\033') {
				p->state = ANSI_BRACKET;
				p->index = 0;
				p->stack[p->index].value = 0;
				p->stack[p->index].empty = true;
			} else {
				append(c);
			}
			break;
		case ANSI_BRACKET:
			if (c == '[') {
				p->state = ANSI_ATTR;
			} else {
				p->state = ANSI_ESC;
				append(c);
			}
			break;
		case ANSI_ATTR:
			if (isdigit(c)) {
				p->stack[p->index].value *= 10;
				p->stack[p->index].value += (c - '0');
				p->stack[p->index].empty = false;
				break;
			} else {
				if (p->index < 8)
					p->index++;
				p->stack[p->index].value = 0;
				p->stack[p->index].empty = true;
				p->state = ANSI_ENDVAL;
			}
			[[fallthrough]];
		case ANSI_ENDVAL:
			switch (c) {
				case ';':
					p->state = ANSI_ATTR;
					return;
				case 'm':
					for (int i = 0; i < p->index; i++) {
						if (p->stack[i].empty || p->stack[i].value == 0) {
							tty.fg = VGA_WHITE;
							tty.bg = VGA_BLACK;
						} else if (p->stack[i].value == 7) {
							tty.fg = tty.fg ^ tty.bg;
							tty.bg = tty.fg ^ tty.bg;
							tty.fg = tty.fg ^ tty.bg;
						} else {
							int attr = p->stack[i].value;
							if (attr >= 30 && attr <= 37) tty.fg = attr - 30;
							if (attr >= 90 && attr <= 97) tty.fg = attr - 82;
							if (attr >= 40 && attr <= 47) tty.bg = attr - 40;
							if (attr >= 100 && attr <= 107) tty.bg = attr - 92;
						}
					}
					break;
				case 'A':
					tty.cursor_y -= p->stack[0].empty ? 1 : p->stack[0].value;
					if (tty.cursor_y >= tty.height) tty.cursor_y = 0;
					break;
				case 'B':
					tty.cursor_y += p->stack[0].empty ? 1 : p->stack[0].value;
					if (tty.cursor_y >= tty.height) tty.cursor_y = tty.height - 1;
					break;
				case 'C':
					tty.cursor_x += p->stack[0].empty ? 1 : p->stack[0].value;
					if (tty.cursor_x >= tty.width) tty.cursor_x = tty.width - 1;
					break;
				case 'D':
					tty.cursor_x -= p->stack[0].empty ? 1 : p->stack[0].value;
					if (tty.cursor_x >= tty.width) tty.cursor_x = 0;
					break;
				case 'H':
					if (p->stack[0].empty)
						tty.cursor_y = 0;
					else if (p->stack[0].value > (int) tty.height)
						tty.cursor_y = tty.height - 1;
					else
						tty.cursor_y = p->stack[0].value - 1;
					if (p->stack[1].empty)
						tty.cursor_x = 0;
					else if (p->stack[1].value > (int) tty.width)
						tty.cursor_x = tty.width - 1;
					else
						tty.cursor_x = p->stack[1].value - 1;
					break;
				case 'J': {
					u64 start, end;
					if (p->stack[0].empty || p->stack[0].value == 0) {
						start = tty.cursor_y * tty.width + tty.cursor_x;
						end = tty.width * tty.height;
					} else if (p->stack[0].value == 1) {
						start = 0;
						end = tty.cursor_y * tty.width + tty.cursor_x;
					} else if (p->stack[0].value == 2) {
						start = 0;
						end = tty.width * tty.height;
					}
					for (; start < end; start++) {
						tty.buffer[start].ch = ' ';
						tty.buffer[start].fg = tty.fg;
						tty.buffer[start].bg = tty.bg;
					}
					break;
				}
				case 'K': {
					u64 start, end;
					if (p->stack[0].empty || p->stack[0].value == 0) {
						start = tty.cursor_x + tty.width * tty.cursor_y;
						end = (tty.cursor_y + 1) * tty.width;
					} else if (p->stack[0].value == 1) {
						start = tty.cursor_y * tty.width;
						end = tty.cursor_x + tty.width * tty.cursor_y;
					} else if (p->stack[0].value == 2) {
						start = tty.cursor_y * tty.width;
						end = start + tty.width;
					}
					for (; start < end; start++) {
						tty.buffer[start].ch = ' ';
						tty.buffer[start].fg = tty.fg;
						tty.buffer[start].bg = tty.bg;
					}
					break;
				}
			}
			p->state = ANSI_ESC;
			break;
	}
}

void tty_putchar(char c) {
	tty_write(c);
	update_cursor();
}

void tty_puts(const char* s) {
	while (*s) tty_write(*s++);
	update_cursor();
}

u64 tty_flush() {
	char buffer[256];
	u64 r, total = 0;
	while ((r = stream_read(stdout, buffer, sizeof(buffer) - 1))) {
		buffer[r] = 0;
		for (u64 i = 0; i < r; i++)
			tty_write(buffer[i]);
		update_cursor();
		total += r;
	}
	return total;
}
