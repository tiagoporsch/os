#include "elf.h"

#include "boot.h"
#include "disk.h"
#include "memory.h"
#include "string.h"
#include "tty.h"

void* elf_load(const char* path) {
	union file file;
	if (!file_find(&file, path))
		panic("panic: kernel: not found");

	char* buffer = (char*) ((BOOT_INFO->total_memory - file.size) & (~0x1FF));

	struct elf_ehdr* ehdr = (struct elf_ehdr*) buffer;
	file_read(&file, 0, ehdr, sizeof(*ehdr));
	if (ehdr->phnum != 1)
		panic("panic: kernel: too many program headers");
	void* entry = (void*) ehdr->entry;

	struct elf_phdr* phdr = (struct elf_phdr*) buffer;
	file_read(&file, ehdr->phoff, phdr, sizeof(*phdr));
	if (phdr->type != PT_LOAD)
		panic("panic: kernel: invalid program header");

	u64 pages = (phdr->memsz + PAGE_SIZE - 1) / PAGE_SIZE;
	void* dest = (void*) memory_frame_alloc(pages);
	memset(dest + phdr->filesz, 0, phdr->memsz - phdr->filesz);
	file_read(&file, phdr->offset, dest, phdr->filesz);

	u64 page_map;
	asm volatile ("mov %%cr3, %0" : "=r" (page_map));
	memory_map(page_map, (u64) dest, phdr->vaddr, pages);

	return entry;
}
