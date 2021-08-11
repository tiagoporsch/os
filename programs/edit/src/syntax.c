#include "syntax.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

bool is_separator(int c) {
	return isspace(c) || c == 0 || strchr(",;()+-/*=~%<>[];", c) != NULL;
}

bool is_digit(int c, int base) {
	const char* repr = "0123456789ABCEDF";
	for (int i = 0; i < base; i++)
		if (toupper(c) == repr[i])
			return true;
	return false;
}

u64 check_keyword(const char* buffer, const char** list) {
	for (int i = 0; list[i]; i++) {
		u64 n = strlen(list[i]);
		if (!strncmp(buffer, list[i], n) && is_separator(buffer[n]))
			return n;
	}
	return 0;
}

bool syntax_highlight_c(row_t* row) {
	static const char* keywords[] = {
		"asm", "auto", "break", "case", "continue", "default", "do",
		"else", "for", "goto", "if", "return", "sizeof", "switch",
		"while",
		NULL
	};
	static const char* types[] = {
		"bool", "char", "const", "double", "enum", "extern", "float",
		"i8", "i16", "i32", "i64", "inline", "int", "long", "register",
		"restrict", "short", "signed", "static", "struct", "typedef",
		"u8", "u16", "u32", "u64", "union", "unsigned", "va_list",
		"void", "volatile",
		NULL
	};
	static const char* special[] = {
		"false", "NULL", "true",
		NULL
	};

	bool was_sep = true;
	bool in_comment = (row->number > 0 && row[-1].in_comment);
	bool in_define = (row->number > 0 && row[-1].in_define);
	bool in_include = false;
	char in_string = 0;
	char in_num = 0;

	row->color = realloc(row->color, row->rsize);
	memset(row->color, in_define ? VGA_MAGENTA : VGA_WHITE, row->rsize);

	for (int i = 0; i < row->rsize;) {
		char c = row->render[i];

		// Comments
		if (!in_string) {
			if (in_comment) {
				row->color[i] = VGA_BRIGHT_BLACK;
				if (!strncmp(&row->render[i], "*/", 2)) {
					memset(&row->color[i], VGA_BRIGHT_BLACK, 2);
					was_sep = true;
					in_comment = false;
					i += 2;
					continue;
				} else {
					i++;
					continue;
				}
			} else if (!strncmp(&row->render[i], "/*", 2)) {
				memset(&row->color[i], VGA_BRIGHT_BLACK, 2);
				in_comment = true;
				i += 2;
				continue;
			} else if (!strncmp(&row->render[i], "//", 2)) {
				memset(&row->color[i], VGA_BRIGHT_BLACK, row->rsize - i);
				break;
			}
		}

		// Strings
		if (in_string) {
			row->color[i++] = VGA_GREEN;
			if (c == '\\' && i < row->rsize) {
				row->color[i++] = VGA_GREEN;
				continue;
			}
			if (c == in_string || (in_string == '<' && c == '>'))
				in_string = 0;
			was_sep = true;
			continue;
		} else if (c == '"' || c == '\'' || (in_include && c == '<')) {
			row->color[i++] = VGA_GREEN;
			in_string = c;
			continue;
		}

		// Numbers
		if (in_num) {
			if (in_num == '0') {
				switch (c) {
					case 'b': in_num = 2; break;
					case 'o': in_num = 8; break;
					case 'x': in_num = 16; break;
					default: in_num = isdigit(c) ? 10 : 0; break;
				}
				if (in_num != 0) {
					row->color[i++] = VGA_YELLOW;
					continue;
				}
			} else if (is_digit(c, in_num)) {
				row->color[i++] = VGA_YELLOW;
				continue;
			}
			in_num = 0;
		} else if (isdigit(c) && was_sep) {
			row->color[i++] = VGA_YELLOW;
			was_sep = false;
			in_num = c;
			continue;
		}

		if (in_define) {
			if (i == row->rsize - 1 && c != '\\')
				in_define = false;
		} else if (was_sep) {
			// Preprocessor
			if (!strncmp(&row->render[i], "#define ", 8)) {
				memset(&row->color[i], VGA_MAGENTA, row->rsize - i);
				was_sep = true;
				in_define = true;
				i += 7;
				continue;
			}
			if (!strncmp(&row->render[i], "#include ", 9)) {
				memset(&row->color[i], VGA_BRIGHT_BLUE, 8);
				was_sep = true;
				in_include = true;
				i += 8;
				continue;
			}
			if (!strncmp(&row->render[i], "#pragma ", 8)) {
				memset(&row->color[i], VGA_BRIGHT_YELLOW, row->rsize - i);
				break;
			}

			// Keywords types and special
			u64 n;
			if ((n = check_keyword(&row->render[i], keywords))) {
				memset(&row->color[i], VGA_MAGENTA, n);
				was_sep = false;
				i += n;
				continue;
			}
			if ((n = check_keyword(&row->render[i], types))) {
				memset(&row->color[i], VGA_BRIGHT_YELLOW, n);
				was_sep = false;
				i += n;
				continue;
			}
			if ((n = check_keyword(&row->render[i], special))) {
				memset(&row->color[i], VGA_CYAN, n);
				was_sep = false;
				i += n;
				continue;
			}
		}

		was_sep = is_separator(c);
		i++;
	}

	bool update_next = (row->in_comment != in_comment) || (row->in_define != in_define);
	row->in_comment = in_comment;
	row->in_define = in_define;
	return update_next;
}

syntax_f syntax_function(const char* path) {
	if (path == NULL)
		return NULL;

	char* ext = strrchr(path, '.');
	if (ext == NULL)
		return NULL;

	if (!strcmp(ext, ".c")) return &syntax_highlight_c;
	if (!strcmp(ext, ".h")) return &syntax_highlight_c;
	return NULL;
}
