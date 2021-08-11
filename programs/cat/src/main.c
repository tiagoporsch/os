#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stream.h>
#include <string.h>

int main(int argc, char** argv) {
	if (argc == 1) {
		printf("%s: Invalid arguments\n", argv[0]);
		return 1;
	}

	char* path = realpath(argv[1]);
	if (path == NULL) {
		printf("%s: %s: Invalid path\n", argv[0], argv[1]);
		return 2;
	}

	std_file_t* file = std_file_open(path, 0);
	if (file == NULL) {
		printf("%s: %s: Couldn't open file\n", argv[0], path);
		free(path);
		return 3;
	}
	free(path);

	char* buffer = malloc(file->size + 1);
	if (buffer == NULL) {
		printf("%s: Error allocating memory\n", argv[0]);
		std_file_close(file);
		return 5;
	}

	buffer[std_file_read(file, 0, buffer, file->size)] = 0;
	std_file_close(file);

	puts(buffer);
	free(buffer);
	return 0;
}
