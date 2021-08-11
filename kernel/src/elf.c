#include "elf.h"

#include <stddef.h>
#include <stdfile.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

void* elf_load(u64 page_map, const char* path) {
	std_file_t* file = std_file_open(path, 0);
	if (file == NULL)
		return NULL;
	char* buffer = malloc(file->size);
	if (buffer == NULL) {
		std_file_close(file);
		return NULL;
	}
	std_file_read(file, 0, buffer, file->size);
	std_file_close(file);

	struct elf_ehdr* ehdr = (void*) &buffer[0];
	struct elf_phdr* phdr = (void*) &buffer[ehdr->phoff];
	for (u64 i = 0; i < ehdr->phnum; i++) {
		struct elf_phdr* p = &phdr[i];
		if (p->type != PT_LOAD)
			continue;
		void* vaddr = (void*) (p->vaddr & ~((u64) 0xFFF));
		u64 pages = ((p->vaddr & 0xFFF) + p->memsz + PAGE_SIZE - 1) / PAGE_SIZE;
		memory_alloc(page_map, vaddr, pages);
		void* share = memory_share(memory_pm_get(), page_map, vaddr, pages);
		void* dest = share + (p->vaddr & 0xFFF);
		memset(dest + p->filesz, 0, p->memsz - p->filesz);
		memcpy(dest, buffer + p->offset, p->filesz);
		memory_unmap(memory_pm_get(), share, pages);
	}

	free(buffer);
	return (void*) ehdr->entry;
}
