#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <system.h>

char* cwd = NULL;

char* hist[256];
int hist_size = 0;

void hist_add(char* line) {
	hist[hist_size] = line;
	if (++hist_size == 256) {
		for (int i = 0; i < 32; i++)
			free(hist[i]);
		memmove(hist, &hist[32], sizeof(*hist) * 224);
		hist_size -= 32;
	}
}

void erase_prompt(const char* line) {
	i64 len = (i64) (strlen(cwd) + strlen(line) + 3);
	while (len >= 0) {
		puts("\x1b[2K");
		if (len >= 80)
			puts("\x1b[A");
		len -= 80;
	}
	puts("\x1b[80D");
}

void draw_prompt(const char* line, int index) {
	u64 len = strlen(line);
	u64 off = strlen(cwd) + len + 3;
	printf("\033[36m%s \033[97m$ \033[m%s", cwd, line);
	u64 x = off % 80;
	u64 back = len - index;
	while (1) {
		if (back > x) {
			puts("\x1b[A\x1b[80C");
			back -= x + 1;
			x = 79;
		} else {
			printf("\x1b[%dD", back);
			break;
		}
	}
}

char* fetch() {
	char* line = calloc(256);
	int index = 0, size = 0;
	int hist_index = hist_size;
	hist_add(line);
	draw_prompt(line, index);
	while (1) {
		int c = getkey();
		erase_prompt(line);
		switch (c) {
			case KEY_HOME: index = 0; break;
			case KEY_END: index = size; break;
			case KEY_LEFT: if (index > 0) index--; break;
			case KEY_RIGHT: if (index < size) index++; break;
			case KEY_UP:
				if (hist_index == 0)
					break;
				line = hist[--hist_index];
				index = size = strlen(line);
				break;
			case KEY_DOWN:
				if (hist_index == hist_size - 1)
					break;
				line = hist[++hist_index];
				index = size = strlen(line);
				break;
			case KEY_DELETE:
				if (size == 0 || index == size)
					break;
				memmove(&line[index], &line[index+1], 255 - index);
				size--;
				break;
			case '\n':
				line[size] = 0;
				if (hist_index != hist_size - 1) {
					free(hist[--hist_size]);
					hist_add(line);
				}
				draw_prompt(line, size);
				putchar('\n');
				return line;
			case '\b':
				if (index == 0)
					break;
				if (index != size) {
					memmove(&line[index-1], &line[index], 256 - index);
					index--;
				} else {
					line[--index] = 0;
				}
				size--;
				break;
			default:
				if (size >= 255 || !isprint(c))
					break;
				if (index != size)
					memmove(&line[index], &line[index-1], 256 - index);
				line[index++] = c;
				size++;
				break;
		}
		draw_prompt(line, index);
	}
}

char** split(const char* line, int* argc) {
	*argc = 0;
	char** argv = malloc(sizeof(char*));
	char state = ' ';
	u64 len = strlen(line);
	u64 base = 0;
	for (u64 i = 0; i <= len; i++) {
		if (state == ' ') {
			if (line[i] == '"') {
				base = i + 1;
				state = '"';
			} else if (line[i] != ' ') {
				base = i;
				state = 't';
			}
		} else if (state == '"') {
			if (line[i] == '"' || !line[i]) {
				argv[*argc] = malloc(i - base + 1);
				strncpy(argv[*argc], &line[base], i - base);
				argv[*argc][i - base] = 0;
				argv = realloc(argv, (++*argc + 1) * sizeof(char*));
				state = ' ';
			}
		} else if (state == 't') {
			if (line[i] == ' ' || !line[i]) {
				argv[*argc] = malloc(i - base + 1);
				strncpy(argv[*argc], &line[base], i - base);
				argv[*argc][i - base] = 0;
				argv = realloc(argv, (++*argc + 1) * sizeof(char*));
				state = ' ';
			}
		}
	}
	argv[*argc] = NULL;
	return argv;
}

bool execute(int argc, char** argv) {
	if (argc == 0) {
		return false;
	} else if (!strcmp(argv[0], "cd")) {
		chdir(argc > 1 ? argv[1] : "/");
		free(cwd);
		cwd = getcwd(NULL);
	} else if (!strcmp(argv[0], "clear")) {
		printf("\033[2J\033[H");
	} else if (!strcmp(argv[0], "exit")) {
		return true;
	} else {
		char* command;
		if (strchr(argv[0], '/') == NULL) {
			command = malloc(strlen(argv[0]) + 6);
			strcpy(command, "/bin/");
			strcpy(&command[5], argv[0]);
		} else {
			command = argv[0];
		}
		u64 pid = exec(command, argv);
		if (pid == 0) {
			printf("'%s': command not found\n", argv[0]);
		} else {
			wait(pid);
		}
	}
	return false;
}

int main() {
	cwd = getcwd(NULL);
	bool quit = false;
	while (!quit) {
		char* line = fetch();

		int argc;
		char** argv = split(line, &argc);

		quit = execute(argc, argv);

		for (int i = 0; i < argc; i++)
			free(argv[i]);
		free(argv);
	}
	for (int i = 0; i < hist_size; i++)
		free(hist[i]);
	return 0;
}
