#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FILE_NAME_LENGTH 460
#define FILE_ROOT_BLOCK 2048

enum file_type {
	FILE_FILE, FILE_DIRECTORY,
};

union file {
	struct {
		u64 index;
		u64 parent;
		u64 child;
		u64 next;
		u64 size;
		u64 time;
		u32 type;
		char name[FILE_NAME_LENGTH];
	} __attribute__ ((packed));
	u64 pointer[64];
};

bool file_find(union file*, const char*);
bool file_next(union file*, union file*);
bool file_child(union file*, union file*, const char*);
u64 file_read(union file*, u64, void*, u64);
