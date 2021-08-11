#include "syscall.h"

#include <system.h>

#include "ata.h"
#include "memory.h"
#include "panic.h"
#include "proc.h"
#include "tty.h"

static void* _mmap(void* vaddr, u64 size) {
	u64* rbp;
	extern u64 kernel_page_map;
	asm volatile ("mov %%rbp, %0" : "=r" (rbp));
	if (rbp[2] > MEM_AT_KERN(0))
		return memory_alloc(kernel_page_map, vaddr, size);
	return memory_alloc(memory_pm_get(), vaddr, size);
}

static void _munmap(void* vaddr, u64 size) {
	u64* rbp;
	extern u64 kernel_page_map;
	asm volatile ("mov %%rbp, %0" : "=r" (rbp));
	if (rbp[2] > MEM_AT_KERN(0))
		return memory_free(kernel_page_map, vaddr, size);
	memory_free(memory_pm_get(), vaddr, size);
}

static void _disk_read(u64 block, void* buffer) {
	ata_read_sector(ata_get_device(0), block, buffer);
}

static void _disk_write(u64 block, void* buffer) {
	ata_write_sector(ata_get_device(0), block, buffer);
}

void (*syscall_handlers[]) = {
	[SYS_YIELD] = proc_yield,
	[SYS_EXIT] = proc_exit,
	[SYS_EXEC] = proc_exec,
	[SYS_WAIT] = proc_wait,
	[SYS_GETCWD] = proc_getcwd,
	[SYS_CHDIR] = proc_chdir,

	[SYS_MMAP] = _mmap,
	[SYS_MUNMAP] = _munmap,

	[SYS_DISK_READ] = _disk_read,
	[SYS_DISK_WRITE] = _disk_write,
};

static inline u64 rdmsr(u64 msr) {
	u32 low, high;
	asm volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
	return ((u64) high << 32) | low;
}

static inline void wrmsr(u64 msr, u64 data) {
	u32 low = data & 0xFFFFFFFF;
	u32 high = data >> 32;
	asm volatile ("wrmsr" : : "c" (msr), "a" (low), "d" (high));
}

void syscall_init() {
	wrmsr(0xC0000080, rdmsr(0xC0000080) | 1);
	wrmsr(0xC0000081, ((u64) 16 << 48) | ((u64) 8 << 32));
	wrmsr(0xC0000082, (u64) &syscall_handler);
	wrmsr(0xC0000084, 0);
}
