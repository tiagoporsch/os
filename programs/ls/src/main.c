#include <stddef.h>
#include <stdfile.h>
#include <stdlib.h>
#include <stdio.h>
#include <system.h>

int main(int argc, char** argv) {
	char* path = argc > 1 ? realpath(argv[1]) : getcwd(NULL);
	if (path == NULL) {
		printf("%s: Invalid path\n", argv[0]);
		return 1;
	}

	std_file_t* file = std_file_open(path, 0);
	free(path);
	if (file == NULL) {
		printf("%s: %s: No such file or directory\n", argv[0], argv[1]);
		return 2;
	}
	if (file->type != STD_DIRECTORY) {
		printf("%s: %s: Not a directory\n", argv[0], argv[1]);
		std_file_close(file);
		return 3;
	}

	if (!std_file_child(file, file, NULL)) {
		std_file_close(file);
		return 0;
	}

	do {
		if (file->type == STD_DIRECTORY) {
			printf("%7d  \033[94m%s\033[0m\n", file->size, file->name);
		} else {
			u64 size = file->size;
			char size_unit = 'B';
			if (size >= 1024) {
				size /= 1024;
				size_unit = 'K';
			}
			if (size >= 1024) {
				size /= 1024;
				size_unit = 'M';
			}
			if (size >= 1024) {
				size /= 1024;
				size_unit = 'G';
			}
			printf("%7d%c %s\n", size, size_unit, file->name);
		}
	} while (std_file_next(file, file));

	std_file_close(file);
	return 0;
}
