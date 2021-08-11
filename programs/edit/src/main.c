#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stream.h>
#include <string.h>

#include "editor.h"
#include "syntax.h"

// Macros
#define TERM_WIDTH 80
#define TERM_HEIGHT 25

#define VIEW_WIDTH (TERM_WIDTH - 4)
#define VIEW_HEIGHT (TERM_HEIGHT - 1)

#define CTRL_KEY(d) ((d) & 0x1F)

// Global Variables
int rx = 0, cx = 0, cy = 0;
int row_off = 0, col_off = 0;
int row_count = 0;
row_t* rows = NULL;

const char* filename = NULL;
bool modified = false;
syntax_f syntax_highlight;

// Helpers
void set_cursor(int x, int y) { printf("\x1b[%d;%dH", y + 1, x + 1); }
void set_fg(int fg) { printf("\x1b[%dm", fg); }
void set_bg(int bg) { printf("\x1b[%dm", bg + 10); }
void clear_line() { puts("\x1b[2K"); }

// Status
char status_str[78];

void status_set(const char* format, ...) {
	va_list args;
	va_start(args, format);
	vsnprintf(status_str, sizeof(status_str), format, args);
	va_end(args);
}

void status_draw() {
	set_fg(VGA_BLACK);
	set_bg(VGA_WHITE);
	set_cursor(1, VIEW_HEIGHT);
	clear_line();
	if (status_str[0] != 0) {
		puts(status_str);
		status_str[0] = 0;
	} else {
		if (cx != rx) {
			status_set("%d/%d,%d-%d %s%s",
				cy + 1, row_count, cx + 1, rx + 1,
				filename ? filename : "[No Name]",
				modified ? " (modified)" : ""
			);
		} else {
			status_set("%d/%d,%d %s%s",
				cy + 1, row_count, cx + 1,
				filename ? filename : "[No Name]",
				modified ? " (modified)" : ""
			);
		}
		puts(status_str);
		status_str[0] = 0;
	}
}

// Screen
void screen_draw(int y) {
	set_fg(VGA_WHITE);
	set_bg(VGA_BLACK);
	set_cursor(0, y);
	clear_line();

	int r = y + row_off;
	if (r >= row_count) {
		putchar('~');
		return;
	}

	row_t* row = &rows[r];
	printf("%3d ", r + 1);
	i64 n = row->rsize - col_off;
	if (n < 0) n = 0;
	if (n >= VIEW_WIDTH) n = VIEW_WIDTH - 1;
	if (syntax_highlight) {
		char* s = &row->render[col_off];
		char* c = &row->color[col_off];
		char color = VGA_WHITE;
		for (int i = 0; i < n; i++) {
			if (c[i] != color) {
				color = c[i];
				set_fg(color);
			}
			putchar(s[i]);
		}
	} else {
		stream_write(stdout, &row->render[col_off], n);
	}
}

void screen_draw_range(int from, int to) {
	assert(from >= 0);
	assert(to <= VIEW_HEIGHT);
	for (int y = from; y < to; y++)
		screen_draw(y);
}

void screen_refresh() {
	rx = 0;
	if (cy < row_count) {
		for (int i = 0; i < cx; i++) {
			if (rows[cy].chars[i] == '\t')
				rx += 3 - (rx % 4);
			rx++;
		}
	}

	bool redraw = false;
	if (cy - row_off < VIEW_HEIGHT / 2 && row_off > 0) {
		row_off = cy - VIEW_HEIGHT / 2;
		if (row_off < 0) row_off = 0;
		redraw = true;
	}
	if (cy - row_off > VIEW_HEIGHT / 2 && row_off <= row_count - VIEW_HEIGHT) {
		row_off = cy - VIEW_HEIGHT / 2;
		if (row_off > row_count - VIEW_HEIGHT + 1)
			row_off = row_count - VIEW_HEIGHT + 1;
		redraw = true;
	}
	if (rx < col_off) {
		col_off = rx;
		redraw = true;
	}
	if (rx >= col_off + VIEW_WIDTH) {
		col_off = rx - VIEW_WIDTH + 1;
		redraw = true;
	}
	if (redraw)
		screen_draw_range(0, VIEW_HEIGHT);

	status_draw();
	set_cursor(rx - col_off + 4, cy - row_off);
}

// Row
void row_render(row_t* row) {
	int tabs = 0;
	for (int i = 0; i < row->size; i++)
		if (row->chars[i] == '\t')
			tabs++;
	row->render = realloc(row->render, row->size + tabs * 3 + 1);
	int ri = 0;
	for (int i = 0; i < row->size; i++) {
		if (row->chars[i] == '\t') {
			do row->render[ri++] = ' ';
			while (ri % 4 != 0);
		} else {
			row->render[ri++] = row->chars[i];
		}
	}
	row->render[ri] = 0;
	row->rsize = ri;

	if (syntax_highlight) {
		while (syntax_highlight(row)) {
			if (row->number >= row_off && row->number < row_off + VIEW_HEIGHT)
				screen_draw(row->number - row_off);
			if (row->number >= row_count - 1)
				break;
			row += 1;
		}
	}
}

void row_insert(row_t* row, int x, char c) {
	if (x < 0 || x > row->size) return;
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[x + 1], &row->chars[x], row->size - x + 1);
	row->size++;
	row->chars[x] = c;
	row_render(row);
	modified = true;
}

void row_delete(row_t* row, int x) {
	if (x < 0 || x >= row->size) return;
	memmove(&row->chars[x], &row->chars[x + 1], row->size - x);
	row->size--;
	row_render(row);
	modified = true;
}

void row_append(row_t* row, char* s, u64 n) {
	row->chars = realloc(row->chars, row->size + n + 1);
	memcpy(&row->chars[row->size], s, n);
	row->size += n;
	row->chars[row->size] = 0;
	row_render(row);
	modified = true;
}

void row_free(row_t* row) {
	free(row->render);
	free(row->chars);
	free(row->color);
}

// Rows
void rows_add(int r, char* s, u64 n) {
	if (r < 0 || r > row_count)
		return;
	rows = realloc(rows, sizeof(row_t) * (row_count + 1));
	memmove(&rows[r + 1], &rows[r], sizeof(row_t) * (row_count - r));
	for (int i = r + 1; i <= row_count; i++) rows[i].number++;

	row_t* row = &rows[r];
	row->number = r;
	row->size = n;
	row->chars = malloc(n + 1);
	memcpy(row->chars, s, n + 1);

	row->rsize = 0;
	row->render = NULL;
	row->color = NULL;
	row->in_comment = false;
	row->in_define = false;
	row_render(row);

	row_count++;
	modified = true;
}

void rows_delete(int r) {
	if (r < 0 || r >= row_count) return;
	row_free(&rows[r]);
	memmove(&rows[r], &rows[r + 1], sizeof(row_t) * (row_count - r - 1));
	for (int i = r; i < row_count - 1; i++) rows[i].number--;
	row_count--;
	modified = true;
}

char* process_prompt(char* prompt) {
	u64 buf_size = 128;
	char* buf = malloc(buf_size);

	u64 buf_len = 0;
	buf[0] = 0;

	while (1) {
		status_set(prompt, buf);
		u64 status_len = strlen(status_str);
		screen_refresh();
		set_cursor(status_len + 1, VIEW_HEIGHT);

		int c = getkey();
		if (c == KEY_ESC) {
			free(buf);
			return NULL;
		} else if (c == '\n') {
			if (buf_len != 0)
				return buf;
		} else if (c == '\b') {
			if (buf_len > 0)
				buf[--buf_len] = 0;
		} else if (c < 128 && isprint((char) c)) {
			if (buf_len == buf_size - 1) {
				buf_size *= 2;
				buf = realloc(buf, buf_size);
			}
			buf[buf_len++] = (char) c;
			buf[buf_len] = 0;
		}
	}
}

// File I/O
void file_save() {
	if (filename == NULL) {
		filename = process_prompt("save as: %s");
		if (filename == NULL)
			return;
		syntax_highlight = syntax_function(filename);
		if (syntax_highlight) {
			for (int r = 0; r < row_count; r++)
				syntax_highlight(&rows[r]);
			screen_draw_range(0, VIEW_HEIGHT);
		}
	}

	std_file_t* file = std_file_open(filename, STD_FILE_CREATE | STD_FILE_CLEAR);
	if (file == NULL) {
		status_set("error opening file");
		return;
	}

	struct stream* stream = stream_open(file);
	if (stream == NULL) {
		status_set("error opening file stream");
		std_file_close(file);
		return;
	}

	u64 total = 0;
	char nl = '\n';
	for (int r = 0; r < row_count; r++) {
		i64 written = (i64) stream_write(stream, rows[r].chars, rows[r].size);
		if (written != rows[r].size || stream_write(stream, &nl, 1) != 1) {
			status_set("error writing file");
			stream_close(stream);
			std_file_close(file);
			return;
		}
		total += written + 1;
	}

	stream_close(stream);
	std_file_close(file);
	modified = false;
	status_set("%d bytes written", total);
}

void file_load(const char* path) {
	std_file_t* file = std_file_open(path, STD_FILE_CREATE);
	if (file == NULL) {
		status_set("error opening file");
		return;
	}

	struct stream* stream = stream_open(file);
	if (stream == NULL) {
		status_set("error opening file stream", path);
		std_file_close(file);
		return;
	}

	filename = (char*) path;
	syntax_highlight = syntax_function(filename);

	char buf[1024];
	int index;
	while (stream->offset < stream->length) {
		index = 0;
		do if (!stream_read(stream, &buf[index], 1)) break;
		while (buf[index++] != '\n');
		buf[--index] = 0;
		rows_add(row_count, buf, index);
	}
	stream_close(stream);
	std_file_close(file);

	modified = false;
}

// Input
void process_keypress() {
	static bool quit_confirm = true;

	int c = getkey();
	switch (c) {
		case CTRL_KEY('q'):
			if (modified && quit_confirm) {
				status_set("warning: unsaved changes");
				quit_confirm = false;
				return;
			}
			set_fg(VGA_WHITE);
			set_bg(VGA_BLACK);
			set_cursor(0, TERM_HEIGHT - 1);
			clear_line();
			exit(0);
			break;
		case CTRL_KEY('s'):
			file_save();
			break;
		case KEY_HOME:
			cx = 0;
			break;
		case KEY_END:
			cx = rows[cy].size;
			break;
		case KEY_UP:
			if (cy == 0)
				break;
			if (cx > rows[--cy].size)
				cx = rows[cy].size;
			break;
		case KEY_DOWN:
			if (cy == row_count)
				break;
			if (++cy == row_count)
				cx = 0;
			else if (cx > rows[cy].size)
				cx = rows[cy].size;
			break;
		case KEY_LEFT:
			if (cx > 0)
				cx--;
			break;
		case KEY_RIGHT:
			if (cx < rows[cy].size)
				cx++;
			break;
		case KEY_PAGE_DOWN:
			cy += VIEW_HEIGHT;
			if (cy >= row_count) {
				cy = row_count;
				cx = 0;
			} else {
				cx = rows[cy].size;
			}
			break;
		case KEY_PAGE_UP:
			cy -= VIEW_HEIGHT;
			if (cy < 0)
				cy = 0;
			break;
		case KEY_DELETE:
			if (cy == row_count)
				break;
			if (cx == rows[cy].size) {
				if (cy >= row_count - 1)
					break;
				row_append(&rows[cy], rows[cy + 1].chars, rows[cy + 1].size);
				rows_delete(cy + 1);
				screen_draw_range(cy - row_off, VIEW_HEIGHT);
			} else {
				row_delete(&rows[cy], cx);
				screen_draw(cy - row_off);
			}
			break;
		case KEY_INSERT:
			break;
		case '\b':
			if (cy == row_count)
				break;
			if (cx == 0) {
				if (cy == 0)
					break;
				cx = rows[cy - 1].size;
				row_append(&rows[cy - 1], rows[cy].chars, rows[cy].size);
				rows_delete(cy--);
				screen_draw_range(cy - row_off, VIEW_HEIGHT);
			} else {
				row_delete(&rows[cy], --cx);
				screen_draw(cy - row_off);
			}
			break;
		case '\n':
			if (cx == 0) {
				rows_add(cy, "", 0);
			} else {
				row_t* row = &rows[cy];
				rows_add(cy + 1, &row->chars[cx], row->size - cx);
				row = &rows[cy];
				row->size = cx;
				row->chars[row->size] = 0;
				row_render(row);
			}
			screen_draw_range(cy - row_off, VIEW_HEIGHT);
			cy++;
			cx = 0;
			break;
		default:
			if (cy == row_count) {
				rows_add(row_count, "", 0);
				screen_draw_range(cy - row_off + 1, VIEW_HEIGHT);
			}
			row_insert(&rows[cy], cx++, c);
			screen_draw(cy - row_off);
			break;
	}

	quit_confirm = true;
}

// Main
int main(int argc, char** argv) {
	if (argc == 2)
		file_load(argv[1]);
	screen_draw_range(0, VIEW_HEIGHT);
	for (;;) {
		screen_refresh();
		process_keypress();
	}
	return 0;
}
