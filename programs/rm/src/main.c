#include <stddef.h>
#include <stdfile.h>
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

	if (!std_file_remove(file)) {
		printf("%s: %s: Error removing file\n", argv[0], argv[1]);
		std_file_close(file);
		free(path);
		return 4;
	}

	std_file_close(file);
	free(path);
	return 0;
}
