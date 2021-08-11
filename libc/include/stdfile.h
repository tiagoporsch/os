#pragma once

#include "stdbool.h"
#include "stdint.h"

#define STD_FILE_NAME_LENGTH 460
#define STD_FILE_ROOT_BLOCK 2048

#define STD_FILE_CREATE (1 << 0)
#define STD_FILE_CLEAR (1 << 1)

enum std_file_type {
	STD_FILE, STD_DIRECTORY
};

typedef union std_file {
	struct {
		u64 index;
		u64 parent;
		u64 child;
		u64 next;
		u64 size;
		u64 time;
		u32 type;
		char name[STD_FILE_NAME_LENGTH];
	} __attribute__ ((packed));
	struct {
		u8 boot_code[486];
		u64 total_blocks;
		u64 bitmap_blocks;
		u64 bitmap_offset;
		u16 boot_signature;
	} __attribute__ ((packed));
	u64 pointer[64];
} __attribute__ ((packed)) std_file_t;

std_file_t* std_file_open(const char*, u64);
void std_file_close(std_file_t*);
bool std_file_parent(std_file_t*, std_file_t*);
bool std_file_child(std_file_t*, std_file_t*, const char*);
bool std_file_next(std_file_t*, std_file_t*);

bool std_file_add(std_file_t*, enum std_file_type, const char*);
bool std_file_remove(std_file_t*);

u64 std_file_write(std_file_t*, u64, const void*, u64);
u64 std_file_read(std_file_t*, u64, void*, u64);
