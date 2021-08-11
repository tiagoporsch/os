#include "memory.h"

#include "boot.h"
#include "string.h"
#include "tty.h"

#define P4_INDEX(vaddr) ((((u64) (vaddr)) >> 39) & 0x1FF)
#define P3_INDEX(vaddr) ((((u64) (vaddr)) >> 30) & 0x1FF)
#define P2_INDEX(vaddr) ((((u64) (vaddr)) >> 21) & 0x1FF)
#define P1_INDEX(vaddr) ((((u64) (vaddr)) >> 12) & 0x1FF)

static u64 total_pages;
static u8* frame_bitmap = (u8*) 0x100000;

void memory_init() {
	total_pages = (BOOT_INFO->total_memory + PAGE_SIZE - 1) / PAGE_SIZE;
	memory_frame_set(0, (0x100000 + (total_pages + 7) / 8 + PAGE_SIZE - 1) / PAGE_SIZE);

	struct page_table* p4;
	asm volatile ("mov %%cr3, %0" : "=r" (p4));
	for (u64 addr = 0; addr < BOOT_INFO->total_memory; addr += PAGE_SIZE_1GIB) {
		struct page_table_entry* p4e = &p4->entry[P4_INDEX(MEM_AT_PHYS(addr))];
		if (!p4e->present) {
			p4e->present = 1;
			p4e->writable = 1;
			p4e->user_accessible = 0;
			p4e->frame = memory_frame_alloc(1) / PAGE_SIZE;
			memset((void*) (u64) (p4e->frame * PAGE_SIZE), 0, PAGE_SIZE);
		}
		struct page_table* p3 = (struct page_table*) (u64) (p4e->frame * PAGE_SIZE);
		struct page_table_entry* p3e = &p3->entry[P3_INDEX(MEM_AT_PHYS(addr))];
		if (!p3e->present) {
			p3e->present = 1;
			p3e->writable = 1;
			p3e->user_accessible = 0;
			p3e->huge = 1;
			p3e->frame = addr / PAGE_SIZE;
		}
	}

	memory_map((u64) p4, 0x6000, MEM_AT_PHYS(-0x1000), 1);

	u64* rbp;
	asm volatile ("mov %%rbp, %0" : "=r" (rbp));
	rbp = (u64*) MEM_AT_PHYS(rbp);
	rbp[0] = MEM_AT_PHYS(rbp[0]);
	rbp[1] = MEM_AT_PHYS(rbp[1]);
	asm volatile ("mov %0, %%rbp" : : "r" (rbp));
}

bool memory_frame_check(u64 frame, u64 length) {
	while (length--) {
		if (frame_bitmap[frame / 8] & (1 << (7 - (frame % 8))))
			return true;
		frame++;
	}
	return false;
}

void memory_frame_set(u64 frame, u64 length) {
	while (length > 0) {
		if (frame % 8 == 0 && length >= 8) {
			frame_bitmap[frame / 8] = 0xFF;
			frame += 8;
			length -= 8;
		} else {
			frame_bitmap[frame / 8] |= (1 << (7 - (frame % 8)));
			frame++;
			length--;
		}
	}
}

u64 memory_frame_alloc(u64 length) {
	for (u64 i = 0; i < total_pages; i++) {
		if (memory_frame_check(i, length))
			continue;
		memory_frame_set(i, length);
		return i * PAGE_SIZE;
	}
	panic("panic: out of memory");
}

void memory_map(u64 page_map, u64 paddr, u64 vaddr, u64 count) {
	struct page_table* p4 = (struct page_table*) MEM_AT_PHYS(page_map);
	while (count--) {
		struct page_table_entry* p4e = &p4->entry[P4_INDEX(vaddr)];
		if (!p4e->present) {
			p4e->present = 1;
			p4e->writable = 1;
			p4e->user_accessible = 0;
			p4e->frame = memory_frame_alloc(1) / PAGE_SIZE;
			memset((void*) MEM_AT_PHYS(p4e->frame * PAGE_SIZE), 0, PAGE_SIZE);
		}

		struct page_table* p3 = (void*) MEM_AT_PHYS(p4e->frame * PAGE_SIZE);
		struct page_table_entry* p3e = &p3->entry[P3_INDEX(vaddr)];
		if (!p3e->present) {
			p3e->present = 1;
			p3e->writable = 1;
			p3e->user_accessible = 0;
			p3e->frame = memory_frame_alloc(1) / PAGE_SIZE;
			memset((void*) MEM_AT_PHYS(p3e->frame * PAGE_SIZE), 0, PAGE_SIZE);
		}

		struct page_table* p2 = (void*) MEM_AT_PHYS(p3e->frame * PAGE_SIZE);
		struct page_table_entry* p2e = &p2->entry[P2_INDEX(vaddr)];
		if (!p2e->present) {
			p2e->present = 1;
			p2e->writable = 1;
			p2e->user_accessible = 0;
			p2e->frame = memory_frame_alloc(1) / PAGE_SIZE;
			memset((void*) MEM_AT_PHYS(p2e->frame * PAGE_SIZE), 0, PAGE_SIZE);
		}

		struct page_table* p1 = (void*) MEM_AT_PHYS(p2e->frame * PAGE_SIZE);
		struct page_table_entry* p1e = &p1->entry[P1_INDEX(vaddr)];
		if (p1e->present)
			panic("panic: attempted to remap %x\n", vaddr);
		p1e->present = 1;
		p1e->writable = 1;
		p1e->user_accessible = 0;
		p1e->frame = paddr / PAGE_SIZE;

		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	}
}
