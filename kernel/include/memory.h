#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PAGE_SIZE 0x1000
#define PAGE_SIZE_2MIB 0x200000
#define PAGE_SIZE_1GIB 0x40000000

#define MEM_AT_KERN(x) (0xFFFFFF8000000000 + (u64) (x))
#define MEM_AT_PHYS(x) (0xFFFFFFC000000000 + (u64) (x))

struct page_table_entry {
	u64 present : 1;
	u64 writable : 1;
	u64 user_accessible : 1;
	u64 write_through_caching : 1;
	u64 disable_cache : 1;
	u64 accessed : 1;
	u64 dirty : 1;
	u64 huge : 1;
	u64 global : 1;
	u64 unused : 3;
	u64 frame : 40;
	u64 unused1 : 11;
	u64 no_execute : 1;
} __attribute__ ((packed));

struct page_table {
	struct page_table_entry entry[512];
};

void memory_init();

void* memory_map(u64, u64, void*, u64);
void memory_unmap(u64, void*, u64);

void* memory_share(u64, u64, void*, u64);
void* memory_alloc(u64, void*, u64);
void memory_free(u64, void*, u64);

u64 memory_pm_get();
u64 memory_pm_new();
void memory_pm_free();
