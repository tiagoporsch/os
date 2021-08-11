#include "memory.h"

#include <stddef.h>

#include "boot.h"
#include "panic.h"
#include "string.h"
#include "tty.h"

u64 kernel_page_map;
u64 total_pages;
u8* frame_bitmap = (u8*) MEM_AT_PHYS(0x100000);

void memory_init() {
	total_pages = (BOOT_INFO->total_memory + PAGE_SIZE - 1) / PAGE_SIZE;
	kernel_page_map = memory_pm_get();
	memory_unmap(kernel_page_map, 0, 1);
}

/*
 * Physical memory
 */

bool memory_frame_check(u64 frame, u64 length) {
	if (frame + length >= total_pages)
		return false;
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
	panic("memory_frame_alloc: out of memory");
}

void memory_frame_free(u64 frame, u64 length) {
	while (length > 0) {
		if (frame % 8 == 0 && length >= 8) {
			frame_bitmap[frame / 8] = 0;
			frame += 8;
			length -= 8;
		} else {
			frame_bitmap[frame / 8] &= ~(1 << (7 - (frame % 8)));
			frame++;
			length--;
		}
	}
}


/*
 * Virtual memory
 */

#define P4_INDEX(vaddr) ((((u64) (vaddr)) >> 39) & 0x1FF)
#define P3_INDEX(vaddr) ((((u64) (vaddr)) >> 30) & 0x1FF)
#define P2_INDEX(vaddr) ((((u64) (vaddr)) >> 21) & 0x1FF)
#define P1_INDEX(vaddr) ((((u64) (vaddr)) >> 12) & 0x1FF)
#define P0_INDEX(vaddr) ((((u64) (vaddr))      ) & 0xFFF)

static inline void invlpg(void* addr) {
    asm volatile ("invlpg (%0)" : : "b" (addr) : "memory");
}

void memory_map(u64 page_map, u64 paddr, void* vaddr, u64 count) {
	assert(P0_INDEX(paddr) == 0);
	assert(P0_INDEX(vaddr) == 0);
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
			panic("memory_map: attempted to remap %x", vaddr);
		p1e->present = 1;
		p1e->writable = 1;
		p1e->user_accessible = 0;
		p1e->frame = paddr / PAGE_SIZE;

		invlpg(vaddr);
		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	}
}

static bool table_is_empty(struct page_table* table) {
	for (int i = 0; i < 512; i++)
		if (table->entry[i].present)
			return false;
	return true;
}

void memory_unmap(u64 page_map, void* vaddr, u64 page_count) {
	assert(P0_INDEX(vaddr) == 0);
	struct page_table* p4 = (void*) MEM_AT_PHYS(page_map);
	for (u64 page = 0; page < page_count; page++) {
		struct page_table_entry* p4e = &p4->entry[P4_INDEX(vaddr)];
		if (!p4e->present)
			return;
		struct page_table* p3 = (void*) MEM_AT_PHYS(p4e->frame * PAGE_SIZE);
		struct page_table_entry* p3e = &p3->entry[P3_INDEX(vaddr)];
		if (!p3e->present)
			return;
		if (p3e->huge) {
			p3e->present = false;
			if (table_is_empty(p3)) {
				p4e->present = false;
				memory_frame_free(p4e->frame, 1);
			}
			invlpg(vaddr);
			vaddr += PAGE_SIZE_1GIB;
			continue;
		}
		struct page_table* p2 = (void*) MEM_AT_PHYS(p3e->frame * PAGE_SIZE);
		struct page_table_entry* p2e = &p2->entry[P2_INDEX(vaddr)];
		if (!p2e->present)
			return;
		if (p2e->huge) {
			p2e->present = false;
			if (table_is_empty(p2)) {
				p3e->present = false;
				memory_frame_free(p3e->frame, 1);
				if (table_is_empty(p3)) {
					p4e->present = false;
					memory_frame_free(p4e->frame, 1);
				}
			}
			invlpg(vaddr);
			vaddr += PAGE_SIZE_2MIB;
			continue;
		}
		struct page_table* p1 = (void*) MEM_AT_PHYS(p2e->frame * PAGE_SIZE);
		struct page_table_entry* p1e = &p1->entry[P1_INDEX(vaddr)];
		if (!p1e->present)
			return;
		p1e->present = false;
		if (table_is_empty(p1)) {
			p2e->present = false;
			memory_frame_free(p2e->frame, 1);
			if (table_is_empty(p2)) {
				p3e->present = false;
				memory_frame_free(p3e->frame, 1);
				if (table_is_empty(p3)) {
					p4e->present = false;
					memory_frame_free(p4e->frame, 1);
				}
			}
		}

		invlpg(vaddr);
		vaddr += PAGE_SIZE;
	}
}

static bool memory_virt_used(u64 page_map, void* vaddr, u64 size) {
	struct page_table* p4 = (void*) MEM_AT_PHYS(page_map);
	for (; size--; vaddr += PAGE_SIZE) {
		struct page_table_entry* p4e = &p4->entry[P4_INDEX(vaddr)];
		if (!p4e->present)
			continue;
		struct page_table* p3 = (void*) MEM_AT_PHYS(p4e->frame * PAGE_SIZE);
		struct page_table_entry* p3e = &p3->entry[P3_INDEX(vaddr)];
		if (!p3e->present)
			continue;
		if (p3e->huge)
			return true;
		struct page_table* p2 = (void*) MEM_AT_PHYS(p3e->frame * PAGE_SIZE);
		struct page_table_entry* p2e = &p2->entry[P2_INDEX(vaddr)];
		if (!p2e->present)
			continue;
		if (p2e->huge)
			return true;
		struct page_table* p1 = (void*) MEM_AT_PHYS(p2e->frame * PAGE_SIZE);
		struct page_table_entry* p1e = &p1->entry[P1_INDEX(vaddr)];
		if (p1e->present)
			return true;
	}
	return false;
}

static void* memory_virt_alloc(u64 page_map, u64 size) {
	u64 vaddr = page_map == kernel_page_map ? MEM_AT_KERN(0) : 0x1000;
	u64 limit = page_map == kernel_page_map ? MEM_AT_PHYS(0) : MEM_AT_KERN(0);
	for (; vaddr < limit; vaddr += PAGE_SIZE)
		if (!memory_virt_used(page_map, (void*) vaddr, size))
			return (void*) vaddr;
	panic("memory_virt_alloc: ran out of virtual memory!?\n");
}

void* memory_alloc(u64 page_map, void* vaddr, u64 size) {
	if (vaddr == NULL)
		vaddr = memory_virt_alloc(page_map, size);
	assert(P0_INDEX(vaddr) == 0);
	for (void* addr = vaddr; size--; addr += PAGE_SIZE)
		memory_map(page_map, memory_frame_alloc(1), addr, 1);
	return vaddr;
}

static u64 memory_translate(u64 page_map, void* vaddr) {
	struct page_table* p4 = (void*) MEM_AT_PHYS(page_map);
	struct page_table_entry* p4e = &p4->entry[P4_INDEX(vaddr)];
	if (!p4e->present)
		return 0;
	struct page_table* p3 = (void*) MEM_AT_PHYS(p4e->frame * PAGE_SIZE);
	struct page_table_entry* p3e = &p3->entry[P3_INDEX(vaddr)];
	if (!p3e->present)
		return 0;
	if (p3e->huge)
		return p3e->frame * PAGE_SIZE_1GIB;
	struct page_table* p2 = (void*) MEM_AT_PHYS(p3e->frame * PAGE_SIZE);
	struct page_table_entry* p2e = &p2->entry[P2_INDEX(vaddr)];
	if (!p2e->present)
		return 0;
	if (p2e->huge)
		return p2e->frame * PAGE_SIZE_2MIB;
	struct page_table* p1 = (void*) MEM_AT_PHYS(p2e->frame * PAGE_SIZE);
	struct page_table_entry* p1e = &p1->entry[P1_INDEX(vaddr)];
	if (!p1e->present)
		return 0;
	return p1e->frame * PAGE_SIZE + P0_INDEX(vaddr);
}

void* memory_share(u64 pmd, u64 pms, void* vaddr, u64 size) {
	assert(P0_INDEX(vaddr) == 0);
	void* addr = memory_virt_alloc(pmd, size);
	for (u64 page = 0; page < size; page++) {
		u64 paddr = memory_translate(pms, vaddr + PAGE_SIZE * page);
		memory_map(pmd, paddr, addr + PAGE_SIZE * page, 1);
	}
	return addr;
}

void memory_free(u64 page_map, void* vaddr, u64 size) {
	assert(P0_INDEX(vaddr) == 0);
	struct page_table* p4 = (void*) MEM_AT_PHYS(page_map);
	while (size--) {
		struct page_table_entry* p4e = &p4->entry[P4_INDEX(vaddr)];
		if (!p4e->present)
			panic("memory_free: already free");
		struct page_table* p3 = (void*) MEM_AT_PHYS(p4e->frame * PAGE_SIZE);
		struct page_table_entry* p3e = &p3->entry[P3_INDEX(vaddr)];
		if (!p3e->present)
			panic("memory_free: already free");
		if (p3e->huge)
			panic("memory_free: huge p3e");
		struct page_table* p2 = (void*) MEM_AT_PHYS(p3e->frame * PAGE_SIZE);
		struct page_table_entry* p2e = &p2->entry[P2_INDEX(vaddr)];
		if (!p2e->present)
			panic("memory_free: already free");
		if (p2e->huge)
			panic("memory_free: huge p2e");
		struct page_table* p1 = (void*) MEM_AT_PHYS(p2e->frame * PAGE_SIZE);
		struct page_table_entry* p1e = &p1->entry[P1_INDEX(vaddr)];
		if (!p1e->present)
			panic("memory_free: already free");
		p1e->present = false;
		memory_frame_free(p1e->frame, 1);
		if (table_is_empty(p1)) {
			p2e->present = false;
			memory_frame_free(p2e->frame, 1);
			if (table_is_empty(p2)) {
				p3e->present = false;
				memory_frame_free(p3e->frame, 1);
				if (table_is_empty(p3)) {
					p4e->present = false;
					memory_frame_free(p4e->frame, 1);
				}
			}
		}
		vaddr += PAGE_SIZE;
	}
}

/*
 * Page map
 */

u64 memory_pm_get() {
	u64 page_map;
	asm volatile ("movq %%cr3, %0" : "=r" (page_map));
	return page_map;
}

u64 memory_pm_new() {
	u64 page_map = memory_frame_alloc(1);
	struct page_table* p4 = (void*) MEM_AT_PHYS(page_map);
	memset(p4, 0, PAGE_SIZE);
	struct page_table* kp4 = (void*) MEM_AT_PHYS(kernel_page_map);
	p4->entry[511] = kp4->entry[511];
	return page_map;
}

void memory_pm_free(u64 page_map) {
	struct page_table* p4 = (void*) MEM_AT_PHYS(page_map);
	for (u64 p4i = 0; p4i < 511; p4i++) {
		struct page_table_entry* p4e = &p4->entry[p4i];
		if (!p4e->present)
			continue;
		struct page_table* p3 = (void*) MEM_AT_PHYS(p4e->frame * PAGE_SIZE);
		for (u64 p3i = 0; p3i < 512; p3i++) {
			struct page_table_entry* p3e = &p3->entry[p3i];
			if (!p3e->present || p3e->huge)
				continue;
			struct page_table* p2 = (void*) MEM_AT_PHYS(p3e->frame * PAGE_SIZE);
			for (u64 p2i = 0; p2i < 512; p2i++) {
				struct page_table_entry* p2e = &p2->entry[p2i];
				if (!p2e->present || p2e->huge)
					continue;
				struct page_table* p1 = (void*) MEM_AT_PHYS(p2e->frame * PAGE_SIZE);
				for (u64 p1i = 0; p1i < 512; p1i++) {
					struct page_table_entry* p1e = &p1->entry[p1i];
					if (!p1e->present)
						continue;
					memory_frame_free(p1e->frame, 1);
				}
				memory_frame_free(p2e->frame, 1);
			}
			memory_frame_free(p3e->frame, 1);
		}
		memory_frame_free(p4e->frame, 1);
	}
	memory_frame_free(page_map / PAGE_SIZE, 1);
}
